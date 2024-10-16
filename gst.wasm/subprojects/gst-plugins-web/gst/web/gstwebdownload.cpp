/*
 * GStreamer - GStreamer WebCanvasSrc source
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
#include <emscripten/threading.h>
#include <gst/web/gstwebutils.h>
#include <gst/web/gstwebvideoframe.h>

using namespace emscripten;

#define GST_TYPE_WEB_DOWNLOAD (gst_web_download_get_type ())
#define GST_CAT_DEFAULT web_download_debug
#define parent_class gst_web_download_parent_class

#define DEFAULT_STATIC_CAPS \
    GST_VIDEO_CAPS_MAKE (GST_WEB_MEMORY_VIDEO_FORMATS_STR) ";" \
    GST_VIDEO_CAPS_MAKE_WITH_FEATURES (GST_CAPS_FEATURE_MEMORY_WEB_VIDEO_FRAME, GST_WEB_MEMORY_VIDEO_FORMATS_STR)

#define DEFAULT_IS_LIVE TRUE
#define DEFAULT_FPS_N 30
#define DEFAULT_FPS_D 1

enum
{
  PROP_0,
  PROP_ID,
  PROP_LAST
};

struct _GstWebDownload
{
  GstBaseTransform element;
  GstWebCanvas *canvas;

  /*< private >*/
  gchar *id;
  GstCaps *caps;
  guint n_frames;
};

G_DECLARE_FINAL_TYPE (
    GstWebDownload, gst_web_download, GST, WEB_DOWNLOAD, GstBaseTransform)
G_DEFINE_TYPE (GstWebDownload, gst_web_download, GST_TYPE_BASE_TRANSFORM);
GST_ELEMENT_REGISTER_DEFINE (
    web_download, "webdownload", GST_RANK_NONE, GST_TYPE_WEB_DOWNLOAD);
GST_DEBUG_CATEGORY_STATIC (web_download_debug);

static GstStaticPadTemplate gst_web_download_sink_pad_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (DEFAULT_STATIC_CAPS));

static GstStaticPadTemplate gst_web_download_src_pad_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (DEFAULT_STATIC_CAPS));

static void
gst_web_download_init (GstWebDownload *self)
{
  gst_base_transform_set_prefer_passthrough (GST_BASE_TRANSFORM (self),
      TRUE);
}

static val
gst_web_download_get_canvas (GstWebDownload *self)
{
  val document = val::global ("document");
  val canvas =
      document.call<val> ("getElementById", val (std::string (self->id)));

  if (canvas == val::null ()) {
    GST_LOG_OBJECT (self,
        "Cannot retrieve canvas with id '%s'."
        "Trying to retrieve canvas from global variable '%s'.",
        self->id, self->id);

    canvas = val::global (self->id);
  }

  return canvas;
}

static GstBuffer *
gst_web_download_buffer_from_canvas (GstWebDownload *self)
{
  GstBuffer *buf;
  val canvas = gst_web_download_get_canvas (self);
  if (canvas == val::null ()) {
    GST_ERROR_OBJECT (self, "Cannot retrieve a canvas.");
    return NULL;
  }

  int width = canvas["width"].as<int> ();
  int height = canvas["height"].as<int> ();

  val context2d_data = val::object ();
  context2d_data.set ("willReadFrequently", true);

  val context2d =
      canvas.call<val> ("getContext", std::string ("2d"), context2d_data);
  val image_data = context2d.call<val> ("getImageData", 0, 0, width, height);
  val data = image_data["data"];

  std::vector<uint8_t> raw_data =
      emscripten::convertJSArrayToNumberVector<uint8_t> (data);

  if (width * height * 4 != (int) raw_data.size ()) {
    GST_ERROR_OBJECT (
        self, "Expected data to have size: %d.", width * height * 4);
    return NULL;
  }

  buf = gst_buffer_new_memdup (raw_data.data (), raw_data.size ());

  if (buf == NULL) {
    GST_ERROR_OBJECT (
        self, "Cannot create buffer from canvas '%s' data", self->id);
    return NULL;
  }

  GST_LOG_OBJECT (
      self, "Created a new buffer from canvas '%s' data", self->id);
  return buf;
}

