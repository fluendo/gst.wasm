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

#ifndef __GST_WEB_TRANSFERABLE_H__
#define __GST_WEB_TRANSFERABLE_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_WEB_TRANSFERABLE (gst_web_transferable_get_type ())
#define GST_WEB_TRANSFERABLE(obj)                                             \
  (G_TYPE_CHECK_INSTANCE_CAST (                                               \
      (obj), GST_TYPE_WEB_TRANSFERABLE, GstWebTransferable))
#define GST_IS_WEB_TRANSFERABLE(obj)                                          \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_WEB_TRANSFERABLE))
#define GST_WEB_TRANSFERABLE_GET_INTERFACE(obj)                               \
  (G_TYPE_INSTANCE_GET_INTERFACE (                                            \
      (obj), GST_TYPE_WEB_TRANSFERABLE, GstWebTransferableInterface))

/**
 * GstWebTransferable:
 *
 * Opaque #GstWebTransferable data structure.
 */
typedef struct _GstWebTransferable GstWebTransferable; /* dummy object */
typedef struct _GstWebTransferableInterface GstWebTransferableInterface;

/**
 * GstWebTransferableInterface:
 * @parent: parent interface type.
 *
 * #GstWebTransferable interface.
 */
struct _GstWebTransferableInterface
{
  GTypeInterface parent;

  gboolean (*can_transfer) (
      GstWebTransferable *self, const gchar *object_name);
  void (*transfer) (
      GstWebTransferable *self, const gchar *object_name, GstMessage *msg);
  gpointer _gst_reserved[GST_PADDING];
};

GType gst_web_transferable_get_type (void);
void gst_web_transferable_register_on_message (GstWebTransferable *self);
void gst_web_transferable_unregister_on_message (GstWebTransferable *self);
gboolean gst_web_transferable_request_object (GstWebTransferable *self,
    const gchar *object_name, const gchar *js_cb, gpointer user_data);
gboolean gst_web_transferable_handle_request_object (
    GstWebTransferable *self, GstMessage *msg);
void gst_web_transferable_transfer_object (
    GstWebTransferable *self, GstMessage *msg, guintptr object);

G_END_DECLS

#endif /* __GST_WEB_TRANSFERABLE_H__ */
