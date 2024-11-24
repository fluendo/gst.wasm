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
#include "config.h"
#endif

#include <gst/gst.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "gstwebtransferable.h"

#define GST_CAT_DEFAULT gst_web_transferable_debug
#define GST_WEB_TRANSFERABLE_MESSAGE_REQUEST_OBJECT_NAME                      \
  "GstWebRequestObjectMessage"

using namespace emscripten;

GST_DEBUG_CATEGORY_STATIC (gst_web_transferable_debug);

/* Interface to handle the transferable nature of JS objects
 * Implementing this interface will ease the logic required
 * for elements to share objects with other elements
 */

/* clang-format off */
EM_JS (void, gst_web_transferable_js_worker_handle_message, (val e), {
  let msgData = e.data;
  let cmd = msgData["gst_cmd"];
  let data = msgData["data"];
  if (cmd && cmd == "transferObject") {
    cb_name = data["cb_name"];
    cb_user_data = data["user_data"];
    obj = data["object"];
    obj_name = data["object_name"];
    sender_tid = data["sender_tid"];
    cb = Module[cb_name];
    cb (sender_tid, obj_name, obj, cb_user_data);
  }
});

EM_JS (void, gst_web_transferable_js_main_thread_handle_message, (val e), {
  let msgData = e.data;
  let cmd = msgData["gst_cmd"];
  let data = msgData["data"];
  if (cmd && cmd == "transferObject") {
    /* In this moment, the object is transfered to the main thread
     * Send it to the corresponding worker
     * TODO handle transferring to the main thread
     */
    var worker = PThread.pthreads[data["tid"]];
    var object = data["object"];
    worker.postMessage({
      gst_cmd: "transferObject",
      data: {
        cb_name: data["cb_name"],
        object_name: data["object_name"],
        object: object,
        user_data: data["user_data"],
        tid: data["tid"],
        sender_tid: data["sender_tid"]
      }
    }, [object]);
  }
});

EM_JS (void, gst_web_transferable_js_worker_transfer_object, (val cb_name, val object_name, val object, gint user_data, gint tid, gint sender_tid), {
  postMessage({
    gst_cmd: "transferObject",
    data: {
      cb_name: cb_name,
      object_name: object_name,
      object: object,
      user_data: user_data,
      tid: tid,
      sender_tid: sender_tid
    }
  }, [object]);
});
/* clang-format on */

static GstMessage *
gst_web_transferable_new_request_object (GstElement *src, const gchar *cb_name,
    const gchar *object_name, gpointer user_data)
{
  GstMessage *ret;
  GstStructure *s;

  /* TODO later instead of fetching the pthread id, use the actual GThread
   * pointer and keep a list similar to what Emscripten does for pthreads
   */
  s = gst_structure_new (GST_WEB_TRANSFERABLE_MESSAGE_REQUEST_OBJECT_NAME,
      "callback-name", G_TYPE_STRING, cb_name, "object-name", G_TYPE_STRING,
      object_name, "user-data", G_TYPE_POINTER, user_data, "tid", G_TYPE_INT,
      pthread_self (), nullptr);
  ret = gst_message_new_element (GST_OBJECT_CAST (src), s);
  return ret;
}

static void
gst_web_transferable_class_init (gpointer g_class, gpointer class_data)
{
}

static void
gst_web_transferable_base_init (gpointer g_class)
{
}

GType
gst_web_transferable_get_type (void)
{
  static gsize type = 0;

  if (g_once_init_enter (&type)) {
    GType _type;
    static const GTypeInfo info = {
      sizeof (GstWebTransferableInterface),
      gst_web_transferable_base_init,  /* base_init */
      NULL,                            /* base_finalize */
      gst_web_transferable_class_init, /* class_init */
      NULL,                            /* class_finalize */
      NULL,                            /* class_data */
      0, 0,                            /* n_preallocs */
      NULL                             /* instance_init */
    };

    _type = g_type_register_static (
        G_TYPE_INTERFACE, "GstWebTransferable", &info, (GTypeFlags) 0);

    g_type_interface_add_prerequisite (_type, GST_TYPE_ELEMENT);
    g_once_init_leave (&type, (gsize) _type);
    GST_DEBUG_CATEGORY_INIT (gst_web_transferable_debug, "webtransferable", 0,
        "WebTransferable interface");
  }
  return type;
}

void
gst_web_transferable_register_on_message (GstWebTransferable *self)
{
  g_return_if_fail (GST_IS_WEB_TRANSFERABLE (self));

  /* Just register our own message event handler to support the 'transferred'
   * event
   */
  GST_DEBUG_OBJECT (self, "Registering message handling");

  /* clang-format off */
  /* We register the function on the worker when a message comes from the main thread */
  EM_ASM ({
    addEventListener ("message", gst_web_transferable_js_worker_handle_message);
  });
  /* And another event listener when the mssages comes from the worker */
  /* TODO later instead of fetching the pthread id, use the actual GThread
   * pointer and keep a list similar to what Emscripten does for pthreads
   */
  MAIN_THREAD_EM_ASM ({
    var worker = PThread.pthreads[$0];
    worker.addEventListener ("message", gst_web_transferable_js_main_thread_handle_message);
  }, pthread_self());
  /* clang-format on */
}

