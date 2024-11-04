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

#include "gstwebcanvas.h"
#include "gstwebutils.h"

using namespace emscripten;

GST_DEBUG_CATEGORY_STATIC (web_utils_debug);
GST_DEBUG_CATEGORY_STATIC (GST_CAT_CONTEXT);

#define GST_CAT_DEFAULT web_utils_debug

/* Function borrowed from gst-libs/gl/glutils.c, must be shared */
static gboolean
gst_web_utils_element_canvas_found (GstElement *element, GstWebCanvas *canvas)
{
  if (canvas) {
    GST_LOG_OBJECT (element, "already have a canvas (%p)", canvas);
    return TRUE;
  }

  return FALSE;
}

/* Calls a query on the pad
 * Function borrowed from gst-libs/gl/glutils.c, must be shared
 */
static gboolean
gst_web_utils_pad_query (const GValue *item, GValue *value, gpointer user_data)
{
  GstPad *pad = (GstPad *) g_value_get_object (item);
  GstQuery *query = (GstQuery *) user_data;
  gboolean res;

  res = gst_pad_peer_query (pad, query);

  if (res) {
    g_value_set_boolean (value, TRUE);
    return FALSE;
  }

  GST_CAT_INFO_OBJECT (GST_CAT_CONTEXT, pad, "pad peer query failed");
  return TRUE;
}

/* Runs a query on every src or sink pad of an element depending on the
 * direction
 * Function borrowed from gst-libs/gl/glutils.c, must be shared
 */
static gboolean
gst_web_utils_element_run_query (
    GstElement *element, GstQuery *query, GstPadDirection direction)
{
  GstIterator *it;
  GstIteratorFoldFunction func = gst_web_utils_pad_query;
  GValue res = { 0 };

  g_value_init (&res, G_TYPE_BOOLEAN);
  g_value_set_boolean (&res, FALSE);

  /* Ask neighbor */
  if (direction == GST_PAD_SRC)
    it = gst_element_iterate_src_pads (element);
  else
    it = gst_element_iterate_sink_pads (element);

  while (gst_iterator_fold (it, func, &res, query) == GST_ITERATOR_RESYNC)
    gst_iterator_resync (it);

  gst_iterator_free (it);

  return g_value_get_boolean (&res);
}

/* Request a GstWebCanvas downstream, upstream or post a message
 * Function borrowed from gst-libs/gl/glutils.c, must be shared
 */
static void
gst_web_utils_element_context_query (
    GstElement *element, const gchar *context_type)
{
  GstQuery *query;
  GstContext *ctxt;

  /*  2a) Query downstream with GST_QUERY_CONTEXT for the context and
   *      check if downstream already has a context of the specific type
   *  2b) Query upstream as above.
   */
  query = gst_query_new_context (context_type);
  if (gst_web_utils_element_run_query (element, query, GST_PAD_SRC)) {
    gst_query_parse_context (query, &ctxt);
    GST_CAT_INFO_OBJECT (GST_CAT_CONTEXT, element,
        "found context (%p) in downstream query", ctxt);
    gst_element_set_context (element, ctxt);
  } else if (gst_web_utils_element_run_query (element, query, GST_PAD_SINK)) {
    gst_query_parse_context (query, &ctxt);
    GST_CAT_INFO_OBJECT (GST_CAT_CONTEXT, element,
        "found context (%p) in upstream query", ctxt);
    gst_element_set_context (element, ctxt);
  } else {
    /* 3) Post a GST_MESSAGE_NEED_CONTEXT message on the bus with
     *    the required context type and afterwards check if a
     *    usable context was set now as in 1). The message could
     *    be handled by the parent bins of the element and the
     *    application.
     */
    GstMessage *msg;

    GST_CAT_INFO_OBJECT (
        GST_CAT_CONTEXT, element, "posting need context message");
    msg =
        gst_message_new_need_context (GST_OBJECT_CAST (element), context_type);
    gst_element_post_message (element, msg);
  }

  /*
   * Whomever responds to the need-context message performs a
   * GstElement::set_context() with the required context in which the element
   * is required to update the canvas_ptr or call gst_gl_handle_set_context().
   */

  gst_query_unref (query);
}

/*  4) Create a context by itself and post a GST_MESSAGE_HAVE_CONTEXT
 *     message.
 */
