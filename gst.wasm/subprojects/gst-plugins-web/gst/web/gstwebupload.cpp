 /*
 * GStreamer - gst.wasm WebUpload source
 *
 * Copyright 2025 Fluendo S.A.
 * @author: Alexander Slobodeniuk <aslobodeniuk@fluendo.com>
 * @author: Cesar Fabian Orccon Chipana <forccon@fluendo.com>
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
#include <gst/web/gstwebutils.h>
#include <gst/web/gstwebvideoframe.h>
#include "gstweb.h"

#define GST_TYPE_WEB_UPLOAD (gst_web_upload_get_type ())
#define GST_CAT_DEFAULT web_upload_debug
#define parent_class gst_web_upload_parent_class

#define DEFAULT_STATIC_CAPS                                                   \
  GST_VIDEO_CAPS_MAKE (GST_WEB_MEMORY_VIDEO_FORMATS_STR)                      \
  ";" GST_VIDEO_CAPS_MAKE_WITH_FEATURES (                                     \
      GST_CAPS_FEATURE_MEMORY_WEB_VIDEO_FRAME,                                \
      GST_WEB_MEMORY_VIDEO_FORMATS_STR)

struct _GstWebUpload
{
  GstBaseTransform element;

  /*< private >*/
  GstVideoInfo vinfo;
  GstWebCanvas *canvas;
};

G_DECLARE_FINAL_TYPE (
    GstWebUpload, gst_web_upload, GST, WEB_UPLOAD, GstBaseTransform)
G_DEFINE_TYPE (GstWebUpload, gst_web_upload, GST_TYPE_BASE_TRANSFORM);
GST_ELEMENT_REGISTER_DEFINE (
    web_upload, "webupload", GST_RANK_NONE, GST_TYPE_WEB_UPLOAD);
GST_DEBUG_CATEGORY_STATIC (web_upload_debug);

static GstStaticPadTemplate gst_web_upload_sink_pad_template =
    GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
        GST_STATIC_CAPS (DEFAULT_STATIC_CAPS));

static GstStaticPadTemplate gst_web_upload_src_pad_template =
    GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
        GST_STATIC_CAPS (DEFAULT_STATIC_CAPS));

static GstCaps *
gst_web_upload_transform_caps (GstBaseTransform *bt,
    GstPadDirection direction, GstCaps *caps, GstCaps *filter)
{
  GstWebUpload *self = GST_WEB_UPLOAD (bt);

  GstCaps *tmp, *res;

  GST_DEBUG_OBJECT (self, "caps: %" GST_PTR_FORMAT, caps);
  GST_DEBUG_OBJECT (self, "filter: %" GST_PTR_FORMAT, filter);
  GST_DEBUG_OBJECT (self, "direction: %d", (int) direction);

  const gchar *mem = (direction == GST_PAD_SRC) ?
      GST_CAPS_FEATURE_MEMORY_SYSTEM_MEMORY :
      GST_CAPS_FEATURE_MEMORY_WEB_VIDEO_FRAME;
  
  tmp = gst_caps_copy (caps);
  gst_caps_set_features_simple (tmp, gst_caps_features_from_string (mem));
  tmp = gst_caps_merge (gst_caps_ref (caps), tmp);

  if (filter) {
    res = gst_caps_intersect_full (filter, tmp, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (tmp);
  } else {
    res = tmp;
  }

  GST_DEBUG_OBJECT (self, "Transformed caps to %" GST_PTR_FORMAT, caps);

  return res;
}

static GstFlowReturn
gst_web_upload_prepare_output_buffer (
    GstBaseTransform *bt, GstBuffer *inbuf, GstBuffer **outbuf)
{
  if (gst_base_transform_is_passthrough (bt)) {
    *outbuf = inbuf;
    return GST_FLOW_OK;
  }

  *outbuf = gst_buffer_new ();
  return GST_FLOW_OK;
}

static gboolean
gst_web_upload_set_caps (
    GstBaseTransform *bt, GstCaps *in_caps, GstCaps *out_caps)
{
  GstWebUpload *self = GST_WEB_UPLOAD (bt);

  gst_video_info_from_caps (&self->vinfo, out_caps);

  return TRUE;
}

static val
gst_web_upload_create_video_frame(val memoryView, GstVideoInfo *vinfo)
{
    val layout = val::array();

    for (int i = 0; i < vinfo->finfo->n_planes; i++) {
      layout.set(i, val::object());
      layout[i].set("offset", vinfo->offset[i]);
      layout[i].set("stride", vinfo->stride[i]);
    }

    val options = val::object();
    options.set("format",
        val(gst_web_utils_video_format_to_web_format (vinfo->finfo->format)));
    options.set("codedWidth", vinfo->width);
    options.set("codedHeight", vinfo->height);
    options.set("layout", layout);
    options.set("data", memoryView);
    options.set("timestamp", 0);

    return val::global("VideoFrame").new_(memoryView, options);
}

struct GstWebUploadMKVFData
{
   GstWebRunner *runner;
   GstBuffer *inbuf;
   GstWebUpload *self;
   GstWebVideoFrame *memory;
};

static void
gst_web_upload_make_video_frame (gpointer data)
{
  GstMapInfo map;
  val video_frame;
  val buffer_data;
  val image_data_data;
  GstWebUploadMKVFData *mkvf_data = (GstWebUploadMKVFData *)data;
  GstWebUpload *self = mkvf_data->self;

  gst_buffer_map (mkvf_data->inbuf, &map, GST_MAP_READ);
  buffer_data = val (typed_memory_view (map.size, map.data));
  image_data_data = val::global ("Uint8ClampedArray").new_ (buffer_data);

  video_frame = gst_web_upload_create_video_frame (image_data_data, &self->vinfo);

  mkvf_data->memory = gst_web_video_frame_wrap (video_frame, mkvf_data->runner);
  gst_buffer_unmap (mkvf_data->inbuf, &map);
}

static GstFlowReturn
gst_web_upload_transform (
    GstBaseTransform *bt, GstBuffer *inbuf, GstBuffer *outbuf)
{
  GstWebUpload *self = GST_WEB_UPLOAD (bt);

  GST_DEBUG_OBJECT (self, "Transform start");

  GstWebUploadMKVFData mkvf_data;
  mkvf_data.runner = gst_web_canvas_get_runner (self->canvas);
  mkvf_data.inbuf = inbuf;
  mkvf_data.self = self;
  
  gst_web_runner_send_message (
     mkvf_data.runner, gst_web_upload_make_video_frame, &mkvf_data);
  gst_object_unref (mkvf_data.runner);

  gst_buffer_insert_memory (outbuf, -1, GST_MEMORY_CAST (mkvf_data.memory));
  gst_buffer_copy_into (
     outbuf, inbuf,
     GstBufferCopyFlags (GST_BUFFER_COPY_META | GST_BUFFER_COPY_TIMESTAMPS), 0, -1);

  GST_DEBUG_OBJECT (self, "Transform end");
  return GST_FLOW_OK;
}

static gboolean
gst_web_upload_start (GstBaseTransform * bt)
{
  GstWebUpload *self = GST_WEB_UPLOAD (bt);
  GstWebRunner *runner;
  gboolean ret = FALSE;

  if (!gst_web_utils_element_ensure_canvas (
          GST_ELEMENT (self), &self->canvas, NULL)) {
    GST_ERROR_OBJECT (self, "Failed requesting a WebCanvas context");
    return FALSE;
  }

  runner = gst_web_canvas_get_runner (self->canvas);
  if (!gst_web_runner_run (runner, NULL)) {
    GST_ERROR_OBJECT (self, "Impossible to run the runner");
    goto done;
  }

  ret = TRUE;
done:
  gst_object_unref (runner);
  return ret;
}

static gboolean
gst_web_upload_query (GstElement *element, GstQuery *query)
{
  GstWebUpload *self = GST_WEB_UPLOAD (element);
  gboolean ret = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONTEXT:
      ret = gst_web_utils_element_handle_context_query (
          element, query, self->canvas);
      break;
    default:
      break;
  }

  if (!ret)
    ret = GST_ELEMENT_CLASS (parent_class)->query (element, query);

  return ret;
}