void
gst_web_transferable_unregister_on_message (GstWebTransferable *self)
{
  g_return_if_fail (GST_IS_WEB_TRANSFERABLE (self));

  /* Just unregister our handlers registered on
   * gst_web_transferable_js_register_on_message */
  GST_DEBUG_OBJECT (self, "Unregistering message handling");
  /* clang-format off */
  EM_ASM ({
    removeEventListener ("message", gst_web_transferable_js_worker_handle_message);
  });
  /* TODO later instead of fetching the pthread id, use the actual GThread
   * pointer and keep a list similar to what Emscripten does for pthreads
   */
  MAIN_THREAD_EM_ASM ({
    var worker = PThread.pthreads[$0];
    worker.removeEventListener ("message", gst_web_transferable_js_main_thread_handle_message);
  }, pthread_self());
  /* clang-format on */
}

/* make sure the thread has registered the worker message to receive the object
 */
gboolean
gst_web_transferable_request_object (GstWebTransferable *self,
    const gchar *object_name, const gchar *js_cb, gpointer user_data)
{
  GstMessage *msg;

  g_return_val_if_fail (GST_IS_WEB_TRANSFERABLE (self), FALSE);

  msg = gst_web_transferable_new_request_object (
      GST_ELEMENT_CAST (self), js_cb, object_name, user_data);
  /* First try to request the object in the caller element by invoking the
   * can_transfer/transfer combo of the interface. If it fails, push the
   * message to the bus to see if the parent element (or the application)
   * can handle it
   */
  if (!gst_web_transferable_handle_request_object (self, msg)) {
    GST_DEBUG_OBJECT (self, "Posting message");
    return gst_element_post_message (GST_ELEMENT_CAST (self), msg);
  }

  return TRUE;
}

gboolean
gst_web_transferable_handle_request_object (
    GstWebTransferable *self, GstMessage *msg)
{
  GstWebTransferableInterface *iface;
  const GstStructure *s;
  const gchar *object_name;

  g_return_val_if_fail (GST_IS_WEB_TRANSFERABLE (self), FALSE);

  /* check if the message is of the type we expect */
  if (GST_MESSAGE_TYPE (msg) != GST_MESSAGE_ELEMENT)
    return FALSE;

  s = gst_message_get_structure (msg);
  if (!s)
    return FALSE;
  if (g_strcmp0 (gst_structure_get_name (s),
          GST_WEB_TRANSFERABLE_MESSAGE_REQUEST_OBJECT_NAME))
    return FALSE;

  object_name = gst_structure_get_string (s, "object-name");
  GST_DEBUG_OBJECT (self,
      "Handling the request of object to '%s' to element %s'", object_name,
      GST_OBJECT_NAME (GST_MESSAGE_SRC (msg)));

  /* check if the rquested object is one of the transferable objects */
  iface = GST_WEB_TRANSFERABLE_GET_INTERFACE (self);
  if (iface->can_transfer && iface->can_transfer (self, object_name)) {
    /* if so, transfer it */
    if (iface->transfer) {
      iface->transfer (self, object_name, msg);
    }
    return TRUE;
  } else {
    return FALSE;
  }
}

/* make sure the thread has registered the worker message to receive the object
 */
void
gst_web_transferable_transfer_object (
    GstWebTransferable *self, GstMessage *msg, guintptr object)
{
  const GstStructure *s = gst_message_get_structure (msg);
  const gchar *object_name;
  const gchar *cb_name;
  gint tid;
  guintptr user_data;

  g_return_if_fail (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ELEMENT);
  g_return_if_fail (!g_strcmp0 (gst_structure_get_name (s),
      GST_WEB_TRANSFERABLE_MESSAGE_REQUEST_OBJECT_NAME));

  GST_DEBUG_OBJECT (self, "Transferring object to '%s' element",
      GST_OBJECT_NAME (GST_MESSAGE_SRC (msg)));

  gst_structure_get (s, "callback-name", G_TYPE_STRING, &cb_name,
      "object-name", G_TYPE_STRING, &object_name, "user-data", G_TYPE_POINTER,
      &user_data, "tid", G_TYPE_INT, &tid, nullptr);

  /* clang-format off */
  EM_ASM ({
    gst_web_transferable_js_worker_transfer_object (Emval.toValue ($0),
        Emval.toValue ($1), Emval.toValue ($2), $3, $4, $5);
    },
    val::u8string (cb_name).as_handle (),
    val::u8string (object_name).as_handle (), object, user_data, tid,
    pthread_self ()
  );
  /* clang-format on */
  GST_DEBUG_OBJECT (self, "Transferred object to '%s' element done",
      GST_OBJECT_NAME (GST_MESSAGE_SRC (msg)));
}

EMSCRIPTEN_BINDINGS (gst_web_transferable)
{
  function ("gst_web_transferable_js_worker_transfer_object",
      &gst_web_transferable_js_worker_transfer_object);
}
