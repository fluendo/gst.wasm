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

#ifndef __GST_WEB_VIDEO_FRAME_H__
#define __GST_WEB_VIDEO_FRAME_H__

#include <gst/gst.h>

#include "gstwebrunner.h"

#define GST_WEB_VIDEO_FRAME_CAST(obj) ((GstWebVideoFrame *) obj)

#define GST_TYPE_WEB_VIDEO_FRAME_ALLOCATOR                                    \
  (gst_web_video_frame_allocator_get_type ())
#define GST_WEB_VIDEO_FRAME_ALLOCATOR_NAME "WebVideoFrame"

GType gst_web_video_frame_allocator_get_type (void);

#define GST_CAPS_FEATURE_MEMORY_WEB_VIDEO_FRAME "memory:WebVideoFrame"

/**
 * GST_WEB_VIDEO_FRAME_FORMATS_STR:
 *
 * List of video formats that are supported by #GstGLMemory
 */
#define GST_WEB_MEMORY_VIDEO_FORMATS_STR                                      \
  "{ RGBA, RGBx, BGRA, BGRx, I420, A420, Y42B, Y444, NV12 }"

typedef struct _GstWebVideoFrame GstWebVideoFrame;
typedef struct _GstWebVideoFramePrivate GstWebVideoFramePrivate;
typedef struct _GstWebVideoFrameAllocationParams
    GstWebVideoFrameAllocationParams;

struct _GstWebVideoFrame
{
  GstMemory base;
  GstWebVideoFramePrivate *priv;

  /*< private >*/
  gpointer _padding[GST_PADDING];
};

#ifdef __cplusplus

using namespace emscripten;

/* Note that we cannot subclass GstAllocationParams */
struct _GstWebVideoFrameAllocationParams
{
  GstWebRunner *runner;
  val video_frame;
};

GstWebVideoFrame *gst_web_video_frame_wrap (
    val &video_frame, GstWebRunner *runner);
val gst_web_video_frame_get_handle (GstWebVideoFrame *self);

#endif

G_BEGIN_DECLS

GType gst_web_video_frame_get_type (void);
void gst_web_video_frame_init (void);

gboolean
gst_web_video_frame_copy_to (
   GstWebVideoFrame *self, GstVideoInfo *info, guint8 *data, gsize size);

G_END_DECLS

#endif /* __GST_WEB_VIDEO_FRAME_H__ */
