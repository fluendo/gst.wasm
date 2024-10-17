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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <gst/web/gstwebutils.h>

#include "gstwebtransportsrc.h"

using namespace emscripten;

#define GST_TYPE_WEB_TRANSPORT_STREAM_SRC                                     \
  (gst_web_transport_stream_src_get_type ())
#define parent_class gst_web_transport_stream_src_parent_class
#define GST_CAT_DEFAULT gst_web_transport_stream_src_debug

typedef struct _GstWebTransportStreamSrc
{
  GstPushSrc base;
  val reader;
} GstWebTransportStreamSrc;

G_DECLARE_FINAL_TYPE (GstWebTransportStreamSrc, gst_web_transport_stream_src,
    GST, WEB_TRANSPORT_STREAM_SRC, GstPushSrc)
G_DEFINE_TYPE (
    GstWebTransportStreamSrc, gst_web_transport_stream_src, GST_TYPE_PUSH_SRC)

GST_DEBUG_CATEGORY_STATIC (gst_web_transport_stream_src_debug);

static void
gst_web_transport_stream_src_init (GstWebTransportStreamSrc *src)
{
}

static void
gst_web_transport_stream_src_on_stream (guintptr obj, guintptr user_data)
{
  GST_ERROR ("Received stream");
}

static GstFlowReturn
gst_web_transport_stream_src_create (GstPushSrc *psrc, GstBuffer **outbuf)
{
  GstWebTransportStreamSrc *self = GST_WEB_TRANSPORT_STREAM_SRC (psrc);
  GstFlowReturn ret = GST_FLOW_OK;
  static gboolean registered = FALSE;

  /* We have the stream lock taken */
  if (!registered) {
    GstMessage *m;
    /* Register our own JS event handler */
    /* FIXME use the proper name */
    gst_web_utils_js_register_on_message ();
    m = gst_web_utils_message_new_request_object (GST_ELEMENT_CAST (self),
        gst_web_transport_stream_src_on_stream,
        "WebTransportReceiveStream/unidi_0", self);
    /* Request the WebTransportReceiveStream */
    gst_element_post_message (GST_ELEMENT_CAST (self), m);
    /* In a way type: WebTransportReceiveStream, name: element_name */
    /* Get the reader, read from it until data */
  }

  GST_ERROR_OBJECT (psrc, "pushing buffer");
  return ret;
}

static void
gst_web_transport_stream_src_finalize (GObject *obj)
{
  GstWebTransportStreamSrc *self = GST_WEB_TRANSPORT_STREAM_SRC (obj);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_web_transport_stream_src_class_init (GstWebTransportStreamSrcClass *klass)
{

  static GstStaticPadTemplate srcpadtemplate = GST_STATIC_PAD_TEMPLATE (
      "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstPushSrcClass *pushsrc_class = (GstPushSrcClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_web_transport_stream_src_debug,
      "webtransportstreamsrc", 0, "WebTransport Stream Source");

  pushsrc_class->create = gst_web_transport_stream_src_create;
  gst_element_class_add_pad_template (
      element_class, gst_static_pad_template_get (&srcpadtemplate));

  gobject_class->finalize = gst_web_transport_stream_src_finalize;

  gst_element_class_set_static_metadata (element_class,
      "WebTransport Stream Source", "Source/Network",
      "Receives data from the nwtwork using WebTransport API",
      "Jorge Zapata <jzapata@fluendo.com>");
}

GstElement *
gst_web_transport_stream_src_new (void)
{
  GstElement *ret;

  ret = GST_ELEMENT_CAST (
      g_object_new (GST_TYPE_WEB_TRANSPORT_STREAM_SRC, NULL));
  /* TODO set the name */
  return ret;
}
