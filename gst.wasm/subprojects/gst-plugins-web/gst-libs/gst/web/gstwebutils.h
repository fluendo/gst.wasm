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

#ifndef __GST_WEB_UTILS_H__
#define __GST_WEB_UTILS_H__

#include "gstwebcanvas.h"

#define GST_WEB_UTILS_MESSAGE_PROPOSE_OBJECT_NAME "GstWebProposeObjectMessage"
#define GST_WEB_UTILS_MESSAGE_REQUEST_OBJECT_NAME "GstWebRequestObjectMessage"

G_BEGIN_DECLS

void gst_web_utils_init (void);

void gst_web_utils_context_set_web_canvas (
    GstContext *context, GstWebCanvas *canvas);
gboolean gst_web_utils_context_get_web_canvas (
    GstContext *context, GstWebCanvas **canvas);
gboolean gst_web_utils_element_ensure_canvas (
    gpointer element, GstWebCanvas **canvas_ptr, const gchar *canvas_name);
gboolean gst_web_utils_element_handle_context_query (
    GstElement *element, GstQuery *query, GstWebCanvas *canvas);
gboolean gst_web_utils_element_set_context (
    GstElement *element, GstContext *context, GstWebCanvas **canvas);
void gst_web_utils_js_register_on_message (void);
void gst_web_utils_js_unregister_on_message (void);
void gst_web_utils_element_process_request_object (
    GstElement *e, GstMessage *msg, guintptr object);

GstMessage *gst_web_utils_message_new_request_object (GstElement *src,
    const gchar *cb_name, const gchar *object_name, gpointer user_data);

G_END_DECLS

#endif
