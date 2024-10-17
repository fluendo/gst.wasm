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

/*
 * This is a bin that creates multiple sources depending on the
 * available streams in the WebTransport connection
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "gstwebtransportsrc.h"
#include "gstweb.h"

using namespace emscripten;

#define GST_TYPE_WEB_TRANSPORT_SRC (gst_web_transport_src_get_type ())
#define parent_class gst_web_transport_src_parent_class
#define GST_CAT_DEFAULT gst_web_transport_src_debug

typedef struct _GstWebTransportSrc
{
  GstBin base;
  gchar *uri;
  val transport;
  std::unordered_map<std::string, val> streams;

  GThread *process;
  GError *process_err;
} GstWebTransportSrc;

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_MAX
};

GST_DEBUG_CATEGORY_STATIC (gst_web_transport_src_debug);

G_DECLARE_FINAL_TYPE (
    GstWebTransportSrc, gst_web_transport_src, GST, WEB_TRANSPORT_SRC, GstBin)

static GstURIType
gst_web_transport_src_urihandler_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_web_transport_src_urihandler_get_protocols (GType type)
{
  static const gchar *protocols[] = { "https", NULL };

  return protocols;
}

static gboolean
gst_web_transport_src_urihandler_set_uri (
    GstURIHandler *handler, const gchar *uri, GError **error)
{
  GstWebTransportSrc *self = GST_WEB_TRANSPORT_SRC (handler);

  g_return_val_if_fail (GST_IS_URI_HANDLER (handler), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  GST_OBJECT_LOCK (self);
  g_free (self->uri);
  self->uri = g_strdup (uri);
  GST_OBJECT_UNLOCK (self);

  return TRUE;
}

static gchar *
gst_web_transport_src_urihandler_get_uri (GstURIHandler *handler)
{
  GstWebTransportSrc *self;
  gchar *ret;

  g_return_val_if_fail (GST_IS_URI_HANDLER (handler), NULL);
  self = GST_WEB_TRANSPORT_SRC (handler);

  GST_OBJECT_LOCK (self);
  ret = g_strdup (self->uri);
  GST_OBJECT_UNLOCK (self);

  return ret;
}

static void
gst_web_transport_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *uri_iface = (GstURIHandlerInterface *) g_iface;

  uri_iface->get_type = gst_web_transport_src_urihandler_get_type;
  uri_iface->get_protocols = gst_web_transport_src_urihandler_get_protocols;
  uri_iface->get_uri = gst_web_transport_src_urihandler_get_uri;
  uri_iface->set_uri = gst_web_transport_src_urihandler_set_uri;
}

G_DEFINE_TYPE_WITH_CODE (GstWebTransportSrc, gst_web_transport_src,
    GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE (
        GST_TYPE_URI_HANDLER, gst_web_transport_src_uri_handler_init));

GST_ELEMENT_REGISTER_DEFINE (web_transport_src, "webtransportsrc",
    GST_RANK_SECONDARY, GST_TYPE_WEB_TRANSPORT_SRC);

static GstStaticPadTemplate gst_web_transport_src_unidi_template =
    GST_STATIC_PAD_TEMPLATE (
        "unidi_%u", GST_PAD_SRC, GST_PAD_SOMETIMES, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_web_transport_src_bidi_template =
    GST_STATIC_PAD_TEMPLATE (
        "bidi_%u", GST_PAD_SRC, GST_PAD_SOMETIMES, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_web_transport_src_datagram_template =
    GST_STATIC_PAD_TEMPLATE (
        "datagram_%u", GST_PAD_SRC, GST_PAD_SOMETIMES, GST_STATIC_CAPS_ANY);

#if 0
static void
pthreads_check2 (void)
{
  /* clang-format off */
  EM_ASM (
      {
        addEventListener (
            "message", (e) => {
              console.error ("Message received from parent:");
              console.error (e);
            });
      },
      100);

  MAIN_THREAD_EM_ASM({
    var worker = PThread.pthreads[$0];
    worker.addEventListener("message", (e) => {
      console.error("Message received from worker:");
      console.error(e);
      e.target.postMessage ({cmd: "Bye", d: "bye"});
    });
  }, pthread_self());

  EM_ASM({
    postMessage ({cmd: "Hello", d: "foo"});
  });
  /* clang-format on */
}
#endif

/* FIXME This should belong to a generic lib, for now, we place it here until
 * more elements actually use this
 */
