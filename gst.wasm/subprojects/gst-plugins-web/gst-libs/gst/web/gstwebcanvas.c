/*
 * GStreamer - gst.wasm WebCanvas source
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

/*
 * A GstWebCanvas is something attachable into a GstContext that allows
 * multiple web related elements to share the same thread of work despite how
 * the actual pipeline is composed. It is related to GstGLWebDisplay but only
 * for Web related elements.
 *
 * This is needed because every emscripten::val is per thread and every thread
 * (WebWorker) has it's own JS state. So emscripten::val can not be shared
 * unless every thread on the application has it by using thread_local which
 * might be overkill. Check
 * https://emscripten.org/docs/api_reference/val.h.html for more information.
 *
 * When a GstWebCanvasSink creates a GstWebCanvas the actual rendering process
 * will happen in the GstWebCanvas's GstWebRunner working thread of the sink,
 * not the streaming thread.
 *
 * If there is a WebCodecsVideoDecoder upstream and also requests a
 * GstWebCanvas to do a fast-path the streaming thread of the
 * WebCodecsVideoDecoder is also the same thread as the WebCanvas avoiding a
 * memcpy
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

#include "gstwebcanvas.h"
#include "gstwebrunner.h"

struct _GstWebCanvasPrivate
{
  gchar *canvases;
  GstWebRunner *runner;
};

#define gst_web_canvas_parent_class parent_class
G_DEFINE_TYPE_WITH_PRIVATE (GstWebCanvas, gst_web_canvas, GST_TYPE_OBJECT);

static void
gst_web_canvas_dispose (GObject *object)
{
  GstWebCanvas *self = GST_WEB_CANVAS (object);

  gst_object_unref (self->priv->runner);
  g_free (self->priv->canvases);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_web_canvas_init (GstWebCanvas *self)
{
}

static void
gst_web_canvas_class_init (GstWebCanvasClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = gst_web_canvas_dispose;
}

GstWebCanvas *
gst_web_canvas_new (const gchar *canvases)
{
  GstWebCanvas *self;

  self = g_object_new (GST_TYPE_WEB_CANVAS, NULL);
  self->priv = gst_web_canvas_get_instance_private (self);
  self->priv->canvases = g_strdup (canvases);
  /* FIXME use a property runner (RDI-2852) */
  /* Create the runner with the passed in canvas (RDI-2852) */
  self->priv->runner = gst_web_runner_new (canvases);
  /* FIXME use a property canvases */
  /* FIXME create a new fundamental type GstWebStringList (RDI-2852) */
  gst_object_ref_sink (self);

  return self;
}

/* FIXME until we have the property (RDI-2852) */
/**
 * gst_web_canvas_get_runner:
 * @self: The #GstWebCanvas
 *
 * Gets the underlying GstWebCanvasRunner.
 *
 * Returns: (transfer full): a reference to the underlying runner.
 */
GstWebRunner *
gst_web_canvas_get_runner (GstWebCanvas *self)
{
  return gst_object_ref (GST_OBJECT (self->priv->runner));
}
