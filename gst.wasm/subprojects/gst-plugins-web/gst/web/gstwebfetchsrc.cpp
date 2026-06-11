/*
 * GStreamer - gst.wasm Web Fetch HTTP source (bin)
 *
 * Copyright 2026 Fluendo S.A.
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
 * This is a bin that creates a single source for HTTP/HTTPS fetch streams
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

#include "gstwebfetchsrc.h"
#include "stream/gstwebstreamreadersrc.h"
#include "gstweb.h"

using namespace emscripten;

#define GST_TYPE_WEB_FETCH_SRC (gst_web_fetch_src_get_type ())
#define parent_class gst_web_fetch_src_parent_class
#define GST_CAT_DEFAULT gst_web_fetch_src_debug

typedef struct _GstWebFetchSrc
{
  GstBin base;
  gchar *uri;
  GstWebRunner *runner;
  GstWebTransferableThread owner_thread;

  val stream;
} GstWebFetchSrc;

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_MAX
};

GST_DEBUG_CATEGORY_STATIC (gst_web_fetch_src_debug);

G_DECLARE_FINAL_TYPE (
    GstWebFetchSrc, gst_web_fetch_src, GST, WEB_FETCH_SRC, GstBin)

static GstURIType
gst_web_fetch_src_urihandler_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_web_fetch_src_urihandler_get_protocols (GType type)
{
  static const gchar *protocols[] = { "http", "https", NULL };

  return protocols;
}

static gboolean
gst_web_fetch_src_urihandler_set_uri (
    GstURIHandler *handler, const gchar *uri, GError **error)
{
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (handler);

  g_return_val_if_fail (GST_IS_URI_HANDLER (handler), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  GST_OBJECT_LOCK (self);
  g_free (self->uri);
  self->uri = g_strdup (uri);
  GST_OBJECT_UNLOCK (self);

  return TRUE;
}

static gchar *
gst_web_fetch_src_urihandler_get_uri (GstURIHandler *handler)
{
  GstWebFetchSrc *self;
  gchar *ret;

  g_return_val_if_fail (GST_IS_URI_HANDLER (handler), NULL);
  self = GST_WEB_FETCH_SRC (handler);

  GST_OBJECT_LOCK (self);
  ret = g_strdup (self->uri);
  GST_OBJECT_UNLOCK (self);

  return ret;
}

static void
gst_web_fetch_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *uri_iface = (GstURIHandlerInterface *) g_iface;

  uri_iface->get_type = gst_web_fetch_src_urihandler_get_type;
  uri_iface->get_protocols = gst_web_fetch_src_urihandler_get_protocols;
  uri_iface->get_uri = gst_web_fetch_src_urihandler_get_uri;
  uri_iface->set_uri = gst_web_fetch_src_urihandler_set_uri;
}

static GstWebTransferableThread *
gst_web_fetch_src_transferable_can_transfer (
    GstWebTransferable *transferable, const gchar *object_name)
{
  GstWebFetchSrc *self;

  g_return_val_if_fail (GST_IS_WEB_TRANSFERABLE (transferable), NULL);
  self = GST_WEB_FETCH_SRC (transferable);

  GST_DEBUG_OBJECT (self, "Requesting object %s", object_name);
  if (strncmp (object_name, "ReadableStream/", 15))
    return NULL;

  return &self->owner_thread;
}

static void
gst_web_fetch_src_transferable_transfer (GstWebTransferable *transferable,
    const gchar *object_name, GstMessage *msg)
{
  GstWebFetchSrc *self;
  val object;

  g_return_if_fail (GST_IS_WEB_TRANSFERABLE (transferable));
  self = GST_WEB_FETCH_SRC (transferable);

  /* Always called on the owner thread by handle_request_object */
  if (!strncmp (object_name, "ReadableStream/", 15)) {
    object = self->stream;
  } else {
    GST_ERROR_OBJECT (self, "Unsupported object '%s'", object_name);
    return;
  }
  gst_web_transferable_transfer_object (
      (GstWebTransferable *) self, msg, (guintptr) object.as_handle ());
}

static void
gst_web_fetch_src_transferable_init (gpointer g_iface, gpointer iface_data)
{
  GstWebTransferableInterface *iface = (GstWebTransferableInterface *) g_iface;

  iface->can_transfer = gst_web_fetch_src_transferable_can_transfer;
  iface->transfer = gst_web_fetch_src_transferable_transfer;
}

G_DEFINE_TYPE_WITH_CODE (GstWebFetchSrc, gst_web_fetch_src, GST_TYPE_BIN,
                         G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
                             gst_web_fetch_src_uri_handler_init);
                         G_IMPLEMENT_INTERFACE (GST_TYPE_WEB_TRANSFERABLE,
                             gst_web_fetch_src_transferable_init));

GST_ELEMENT_REGISTER_DEFINE (
    web_fetch_src, "webfetchsrc", GST_RANK_PRIMARY, GST_TYPE_WEB_FETCH_SRC);

