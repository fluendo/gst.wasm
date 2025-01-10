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
#include <gst/web/gstwebtransferable.h>

#include "gstwebtransportstreamsrc.h"

using namespace emscripten;

#define GST_TYPE_WEB_TRANSPORT_STREAM_SRC                                     \
  (gst_web_transport_stream_src_get_type ())
#define parent_class gst_web_transport_stream_src_parent_class
#define GST_CAT_DEFAULT gst_web_transport_stream_src_debug

typedef struct _GstWebTransportStreamSrc
{
  GstPushSrc base;
  gchar *name;
  val stream;
  val reader;
} GstWebTransportStreamSrc;

G_DECLARE_FINAL_TYPE (GstWebTransportStreamSrc, gst_web_transport_stream_src,
    GST, WEB_TRANSPORT_STREAM_SRC, GstPushSrc)

static void
gst_web_transport_stream_src_transferable_init (
    gpointer g_iface, gpointer iface_data)
{
}

G_DEFINE_TYPE_WITH_CODE (GstWebTransportStreamSrc,
    gst_web_transport_stream_src, GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (GST_TYPE_WEB_TRANSFERABLE,
        gst_web_transport_stream_src_transferable_init));

GST_DEBUG_CATEGORY_STATIC (gst_web_transport_stream_src_debug);

static void
gst_web_transport_stream_resume_streaming_thread (
    GstElement *e, gpointer user_data)
{
  GstWebTransportStreamSrc *self = GST_WEB_TRANSPORT_STREAM_SRC (e);
  GST_DEBUG_OBJECT (self, "Resuming Streaming thread");
  gst_pad_resume_task (GST_BASE_SRC_PAD (GST_BASE_SRC_CAST (self)));
  GST_DEBUG_OBJECT (self, "Streaming thread resumed");
}

static void
gst_web_transport_stream_src_on_stream (
    gint sender_tid, val object_name, val object, guintptr user_data)
{
  GstWebTransportStreamSrc *self =
      reinterpret_cast<GstWebTransportStreamSrc *> (user_data);

  GST_DEBUG_OBJECT (self, "Stream %s (%s) received",
      object_name.as<std::string> ().c_str (),
      object.typeOf ().as<std::string> ().c_str ());
  self->reader = object.call<val> ("getReader");
  /* We shouldn't resume from the thread itself, but from other thread */
  gst_element_call_async (GST_ELEMENT_CAST (self),
      gst_web_transport_stream_resume_streaming_thread, NULL, NULL);
}

static GstFlowReturn
gst_web_transport_stream_src_create (GstPushSrc *psrc, GstBuffer **outbuf)
{
  GstWebTransportStreamSrc *self = GST_WEB_TRANSPORT_STREAM_SRC (psrc);
  static gboolean registered = FALSE;

  /* We have the stream lock taken */
  if (!registered) {
    gboolean requested;
    gchar *object_name;

    /* Register our own JS event handler */
    GST_DEBUG_OBJECT (self, "Requesting stream %s", self->name);
    object_name = g_strdup_printf ("WebTransportReceiveStream/%s", self->name);
    gst_web_transferable_register_on_message ((GstWebTransferable *) self);
    /* Request the WebTransportReceiveStream */
    requested =
        gst_web_transferable_request_object ((GstWebTransferable *) self,
            object_name, "gst_web_transport_stream_src_on_stream", self);
    g_free (object_name);
    if (!requested) {
      GST_ERROR_OBJECT (self, "Requesting object failed");
      return GST_FLOW_ERROR;
    }
    registered = TRUE;
    return GST_FLOW_CUSTOM_SUCCESS;
  }

  GST_DEBUG_OBJECT (self, "Waiting for data to be received");
  val res = self->reader.call<val> ("read").await ();
  GST_DEBUG_OBJECT (self, "Data might be received");
  if (res["done"].as<bool> ()) {
    GST_DEBUG_OBJECT (self, "In EOS");
    return GST_FLOW_EOS;
  } else {
    GST_DEBUG_OBJECT (self, "Data received, pushing buffer");
    /* FIXME we are doing a copy of the data, check how to
     * 1. Use the JS uint8array directly as part of the buffer
     * 2. Use a ReadableStreamBYOBReader with the buffer data
     */
    std::string content = res["value"].as<std::string> ();
    *outbuf = gst_buffer_new_wrapped (
        g_memdup2 (content.c_str (), content.length ()), content.length ());
    return GST_FLOW_OK;
  }
}

static void
gst_web_transport_stream_src_finalize (GObject *obj)
{
  GstWebTransportStreamSrc *self = GST_WEB_TRANSPORT_STREAM_SRC (obj);

  g_free (self->name);
  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_web_transport_stream_src_init (GstWebTransportStreamSrc *src)
{
  /* configure basesrc to be a live source */
  gst_base_src_set_live (GST_BASE_SRC (src), TRUE);
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
      "Receives data from the network using WebTransport API",
      "Jorge Zapata <jzapata@fluendo.com>");
}

GstElement *
gst_web_transport_stream_src_new (const gchar *name)
{
  GstWebTransportStreamSrc *self;

  self = GST_WEB_TRANSPORT_STREAM_SRC (
      g_object_new (GST_TYPE_WEB_TRANSPORT_STREAM_SRC, "name", name, NULL));
  self->name = g_strdup (name);
  return GST_ELEMENT_CAST (self);
}

EMSCRIPTEN_BINDINGS (gst_web_transport_stream_src)
{
  function ("gst_web_transport_stream_src_on_stream",
      &gst_web_transport_stream_src_on_stream);
}
