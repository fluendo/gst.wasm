/*
 * GStreamer - GStreamer SDL2 video sink
 *
 * Copyright 2024 Fluendo S.A.
 *  @author: Alexander Slobodeniuk <aslobodeniuk@fluendo.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gstsdl2elements.h"
#include <gst/emscripten/gstemscripten.h>

#include <SDL2/SDL.h>

#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>

#define SURFACE_POOL_SIZE 4

GST_DEBUG_CATEGORY_STATIC (sdl2sink_dbg);
#define GST_CAT_DEFAULT sdl2sink_dbg

typedef struct _GstSDL2Sink
{
  GstVideoSink videosink;
  GstVideoInfo info;

  struct {
    SDL_Window *window;
    SDL_Renderer *target;
    GAsyncQueue *queue;
    GMutex lock;
    GCond ready;
    gboolean cancelled;
  } render;

  GAsyncQueue *surface_pool;
} GstSDL2Sink;

typedef struct _GstSDL2SinkClass
{
  GstVideoSinkClass parent_class;
} GstSDL2SinkClass;

typedef struct {
  SDL_Surface *s;
  GAsyncQueue *pool;
} GstSDL2Surface;

static GstSDL2Surface *
sdl2_surface_new (GAsyncQueue *pool, int video_width, int video_height)
{
  GstSDL2Surface *ret = g_new (GstSDL2Surface, 1);

  ret->s = SDL_CreateRGBSurface (0, video_width, video_height, 32, 0, 0, 0, 0);
  ret->pool = pool;
      
  return ret;
}

static void
surface_destroy (gpointer data)
{
  GstSDL2Surface *self = (GstSDL2Surface *)data;
  
  g_return_if_fail (data != NULL);
  
  SDL_FreeSurface (self->s);
  g_free (self);
}

static GAsyncQueue *
surface_pool_create (gint size, gint video_width, gint video_height)
{
  GAsyncQueue *pool;

  pool = g_async_queue_new_full (surface_destroy);
  for (int i = 0; i < size; i++)
    g_async_queue_push (pool, sdl2_surface_new (pool, video_width, video_height));

  return pool;
}

static void
surface_return_to_pool (gpointer data)
{
  GstSDL2Surface *self = (GstSDL2Surface *)data;
  
  g_return_if_fail (data != NULL);

  // We use g_async_queue_push_front() to avoid the "surface" carucelle
  // when it's not needed (most usual case). So copying will touch the same
  // memory pages with possible
  g_async_queue_push_front (self->pool, data);
}

static void
gst_sdl2_sink_create_window (gpointer data)
{
  GstSDL2Sink * self = (GstSDL2Sink *)data;
  
  SDL_Init (SDL_INIT_VIDEO);

  /* FIXME: isn't it weird that we create a window here? */
  GST_DEBUG ("Create window");
  SDL_CreateWindowAndRenderer (self->info.width, self->info.height,
      0, &self->render.window, &self->render.target);

  GST_DEBUG ("Create pool");
  // Create surface pool. Draft for the possible future buffer pool.
  // We need it to avoid copying raw data in the main loop
  self->surface_pool =
      surface_pool_create (SURFACE_POOL_SIZE, self->info.width, self->info.height);

  // unblock prepare if it's waiting for the initialization
  g_mutex_lock (&self->render.lock);
  g_cond_signal (&self->render.ready);
  g_mutex_unlock (&self->render.lock);
}

static gboolean
gst_sdl2_sink_render_mainloop (gpointer data)
{
  SDL_Texture *tex = NULL;
  GstSDL2Surface *surf;
  GstSDL2Sink * self = (GstSDL2Sink *)data;

  GST_DEBUG_OBJECT (self, "main loop");
  
  if (G_UNLIKELY (self->render.target == NULL))
    gst_sdl2_sink_create_window (self);
  
  surf = (GstSDL2Surface *) g_async_queue_try_pop (self->render.queue);
  if (!surf)
    return G_SOURCE_CONTINUE; // No new frame to render, happens quite often

  GST_DEBUG ("Have surface");
  tex = SDL_CreateTextureFromSurface (self->render.target, surf->s);
  if (!tex)
    goto cleanup;

  if (0 != SDL_RenderClear (self->render.target))
    goto cleanup;

  GST_DEBUG ("Copying to render");
  if (0 != SDL_RenderCopy (self->render.target, tex, NULL, NULL))
    goto cleanup;

  GST_DEBUG ("Rendering");
  SDL_RenderPresent (self->render.target);
cleanup:
  if (tex)
    SDL_DestroyTexture (tex);
  surface_return_to_pool (surf);

  return G_SOURCE_CONTINUE;
}

