/*
 * GStreamer
 * Copyright (C) 2024 Jorge Zapata <jzapata@fluendo.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/*
 * GstWebVideoFrame is a GstMemory used to describe a VideoFrame WebAPI
 * Note that VideoFrames are only available for the thread that created it
 * and is not shared among any other JS thread, as each thread has it's own
 * local variables. Make sure to use this under a GstWebRunner only
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <emscripten/bind.h>

#include "gstwebrunner.h"
#include "gstwebvideoframe.h"

#define GST_IS_WEB_VIDEO_FRAME_ALLOCATOR(obj)                                 \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_WEB_VIDEO_FRAME_ALLOCATOR))
#define GST_IS_WEB_VIDEO_FRAME_ALLOCATOR_CLASS(klass)                         \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_WEB_VIDEO_FRAME_ALLOCATOR))
#define GST_WEB_VIDEO_FRAME_ALLOCATOR_GET_CLASS(obj)                          \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_WEB_VIDEO_FRAME_ALLOCATOR,      \
      GstWebVideoFrameAllocatorClass))
#define GST_WEB_VIDEO_FRAME_ALLOCATOR(obj)                                    \
  (G_TYPE_CHECK_INSTANCE_CAST (                                               \
      (obj), GST_TYPE_WEB_VIDEO_FRAME_ALLOCATOR, GstWebVideoFrameAllocator))
#define GST_WEB_VIDEO_FRAME_ALLOCATOR_CLASS(klass)                            \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_WEB_VIDEO_FRAME_ALLOCATOR,      \
      GstWebVideoFrameAllocatorClass))
#define GST_WEB_VIDEO_FRAME_ALLOCATOR_CAST(obj)                               \
  ((GstWebVideoFrameAllocator *) (obj))

using namespace emscripten;

/* The allocator to register with gst_allocator_register */
static GstAllocator *gst_web_video_frame_allocator;

GST_DEBUG_CATEGORY_STATIC (GST_CAT_WEB_VIDEO_FRAME);
#define GST_CAT_DEFAULT GST_CAT_WEB_VIDEO_FRAME

struct _GstWebVideoFramePrivate
{
  /* The runner this video frame belongs to */
  GstWebRunner *runner;
  /* The Emscripten's JS VideoFrame */
  val video_frame;
};

typedef struct _GstWebVideoFrameAllocator
{
  GstAllocator parent_class;
} GstWebVideoFrameAllocator;

typedef struct _GstWebVideoFrameAllocatorClass
{
  GstAllocatorClass parent_class;
} GstWebVideoFrameAllocatorClass;

typedef struct _GstWebVideoFrameAllocationSizeData
{
  val video_frame;
  gsize ret;
} GstWebVideoFrameAllocationSizeData;

typedef struct _GstWebVideoFrameCloseData
{
  val video_frame;
} GstWebVideoFrameCloseData;

static void
gst_web_video_frame_allocation_size (gpointer data)
{
  GstWebVideoFrameAllocationSizeData *allocation_size_data =
      (GstWebVideoFrameAllocationSizeData *) data;
  allocation_size_data->ret =
      allocation_size_data->video_frame.call<int> ("allocationSize");
}

static void
gst_web_video_frame_close (gpointer data)
{
  GstWebVideoFrameCloseData *close_data = (GstWebVideoFrameCloseData *) data;
  close_data->video_frame.call<void> ("close");
}

GST_DEFINE_MINI_OBJECT_TYPE (GstWebVideoFrame, gst_web_video_frame);

G_DEFINE_TYPE (GstWebVideoFrameAllocator, gst_web_video_frame_allocator,
    GST_TYPE_ALLOCATOR);

static gpointer
gst_web_video_frame_allocator_map_full (
    GstWebVideoFrame *mem, GstMapInfo *info, gsize size)
{
  GST_FIXME ("Trying to map");

  return NULL;
}

static void
gst_web_video_frame_allocator_unmap_full (
    GstWebVideoFrame *mem, GstMapInfo *info)
{
  GST_FIXME ("Trying to unmap");
}

static GstMemory *
gst_web_video_frame_allocator_share (
    GstWebVideoFrame *mem, gssize offset, gssize size)
{
  return NULL;
}

static gboolean
gst_web_video_frame_allocator_is_span (
    GstWebVideoFrame *mem1, GstWebVideoFrame *mem2, gsize *offset)
{
  return FALSE;
}

static GstMemory *
gst_web_video_frame_allocator_copy (
    GstWebVideoFrame *src, gssize offset, gssize size)
{
  return NULL;
}