static void
gst_web_download_update_caps_from_canvas (GstWebDownload *self)
{
  GstCaps *new_caps;
  val canvas = gst_web_download_get_canvas (self);

  new_caps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING,
      "RGBA", "framerate", GST_TYPE_FRACTION, DEFAULT_FPS_N, DEFAULT_FPS_D,
      "width", G_TYPE_INT, canvas["width"].as<int> (), "height", G_TYPE_INT,
      canvas["height"].as<int> (), nullptr);

  gst_caps_replace (&self->caps, new_caps);
  // g_assert (gst_pad_set_caps (GST_BASE_SRC_PAD (self), new_caps));

  GST_DEBUG_OBJECT (self, "Setting new caps: %" GST_PTR_FORMAT, new_caps);

  gst_caps_unref (new_caps);
}

static void
gst_web_download_create_internal (GstWebDownload *self, GstBuffer **buffer)
{
  // TODO: Support canvas dimension changes.
  if (!self->caps)
    gst_web_download_update_caps_from_canvas (self);

  *buffer = gst_web_download_buffer_from_canvas (self);
}

static GstFlowReturn
gst_web_download_create (GstBaseTransform *psrc, GstBuffer **buffer)
{
  GstWebDownload *self;
  GstBuffer *buf;

  self = GST_WEB_DOWNLOAD (psrc);

  emscripten_sync_run_in_main_runtime_thread (EM_FUNC_SIG_VII,
      (void *) (gst_web_download_create_internal), self, buffer, NULL);

  if (*buffer == NULL)
    return GST_FLOW_ERROR;

  buf = *buffer;
  GST_BUFFER_PTS (buf) = gst_util_uint64_scale_ceil (
      self->n_frames * GST_SECOND, DEFAULT_FPS_D, DEFAULT_FPS_N);
  GST_BUFFER_DURATION (buf) =
      gst_util_uint64_scale (GST_SECOND, DEFAULT_FPS_D, DEFAULT_FPS_N);
  GST_BUFFER_OFFSET (buf) = self->n_frames;

  self->n_frames++;

  return GST_FLOW_OK;
}

static gboolean
gst_web_download_transform_start (GstBaseTransform * bt)
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
gst_web_download_transform_stop (GstBaseTransform * bt)
{
  return TRUE;
}


static GstCaps *
gst_web_download_transform_caps (GstBaseTransform * bt,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (bt);

  GstCaps *tmp, *res;

  GST_DEBUG_OBJECT (self, "Caps: %" GST_PTR_FORMAT ". Direction: %d", caps, (int) direction);
  GST_DEBUG_OBJECT (self, "Filter: %" GST_PTR_FORMAT ". Direction: %d", filter, (int) direction);


  if (direction == GST_PAD_SRC) {
    GstCaps *sys_caps, *video_frame_caps;

    sys_caps = gst_caps_copy (caps);
    gst_caps_set_features_simple (sys_caps,
      gst_caps_features_from_string (GST_CAPS_FEATURE_MEMORY_SYSTEM_MEMORY));
    video_frame_caps = gst_caps_copy (caps);
    gst_caps_set_features_simple (video_frame_caps,
      gst_caps_features_from_string (GST_CAPS_FEATURE_MEMORY_WEB_VIDEO_FRAME));

    tmp = gst_caps_merge (video_frame_caps, sys_caps);
  } else {
    tmp = gst_caps_copy (caps);
    // gst_caps_set_features_simple (tmp,
    //   gst_caps_features_from_string (GST_CAPS_FEATURE_MEMORY_SYSTEM_MEMORY));
  }

  if (filter) {
    res = gst_caps_intersect_full (filter, tmp, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (tmp);
  } else {
    res = tmp;
  }


  GST_DEBUG_OBJECT (self, "Transformed caps to %" GST_PTR_FORMAT ". Direction: %d", res, (int) direction);
  
  return res;
}

static gboolean
gst_web_download_set_caps (GstBaseTransform * bt, GstCaps * in_caps,
    GstCaps * out_caps)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (bt);
  GstCapsFeatures *features = NULL;

  GST_DEBUG_OBJECT (self, "in_caps: %" GST_PTR_FORMAT, in_caps);
  GST_DEBUG_OBJECT (self, "out_caps: %" GST_PTR_FORMAT, out_caps);

  // features = gst_caps_get_features (out_caps, 0);

  // if (gst_caps_features_contains (features, GST_CAPS_FEATURE_MEMORY_WEB_VIDEO_FRAME)) {
  // }

  return TRUE;
}