static gboolean
gst_sdl2_sink_setcaps (GstBaseSink * bsink, GstCaps * caps)
{
  GstSDL2Sink * self = (GstSDL2Sink *)bsink;
  GstVideoInfo info;

  GST_DEBUG_OBJECT (self, "Setting caps %" GST_PTR_FORMAT, caps);

  if (!gst_video_info_from_caps (&info, caps)) {
    GST_ERROR_OBJECT (self, "Bad caps %" GST_PTR_FORMAT, caps);
    return FALSE;
  }

  /* Can't run twice for now */
  g_assert (self->render.queue == NULL);
  gst_emscripten_ui_attach_callback (gst_sdl2_sink_render_mainloop, self, gst_object_unref);
  
  GST_DEBUG_OBJECT (self, "Setting");
  self->info = info;

  GST_DEBUG ("Create rendering queue");
  self->render.queue = g_async_queue_new_full (surface_return_to_pool);

  GST_DEBUG ("done");
  return TRUE;
}

static GstFlowReturn
gst_sdl2_sink_show_frame (GstVideoSink * bsink, GstBuffer * buf)
{
  GstSDL2Sink * self = (GstSDL2Sink *)bsink;
  GstSDL2Surface *surf;

  GST_DEBUG ("Show frame");

  // Query surface from a pool.
  // When the mainloop doesn't process surfaces we will
  // drop the sample
  if (!self->surface_pool)
    return GST_FLOW_OK;
  
  surf = g_async_queue_try_pop (self->surface_pool);
  if (surf) {
    GstMapInfo info;

    gst_buffer_map (buf, &info, GST_MAP_READ);

    GST_DEBUG ("Copy %" G_GSIZE_FORMAT " bytes to surface", info.size);

    // surface_map
    if (SDL_MUSTLOCK (surf->s))
      SDL_LockSurface (surf->s);

    // Copy the buffer to surface.
    // To avoid this copy we would have to implement a bufferpool
    // of 1 buffer, with our own implementation of the gst_buffer_map
    memcpy (surf->s->pixels, info.data, info.size);

    // surface_unmap
    if (SDL_MUSTLOCK (surf->s))
      SDL_UnlockSurface (surf->s);

    gst_buffer_unmap (buf, &info);

    GST_DEBUG ("Pushing surface %p to the UI main loop", surf);
    g_async_queue_push (self->render.queue, surf);
  } else {
    /* This happens when the rendering is blocked by the browser:
     * e.g. when the window is hidden, so it doesn't update the UI */
    GST_INFO ("No free surfaces, skipping the buffer %"
        GST_PTR_FORMAT, buf);
  }

  GST_DEBUG ("Show frame done");
  return GST_FLOW_OK;
}

static gboolean
gst_sdl2_sink_unlock (GstBaseSink * bsink)
{
  GstSDL2Sink * self = (GstSDL2Sink *)bsink;

  g_mutex_lock (&self->render.lock);
  self->render.cancelled = TRUE;
  g_cond_signal (&self->render.ready);
  g_mutex_unlock (&self->render.lock);

  return TRUE;
}

static gboolean
gst_sdl2_sink_unlock_stop (GstBaseSink * bsink)
{
  GstSDL2Sink * self = (GstSDL2Sink *)bsink;

  g_mutex_lock (&self->render.lock);
  self->render.cancelled = FALSE;
  g_mutex_unlock (&self->render.lock);

  return TRUE;
}

static GstFlowReturn
gst_sdl2_sink_prepare (GstBaseSink * bsink, GstBuffer * buf)
{
  GstSDL2Sink * self = (GstSDL2Sink *)bsink;
  GstFlowReturn ret = GST_FLOW_OK;

  if (G_LIKELY (self->render.target != NULL))
    return GST_FLOW_OK;
      
  /* By this time our UI callback has to be dispatched, but
   * however it's possible that it won't if the UI doesn't
   * get update by the browser (e.g. window is hidden).
   * If we wait too much here the buffer will be just late and
   * therefore dropped, that is fine for us. */
  g_mutex_lock (&self->render.lock);
  while (self->render.target == NULL
      && !self->render.cancelled) {
    g_cond_wait (&self->render.ready, &self->render.lock);
  }

  if (self->render.cancelled)
    ret = GST_FLOW_FLUSHING;
  
  g_mutex_unlock (&self->render.lock);
  
  return ret;
}

