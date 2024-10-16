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
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <gst/base/gstpushsrc.h>

using namespace emscripten;

typedef struct _GstWebTransportStreamSrc
{
  GstPushSrc base;
} GstWebTransportStreamSrc;

G_DECLARE_FINAL_TYPE (GstWebTransportStreamSrc, gst_web_transport_stream_src,
    GST, WEB_TRANSPORT_STREAM_SRC, GstPushSrc)
G_DEFINE_TYPE_WITH_CODE (GstWebTransportStreamSrc,
    gst_web_transport_stream_src, GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (
        GST_TYPE_URI_HANDLER, gst_web_transport_stream_src_uri_handler_init));
GST_ELEMENT_REGISTER_DEFINE (web_transport_stream_src, "webfetchsrc",
    GST_RANK_SECONDARY, GST_TYPE_WEB_TRANSPORT_STREAM_SRC);

GST_DEBUG_CATEGORY_STATIC (gst_web_transport_stream_src_debug);

static void
gst_web_transport_stream_src_init (GstWebTransportStreamSrc *src)
{
}

static GstFlowReturn
gst_web_transport_stream_src_create (GstPushSrc *psrc, GstBuffer **outbuf)
{
  GstWebTransportStreamSrc *self = GST_WEB_TRANSPORT_STREAM_SRC (psrc);
  GstFlowReturn ret = GST_FLOW_OK;
  gchar *uri;

  /* We have the stream lock taken */
  /* Register our own JS event handler */
  /* Request the WebTransportReceiveStream */
  /* In a way type: WebTransportReceiveStream, name: element_name */
  /* Get the reader, read from it until data */
  return ret;
}

static void
gst_web_transport_stream_src_finalize (GObject *obj)
{
  GstWebTransportStreamSrc *self = GST_WEB_TRANSPORT_STREAM_SRC (obj);

  g_free (self->uri);

  G_OBJECT_CLASS (gst_web_transport_stream_src_parent_class)->finalize (obj);
}

static void
gst_web_transport_stream_src_class_init (GstWebTransportStreamSrcClass *klass)
{

  static GstStaticPadTemplate srcpadtemplate = GST_STATIC_PAD_TEMPLATE (
      "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBaseSrcClass *basesrc_class = (GstBaseSrcClass *) klass;
  GstPushSrcClass *pushsrc_class = (GstPushSrcClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_web_transport_stream_src_debug, "webfetchsrc",
      0, "HTTP Client Source using Web Fetch API");

  pushsrc_class->create = gst_web_transport_stream_src_create;
  gst_element_class_add_pad_template (
      element_class, gst_static_pad_template_get (&srcpadtemplate));

  gobject_class->finalize = gst_web_transport_stream_src_finalize;

  gst_element_class_set_static_metadata (element_class,
      "HTTP Client Source using Web Fetch API", "Source/Network",
      "Receiver data as a client over a network via HTTP using Web Fetch API",
      "Alexander Slobodeniuk <aslobodeniuk@fluendo.com>");
}

GstElement *
gst_web_transport_stream_src_new (void)
{
  GstElement *ret;

  ret = g_object_new (GST_TYPE_WEB_TRANSPORT_STREAM_SRC, NULL);
  /* TODO set the name */
  return ret;
}
