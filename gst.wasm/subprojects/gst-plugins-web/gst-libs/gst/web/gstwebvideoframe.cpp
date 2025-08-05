/*
 * GStreamer - gst.wasm WebVideoFrame source
 *
 * Copyright 2024 Fluendo S.A.
 *  @author: Jorge Zapata <jzapata@fluendo.com>
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
#include <emscripten.h>
#include <emscripten/bind.h>
#include <gst/video/gstvideometa.h>
#include <gst/web/gstwebutils.h>

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
  guint8 *data;
};

typedef struct _GstWebVideoFrameAllocator
{
  GstAllocator parent_class;
} GstWebVideoFrameAllocator;

typedef struct _GstWebVideoFrameAllocatorClass
{
  GstAllocatorClass parent_class;
} GstWebVideoFrameAllocatorClass;

typedef struct _GstWebVideoFrameAllocatorMapCpuData
{
  GstWebVideoFrame *self;
  guint8 *data;
  gsize size;
  GstVideoInfo *info;
  gboolean result;
} GstWebVideoFrameAllocatorMapCpuData;

typedef struct _GstWebVideoFrameAllocationSizeData
{
  val video_frame;
  gsize ret;
} GstWebVideoFrameAllocationSizeData;

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
  GstWebVideoFrame *self = (GstWebVideoFrame *)data;

  self->priv->video_frame.call<void> ("close");
  self->priv->video_frame = val::undefined ();
}

GST_DEFINE_MINI_OBJECT_TYPE (GstWebVideoFrame, gst_web_video_frame);

G_DEFINE_TYPE (GstWebVideoFrameAllocator, gst_web_video_frame_allocator,
    GST_TYPE_ALLOCATOR);

static gboolean
gst_web_video_frame_map_internal (
    GstWebVideoFrame *self, GstVideoInfo *info, gpointer data, gsize size)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (
      info == NULL || (info != NULL && info->finfo != NULL), FALSE);
  g_return_val_if_fail (data != NULL, FALSE);

  val video_frame = gst_web_video_frame_get_handle (self);
  val data_view =
    val (typed_memory_view (size, (guint8 *)data));
  val options = val::object ();

#if 0
  // Setting the output format for CopyTo makes the browser think
  // you need a convertion even if the subsampling format is the same.
  // So downloading I420 will make it fail saying it only converts to RGBA.
  // While not setting the format downloads I420 by copying just fine.
  if (info != NULL) {
    const char *format;

    format = gst_web_utils_video_format_to_web_format (
        GST_VIDEO_FORMAT_INFO_FORMAT (info->finfo));
    if (format == NULL) {
      GST_ERROR ("Format %s is not supported.",
          GST_VIDEO_FORMAT_INFO_NAME (info->finfo));
      return FALSE;
    }
    options.set ("format", format);
  } else {
    GST_DEBUG ("Use input format.");
  }
#endif

  video_frame.call<val> ("copyTo", data_view, options).await ();

  return TRUE;
}

static void
gst_web_video_frame_allocator_map_cpu_access (gpointer data)
{
  GstWebVideoFrameAllocatorMapCpuData *cdata =
      (GstWebVideoFrameAllocatorMapCpuData *) data;
  GstWebVideoFrame *self = cdata->self;

  cdata->result = gst_web_video_frame_map_internal (
      self, cdata->info, cdata->data, cdata->size);
}

gboolean
gst_web_video_frame_copy_to (
    GstWebVideoFrame *self, GstVideoInfo *info, guint8 *data, gsize size)
{
  GstWebVideoFrameAllocatorMapCpuData cdata = { .self = self,
    .data = data,
    .size = info->size,
    .info = info,
    .result = FALSE };

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (info != NULL, FALSE);

  gst_web_runner_send_message (self->priv->runner,
      gst_web_video_frame_allocator_map_cpu_access, &cdata);

  return cdata.result;
}

static gpointer
gst_web_video_frame_allocator_map_full (
    GstWebVideoFrame *self, GstMapInfo *info, gsize size)
{
  GstWebVideoFrameAllocatorMapCpuData data = {
    .self = self, .info = NULL, .result = FALSE
  };

  if ((info->flags & GST_MAP_WRITE) == GST_MAP_WRITE) {
    GST_DEBUG ("Write flags not supported");
    return NULL;
  }

  if (self->priv->data)
    goto beach;

  data.data = self->priv->data = (guint8 *) g_malloc (size);
  data.size = size;

  gst_web_runner_send_message (
      self->priv->runner, gst_web_video_frame_allocator_map_cpu_access, &data);

  if (!data.result) {
    return NULL;
  }

beach:
  return self->priv->data;
}

static void
gst_web_video_frame_allocator_unmap_full (
    GstWebVideoFrame *mem, GstMapInfo *info)
{
  /* Nothing to do. */
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
  mem->priv->runner = (GstWebRunner*)gst_object_ref (vf_params->runner);
  mem->priv->data = NULL;

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

  /* FIXME can be async (RDI-2856) */
  gst_web_runner_send_message (
      self->priv->runner, gst_web_video_frame_close, self);

  gst_object_unref (self->priv->runner);
  g_free (self->priv->data);
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
gst_web_video_frame_wrap (val &video_frame, GstWebRunner *runner)
{
  GstWebVideoFrameAllocationParams params;
  GstWebVideoFrameAllocationSizeData allocation_size_data;
  GstAllocatorClass *allocator_class;
  GstMemory *mem;

  /* Create the AllocatorParams */
  /* TODO Create an gst_web_video_frame_allocation_parameters_init()
   * once we start sharing this among elements and it is actually
   * needed (RDI-2855)
   */
  params.video_frame = video_frame;
  params.runner = runner;

  /* We need to get the allocationSize from the video_frame to know the buffer
   * size */
  allocation_size_data.video_frame = video_frame;
  gst_web_runner_send_message (
      runner, gst_web_video_frame_allocation_size, &allocation_size_data);

  /* FIXME this should be gst_allocator_alloc, but the params are not
   * subclassable (RDI-2854) */
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
