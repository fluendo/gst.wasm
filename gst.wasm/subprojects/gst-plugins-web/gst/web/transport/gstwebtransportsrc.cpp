/*
 * GStreamer - gst.wasm WebTransport source
 *
 * Copyright 2024 Fluendo S.A.
 * @author: Jorge Zapata <jzapata@fluendo.com>
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

/*
 * This is a bin that creates multiple sources depending on the
 * available streams in the WebTransport connection
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gst/gst.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <gst/web/gstwebtransferable.h>
#include <gst/web/gstwebrunner.h>
#include <gst/web/gstwebtask.h>
#include <gst/web/gstwebtaskpool.h>

#include "gstwebtransportsrc.h"
#include "gstwebtransportstreamsrc.h"
#include "gstweb.h"

using namespace emscripten;

#define GST_TYPE_WEB_TRANSPORT_SRC (gst_web_transport_src_get_type ())
#define parent_class gst_web_transport_src_parent_class
#define GST_CAT_DEFAULT gst_web_transport_src_debug

typedef enum _GstWebTransportSrcStreamType
{
  GST_WEB_TRANSPORT_SRC_STREAM_TYPE_UNI,
  GST_WEB_TRANSPORT_SRC_STREAM_TYPE_BIDI,
  GST_WEB_TRANSPORT_SRC_STREAM_TYPE_DATAGRAM,
  GST_WEB_TRANSPORT_SRC_STREAM_TYPES
} GstWebTransportSrcStreamType;

typedef struct _GstWebTransportSrc
{
  GstBin base;
  gchar *uri;
  GValue hashes;
  GstWebRunner *runner;

  /* temporary property values until the connection is created */
  gint incoming_high_water_mark;

  val transport;                                // owner: runner
  std::unordered_map<std::string, val> streams; // owner: runner
  val stream_readers[2];                        // owner: runner
  val stream_promises[2];                       // owner: runner
  gint nstreams[2];                             // owner: runner
} GstWebTransportSrc;

typedef struct _GstWebTransportSrcRequestObjectMsgData
{
  GstWebTransportSrc *self;
  GstMessage *msg;
} GstWebTransportSrcRequestObjectMsgData;

/* FIXME we use this for setting/getting a single property. If there
 * are more properties to set, wrap the actual generic property_set/get
 * into a message, instead of doing it per property
 */
typedef struct _GstWebTransportSrcDatagramsIncomingHighWaterMarkMsgData
{
  GstWebTransportSrc *self;
  gint value;
} GstWebTransportSrcDatagramsIncomingHighWaterMarkMsgData;

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_SERVER_CERTIFICATE_HASHES,
  PROP_DATAGRAMS_INCOMING_HIGH_WATER_MARK,
  PROP_MAX
};

static void gst_web_transport_src_check_streams (GstWebTransportSrc *self);

GST_DEBUG_CATEGORY_STATIC (gst_web_transport_src_debug);

G_DECLARE_FINAL_TYPE (
    GstWebTransportSrc, gst_web_transport_src, GST, WEB_TRANSPORT_SRC, GstBin)

static void
gst_web_transport_src_request_object_msg_data_free (
    GstWebTransportSrcRequestObjectMsgData *data)
{
  gst_message_unref (data->msg);
  g_free (data);
}

static void
gst_web_transport_src_request_object_msg (
    GstWebTransportSrcRequestObjectMsgData *data)
{
  GstWebTransportSrc *self = data->self;
  const GstStructure *s = gst_message_get_structure (data->msg);
  const gchar *object_name;
  val object;

  object_name = gst_structure_get_string (s, "object-name");
  GST_DEBUG_OBJECT (
      data->self, "Processing the request object of '%s'", object_name);
  if (!strncmp (object_name, "WebTransportReceiveStream/", 25)) {
    const gchar *stream_name =
        &object_name[26]; // the lenght of the above string
    /* FIXME how to handle the case of a stream not found */
    object = self->streams[stream_name];
  } else {
    GST_ERROR_OBJECT (self, "Unsupported object '%s'", object_name);
    return;
  }
  gst_web_transferable_transfer_object (
      (GstWebTransferable *) self, data->msg, (guintptr) object.as_handle ());
}

static void
gst_web_transport_src_set_datagrams_incoming_high_water_mark_msg (
    GstWebTransportSrcDatagramsIncomingHighWaterMarkMsgData *data)
{
  GstWebTransportSrc *self = data->self;

  self->transport["datagrams"].set ("incomingHighWaterMark", data->value);
}

