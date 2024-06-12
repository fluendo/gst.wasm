/* Copyright (C) Fluendo S.A. <support@fluendo.com> */

#include <stdio.h>
#include <SDL2/SDL.h>

#include <emscripten.h>
#include <emscripten/threading.h>

#include <gst/gst.h>

#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480
#define SURFACE_POOL_SIZE 4

GST_DEBUG_CATEGORY_STATIC (example_dbg);
#define GST_CAT_DEFAULT example_dbg

static struct
{
  SDL_Window *window;
  SDL_Renderer *renderer;
  GAsyncQueue *rendering_queue;

  GAsyncQueue *surface_pool;

  GstElement *pipe;
  GstElement *appsink;
} context;


static void
surface_destroy (gpointer data)
{
  g_return_if_fail (data != NULL);

  SDL_FreeSurface ((SDL_Surface *) data);
}

GAsyncQueue *
surface_pool_create (gint size, gint video_width, gint video_height)
{
  GAsyncQueue *pool;

  pool = g_async_queue_new_full (surface_destroy);
  for (int i = 0; i < size; i++)
    g_async_queue_push (pool,
        SDL_CreateRGBSurface (0, video_width, video_height, 32, 0, 0, 0, 0));

  return pool;
}

static void
surface_return_to_pool (gpointer data)
{
  g_return_if_fail (data != NULL);

  // We use g_async_queue_push_front() to avoid the "surface" carucelle
  // when it's not needed (most usual case). So copying will touch the same
  // memory pages with possible
  g_async_queue_push_front (context.surface_pool, data);
}

static void
surface_absorb_sample (SDL_Surface *surf, GstSample *samp)
{
  GstBuffer *buf;
  GstMapInfo info;

  g_return_if_fail (samp != NULL);

  buf = gst_sample_get_buffer (samp);
  gst_buffer_map (buf, &info, GST_MAP_READ);

  GST_DEBUG ("Copy to surface");

  // surface_map
  if (SDL_MUSTLOCK (surf))
    SDL_LockSurface (surf);

  // Copy the buffer to surface.
  // To avoid this copy we would have to implement a bufferpool
  // of 1 buffer, with our own implementation of the gst_buffer_map
  memcpy (surf->pixels, info.data, info.size);

  // surface_unmap
  if (SDL_MUSTLOCK (surf))
    SDL_UnlockSurface (surf);

  gst_buffer_unmap (buf, &info);
  gst_sample_unref (samp);
}

static GstFlowReturn
appsink_new_sample_cb (GstElement *appsink, gpointer data)
{
  GstSample *samp = NULL;
  SDL_Surface *surf;

  GST_DEBUG ("Have sample");
  g_signal_emit_by_name (appsink, "pull-sample", &samp);

  // Query surface from a pool.
  // When the mainloop doesn't process surfaces we will
  // drop the sample
  surf = g_async_queue_try_pop (context.surface_pool);
  if (surf) {
    surface_absorb_sample (surf, samp);
    g_async_queue_push (context.rendering_queue, surf);
  } else {
    GST_DEBUG ("No free surfaces, dropping the sample");
    gst_sample_unref (samp);
  }

  return GST_FLOW_OK;
}

static void
init_pipeline ()
{
  GST_PLUGIN_STATIC_DECLARE (coreelements);
  GST_PLUGIN_STATIC_DECLARE (app);
  GST_PLUGIN_STATIC_DECLARE (videotestsrc);

  GST_PLUGIN_STATIC_REGISTER (coreelements);
  GST_PLUGIN_STATIC_REGISTER (app);
  GST_PLUGIN_STATIC_REGISTER (videotestsrc);

  {
    gchar *desc = g_strdup_printf ("videotestsrc pattern=ball is-live=true "
        "! capsfilter caps=\"video/x-raw,format=RGBA,width=%d,height=%d,framerate=30/1\" "
        "! appsink name=output", VIDEO_WIDTH, VIDEO_HEIGHT);

    context.pipe = gst_parse_launch (desc, NULL);
    g_free (desc);
  }

  context.appsink = gst_bin_get_by_name (GST_BIN (context.pipe), "output");

  g_object_set (context.appsink, "emit-signals", TRUE, NULL);
  g_signal_connect (context.appsink, "new-sample",
      G_CALLBACK (appsink_new_sample_cb), NULL);

  context.rendering_queue = g_async_queue_new_full (surface_return_to_pool);

  // Create surface pool. Draft for the possible future buffer pool.
  // We need it to avoid copying raw data in the main loop
  context.surface_pool =
      surface_pool_create (SURFACE_POOL_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);
}

static void
render_mainloop (void)
{
  SDL_Texture *tex = NULL;
  SDL_Surface *surf;

  surf = (SDL_Surface *) g_async_queue_try_pop (context.rendering_queue);
  if (!surf)
    return;                     // No new frame to render, happens quite often

  tex = SDL_CreateTextureFromSurface (context.renderer, surf);
  if (!tex)
    goto cleanup;

  if (0 != SDL_RenderClear (context.renderer))
    goto cleanup;

  GST_DEBUG ("Copying to render");
  if (0 != SDL_RenderCopy (context.renderer, tex, NULL, NULL))
    goto cleanup;

  GST_DEBUG ("Rendering");
  SDL_RenderPresent (context.renderer);
cleanup:
  if (tex)
    SDL_DestroyTexture (tex);
  surface_return_to_pool (surf);
}

int
main (int argc, char **argv)
{
  gst_init (NULL, NULL);
  GST_DEBUG_CATEGORY_INIT (example_dbg, "example",
      0, "videotestsrc wasm example");

  gst_debug_set_threshold_for_name ("example", 5);

  GST_DEBUG ("Init SDL");
  SDL_Init (SDL_INIT_VIDEO);

  GST_DEBUG ("Create window");
  SDL_CreateWindowAndRenderer (VIDEO_WIDTH, VIDEO_HEIGHT,
      0, &context.window, &context.renderer);

  init_pipeline ();

  gst_element_set_state (context.pipe, GST_STATE_PLAYING);
  emscripten_set_main_loop (render_mainloop, 0, FALSE);

  // FIXME: where to put SDL_Quit() ?
  return 0;
}
