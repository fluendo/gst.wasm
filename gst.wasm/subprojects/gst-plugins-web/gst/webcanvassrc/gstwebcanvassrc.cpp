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
#include <gst/base/gstpushsrc.h>
#include <gst/video/gstvideometa.h>
#include <emscripten/bind.h>
#include <emscripten/threading.h>

using namespace emscripten;

#define GST_TYPE_WEB_CANVAS_SRC (gst_web_canvas_src_get_type ())
#define GST_CAT_DEFAULT web_canvas_src_debug
#define parent_class gst_web_canvas_src_parent_class

#define DEFAULT_IS_LIVE TRUE
#define DEFAULT_FPS_N 30
#define DEFAULT_FPS_D 1

enum
{
  PROP_0,
  PROP_ID,
  PROP_LAST
};

struct _GstWebCanvasSrc
{
  GstPushSrc element;

  /*< private >*/
  gchar *id;
  GstCaps *caps;
  guint n_frames;
};

G_DECLARE_FINAL_TYPE (
    GstWebCanvasSrc, gst_web_canvas_src, GST, WEB_CANVAS_SRC, GstPushSrc)
G_DEFINE_TYPE (GstWebCanvasSrc, gst_web_canvas_src, GST_TYPE_PUSH_SRC);
GST_ELEMENT_REGISTER_DEFINE (
    webcanvassrc, "webcanvassrc", GST_RANK_NONE, GST_TYPE_WEB_CANVAS_SRC);
GST_DEBUG_CATEGORY_STATIC (web_canvas_src_debug);

static GstStaticPadTemplate gst_web_canvas_src_template =
    GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
        GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("RGBA")));

static void
gst_web_canvas_src_init (GstWebCanvasSrc *self)
{
  self->caps = NULL;
  self->n_frames = 0;
  /* we operate in time */
  gst_base_src_set_format (GST_BASE_SRC (self), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (self), DEFAULT_IS_LIVE);
}

static val
gst_web_canvas_src_get_canvas (GstWebCanvasSrc *self)
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
gst_web_canvas_src_buffer_from_canvas (GstWebCanvasSrc *self)
{
  GstBuffer *buf;
  val canvas = gst_web_canvas_src_get_canvas (self);
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
gst_web_canvas_src_update_caps_from_canvas (GstWebCanvasSrc *self)
{
  GstCaps *new_caps;
  val canvas = gst_web_canvas_src_get_canvas (self);

  new_caps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING,
      "RGBA", "framerate", GST_TYPE_FRACTION, DEFAULT_FPS_N, DEFAULT_FPS_D,
      "width", G_TYPE_INT, canvas["width"].as<int> (), "height", G_TYPE_INT,
      canvas["height"].as<int> (), nullptr);

  gst_caps_replace (&self->caps, new_caps);
  g_assert (gst_pad_set_caps (GST_BASE_SRC_PAD (self), new_caps));

  GST_DEBUG_OBJECT (self, "Setting new caps: %" GST_PTR_FORMAT, new_caps);

  gst_caps_unref (new_caps);
}

static void
gst_web_canvas_src_create_internal (GstWebCanvasSrc *self, GstBuffer **buffer)
{
  // TODO: Support canvas dimension changes.
  if (!self->caps)
    gst_web_canvas_src_update_caps_from_canvas (self);

  *buffer = gst_web_canvas_src_buffer_from_canvas (self);
}

static GstFlowReturn
gst_web_canvas_src_create (GstPushSrc *psrc, GstBuffer **buffer)
{
  GstWebCanvasSrc *self;
  GstBuffer *buf;

  self = GST_WEB_CANVAS_SRC (psrc);

  emscripten_sync_run_in_main_runtime_thread (EM_FUNC_SIG_VII,
      (void *) (gst_web_canvas_src_create_internal), self, buffer, NULL);

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

static GstCaps *
gst_web_canvas_src_get_caps (GstBaseSrc *bsrc, GstCaps *filter)
{
  GstWebCanvasSrc *self = GST_WEB_CANVAS_SRC (bsrc);
  GstCaps *caps;

  if (filter) {
    if (self->caps)
      caps = gst_caps_intersect_full (
          filter, self->caps, GST_CAPS_INTERSECT_FIRST);
    else
      caps = gst_caps_ref (filter);
  } else {
    caps = gst_caps_new_any ();
  }

  GST_DEBUG_OBJECT (self, "Returning %" GST_PTR_FORMAT, caps);
  return caps;
}

static GstStateChangeReturn
gst_web_canvas_src_change_state (
    GstElement *element, GstStateChange transition)
{
  GstWebCanvasSrc *self = GST_WEB_CANVAS_SRC (element);

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
gst_web_canvas_src_dispose (GObject *object)
{
  GstWebCanvasSrc *self = GST_WEB_CANVAS_SRC (object);

  gst_clear_caps (&self->caps);
  g_clear_pointer (&self->id, g_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_web_canvas_src_get_property (
    GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  GstWebCanvasSrc *self = GST_WEB_CANVAS_SRC (object);

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
gst_web_canvas_src_set_property (
    GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  GstWebCanvasSrc *self = GST_WEB_CANVAS_SRC (object);

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
gst_web_canvas_src_class_init (GstWebCanvasSrcClass *klass)
{
  GstPushSrcClass *gstpushsrc_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstElementClass *gstelement_class;
  GObjectClass *gobject_class;

  gstpushsrc_class = (GstPushSrcClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gobject_class = (GObjectClass *) klass;

  gstpushsrc_class->create = gst_web_canvas_src_create;
  gstbasesrc_class->get_caps = gst_web_canvas_src_get_caps;
  gstelement_class->change_state = gst_web_canvas_src_change_state;
  gobject_class->set_property = gst_web_canvas_src_set_property;
  gobject_class->get_property = gst_web_canvas_src_get_property;
  gobject_class->dispose = gst_web_canvas_src_dispose;

  g_object_class_install_property (gobject_class, PROP_ID,
      g_param_spec_string ("id", "id", "id of canvas DOM element", NULL,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS)));

  gst_element_class_set_static_metadata (gstelement_class,
      "Web Canvas test source", "Source/Video",
      "Consumes data from an canvas DOM HTML element.",
      "Fluendo S.A. <engineering@fluendo.com>");

  gst_element_class_add_static_pad_template (
      gstelement_class, &gst_web_canvas_src_template);
}

static gboolean
plugin_init (GstPlugin *plugin)
{
  GST_DEBUG_CATEGORY_INIT (
      web_canvas_src_debug, "webcanvassrc", 0, "WebCanvasSrc Source");

  return GST_ELEMENT_REGISTER (webcanvassrc, plugin);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, webcanvassrc,
    "Creates a test video stream", plugin_init, VERSION, GST_LICENSE,
    GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