static void
gst_web_utils_element_propagate_canvas (
    GstElement *element, GstWebCanvas *canvas)
{
  GstContext *context;
  GstMessage *msg;

  if (!canvas) {
    GST_ERROR_OBJECT (element, "Could not get Web Canvas connection");
    return;
  }

  context = gst_context_new (GST_WEB_CANVAS_CONTEXT_TYPE, TRUE);
  gst_web_utils_context_set_web_canvas (context, canvas);
  gst_element_set_context (element, context);

  GST_CAT_INFO_OBJECT (GST_CAT_CONTEXT, element,
      "posting have context (%p) message with canvas (%p)", context, canvas);
  msg = gst_message_new_have_context (GST_OBJECT_CAST (element), context);
  gst_element_post_message (GST_ELEMENT_CAST (element), msg);
}

static void
gst_web_utils_element_query_canvas (GstElement *element)
{
  gst_web_utils_element_context_query (element, GST_WEB_CANVAS_CONTEXT_TYPE);
}

void
gst_web_utils_init (void)
{
#ifndef GST_DISABLE_GST_DEBUG
  static gsize _init = 0;

  if (g_once_init_enter (&_init)) {
    GST_DEBUG_CATEGORY_GET (GST_CAT_CONTEXT, "GST_CONTEXT");
    g_once_init_leave (&_init, 1);
    GST_DEBUG_CATEGORY_INIT (web_utils_debug, "webutils", 0, "Web Utilities");
  }
#endif
}

/**
 * gst_web_utils_context_set_web_canvas:
 * @context: a #GstContext
 * @canvas: (transfer none) (nullable): resulting #GstWebCanvas
 *
 * Sets @canvas on @context
 */
void
gst_web_utils_context_set_web_canvas (
    GstContext *context, GstWebCanvas *canvas)
{
  GstStructure *s;

  g_return_if_fail (context != NULL);

  if (canvas)
    GST_CAT_LOG (GST_CAT_CONTEXT,
        "setting GstWebCanvas(%" GST_PTR_FORMAT ") on context(%" GST_PTR_FORMAT
        ")",
        canvas, context);

  s = gst_context_writable_structure (context);
  gst_structure_set (
      s, GST_WEB_CANVAS_CONTEXT_TYPE, GST_TYPE_WEB_CANVAS, canvas, nullptr);
}

/**
 * gst_web_utils_context_get_web_canvas
 * @context: a #GstContext
 * @canvas: (out) (optional) (nullable) (transfer full): resulting
 * #GstGLDisplay
 *
 * Returns: Whether @canvas was in @context
 */
gboolean
gst_web_utils_context_get_web_canvas (
    GstContext *context, GstWebCanvas **canvas)
{
  const GstStructure *s;
  gboolean ret;

  g_return_val_if_fail (canvas != NULL, FALSE);
  g_return_val_if_fail (context != NULL, FALSE);

  s = gst_context_get_structure (context);
  ret = gst_structure_get (
      s, GST_WEB_CANVAS_CONTEXT_TYPE, GST_TYPE_WEB_CANVAS, canvas, nullptr);

  GST_CAT_LOG (GST_CAT_CONTEXT, "got GstWebCanvas(%p) from context(%p)",
      *canvas, context);

  return ret;
}

/**
 * gst_web_utils_element_ensure_canvas:
 * @element: (type Gst.Element): the #GstElement running the query
 * @canvas_ptr: (inout): the resulting #GstGLDisplay
 *
 * Perform the steps necessary for retrieving a #GstGLDisplay and (optionally)
 * an application provided #GstGLContext from the surrounding elements or from
 * the application using the #GstContext mechanism.
 *
 * If the contents of @canvas_ptr is not %NULL, then no
 * #GstContext query is necessary for #GstGLDisplay or #GstGLContext retrieval
 * or is performed.
 *
 * Returns: whether a #GstWebCanvas exists in @canvas_ptr
 */