static void
gst_sdl2_sink_init (GstSDL2Sink * self)
{
  GST_DEBUG_OBJECT (self, "init");

  g_mutex_init (&self->render.lock);
  g_cond_init (&self->render.ready);
}

static void
gst_sdl2_sink_finalize (GObject *obj)
{
  GstSDL2Sink * self = (GstSDL2Sink *)obj;

  GST_DEBUG_OBJECT (self, "init");

  g_mutex_clear (&self->render.lock);
  g_cond_clear (&self->render.ready);
}

static gboolean
gst_sdl2_sink_start (GstBaseSink * bsink)
{
  GstSDL2Sink * self = (GstSDL2Sink *)bsink;

  GST_DEBUG_OBJECT (self, "Start");
  
  return TRUE;
}

static gboolean
gst_sdl2_sink_stop (GstBaseSink * bsink)
{
  GstSDL2Sink * self = (GstSDL2Sink *)bsink;

  GST_DEBUG_OBJECT (self, "Stop");
  
  gst_emscripten_ui_remove_callback (gst_sdl2_sink_render_mainloop, self);
  g_clear_pointer (&self->render.queue, g_async_queue_unref);

  if (self->render.target) {
    SDL_DestroyRenderer (self->render.target);
    self->render.target = NULL;
  }

  if (self->render.window) {
    SDL_DestroyWindow (self->render.window);
    self->render.window = NULL;
  }

  self->render.cancelled = FALSE;
  return TRUE;
}

static void
gst_sdl2_sink_class_init (GstSDL2SinkClass * class)
{
  union {
    GObjectClass *gobject;
    GstElementClass *element;
    GstBaseSinkClass *basesink;
    GstVideoSinkClass *videosink;
    GstSDL2SinkClass *sdl2sink;
  } klass = {
    .sdl2sink = class
  };

  klass.gobject->finalize = gst_sdl2_sink_finalize;
  
  gst_element_class_set_static_metadata (klass.element,
      "SDL2 Video sink", "Sink/Video", "Video rendering using SDL2",
      "Alexander Slobodeniuk <aslobodeniuk@fluendo.com>");

  static GstStaticPadTemplate gst_sdl2_sink_template =
      GST_STATIC_PAD_TEMPLATE ("sink",
          GST_PAD_SINK,
          GST_PAD_ALWAYS,
          GST_STATIC_CAPS ("video/x-raw, "
              /* TODO: support more formats through caps negotiation */
              "format = BGRA,"
              /* ----------------------------------------------------------- */
              "framerate = (fraction) [ 0, MAX ], "
              "width = (int) [ 1, MAX ], " "height = (int) [ 1, MAX ]"));
  
  gst_element_class_add_static_pad_template (klass.element,
      &gst_sdl2_sink_template);

  klass.basesink->set_caps = GST_DEBUG_FUNCPTR (gst_sdl2_sink_setcaps);
  klass.basesink->unlock = GST_DEBUG_FUNCPTR (gst_sdl2_sink_unlock);
  klass.basesink->unlock_stop = GST_DEBUG_FUNCPTR (gst_sdl2_sink_unlock_stop);
  klass.basesink->prepare = GST_DEBUG_FUNCPTR (gst_sdl2_sink_prepare);
  klass.basesink->start = GST_DEBUG_FUNCPTR (gst_sdl2_sink_start);
  klass.basesink->stop = GST_DEBUG_FUNCPTR (gst_sdl2_sink_stop);

  klass.videosink->show_frame =
      GST_DEBUG_FUNCPTR (gst_sdl2_sink_show_frame);

  GST_DEBUG_CATEGORY_INIT (sdl2sink_dbg, "sdl2sink",
      0, "SDL2 Sink");
}

GType gst_sdl2_sink_get_type (void);

G_DEFINE_TYPE (GstSDL2Sink, gst_sdl2_sink, GST_TYPE_VIDEO_SINK);

GST_ELEMENT_REGISTER_DEFINE (sdl2sink, "sdl2sink", GST_RANK_NONE,
    gst_sdl2_sink_get_type ());
