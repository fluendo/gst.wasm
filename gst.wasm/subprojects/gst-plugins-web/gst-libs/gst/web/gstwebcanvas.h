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

#ifndef __GST_WEB_CANVAS_H__
#define __GST_WEB_CANVAS_H__

#include "gstwebrunner.h"

#define GST_TYPE_WEB_CANVAS (gst_web_canvas_get_type ())
#define GST_WEB_CANVAS(obj)                                                   \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_WEB_CANVAS, GstWebCanvas))
#define GST_WEB_CANVAS_CLASS(klass)                                           \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_WEB_CANVAS, GstWebCanvasClass))
#define GST_IS_WEB_CANVAS(obj)                                                \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_WEB_CANVAS))
#define GST_IS_WEB_CANVAS_CLASS(klass)                                        \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_WEB_CANVAS))
#define GST_WEB_CANVAS_GET_CLASS(o)                                           \
  (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_WEB_CANVAS, GstWebCanvasClass))

typedef struct _GstWebCanvas GstWebCanvas;
typedef struct _GstWebCanvasClass GstWebCanvasClass;
typedef struct _GstWebCanvasPrivate GstWebCanvasPrivate;

struct _GstWebCanvas
{
  GstObject object;

  GstWebCanvasPrivate *priv;
  /*< private >*/
  gpointer _padding[GST_PADDING];
};

struct _GstWebCanvasClass
{
  GstObjectClass object_class;
  /*< private >*/
  gpointer _padding[GST_PADDING];
};

/**
 * GST_WEB_CANVAS_CONTEXT_TYPE:
 *
 * The name used in #GstContext queries for requesting a #GstWebCanvas
 */
#define GST_WEB_CANVAS_CONTEXT_TYPE "gst.web.Canvas"

G_BEGIN_DECLS

GType gst_web_canvas_get_type (void);
GstWebCanvas *gst_web_canvas_new (const gchar *canvases);
GstWebRunner *gst_web_canvas_get_runner (GstWebCanvas *self);

G_END_DECLS

#endif /* __GST_WEB_CANVAS_H__ */
