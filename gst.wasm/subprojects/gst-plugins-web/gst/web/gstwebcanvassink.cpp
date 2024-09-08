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

/**
 * TODO:
 * - Add the canvas name property
 * - Add support for every kind of memory the context supports:
 *   HTMLImageElement
 *   SVGImageElement
 *   HTMLVideoElement
 *   HTMLCanvasElement
 *   ImageBitmap
 *   OffscreenCanvas
 *   VideoFrame
 * - Support SysyemMemory?
 * - Support events, similar to what we did for gstglwindow_canvas
 * - Support video overlay interface, not for the window itself but
 *   for the render rectangle?
 * - Support video metas?
 * - Resize canvas depending on the input buffer?
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/video/video.h>
#include <gst/video/videooverlay.h>
#include <gst/video/video-color.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <gst/web/gstwebvideoframe.h>
#include <gst/web/gstwebcanvas.h>
#include <gst/web/gstwebutils.h>
#include <gst/web/gstwebrunner.h>

#include "gstweb.h"

using namespace emscripten;

#define GST_TYPE_WEB_CANVAS_SINK (gst_web_canvas_sink_get_type ())
#define GST_WEB_CANVAS_SINK(obj)                                              \
  (G_TYPE_CHECK_INSTANCE_CAST (                                               \
      (obj), GST_TYPE_WEB_CANVAS_SINK, GstWebCanvasSink))
#define GST_WEB_CANVAS_SINK_CLASS(klass)                                      \
  (G_TYPE_CHECK_CLASS_CAST (                                                  \
      (klass), GST_TYPE_WEB_CANVAS_SINK, GstWebCanvasSinkClass))
#define GST_IS_WEB_CANVAS_SINK(obj)                                           \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_WEB_CANVAS_SINK))
#define GST_IS_WEB_CANVAS_SINK_CLASS(klass)                                   \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_WEB_CANVAS_SINK))

#define DEFAULT_CANVAS_ID "#canvas"

typedef struct _GstWebCanvasSink
{
  GstVideoSink base;
  GstWebCanvas *canvas;
  gchar *id;
  val val_context;
  val val_canvas;
} GstWebCanvasSink;

typedef struct _GstWebCanvasSinkClass
{
  GstVideoSinkClass base;
} GstWebCanvasSinkClass;

typedef struct _GstWebCanvasSinkDrawData
{
  GstWebCanvasSink *self;
  GstBuffer *buffer;
} GstWebCanvasSinkDrawData;

typedef struct _GstWebCanvasSinkSetupData
{
  GstWebCanvasSink *self;
} GstWebCanvasSinkSetupData;

enum
{
  PROP_0,
  PROP_ID,
  PROP_LAST
};

#define gst_web_canvas_sink_parent_class parent_class
G_DEFINE_TYPE (GstWebCanvasSink, gst_web_canvas_sink, GST_TYPE_VIDEO_SINK);

GST_ELEMENT_REGISTER_DEFINE (web_canvas_sink, "webcanvassink",
    GST_RANK_SECONDARY, GST_TYPE_WEB_CANVAS_SINK);

static GstStaticPadTemplate static_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
        GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES (
            GST_CAPS_FEATURE_MEMORY_WEB_VIDEO_FRAME,
            GST_WEB_MEMORY_VIDEO_FORMATS_STR)));

static void
gst_web_canvas_sink_draw (gpointer data)
{
  GstWebCanvasSinkDrawData *draw_data = (GstWebCanvasSinkDrawData *) data;
  GstWebCanvasSink *self = draw_data->self;
  GstWebVideoFrame *vf;
  val video_frame;

  GST_DEBUG_OBJECT (self, "About to draw %" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (draw_data->buffer)));
  vf = (GstWebVideoFrame *) gst_buffer_get_memory (draw_data->buffer, 0);
  video_frame = gst_web_video_frame_get_handle (vf);
  self->val_context.call<void> ("drawImage", video_frame, 0, 0,
      video_frame["displayWidth"], video_frame["displayHeight"], 0, 0,
      self->val_canvas["width"], self->val_canvas["height"]);
}

static void
gst_web_canvas_sink_setup (gpointer data)
{
  GstWebCanvasSinkSetupData *setup_data = (GstWebCanvasSinkSetupData *) data;
  GstWebCanvasSink *self = setup_data->self;

  /* FIXME how to handle the case of multiple canvases? */
  self->val_canvas = val::module_property ("canvas");
  self->val_context =
      self->val_canvas.call<val> ("getContext", std::string ("2d"));
}