static GstFlowReturn
gst_web_download_transform (GstBaseTransform * bt,
    GstBuffer * inbuf, GstBuffer * outbuf)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (bt);

  GST_DEBUG_OBJECT (self, "IS PASSTRHOUGH: %d", gst_base_transform_is_passthrough (bt));
  return GST_FLOW_OK;
}


// static GstCaps *
// gst_web_download_get_caps (GstBaseSrc *bsrc, GstCaps *filter)
// {
//   GstWebDownload *self = GST_WEB_DOWNLOAD (bsrc);
//   GstCaps *caps;

//   if (filter) {
//     if (self->caps)
//       caps = gst_caps_intersect_full (
//           filter, self->caps, GST_CAPS_INTERSECT_FIRST);
//     else
//       caps = gst_caps_ref (filter);
//   } else {
//     caps = gst_caps_new_any ();
//   }

//   GST_DEBUG_OBJECT (self, "Returning %" GST_PTR_FORMAT, caps);
//   return caps;
// }

static void
gst_web_download_set_context (
    GstElement *element, GstContext *context)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (element);

  gst_web_utils_element_set_context (element, context, &self->canvas);
}

static GstStateChangeReturn
gst_web_download_change_state (
    GstElement *element, GstStateChange transition)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (element);

  GST_DEBUG_OBJECT (self, "%d -> %d",
      GST_STATE_TRANSITION_CURRENT (transition),
      GST_STATE_TRANSITION_NEXT (transition));

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      self->n_frames = 0;
      break;
    default:
      break;
  }

  return GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
}

static void
gst_web_download_dispose (GObject *object)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (object);

  gst_clear_caps (&self->caps);
  g_clear_pointer (&self->id, g_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_web_download_get_property (
    GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (object);

  switch (prop_id) {
    case PROP_ID:
      g_value_set_string (value, self->id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_web_download_set_property (
    GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  GstWebDownload *self = GST_WEB_DOWNLOAD (object);

  switch (prop_id) {
    case PROP_ID:
      g_free (self->id);
      self->id = g_value_dup_string (value);
      break;
    default:
      break;
  }
}

static void
gst_web_download_class_init (GstWebDownloadClass *klass)
{
  GstBaseTransformClass *gstbasetransform_class;
  GstElementClass *gstelement_class;
  GObjectClass *gobject_class;

  gstbasetransform_class = (GstBaseTransformClass *) klass;
  // gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gobject_class = (GObjectClass *) klass;

  gstbasetransform_class->start = gst_web_download_transform_start;
  gstbasetransform_class->stop = gst_web_download_transform_stop;
  gstbasetransform_class->transform_caps = gst_web_download_transform_caps;
  gstbasetransform_class->set_caps = gst_web_download_set_caps;
  gstbasetransform_class->passthrough_on_same_caps = TRUE;

  gstbasetransform_class->transform = gst_web_download_transform;

  // gstbasetransform_class->create = gst_web_download_create;
  // gstbasesrc_class->get_caps = gst_web_download_get_caps;
  gstelement_class->change_state = gst_web_download_change_state;
  gstelement_class->set_context = gst_web_download_set_context;

  gobject_class->set_property = gst_web_download_set_property;
  gobject_class->get_property = gst_web_download_get_property;
  gobject_class->dispose = gst_web_download_dispose;

  g_object_class_install_property (gobject_class, PROP_ID,
      g_param_spec_string ("id", "id", "id of canvas DOM element", NULL,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS)));

  gst_element_class_set_static_metadata (gstelement_class,
      "Web Canvas test source", "Source/Video",
      "Consumes data from an canvas DOM HTML element.",
      "Fluendo S.A. <engineering@fluendo.com>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_web_download_src_pad_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_web_download_sink_pad_template);

  GST_DEBUG_CATEGORY_INIT (web_download_debug,
      "webdownload", 0, "WebCodecs Video Decoder");
}
