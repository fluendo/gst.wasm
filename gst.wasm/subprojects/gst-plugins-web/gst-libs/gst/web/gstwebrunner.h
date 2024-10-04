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

#ifndef __GST_WEB_RUNNER_H__
#define __GST_WEB_RUNNER_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_WEB_RUNNER (gst_web_runner_get_type ())
#define GST_WEB_RUNNER(obj)                                                   \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_WEB_RUNNER, GstWebRunner))
#define GST_WEB_RUNNER_CLASS(klass)                                           \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_WEB_RUNNER, GstWebRunnerClass))
#define GST_IS_WEB_RUNNER(obj)                                                \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_WEB_RUNNER))
#define GST_IS_WEB_RUNNER_CLASS(klass)                                        \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_WEB_RUNNER))
#define GST_WEB_RUNNER_GET_CLASS(o)                                           \
  (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_WEB_RUNNER, GstWebRunnerClass))

typedef void (*GstWebRunnerCB) (gpointer data);
typedef struct _GstWebRunner GstWebRunner;
typedef struct _GstWebRunnerClass GstWebRunnerClass;
typedef struct _GstWebRunnerPrivate GstWebRunnerPrivate;

struct _GstWebRunner
{
  /*< private >*/
  GstObject base;

  /*< private >*/
  GstWebRunnerPrivate *priv;

  gpointer _reserved[GST_PADDING];
};

struct _GstWebRunnerClass
{
  GstObjectClass base;

  GThread *(*create_thread) (
      GstWebRunner *self, const gchar *name, GThreadFunc run);
  void (*send_message) (
      GstWebRunner *self, GstWebRunnerCB callback, gpointer data);
  void (*send_message_async) (GstWebRunner *self, GstWebRunnerCB callback,
      gpointer data, GDestroyNotify destroy);
  /*< private >*/
  gpointer _reserved[GST_PADDING];
};

GType gst_web_runner_get_type (void);

GstWebRunner *gst_web_runner_new (const gchar *canvases);
gboolean gst_web_runner_run (GstWebRunner *self, GError **error);
void gst_web_runner_send_message_async (GstWebRunner *self,
    GstWebRunnerCB callback, gpointer data, GDestroyNotify destroy);
void gst_web_runner_send_message (
    GstWebRunner *self, GstWebRunnerCB callback, gpointer data);

G_END_DECLS

#endif /* __GST_WEB_RUNNER_H__ */