gboolean
gst_web_utils_element_ensure_canvas (
    gpointer element, GstWebCanvas **canvas_ptr, const gchar *canvas_name)
{
  GstWebCanvas *canvas;

  g_return_val_if_fail (element != NULL, FALSE);
  g_return_val_if_fail (canvas_ptr != NULL, FALSE);

  /*  1) Check if the element already has a context of the specific
   *     type.
   */
  canvas = *canvas_ptr;
  if (gst_web_utils_element_canvas_found (GST_ELEMENT_CAST (element), canvas))
    goto done;

  /* As this call will trigger a QUERY or an EVENT, the canvas will
   * be set to a variable pointed by canvas_ptr and everything will be
   * fine
   */
  gst_web_utils_element_query_canvas (GST_ELEMENT_CAST (element));

  /* Neighbour found and it updated the canvas */
  if (gst_web_utils_element_canvas_found (
          GST_ELEMENT_CAST (element), *canvas_ptr))
    goto done;

  /* If no neighbor, or application not interested, use system default */
  canvas = gst_web_canvas_new (canvas_name);
  *canvas_ptr = canvas;

  gst_web_utils_element_propagate_canvas (GST_ELEMENT_CAST (element), canvas);

done:
  return *canvas_ptr != NULL;
}

/**
 * gst_web_utils_element_handle_context_query:
 * @element: a #GstElement
 * @query: a #GstQuery of type %GST_QUERY_CONTEXT
 * @canvas: (transfer none) (nullable): a #GstWebCanvas
 *
 * Returns: Whether the @query was successfully responded to from the passed
 *          @canvas
 */
gboolean
gst_web_utils_element_handle_context_query (
    GstElement *element, GstQuery *query, GstWebCanvas *canvas)
{
  const gchar *context_type;

  g_return_val_if_fail (GST_IS_ELEMENT (element), FALSE);
  g_return_val_if_fail (GST_IS_QUERY (query), FALSE);
  g_return_val_if_fail (canvas == NULL || GST_IS_WEB_CANVAS (canvas), FALSE);

  GST_LOG_OBJECT (element, "handle context query %" GST_PTR_FORMAT, query);
  gst_query_parse_context_type (query, &context_type);

  if (canvas && g_strcmp0 (context_type, GST_WEB_CANVAS_CONTEXT_TYPE) == 0) {
    GstContext *old_context;
    GstContext *context;

    gst_query_parse_context (query, &old_context);
    if (old_context) {
      GST_ERROR_OBJECT (element, "Previous context value not supported");
      gst_context_unref (old_context);
      return FALSE;
    }

    context = gst_context_new (GST_WEB_CANVAS_CONTEXT_TYPE, TRUE);
    gst_web_utils_context_set_web_canvas (context, canvas);
    gst_query_set_context (query, context);
    gst_context_unref (context);
    GST_DEBUG_OBJECT (element,
        "successfully set %" GST_PTR_FORMAT " on %" GST_PTR_FORMAT, canvas,
        query);

    return TRUE;
  }
  return FALSE;
}

/**
 * gst_gl_handle_set_context:
 * @element: a #GstElement
 * @context: a #GstContext
 * @canvas: (out) (transfer full): location of a #GstWebCanvas
 *
 * Helper function for implementing #GstElementClass.set_context() in
 * Web capable elements.
 *
 * Retrieve's the #GstWebCanvas in @context and places the
 * result in @canvas
 *
 * Returns: whether the @canvas could be set successfully
 */
gboolean
gst_web_utils_element_set_context (
    GstElement *element, GstContext *context, GstWebCanvas **canvas)
{
  const gchar *context_type;

  g_return_val_if_fail (canvas != NULL, FALSE);

  if (!context)
    return FALSE;

  context_type = gst_context_get_context_type (context);

  if (g_strcmp0 (context_type, GST_WEB_CANVAS_CONTEXT_TYPE) == 0) {
    GST_DEBUG_OBJECT (element, "context found %" GST_PTR_FORMAT, *canvas);
    return gst_web_utils_context_get_web_canvas (context, canvas);
  }

  return FALSE;
}