static GstMemory *
gst_web_video_frame_allocator_alloc (
    GstAllocator *allocator, gsize size, GstAllocationParams *params)
{
  GstWebVideoFrameAllocationParams *vf_params =
      reinterpret_cast<GstWebVideoFrameAllocationParams *> (params);
  GstWebVideoFrame *mem;

  mem = g_new0 (GstWebVideoFrame, 1);
  /* We use a priv structure to ease the C->C++ managing */
  mem->priv = g_new0 (GstWebVideoFramePrivate, 1);
  mem->priv->video_frame = vf_params->video_frame;
  mem->priv->runner = vf_params->runner;

  /* Chain the memory init */
  /* We need to get the allocationSize from the video_frame to know the buffer
   * size */
  gst_memory_init (GST_MEMORY_CAST (mem), GST_MEMORY_FLAG_READONLY, allocator,
      NULL, size, 0, 0, size);

  return GST_MEMORY_CAST (mem);
}

static void
gst_web_video_frame_allocator_free (GstAllocator *allocator, GstMemory *memory)
{
  GstWebVideoFrame *self = (GstWebVideoFrame *) memory;
  GstWebVideoFrameCloseData close_data;

  close_data.video_frame = self->priv->video_frame;
  /* FIXME can be async */
  gst_web_runner_send_message (
      self->priv->runner, gst_web_video_frame_close, &close_data);

  gst_object_unref (self->priv->runner);
  self->priv->video_frame = val::undefined ();
  g_free (self->priv);
}

static void
gst_web_video_frame_allocator_init (GstWebVideoFrameAllocator *self)
{
  GstAllocator *allocator = GST_ALLOCATOR_CAST (self);

  allocator->mem_type = GST_WEB_VIDEO_FRAME_ALLOCATOR_NAME;
  GST_OBJECT_FLAG_SET (allocator, GST_ALLOCATOR_FLAG_CUSTOM_ALLOC);

  allocator->mem_map_full =
      (GstMemoryMapFullFunction) gst_web_video_frame_allocator_map_full;
  allocator->mem_unmap_full =
      (GstMemoryUnmapFullFunction) gst_web_video_frame_allocator_unmap_full;
  allocator->mem_copy =
      (GstMemoryCopyFunction) gst_web_video_frame_allocator_copy;
  allocator->mem_share =
      (GstMemoryShareFunction) gst_web_video_frame_allocator_share;
  allocator->mem_is_span =
      (GstMemoryIsSpanFunction) gst_web_video_frame_allocator_is_span;
}

static void
gst_web_video_frame_allocator_class_init (
    GstWebVideoFrameAllocatorClass *klass)
{
  GstAllocatorClass *allocator_class = GST_ALLOCATOR_CLASS (klass);

  allocator_class->alloc = gst_web_video_frame_allocator_alloc;
  allocator_class->free = gst_web_video_frame_allocator_free;
}

void
gst_web_video_frame_init (void)
{
  static gsize _init = 0;

  if (g_once_init_enter (&_init)) {
    GST_DEBUG_CATEGORY_INIT (
        GST_CAT_WEB_VIDEO_FRAME, "webvideoframe", 0, "Web Video Frame");

    gst_web_video_frame_allocator = GST_ALLOCATOR_CAST (
        g_object_new (GST_TYPE_WEB_VIDEO_FRAME_ALLOCATOR, NULL));

    gst_object_ref_sink (gst_web_video_frame_allocator);
    gst_allocator_register (
        GST_WEB_VIDEO_FRAME_ALLOCATOR_NAME, gst_web_video_frame_allocator);
    g_once_init_leave (&_init, 1);
  }
}

GstWebVideoFrame *
gst_web_video_frame_wrap (val video_frame, GstWebRunner *runner)
{
  GstWebVideoFrameAllocationParams params;
  GstWebVideoFrameAllocationSizeData allocation_size_data;
  GstAllocatorClass *allocator_class;
  GstMemory *mem;

  /* Create the AllocatorParams */
  /* TODO Create an gst_web_video_frame_allocation_parameters_init()
   * once we start sharing this among elements and it is actually
   * needed
   */
  params.video_frame = video_frame;
  params.runner = runner;

  /* We need to get the allocationSize from the video_frame to know the buffer
   * size */
  allocation_size_data.video_frame = video_frame;
  gst_web_runner_send_message (
      runner, gst_web_video_frame_allocation_size, &allocation_size_data);

  /* FIXME this should be gst_allocator_alloc, but the params are not
   * subclassable */
  /* Allocate with this params and return the VideoFrame */
  allocator_class = GST_ALLOCATOR_GET_CLASS (gst_web_video_frame_allocator);
  mem = allocator_class->alloc (gst_web_video_frame_allocator,
      allocation_size_data.ret, (GstAllocationParams *) &params);

  return GST_WEB_VIDEO_FRAME_CAST (mem);
}

val
gst_web_video_frame_get_handle (GstWebVideoFrame *self)
{
  return self->priv->video_frame;
}