static void
gst_web_transport_src_get_datagrams_incoming_high_water_mark_msg (
    GstWebTransportSrcDatagramsIncomingHighWaterMarkMsgData *data)
{
  GstWebTransportSrc *self = data->self;

  data->value =
      self->transport["datagrams"]["incomingHighWaterMark"].as<int> ();
}

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

static gboolean
gst_web_transport_src_transferable_can_transfer (
    GstWebTransferable *transferable, const gchar *object_name)
{
  GstWebTransportSrc *self;

  g_return_val_if_fail (GST_IS_WEB_TRANSFERABLE (transferable), FALSE);
  self = GST_WEB_TRANSPORT_SRC (transferable);

  GST_DEBUG_OBJECT (self, "Requesting object %s", object_name);
  /* Check if the requested object corresponds to us */
  if (strncmp (object_name, "WebTransportReceiveStream/", 25))
    return FALSE;

  return TRUE;
}

static void
gst_web_transport_src_transferable_transfer (GstWebTransferable *transferable,
    const gchar *object_name, GstMessage *msg)
{
  GstWebTransportSrc *self;
  GstWebTransportSrcRequestObjectMsgData *data;

  g_return_if_fail (GST_IS_WEB_TRANSFERABLE (transferable));
  self = GST_WEB_TRANSPORT_SRC (transferable);

  data = g_new0 (GstWebTransportSrcRequestObjectMsgData, 1);
  data->msg = gst_message_ref (msg);
  data->self = self;
  gst_web_runner_send_message_async (self->runner,
      (GstWebRunnerCB) gst_web_transport_src_request_object_msg, data,
      (GDestroyNotify) gst_web_transport_src_request_object_msg_data_free);
}

static void
gst_web_transport_src_transferable_init (gpointer g_iface, gpointer iface_data)
{
  GstWebTransferableInterface *iface = (GstWebTransferableInterface *) g_iface;

  iface->can_transfer = gst_web_transport_src_transferable_can_transfer;
  iface->transfer = gst_web_transport_src_transferable_transfer;
}

G_DEFINE_TYPE_WITH_CODE (GstWebTransportSrc, gst_web_transport_src,
                         GST_TYPE_BIN,
                         G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
                             gst_web_transport_src_uri_handler_init);
                         G_IMPLEMENT_INTERFACE (GST_TYPE_WEB_TRANSFERABLE,
                             gst_web_transport_src_transferable_init));

GST_ELEMENT_REGISTER_DEFINE (web_transport_src, "webtransportsrc",
    GST_RANK_SECONDARY, GST_TYPE_WEB_TRANSPORT_SRC);