static GstFlowReturn
gst_web_canvas_sink_show_frame (GstVideoSink *sink, GstBuffer *buf)
{
  GstWebCanvasSink *self = GST_WEB_CANVAS_SINK (sink);
  GstWebCanvasSinkDrawData data;
  GstWebRunner *runner;

  runner = gst_web_canvas_get_runner (self->canvas);
  data.self = self;
  data.buffer = buf;
  gst_web_runner_send_message (runner, gst_web_canvas_sink_draw, &data);

  gst_object_unref (GST_OBJECT (runner));

  return GST_FLOW_OK;
}

static gboolean
gst_web_canvas_sink_start (GstBaseSink *sink)
{
  GstWebCanvasSink *self = GST_WEB_CANVAS_SINK (sink);
  GstWebCanvasSinkSetupData data;
  GstWebRunner *runner = NULL;
  gboolean ret = FALSE;

  /* Ensure that we have a GstWebCanvas context */
  if (!gst_web_utils_element_ensure_canvas (self, &self->canvas, self->id)) {
    GST_ERROR_OBJECT (self, "Failed requesting a WebCanvas context");
    goto done;
  }

  runner = gst_web_canvas_get_runner (self->canvas);
  if (!gst_web_runner_run (runner, NULL)) {
    GST_ERROR_OBJECT (self, "Impossible to run the runner");
    goto done;
  }

  /* TODO pick the context from the canvas */
  data.self = self;
  gst_web_runner_send_message (runner, gst_web_canvas_sink_setup, &data);

  ret = TRUE;

done:
  if (runner)
    gst_object_unref (GST_OBJECT (runner));

  return ret;
}

static void
gst_web_canvas_sink_set_context (GstElement *element, GstContext *context)
{
  GstWebCanvasSink *self = GST_WEB_CANVAS_SINK (element);

  gst_web_utils_element_set_context (element, context, &self->canvas);
}

static gboolean
gst_web_canvas_sink_query (GstElement *element, GstQuery *query)
{
  GstWebCanvasSink *self = GST_WEB_CANVAS_SINK (element);
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
gst_web_canvas_sink_get_property (
    GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  GstWebCanvasSink *src = GST_WEB_CANVAS_SINK (object);

  switch (prop_id) {
    case PROP_ID:
      g_value_set_string (value, src->id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_web_canvas_sink_set_property (
    GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  GstWebCanvasSink *src = GST_WEB_CANVAS_SINK (object);

  switch (prop_id) {
    case PROP_ID:
      g_free (src->id);
      src->id = g_value_dup_string (value);
      break;
    default:
      break;
  }
}

static void
gst_web_canvas_sink_finalize (GObject *object)
{
  GstWebCanvasSink *sink;

  sink = GST_WEB_CANVAS_SINK (object);
  g_clear_pointer (&sink->id, g_free);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_web_canvas_sink_init (GstWebCanvasSink *sink)
{
}

static void
gst_web_canvas_sink_class_init (GstWebCanvasSinkClass *klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstBaseSinkClass *basesink_class;
  GstVideoSinkClass *videosink_class;
  GstCaps *caps;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = gst_web_canvas_sink_set_property;
  gobject_class->get_property = gst_web_canvas_sink_get_property;
  g_object_class_install_property (gobject_class, PROP_ID,
      g_param_spec_string ("id", "id", "id of canvas DOM element",
          DEFAULT_CANVAS_ID,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS)));

  element_class = GST_ELEMENT_CLASS (klass);
  element_class->set_context = gst_web_canvas_sink_set_context;
  element_class->query = gst_web_canvas_sink_query;
  gst_element_class_add_pad_template (
      element_class, gst_static_pad_template_get (&static_sink_template));
  gst_element_class_set_static_metadata (element_class, "Web Canvas sink",
      "Sink/Video", "Video sink for Web Canvas", GST_WEB_AUTHOR);

  basesink_class = GST_BASE_SINK_CLASS (klass);
  basesink_class->start = gst_web_canvas_sink_start;
  videosink_class = GST_VIDEO_SINK_CLASS (klass);
  videosink_class->show_frame = gst_web_canvas_sink_show_frame;

  gobject_class->finalize = gst_web_canvas_sink_finalize;
}