static void
gst_web_transport_src_register_main_thread_events (void)
{
  /* Add a method to handle events which will process transferring objects from
   * one worker to another based on the pthread id.
   * TODO later instead of fetching the pthread id, use the actual GThread
   * pointer and keep a list similar to what Emscripten does for pthreads
   */
  /* clang-format off */
  MAIN_THREAD_EM_ASM ({
    self.addEventListener (
        "message", (e) => {
          console.error ("Message received on the main thread");
          console.error (e);
          let msgData = e.data;
          let cmd = msgData["cmd"];
          let data = msgData["data"];
          if (cmd && cmd == "transferToWorker") {
            /* The data is in the form of:
             * to: tid (Thread id to transfer the object to), object: object
             * (The transferable object)
             */

            /* Get the workers for each thread */
            /* We received the object on the main thread, we own it */
            /* Transfer it to the corresponding worker */
          }
        });
  });
  /* clang-format on */
}

typedef enum _GstWebTransportSrcStreamType
{
  GST_WEB_TRANSPORT_SRC_STREAM_TYPE_UNI,
  GST_WEB_TRANSPORT_SRC_STREAM_TYPE_BIDI,
  GST_WEB_TRANSPORT_SRC_STREAM_TYPE_DATAGRAM,
} GstWebTransportSrcStreamType;

const gchar *gst_web_transport_src_stream_names[] = { "unidi", "bidi",
  "datagram" };

static gboolean
gst_web_transport_src_stream_link (
    GstElement *element, GstPad *pad, gpointer user_data)
{
  GstGhostPad *ghost = GST_GHOST_PAD (user_data);
  gst_ghost_pad_set_target (ghost, pad);
  return FALSE;
}

static void
gst_web_transport_src_stream_linked (
    GstPad *pad, GstPad *peer, gpointer user_data)
{
  GstWebTransportSrc *self = GST_WEB_TRANSPORT_SRC (user_data);
  GstElement *src;

  GST_ERROR ("Pad linked");
  src = gst_web_transport_stream_src_new ();
  gst_bin_add (GST_BIN (self), src);
  gst_element_foreach_src_pad (src, gst_web_transport_src_stream_link, pad);
  gst_element_sync_state_with_parent (src);
  GST_ERROR ("Pad linked done");
}

/* Create a new pad and store the stream */
/* We use basic types to avoid the 'cannot call xxx due to unbound types' */
static void
gst_web_transport_src_add_stream (
    gint self_ptr, gint type_int, gint num, val stream)
{
  GstWebTransportSrc *self = reinterpret_cast<GstWebTransportSrc *> (self_ptr);
  GstPad *pad;

  GST_ERROR_OBJECT (self, "Received stream");
  std::string stream_name = gst_web_transport_src_stream_names[type_int];
  stream_name.append ("_");
  stream_name.append (std::to_string (num));

  GST_ERROR_OBJECT (self, "Stream %s received", stream_name.c_str ());
  self->streams.insert ({ stream_name, stream });
  GST_ERROR_OBJECT (self, "Stream added");
  pad = gst_ghost_pad_new_no_target (stream_name.c_str (), GST_PAD_SRC);
  g_signal_connect (
      pad, "linked", (GCallback) gst_web_transport_src_stream_linked, self);
  gst_element_add_pad (GST_ELEMENT (self), pad);
}

EMSCRIPTEN_BINDINGS (gst_web_transport_src)
{
  function (
      "gst_web_transport_src_add_stream", &gst_web_transport_src_add_stream);
}

static void
gst_web_transport_src_wait_streams (GstWebTransportSrc *self)
{
  /* clang-format off */
  EM_ASM (({
    /* We handle it through Asyncify as explained in
     * https://github.com/emscripten-core/emscripten/issues/8991
     * This requires the -sASINCIFY
     */
    Asyncify.handleAsync(async () => {
      let t = Emval.toValue($0);
      let uds = t.incomingUnidirectionalStreams;
      let bds = t.incomingBidirectionalStreams;
      let streamNumbers = [ 0, 0 ];
      let streamReaders = [ uds.getReader (), bds.getReader () ];
      let streamPromises = [];
      streamReaders.forEach ((r) => streamPromises.push (r.read ()));
      while (true) {
        const promiseAnyIndexed = pp => Promise.any (pp.map ((p, i) => p.then (res => [ res, i ])));
        let res, i;
        try {
          [res, i] = await promiseAnyIndexed (streamPromises);
        } catch (error) {
          break;
        }
        /* Increment the number of streams for that particular type */
        if (res['done'])
          streamPromises[i] = Promise.reject ();
        else
          streamPromises[i] = streamReaders[i].read ();
        /* Transfer the stream to the new children's stream thread */
        Module.gst_web_transport_src_add_stream ($1, i, streamNumbers[i], res["value"]);
        streamNumbers[i]++;
      }
    });
  }), self->transport.as_handle (), reinterpret_cast<int> (self));
  /* clang-format on */
}

