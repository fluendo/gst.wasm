/*
 * gst.wasm - gstwebdownload
 *
 * Copyright 2024 Fluendo S.A.
 *  @author: Cesar Fabian Orccon Chipana <forccon@fluendo.com>
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

#include <string.h>
#include <stdlib.h>
#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/video/gstvideometa.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <gst/web/gstwebutils.h>
#include <gst/web/gstwebvideoframe.h>
#include "gstweb.h"

using namespace emscripten;

#define GST_TYPE_WEB_DOWNLOAD (gst_web_download_get_type ())
#define GST_CAT_DEFAULT web_download_debug
#define parent_class gst_web_download_parent_class

#define DEFAULT_STATIC_CAPS                                                   \
  GST_VIDEO_CAPS_MAKE (GST_WEB_MEMORY_VIDEO_FORMATS_STR)                      \
  ";" GST_VIDEO_CAPS_MAKE_WITH_FEATURES (                                     \
      GST_CAPS_FEATURE_MEMORY_WEB_VIDEO_FRAME,                                \
      GST_WEB_MEMORY_VIDEO_FORMATS_STR)

struct _GstWebDownload
{
  GstBaseTransform element;

  /*< private >*/
  GstWebCanvas *canvas;
  GstVideoInfo vinfo;
};

G_DECLARE_FINAL_TYPE (
    GstWebDownload, gst_web_download, GST, WEB_DOWNLOAD, GstBaseTransform)
G_DEFINE_TYPE (GstWebDownload, gst_web_download, GST_TYPE_BASE_TRANSFORM);
GST_ELEMENT_REGISTER_DEFINE (
    web_download, "webdownload", GST_RANK_NONE, GST_TYPE_WEB_DOWNLOAD);
GST_DEBUG_CATEGORY_STATIC (web_download_debug);

static GstStaticPadTemplate gst_web_download_sink_pad_template =
    GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
        GST_STATIC_CAPS (DEFAULT_STATIC_CAPS));

static GstStaticPadTemplate gst_web_download_src_pad_template =
    GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
        GST_STATIC_CAPS (DEFAULT_STATIC_CAPS));

static void
gst_web_download_init (GstWebDownload *self)
{
  gst_base_transform_set_prefer_passthrough (GST_BASE_TRANSFORM (self), TRUE);
}

static gboolean
gst_web_download_transform_start (GstBaseTransform *bt)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (bt);

  if (!gst_web_utils_element_ensure_canvas (
          GST_ELEMENT (self), &self->canvas, NULL)) {
    GST_ERROR_OBJECT (self, "Failed requesting a WebCanvas context");
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_web_download_transform_stop (GstBaseTransform *bt)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (bt);

  g_clear_pointer (&self->canvas, gst_object_unref);

  return TRUE;
}

static GstCaps *
gst_web_download_transform_caps (GstBaseTransform *bt,
    GstPadDirection direction, GstCaps *caps, GstCaps *filter)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (bt);

  GstCaps *tmp, *res;

  GST_DEBUG_OBJECT (self, "caps: %" GST_PTR_FORMAT, caps);
  GST_DEBUG_OBJECT (self, "filter: %" GST_PTR_FORMAT, filter);
  GST_DEBUG_OBJECT (self, "direction: %d", (int) direction);

  if (direction == GST_PAD_SRC) {
    tmp = gst_caps_copy (caps);
    gst_caps_set_features_simple (
        tmp, gst_caps_features_from_string (
                 GST_CAPS_FEATURE_MEMORY_WEB_VIDEO_FRAME));
    tmp = gst_caps_merge (gst_caps_ref (caps), tmp);
  } else {
    tmp = gst_caps_copy (caps);
    gst_caps_set_features_simple (tmp,
        gst_caps_features_from_string (GST_CAPS_FEATURE_MEMORY_SYSTEM_MEMORY));
    tmp = gst_caps_merge (gst_caps_ref (caps), tmp);
  }

  if (filter) {
    res = gst_caps_intersect_full (filter, tmp, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (tmp);
  } else {
    res = tmp;
  }

  GST_DEBUG_OBJECT (self, "Transformed caps to %" GST_PTR_FORMAT, caps);

  return res;
}

typedef struct _GstWebDownloadOutputBufferInternalData
{
  GstWebDownload *self;
  GstBuffer *inbuf;
  GstBuffer *outbuf;
} GstWebDownloadOutputBufferInternalData;

