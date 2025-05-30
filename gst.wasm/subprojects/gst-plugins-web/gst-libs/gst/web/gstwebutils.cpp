/*
 * GStreamer - gst.wasm WebUtils source
 *
 * Copyright 2024 Fluendo S.A.
 *  @author: Jorge Zapata <jzapata@fluendo.com>
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

  val memory_view = val::global ("HEAPU8").call<val> (
      "subarray", (guintptr) ret.data, (guintptr) ret.data + ret.len);

  memory_view.call<void> ("set", data);

  return ret;
}

GstBuffer *
gst_web_utils_js_array_to_buffer (const val &data)
{
  GByteArray map = gst_web_utils_copy_data_from_js (data);
  return gst_buffer_new_wrapped (map.data, map.len);
}

GstVideoFormat
gst_web_utils_video_format_from_web_format (const char *vf_format)
{
  GstVideoFormat format = GST_VIDEO_FORMAT_UNKNOWN;

  // TODO: gst_video_format_from_string?

  if (!g_strcmp0 (vf_format, "I420")) {
    format = GST_VIDEO_FORMAT_I420;
  } else if (!g_strcmp0 (vf_format, "I420A")) {
    format = GST_VIDEO_FORMAT_A420;
  } else if (!g_strcmp0 (vf_format, "I422")) {
    format = GST_VIDEO_FORMAT_Y42B;
  } else if (!g_strcmp0 (vf_format, "I444")) {
    format = GST_VIDEO_FORMAT_Y444;
  } else if (!g_strcmp0 (vf_format, "NV12")) {
    format = GST_VIDEO_FORMAT_NV12;
  } else if (!g_strcmp0 (vf_format, "RGBA")) {
    format = GST_VIDEO_FORMAT_RGBA;
  } else if (!g_strcmp0 (vf_format, "RGBX")) {
    format = GST_VIDEO_FORMAT_RGBx;
  } else if (!g_strcmp0 (vf_format, "BGRA")) {
    format = GST_VIDEO_FORMAT_BGRA;
  } else if (!g_strcmp0 (vf_format, "BGRX")) {
    format = GST_VIDEO_FORMAT_BGRx;
  } else {
    GST_ERROR ("Unsupported format %s", vf_format);
  }

  return format;
}

const char *
gst_web_utils_video_format_to_web_format (GstVideoFormat format)
{
  const char *vf_format = NULL;

  switch (format) {
    case GST_VIDEO_FORMAT_I420:
      vf_format = "I420";
      break;
    case GST_VIDEO_FORMAT_A420:
      vf_format = "I420A";
      break;
    case GST_VIDEO_FORMAT_Y42B:
      vf_format = "I422";
      break;
    case GST_VIDEO_FORMAT_Y444:
      vf_format = "I444";
      break;
    case GST_VIDEO_FORMAT_NV12:
      vf_format = "NV12";
      break;
    case GST_VIDEO_FORMAT_RGBA:
      vf_format = "RGBA";
      break;
    case GST_VIDEO_FORMAT_RGBx:
      vf_format = "RGBX";
      break;
    case GST_VIDEO_FORMAT_BGRA:
      vf_format = "BGRA";
      break;
    case GST_VIDEO_FORMAT_BGRx:
      vf_format = "BGRX";
      break;
    default:
      GST_ERROR ("Unsupported GstVideoFormat: %d", format);
      break;
  }

  return vf_format;
}