static gpointer
gst_web_transport_src_process (GstWebTransportSrc *self)
{
  val wtclass;

  wtclass = val::global ("WebTransport");
  if (!wtclass.as<bool> ()) {
    GST_ERROR ("No global WebTransport");
    return NULL;
  }

  self->transport = wtclass.new_ (std::string (self->uri));
  self->transport["ready"].await ();

  GST_ERROR ("Process");
  gst_web_transport_src_wait_streams (self);
  GST_ERROR ("Process done");
  return NULL;
}

static gboolean
gst_web_transport_src_start (GstWebTransportSrc *src)
{
  GstWebTransportSrc *self = GST_WEB_TRANSPORT_SRC (src);

  /* Create our own thread to listen for any incoming stream
   * TODO Once we have a webtransportsink implemented we might need to share
   * the connection among the two elements. We'll do that by using a GstContext
   * GstWebTransport context
   */
  self->process = g_thread_new (
      "wt-process", (GThreadFunc) gst_web_transport_src_process, self);
#if 0
  while (!self->priv->created)
    g_cond_wait (&self->priv->create_cond, &self->priv->create_lock);
#endif

  return TRUE;
}

static void
gst_web_transport_src_stop (GstWebTransportSrc *src)
{
  GstWebTransportSrc *self = GST_WEB_TRANSPORT_SRC (src);

  GST_INFO_OBJECT (self, "joining thread");
  /* TODO */
}

static void
gst_web_transport_src_handle_message (GstBin *bin, GstMessage *message)
{
  /* Process the request js object message */
  /* In case the requested object corresponds to us postMessage the object */
  /* Get it from the unordered map, remove it from the unordered map */
  GST_BIN_CLASS (parent_class)->handle_message (bin, message);
}

static GstStateChangeReturn
gst_web_transport_src_change_state (
    GstElement *element, GstStateChange transition)
{
  GstWebTransportSrc *self = GST_WEB_TRANSPORT_SRC (element);
  GstStateChangeReturn ret;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (self->uri == NULL) {
        GST_ELEMENT_ERROR (
            element, RESOURCE, OPEN_READ, ("No URL set."), ("Missing URL"));
        return GST_STATE_CHANGE_FAILURE;
      }
      if (!gst_web_transport_src_start (self))
        return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_web_transport_src_stop (self);
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_web_transport_src_set_property (
    GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  GstWebTransportSrc *self = GST_WEB_TRANSPORT_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      gst_web_transport_src_urihandler_set_uri (
          GST_URI_HANDLER (self), g_value_get_string (value), NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_web_transport_src_get_property (
    GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  GstWebTransportSrc *self = GST_WEB_TRANSPORT_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      g_value_take_string (value,
          gst_web_transport_src_urihandler_get_uri (GST_URI_HANDLER (self)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_web_transport_src_finalize (GObject *obj)
{
  GstWebTransportSrc *self = GST_WEB_TRANSPORT_SRC (obj);

  g_free (self->uri);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_web_transport_src_init (GstWebTransportSrc *self)
{
  self->streams = std::unordered_map<std::string, val> ();

  /* FIXME for now */
  gst_web_transport_src_register_main_thread_events ();
}

static void
gst_web_transport_src_class_init (GstWebTransportSrcClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBinClass *bin_class = (GstBinClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_web_transport_src_debug, "webtransportsrc", 0,
      "HTTP/3 WebTransport Source");

  bin_class->handle_message = gst_web_transport_src_handle_message;

  element_class->change_state = gst_web_transport_src_change_state;

  gst_element_class_add_static_pad_template (
      element_class, &gst_web_transport_src_unidi_template);
  gst_element_class_add_static_pad_template (
      element_class, &gst_web_transport_src_bidi_template);
  gst_element_class_add_static_pad_template (
      element_class, &gst_web_transport_src_datagram_template);

  gobject_class->set_property = gst_web_transport_src_set_property;
  gobject_class->get_property = gst_web_transport_src_get_property;
  gobject_class->finalize = gst_web_transport_src_finalize;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "Location", "URI of resource to read",
          NULL, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  gst_element_class_set_static_metadata (element_class,
      "HTTP/3 WebTransport Source", "Source/Network",
      "Receiver data as a client over a network via HTTP/3 using WebTransport "
      "API",
      GST_WEB_AUTHOR);
}