static void
gst_web_codecs_video_prepare_output_buffer_internal (gpointer data)
{
  GstWebDownloadOutputBufferInternalData *idata =
      (GstWebDownloadOutputBufferInternalData *) data;
  GstWebDownload *self = idata->self;
  GstWebVideoFrame *video_frame_memory;
  GstMapInfo map;
  val video_frame, buf_data_view, options;
  GstBuffer *outbuf;
  const char *video_frame_format;

  video_frame_memory =
      (GstWebVideoFrame *) gst_buffer_get_memory (idata->inbuf, 0);
  video_frame = gst_web_video_frame_get_handle (video_frame_memory);
  video_frame_format = gst_web_utils_convert_video_format (
      GST_VIDEO_INFO_FORMAT (&self->vinfo));

  if (video_frame_format == NULL)
    return;

  options = val::object ();
  options.set ("format", video_frame_format);

  // Prepare buffer and copy VideoFrame.
  // FIXME: Sometimes VideoFrame width and height have padding.
  outbuf = gst_buffer_new_allocate (NULL, self->vinfo.size, NULL);
  gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
  buf_data_view = val (typed_memory_view (map.size, map.data));
  video_frame.call<val> ("copyTo", buf_data_view, options).await ();
  gst_buffer_unmap (outbuf, &map);

  GST_BUFFER_PTS (outbuf) = GST_BUFFER_PTS (idata->inbuf);
  GST_BUFFER_DURATION (outbuf) = GST_BUFFER_DURATION (idata->inbuf);
  idata->outbuf = outbuf;
}

static GstFlowReturn
gst_web_download_prepare_output_buffer (
    GstBaseTransform *bt, GstBuffer *inbuf, GstBuffer **outbuf)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (bt);
  GstWebDownloadOutputBufferInternalData idata = {
    .self = self, .inbuf = inbuf, .outbuf = NULL
  };
  GstWebRunner *runner;

  if (gst_base_transform_is_passthrough (bt)) {
    *outbuf = inbuf;
    return GST_FLOW_OK;
  }

  runner = gst_web_canvas_get_runner (self->canvas);
  g_assert (runner != NULL);

  gst_web_runner_send_message (
      runner, gst_web_codecs_video_prepare_output_buffer_internal, &idata);
  gst_object_unref (GST_OBJECT (runner));

  *outbuf = idata.outbuf;

  return GST_FLOW_OK;
}

static gboolean
gst_web_download_set_caps (
    GstBaseTransform *bt, GstCaps *in_caps, GstCaps *out_caps)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (bt);

  gst_video_info_from_caps (&self->vinfo, out_caps);

  return TRUE;
}

static GstFlowReturn
gst_web_download_transform (
    GstBaseTransform *bt, GstBuffer *inbuf, GstBuffer *outbuf)
{
  return GST_FLOW_OK;
}

static void
gst_web_download_set_context (GstElement *element, GstContext *context)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (element);

  gst_web_utils_element_set_context (element, context, &self->canvas);
}

static void
gst_web_download_class_init (GstWebDownloadClass *klass)
{
  GstBaseTransformClass *gstbasetransform_class;
  GstElementClass *gstelement_class;
  GObjectClass *gobject_class;

  gstbasetransform_class = (GstBaseTransformClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gobject_class = (GObjectClass *) klass;

  gstbasetransform_class->start = gst_web_download_transform_start;
  gstbasetransform_class->stop = gst_web_download_transform_stop;
  gstbasetransform_class->transform_caps = gst_web_download_transform_caps;
  gstbasetransform_class->set_caps = gst_web_download_set_caps;
  gstbasetransform_class->passthrough_on_same_caps = TRUE;
  gstbasetransform_class->transform = gst_web_download_transform;
  gstbasetransform_class->prepare_output_buffer =
      gst_web_download_prepare_output_buffer;
  gstelement_class->set_context = gst_web_download_set_context;

  gst_element_class_set_static_metadata (gstelement_class,
      "Web Canvas Download", "Filter/Video",
      "Converts VideoFrame (memory) to raw pixel data", GST_WEB_AUTHOR);

  gst_element_class_add_static_pad_template (
      gstelement_class, &gst_web_download_src_pad_template);
  gst_element_class_add_static_pad_template (
      gstelement_class, &gst_web_download_sink_pad_template);

  GST_DEBUG_CATEGORY_INIT (
      web_download_debug, "webdownload", 0, "WebCodecs Video Decoder");
}