static GstStaticPadTemplate gst_web_transport_src_unidi_template =
    GST_STATIC_PAD_TEMPLATE (
        "unidi_%u", GST_PAD_SRC, GST_PAD_SOMETIMES, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_web_transport_src_bidi_template =
    GST_STATIC_PAD_TEMPLATE (
        "bidi_%u", GST_PAD_SRC, GST_PAD_SOMETIMES, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_web_transport_src_rbidi_template =
    GST_STATIC_PAD_TEMPLATE (
        "rbidi_%u", GST_PAD_SRC, GST_PAD_REQUEST, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_web_transport_src_datagram_template =
    GST_STATIC_PAD_TEMPLATE (
        "datagram", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static void
gst_web_transport_src_stream_linked (
    GstPad *pad, GstPad *peer, gpointer user_data)
{
  GstWebTransportSrc *self = GST_WEB_TRANSPORT_SRC (user_data);
  GstPad *target;
  GstElement *src;

  GST_DEBUG_OBJECT (self, "Stream linked to %s, setup the stream src",
      GST_OBJECT_NAME (pad));
  src = gst_web_transport_stream_src_new (GST_OBJECT_NAME (pad));
  gst_bin_add (GST_BIN (self), src);
  target = gst_element_get_static_pad (src, "src");
  gst_ghost_pad_set_target (GST_GHOST_PAD (pad), target);
  gst_element_sync_state_with_parent (src);
}

static gboolean
gst_web_transport_src_create_stream (
    GstElement *element, GstPad *pad, gpointer user_data)
{
  GstWebTransportSrc *self = GST_WEB_TRANSPORT_SRC (element);
  val stream;

  if (!g_strcmp0 (GST_PAD_NAME (pad), "datagram")) {
    stream = self->transport["datagrams"]["readable"];
  } else {
    /* TODO support the sendOrder option */
    stream = self->transport.call<val> ("createBidirectionalStream")
                 .await ()["readable"];
  }

  self->streams.insert ({ std::string (GST_PAD_NAME (pad)), stream });
  GST_INFO_OBJECT (self, "Stream %s created", GST_PAD_NAME (pad));

  return TRUE;
}

/* Create a new pad and store the stream
 * This is called from the reader promise context
 */
static void
gst_web_transport_src_add_stream (gint self_ptr, gint type_int, val stream)
{
  GstWebTransportSrc *self = reinterpret_cast<GstWebTransportSrc *> (self_ptr);
  GstPad *pad;
  const gchar *gst_web_transport_src_stream_names[] = { "unidi", "bidi",
    "datagram" };

  /* Update the promise to read again for a new stream */
  self->stream_promises[type_int] =
      self->stream_readers[type_int].call<val> ("read");

  std::string stream_name = gst_web_transport_src_stream_names[type_int];
  stream_name.append ("_");
  stream_name.append (std::to_string (self->nstreams[type_int]));
  self->nstreams[type_int]++;

  GST_INFO_OBJECT (self, "Stream %s received", stream_name.c_str ());
  self->streams.insert ({ stream_name, stream });
  pad = gst_ghost_pad_new_no_target (stream_name.c_str (), GST_PAD_SRC);
  g_signal_connect (
      pad, "linked", (GCallback) gst_web_transport_src_stream_linked, self);
  gst_element_add_pad (GST_ELEMENT (self), pad);
  /* Check for new streams again */
  gst_web_transport_src_check_streams (self);
}

static void
gst_web_transport_src_end_stream (gint self_ptr, gint type_int)
{
  GstWebTransportSrc *self = reinterpret_cast<GstWebTransportSrc *> (self_ptr);
  val promiseclass;

  promiseclass = val::global ("Promise");
  self->stream_promises[type_int] = promiseclass.call<val> ("reject");
  gst_web_transport_src_check_streams (self);
}

static void
gst_web_transport_src_check_streams (GstWebTransportSrc *self)
{
  GST_DEBUG_OBJECT (self, "ReadableStreams ready to be read");
  /* clang-format off */
  EM_ASM (({
    let udp = Emval.toValue ($1);
    let bdp = Emval.toValue ($2);
    let streamPromises = [ udp, bdp ];
    const promiseAnyIndexed = pp => Promise.any (pp.map ((p, i) => p.then (res => [ res, i ])));
    promiseAnyIndexed (streamPromises).then(
      ([res, i]) => {
        if (res["done"])
          Module["gst_web_transport_src_end_stream"] ($0, i);
        else
          Module["gst_web_transport_src_add_stream"] ($0, i, res["value"]);
      },
      (e) => {
        /* No pending promises, all read */
        console.error (e);
      }
    );
  }), reinterpret_cast<guintptr> (self),
      self->stream_promises[0].as_handle (), self->stream_promises[1].as_handle ());
  /* clang-format on */
}

static gint
gst_web_transport_src_get_datagrams_incoming_high_water_mark (
    GstWebTransportSrc *self)
{
  if (!self->runner)
    return self->incoming_high_water_mark;
  else {
    GstWebTransportSrcDatagramsIncomingHighWaterMarkMsgData data;

    data.self = self;
    data.value = 0;
    gst_web_runner_send_message (self->runner,
        (GstWebRunnerCB)
            gst_web_transport_src_get_datagrams_incoming_high_water_mark_msg,
        &data);
    return data.value;
  }
}

static void
gst_web_transport_src_set_datagrams_incoming_high_water_mark (
    GstWebTransportSrc *self, gint value)
{
  if (!self->runner)
    self->incoming_high_water_mark = value;
  else {
    GstWebTransportSrcDatagramsIncomingHighWaterMarkMsgData data;

    data.self = self;
    data.value = value;
    gst_web_runner_send_message (self->runner,
        (GstWebRunnerCB)
            gst_web_transport_src_set_datagrams_incoming_high_water_mark_msg,
        &data);
  }
}

static void
gst_web_transport_src_create_connection (GstWebTransportSrc *self)
{
  gint i;
  val wtclass;
  val unireader;
  val bidireader;
  val dgramreader;
  val options;
  val server_certs;

  wtclass = val::global ("WebTransport");
  if (!wtclass.as<bool> ()) {
    GST_ERROR ("No global WebTransport");
    return;
  }

  options = val::object ();
  /* Check the hashes */
  if (gst_value_array_get_size (&self->hashes)) {
    server_certs = val::global ("Array").new_ ();

    for (i = 0; i < gst_value_array_get_size (&self->hashes); i++) {
      const GValue *ai = gst_value_array_get_value (&self->hashes, i);
      const gchar *ais = g_value_get_string (ai);
      val cert = val::object ();
      val value =
          val::global ("Uint8Array").call<val> ("from", std::string (ais));

      cert.set ("algorithm", std::string ("sha-256"));
      cert.set ("value", value);
      server_certs.call<void> ("push", cert);
    }
    options.set ("serverCertificateHashes", server_certs);
  }

  self->transport = wtclass.new_ (std::string (self->uri), options);
  self->transport["ready"].await ();

  /* Propagate the properties */
  self->transport["datagrams"].set (
      "incomingHighWaterMark", self->incoming_high_water_mark);

  /* The requested streams */
  gst_element_foreach_src_pad (
      GST_ELEMENT_CAST (self), gst_web_transport_src_create_stream, NULL);

  /* The incoming streams */
  self->stream_readers[0] =
      self->transport["incomingUnidirectionalStreams"].call<val> ("getReader");
  self->stream_readers[1] =
      self->transport["incomingBidirectionalStreams"].call<val> ("getReader");
  for (i = 0; i < 2; i++) {
    self->stream_promises[i] = self->stream_readers[i].call<val> ("read");
  }
  gst_web_transferable_register_on_message ((GstWebTransferable *) self);
  gst_web_transport_src_check_streams (self);
}

static void
gst_web_transport_src_destroy_connection (GstWebTransportSrc *self)
{
  /* TODO
   * close every stream
   * close the transport
   */
  gst_web_transferable_unregister_on_message ((GstWebTransferable *) self);
}

static gboolean
gst_web_transport_src_start (GstWebTransportSrc *self)
{
  GST_DEBUG_OBJECT (self, "Starting");
  /* TODO Once we have a webtransportsink implemented we might need to share
   * the connection among the two elements. We'll do that by using a GstContext
   * GstWebTransport context
   */
  self->runner = gst_web_runner_new (NULL);
  gst_web_runner_run (self->runner, NULL);
  gst_web_runner_send_message (self->runner,
      (GstWebRunnerCB) gst_web_transport_src_create_connection, self);

  return TRUE;
}

static void
gst_web_transport_src_stop (GstWebTransportSrc *src)
{
  GstWebTransportSrc *self = GST_WEB_TRANSPORT_SRC (src);

  GST_DEBUG_OBJECT (self, "Stopping");
  gst_web_runner_send_message (self->runner,
      (GstWebRunnerCB) gst_web_transport_src_destroy_connection, self);
  /* TODO
   * destroy the runner
   */
}

static void
gst_web_transport_src_handle_message (GstBin *bin, GstMessage *msg)
{
  GstWebTransportSrc *self = GST_WEB_TRANSPORT_SRC (bin);
  gboolean handled = FALSE;

  GST_LOG_OBJECT (bin, "Handle message of type %s",
      gst_message_type_get_name (GST_MESSAGE_TYPE (msg)));

  if ((handled = gst_web_transferable_handle_request_object (
           (GstWebTransferable *) bin, msg)))
    goto done;

  /* Process the request js object message */
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_STREAM_STATUS: {
      GstStreamStatusType type;
      GstElement *owner;
      GstPad *pad;
      GstWebTask *wtask;
      GstWebTaskPool *wtaskpool;
      const GValue *val;
      GstTask *task = NULL;

      gst_message_parse_stream_status (msg, &type, &owner);
      pad = GST_PAD (GST_MESSAGE_SRC (msg));
      if (type != GST_STREAM_STATUS_TYPE_CREATE)
        break;

      val = gst_message_get_stream_status_object (msg);
      if (G_VALUE_TYPE (val) != GST_TYPE_TASK)
        break;

      task = GST_TASK (g_value_get_object (val));
      GST_DEBUG_OBJECT (
          self, "New task created, replacing it with a GstWebTask");
      wtask = gst_web_task_new (task->func, task->user_data, task->notify);
      wtaskpool = gst_web_task_pool_new ();
      gst_task_set_pool (
          GST_TASK_CAST (wtask), GST_TASK_POOL_CAST (wtaskpool));
      gst_pad_set_task (pad, GST_TASK_CAST (wtask));
      handled = TRUE;
    } break;

    default:
      break;
  }

done:
  if (!handled)
    GST_BIN_CLASS (parent_class)->handle_message (bin, msg);
  else
    gst_message_unref (msg);
}

static GstPad *
gst_web_transport_src_request_new_pad (GstElement *element,
    GstPadTemplate *templ, const gchar *name, const GstCaps *caps)
{
  GstWebTransportSrc *self = GST_WEB_TRANSPORT_SRC (element);
  GstPad *pad;

  GST_INFO_OBJECT (self, "Requesting new pad %s", name);
  pad = gst_ghost_pad_new_no_target (name, GST_PAD_SRC);
  g_signal_connect (
      pad, "linked", (GCallback) gst_web_transport_src_stream_linked, self);
  gst_element_add_pad (element, pad);
  /* TODO handle the case of requesting a pad when the element has already
   * established the connection
   */
  return pad;
}

static void
gst_web_transport_src_release_pad (GstElement *element, GstPad *pad)
{
  GST_FIXME_OBJECT (element, "Release pad");
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
    case PROP_SERVER_CERTIFICATE_HASHES:
      g_value_copy (value, &self->hashes);
      break;
    case PROP_DATAGRAMS_INCOMING_HIGH_WATER_MARK:
      gst_web_transport_src_set_datagrams_incoming_high_water_mark (
          self, g_value_get_int (value));
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
    case PROP_SERVER_CERTIFICATE_HASHES:
      g_value_copy (&self->hashes, value);
      break;
    case PROP_DATAGRAMS_INCOMING_HIGH_WATER_MARK:
      g_value_set_int (value,
          gst_web_transport_src_get_datagrams_incoming_high_water_mark (self));
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

  g_value_unset (&self->hashes);
  g_free (self->uri);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_web_transport_src_init (GstWebTransportSrc *self)
{
  GstPadTemplate *pad_template;
  GstElementClass *element_class;
  GstPad *pad;

  GST_DEBUG_OBJECT (self, "Creating datagram pad");
  element_class = GST_ELEMENT_GET_CLASS (self);
  pad_template =
      gst_element_class_get_pad_template (element_class, "datagram");
  pad = gst_ghost_pad_new_no_target_from_template ("datagram", pad_template);
  g_signal_connect (
      pad, "linked", (GCallback) gst_web_transport_src_stream_linked, self);
  gst_element_add_pad (GST_ELEMENT_CAST (self), pad);

  g_value_init (&self->hashes, GST_TYPE_ARRAY);
  self->transport = val::undefined ();
  self->streams = std::unordered_map<std::string, val> ();
  self->nstreams[0] = self->nstreams[1] = 0;
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
  element_class->request_new_pad = gst_web_transport_src_request_new_pad;
  element_class->release_pad = gst_web_transport_src_release_pad;

  gst_element_class_add_static_pad_template (
      element_class, &gst_web_transport_src_unidi_template);
  gst_element_class_add_static_pad_template (
      element_class, &gst_web_transport_src_bidi_template);
  gst_element_class_add_static_pad_template (
      element_class, &gst_web_transport_src_rbidi_template);
  gst_element_class_add_static_pad_template (
      element_class, &gst_web_transport_src_datagram_template);
  gst_element_class_set_static_metadata (element_class,
      "HTTP/3 WebTransport Source", "Source/Network",
      "Receiver data as a client over a network via HTTP/3 using WebTransport "
      "API",
      GST_WEB_AUTHOR);

  gobject_class->set_property = gst_web_transport_src_set_property;
  gobject_class->get_property = gst_web_transport_src_get_property;
  gobject_class->finalize = gst_web_transport_src_finalize;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "Location", "URI of resource to read",
          NULL,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                         GST_PARAM_MUTABLE_READY)));
  g_object_class_install_property (gobject_class,
      PROP_DATAGRAMS_INCOMING_HIGH_WATER_MARK,
      g_param_spec_int ("datagrams-incoming-high-water-mark",
          "Datagrams Incoming High Water Mark",
          "High water mark for incoming chunks of data", 1, G_MAXINT, 1,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class,
      PROP_SERVER_CERTIFICATE_HASHES,
      gst_param_spec_array ("server-certificate-hashes",
          "Server certificate hashes",
          "List of ECSDA certificate to authenticate the certificate provided "
          "by the server.",
          g_param_spec_string ("hash", "ECSDA certificate hash",
              "ECSDA cetificate hash", NULL,
              (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)),
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                         GST_PARAM_MUTABLE_READY)));
}

EMSCRIPTEN_BINDINGS (gst_web_transport_src)
{
  function (
      "gst_web_transport_src_add_stream", &gst_web_transport_src_add_stream);
  function (
      "gst_web_transport_src_end_stream", &gst_web_transport_src_end_stream);
}