static void
gst_web_fetch_src_stream_linked_cb (
    GstPad *pad, GstPad *peer, gpointer user_data)
{
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (user_data);
  GstPad *target;
  GstElement *src;

  GST_DEBUG_OBJECT (self, "Stream linked, setup the stream src");
  src = gst_web_stream_reader_src_new ("src");
  gst_bin_add (GST_BIN (self), src);
  target = gst_element_get_static_pad (src, "src");
  gst_ghost_pad_set_target (GST_GHOST_PAD (pad), target);
  gst_object_unref (target);
  gst_element_sync_state_with_parent (src);
}

static void
gst_web_fetch_src_create_stream (GstWebFetchSrc *self)
{
  val response;

  GST_DEBUG_OBJECT (self, "Creating fetch stream for %s", self->uri);

  /* Call JavaScript fetch and get the ReadableStream */
  response = val::global ("fetch") (std::string (self->uri)).await ();
  self->stream = response["body"];

  GST_INFO_OBJECT (self, "Stream created for %s", self->uri);

  self->owner_thread =
      gst_web_transferable_register_on_message ((GstWebTransferable *) self);
}

static void
gst_web_fetch_src_destroy_stream (GstWebFetchSrc *self)
{
  gst_web_transferable_unregister_on_message (
      (GstWebTransferable *) self, self->owner_thread);
}

static gboolean
gst_web_fetch_src_start (GstWebFetchSrc *self)
{
  GST_DEBUG_OBJECT (self, "Starting");

  self->runner = gst_web_runner_new (NULL);
  gst_web_runner_run (self->runner, NULL);
  gst_web_runner_send_message (
      self->runner, (GstWebRunnerCB) gst_web_fetch_src_create_stream, self);

  return TRUE;
}

static void
gst_web_fetch_src_stop (GstWebFetchSrc *self)
{
  GST_DEBUG_OBJECT (self, "Stopping");

  gst_web_runner_send_message (
      self->runner, (GstWebRunnerCB) gst_web_fetch_src_destroy_stream, self);
  g_object_unref (self->runner);
  self->runner = NULL;
}

static void
gst_web_fetch_src_handle_message (GstBin *bin, GstMessage *msg)
{
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (bin);
  gboolean handled = FALSE;

  GST_LOG_OBJECT (bin, "Handle message of type %s",
      gst_message_type_get_name (GST_MESSAGE_TYPE (msg)));

  if ((handled = gst_web_transferable_handle_request_object (
           (GstWebTransferable *) bin, msg)))
    goto done;

  /* Process the stream status message */
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

static GstStateChangeReturn
gst_web_fetch_src_change_state (GstElement *element, GstStateChange transition)
{
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (element);
  GstStateChangeReturn ret;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (self->uri == NULL) {
        GST_ELEMENT_ERROR (
            element, RESOURCE, OPEN_READ, ("No URL set."), ("Missing URL"));
        return GST_STATE_CHANGE_FAILURE;
      }
      if (!gst_web_fetch_src_start (self))
        return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_web_fetch_src_stop (self);
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_web_fetch_src_set_property (
    GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      gst_web_fetch_src_urihandler_set_uri (
          GST_URI_HANDLER (self), g_value_get_string (value), NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_web_fetch_src_get_property (
    GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      g_value_take_string (value,
          gst_web_fetch_src_urihandler_get_uri (GST_URI_HANDLER (self)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_web_fetch_src_finalize (GObject *obj)
{
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (obj);

  g_free (self->uri);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_web_fetch_src_init (GstWebFetchSrc *self)
{
  GstPad *pad;

  GST_DEBUG_OBJECT (self, "Creating src pad");
  pad = gst_ghost_pad_new_no_target ("src", GST_PAD_SRC);
  g_signal_connect (
      pad, "linked", (GCallback) gst_web_fetch_src_stream_linked_cb, self);
  gst_element_add_pad (GST_ELEMENT_CAST (self), pad);

  self->stream = val::undefined ();
}

static void
gst_web_fetch_src_class_init (GstWebFetchSrcClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBinClass *bin_class = (GstBinClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_web_fetch_src_debug, "webfetchsrc", 0,
      "HTTP/HTTPS Client Source using Fetch + Streams API");

  bin_class->handle_message = gst_web_fetch_src_handle_message;

  element_class->change_state = gst_web_fetch_src_change_state;

  gst_element_class_set_static_metadata (element_class,
      "HTTP/HTTPS Client Source using Fetch + Streams API", "Source/Network",
      "Receives data as a client over HTTP/HTTPS using Fetch and "
      "ReadableStream",
      "Jorge Zapata <jzapata@fluendo.com>");

  gobject_class->set_property = gst_web_fetch_src_set_property;
  gobject_class->get_property = gst_web_fetch_src_get_property;
  gobject_class->finalize = gst_web_fetch_src_finalize;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "Location", "URI of resource to read",
          NULL,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                         GST_PARAM_MUTABLE_READY)));
}