static void
gst_web_upload_set_context (GstElement *element, GstContext *context)
{
  GstWebUpload *self = GST_WEB_UPLOAD (element);

  gst_web_utils_element_set_context (element, context, &self->canvas);
}

static void
gst_web_upload_init (GstWebUpload *self)
{
  gst_base_transform_set_prefer_passthrough (GST_BASE_TRANSFORM (self), TRUE);
}

static void
gst_web_upload_dispose (GObject *gobj)
{
  GstWebUpload *self = GST_WEB_UPLOAD (gobj);

  if (self->canvas) {
     gst_object_unref (self->canvas);
  }
  
  G_OBJECT_CLASS (parent_class)->dispose (gobj);
}

static void
gst_web_upload_class_init (GstWebUploadClass *klass)
{
  GstBaseTransformClass *gstbasetransform_class;
  GstElementClass *gstelement_class;
  GObjectClass *gobject_class;
  
  gstbasetransform_class = (GstBaseTransformClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gobject_class = (GObjectClass *) klass;

  gstbasetransform_class->transform_caps = gst_web_upload_transform_caps;
  gstbasetransform_class->set_caps = gst_web_upload_set_caps;
  gstbasetransform_class->passthrough_on_same_caps = TRUE;
  gstbasetransform_class->transform = gst_web_upload_transform;
  gstbasetransform_class->prepare_output_buffer =
      gst_web_upload_prepare_output_buffer;
  gstbasetransform_class->start =
      gst_web_upload_start;

  gstelement_class->query = gst_web_upload_query;
  gstelement_class->set_context = gst_web_upload_set_context;

  gst_element_class_set_static_metadata (gstelement_class,
      "Web Canvas Upload", "Filter/Video",
      "Converts raw pixel data to VideoFrame (memory)", GST_WEB_AUTHOR);

  gst_element_class_add_static_pad_template (
      gstelement_class, &gst_web_upload_src_pad_template);
  gst_element_class_add_static_pad_template (
      gstelement_class, &gst_web_upload_sink_pad_template);

  gobject_class->dispose = gst_web_upload_dispose;

  GST_DEBUG_CATEGORY_INIT (
      web_upload_debug, "webupload", 0, "WebCodecs Video Decoder");
}