/* clang-format off */
EM_JS (void, gst_web_utils_js_worker_handle_message, (val e), {
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

EM_JS (void, gst_web_utils_js_main_thread_handle_message, (val e), {
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

EM_JS (void, gst_web_utils_js_worker_transfer_object, (val cb_name, val object_name, val object, gint user_data, gint tid, gint sender_tid), {
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

void
gst_web_utils_js_register_on_message (void)
{
  /* Just register our own message event handler to support the 'transferred'
   * event
   */

  /* clang-format off */
  /* We register the function on the worker when a message comes from the main thread */
  EM_ASM ({
    addEventListener ("message", gst_web_utils_js_worker_handle_message);
  });
  /* And another event listener when the mssages comes from the worker */
  /* TODO later instead of fetching the pthread id, use the actual GThread
   * pointer and keep a list similar to what Emscripten does for pthreads
   */
  MAIN_THREAD_EM_ASM ({
    var worker = PThread.pthreads[$0];
    worker.addEventListener ("message", gst_web_utils_js_main_thread_handle_message);
  }, pthread_self());
  /* clang-format on */
}

void
gst_web_utils_js_unregister_on_message (void)
{
  /* Just unregister our handlers registered on
   * gst_web_utils_js_register_on_message */
  /* clang-format off */
  EM_ASM ({
    removeEventListener ("message", gst_web_utils_js_worker_handle_message);
  });
  /* TODO later instead of fetching the pthread id, use the actual GThread
   * pointer and keep a list similar to what Emscripten does for pthreads
   */
  MAIN_THREAD_EM_ASM ({
    var worker = PThread.pthreads[$0];
    worker.removeEventListener ("message", gst_web_utils_js_main_thread_handle_message);
  }, pthread_self());
  /* clang-format on */
}

GstMessage *
gst_web_utils_message_new_request_object (GstElement *src,
    const gchar *cb_name, const gchar *object_name, gpointer user_data)
{
  GstMessage *ret;
  GstStructure *s;

  /* TODO later instead of fetching the pthread id, use the actual GThread
   * pointer and keep a list similar to what Emscripten does for pthreads
   */
  s = gst_structure_new (GST_WEB_UTILS_MESSAGE_REQUEST_OBJECT_NAME,
      "callback-name", G_TYPE_STRING, cb_name, "object-name", G_TYPE_STRING,
      object_name, "user-data", G_TYPE_POINTER, user_data, "tid", G_TYPE_INT,
      pthread_self (), nullptr);
  ret = gst_message_new_element (GST_OBJECT_CAST (src), s);
  return ret;
}

void
gst_web_utils_element_process_request_object (
    GstElement *e, GstMessage *msg, guintptr object)
{
  const GstStructure *s = gst_message_get_structure (msg);
  const gchar *object_name;
  const gchar *cb_name;
  gint tid;
  guintptr user_data;

  g_return_if_fail (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ELEMENT);
  g_return_if_fail (!g_strcmp0 (
      gst_structure_get_name (s), GST_WEB_UTILS_MESSAGE_REQUEST_OBJECT_NAME));

  gst_structure_get (s, "callback-name", G_TYPE_STRING, &cb_name,
      "object-name", G_TYPE_STRING, &object_name, "user-data", G_TYPE_POINTER,
      &user_data, "tid", G_TYPE_INT, &tid, nullptr);

  GST_DEBUG_OBJECT (
      e, "Transferring object to %s", GST_OBJECT_NAME (GST_MESSAGE_SRC (msg)));
  EM_ASM (
      {
        gst_web_utils_js_worker_transfer_object (Emval.toValue ($0),
            Emval.toValue ($1), Emval.toValue ($2), $3, $4, $5);
      },
      val::u8string (cb_name).as_handle (),
      val::u8string (object_name).as_handle (), object, user_data, tid,
      pthread_self ());
  GST_DEBUG_OBJECT (e, "Transfered object to %s done",
      GST_OBJECT_NAME (GST_MESSAGE_SRC (msg)));
}

GstMessage *
gst_web_utils_message_new_propose_object (GstElement *src, gchar *object_name)
{
  GstMessage *ret;
  GstStructure *s;

  s = gst_structure_new (GST_WEB_UTILS_MESSAGE_PROPOSE_OBJECT_NAME,
      "object-name", G_TYPE_STRING, object_name, nullptr);
  ret = gst_message_new_element (GST_OBJECT_CAST (src), s);
  return ret;
}

GByteArray
gst_web_utils_copy_data_from_js (const val &data)
{
  GByteArray ret;

  ret.len = data["length"].as<gsize> ();
  ret.data = (guint8 *) g_malloc (ret.len);

  (val::global ("HEAPU8").call<val> (
       "subarray", (guintptr) ret.data, (guintptr) ret.data + ret.len))
      .call<void> ("set", data);

  return ret;
}

GstBuffer *
gst_web_utils_js_array_to_buffer (const val &data)
{
  GByteArray map = gst_web_utils_copy_data_from_js (data);
  return gst_buffer_new_wrapped (map.data, map.len);
}

EMSCRIPTEN_BINDINGS (gst_web_transport_src)
{
  function ("gst_web_utils_js_worker_transfer_object",
      &gst_web_utils_js_worker_transfer_object);
}
