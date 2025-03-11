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

#include <string.h>
#include <gst/gst.h>

#include "gstwebrunner.h"

GST_DEBUG_CATEGORY_STATIC (web_runner_debug);
#define GST_CAT_DEFAULT web_runner_debug
#define gst_web_runner_parent_class parent_class

struct _GstWebRunnerPrivate
{
  GThread *thread;

  gchar *canvases;

  GCond create_cond;
  GMutex create_lock;

  GCond destroy_cond;

  gboolean created;
  gboolean alive;

  GMainLoop *loop;
  GMainContext *main_context;

  GMutex sync_message_lock;
  GCond sync_message_cond;

  gint tid;
};

typedef struct _GstWebRunnerAsyncMessage
{
  GstWebRunnerCB callback;
  gpointer data;
  GDestroyNotify destroy;
} GstWebRunnerAsyncMessage;

typedef struct _GstWebRunnerSyncMessage
{
  GstWebRunner *self;
  gboolean fired;

  GstWebRunnerCB callback;
  gpointer data;
} GstWebRunnerSyncMessage;

G_DEFINE_TYPE_WITH_PRIVATE (GstWebRunner, gst_web_runner, GST_TYPE_OBJECT);

static void
_unlock_create_thread (GstWebRunner *self)
{
//  g_mutex_lock (&self->priv->create_lock);
  self->priv->created = TRUE;
  GST_INFO_OBJECT (self, "thread running");
  g_cond_signal (&self->priv->create_cond);
  //g_mutex_unlock (&self->priv->create_lock);
}

static void
_run_message_sync (GstWebRunnerSyncMessage *message)
{
  if (message->callback)
    message->callback (message->data);

  g_mutex_lock (&message->self->priv->sync_message_lock);
  message->fired = TRUE;
  g_cond_broadcast (&message->self->priv->sync_message_cond);
  g_mutex_unlock (&message->self->priv->sync_message_lock);
}

static gboolean
_run_message_async (GstWebRunnerAsyncMessage *message)
{
  if (message->callback)
    message->callback (message->data);

  if (message->destroy)
    message->destroy (message->data);

  g_free (message);

  return FALSE;
}

static void
gst_web_runner_quit (GstWebRunner *self)
{
  g_main_loop_quit (self->priv->loop);
}

gint
gst_web_runner_tid (GstWebRunner *self)
{
  return self->priv->tid;
}

static gpointer
gst_web_runner_run_thread (GstWebRunner *self)
{
  GstWebRunnerClass *klass;

  GST_DEBUG_OBJECT (self, "Creating thread");

  klass = GST_WEB_RUNNER_GET_CLASS (self);
//  g_mutex_lock (&self->priv->create_lock);

//  gst_web_runner_transfer_init_recv ??
  self->priv->tid = pthread_self ();
  self->priv->alive = TRUE;

  /* unlocking of the create_lock happens when the
   * self's loop is running from inside that loop */
  gst_web_runner_send_message_async (
      self, (GstWebRunnerCB) _unlock_create_thread, self, NULL);

  g_main_loop_run (self->priv->loop);

  GST_INFO_OBJECT (self, "loop exited");

  g_mutex_lock (&self->priv->create_lock);
  self->priv->alive = FALSE;
  self->priv->created = FALSE;

  g_cond_signal (&self->priv->destroy_cond);
  g_mutex_unlock (&self->priv->create_lock);

  return NULL;
}

static void
gst_web_runner_default_send_message (
    GstWebRunner *self, GstWebRunnerCB callback, gpointer data)
{
  GstWebRunnerSyncMessage message;

  message.self = self;
  message.callback = callback;
  message.data = data;
  message.fired = FALSE;

  gst_web_runner_send_message_async (
      self, (GstWebRunnerCB) _run_message_sync, &message, NULL);

  g_mutex_lock (&self->priv->sync_message_lock);

  /* block until calls have been executed in the thread */
  while (!message.fired)
    g_cond_wait (
        &self->priv->sync_message_cond, &self->priv->sync_message_lock);
  g_mutex_unlock (&self->priv->sync_message_lock);
}

static void
gst_web_runner_default_send_message_async (GstWebRunner *self,
    GstWebRunnerCB callback, gpointer data, GDestroyNotify destroy)
{
  GstWebRunnerAsyncMessage *message = g_new (GstWebRunnerAsyncMessage, 1);

  message->callback = callback;
  message->data = data;
  message->destroy = destroy;

  g_main_context_invoke (
      self->priv->main_context, (GSourceFunc) _run_message_async, message);
}

static GThread *
gst_web_runner_default_create_thread (
    GstWebRunner *self, const gchar *name, GThreadFunc run)
{
  if (self->priv->canvases) {
    /* Pthread creation needs to happen on the main thread for this to work */
    return g_thread_emscripten_new (name, self->priv->canvases, run, self);
  } else {
    return g_thread_new (name, run, self);
  }
}

static void
gst_web_runner_finalize (GObject *object)
{
  GstWebRunner *self = GST_WEB_RUNNER (object);

  g_mutex_lock (&self->priv->create_lock);
  if (self->priv->alive) {
    gst_web_runner_quit (self);

    GST_INFO_OBJECT (self, "joining thread");
    while (self->priv->alive)
      g_cond_wait (&self->priv->destroy_cond, &self->priv->create_lock);
    GST_INFO_OBJECT (self, "thread joined");
  }
  g_mutex_unlock (&self->priv->create_lock);

  if (self->priv->thread) {
    g_thread_unref (self->priv->thread);
    self->priv->thread = NULL;
  }

  g_mutex_clear (&self->priv->create_lock);

  g_cond_clear (&self->priv->create_cond);
  g_cond_clear (&self->priv->destroy_cond);

  GST_DEBUG_OBJECT (self, "End of finalize");
  G_OBJECT_CLASS (gst_web_runner_parent_class)->finalize (object);
}

static void
gst_web_runner_init (GstWebRunner *self)
{
  self->priv = gst_web_runner_get_instance_private (self);

  self->priv->main_context = g_main_context_new ();
  self->priv->loop = g_main_loop_new (self->priv->main_context, FALSE);
  g_mutex_init (&self->priv->create_lock);
  g_cond_init (&self->priv->create_cond);
  g_cond_init (&self->priv->destroy_cond);
  self->priv->created = FALSE;
}

static void
gst_web_runner_class_init (GstWebRunnerClass *klass)
{
  klass->create_thread =
      GST_DEBUG_FUNCPTR (gst_web_runner_default_create_thread);
  klass->send_message =
      GST_DEBUG_FUNCPTR (gst_web_runner_default_send_message);
  klass->send_message_async =
      GST_DEBUG_FUNCPTR (gst_web_runner_default_send_message_async);

  G_OBJECT_CLASS (klass)->finalize = gst_web_runner_finalize;
  GST_DEBUG_CATEGORY_INIT (
      web_runner_debug, "webrunner", 0, "Web related API's Runner");
}

/**
 * gst_web_runner_send_message_async:
 * @self: a #GstWebRunner
 * @callback: (scope async): function to invoke
 * @data: (closure): data to invoke @callback with
 * @destroy: called when @data is not needed anymore
 *
 * Invoke @callback with @data on the runner thread. The callback may not
 * have been executed when this function returns.
 */
void
gst_web_runner_send_message_async (GstWebRunner *self, GstWebRunnerCB callback,
    gpointer data, GDestroyNotify destroy)
{
  GstWebRunnerClass *klass;

  g_return_if_fail (GST_IS_WEB_RUNNER (self));
  g_return_if_fail (callback != NULL);
  klass = GST_WEB_RUNNER_GET_CLASS (self);
  g_return_if_fail (klass->send_message_async != NULL);

  klass->send_message_async (self, callback, data, destroy);
}

/**
 * gst_web_runner_send_message:
 * @self: a #GstWebRunner
 * @callback: (scope async): function to invoke
 * @data: (closure): data to invoke @callback with
 *
 * Invoke @callback with data on the runner thread. @callback is guaranteed to
 * have executed when this function returns.
 */
void
gst_web_runner_send_message (
    GstWebRunner *self, GstWebRunnerCB callback, gpointer data)
{
  GstWebRunnerClass *klass;

  g_return_if_fail (GST_IS_WEB_RUNNER (self));
  g_return_if_fail (callback != NULL);
  klass = GST_WEB_RUNNER_GET_CLASS (self);
  g_return_if_fail (klass->send_message != NULL);

  klass->send_message (self, callback, data);
}

/**
 * gst_web_runner_run:
 * @self: a #GstWebRunner:
 * @error: a #GError
 *
 * Creates the actual thread for the runner
 *
 * If an error occurs, and @error is not %NULL, then @error will contain
 * details of the error and %FALSE will be returned.
 *
 * Should only be called once.
 *
 * Returns: whether the runner could successfully be created
 */

gboolean
gst_web_runner_run (GstWebRunner *self, GError **error)
{
  gboolean alive = FALSE;

  g_return_val_if_fail (GST_IS_WEB_RUNNER (self), FALSE);

  g_mutex_lock (&self->priv->create_lock);

  if (!self->priv->created) {
    GstWebRunnerClass *klass;

    klass = GST_WEB_RUNNER_GET_CLASS (self);
    self->priv->thread = klass->create_thread (
        self, "webrunner", (GThreadFunc) gst_web_runner_run_thread);

    while (!self->priv->created)
      g_cond_wait (&self->priv->create_cond, &self->priv->create_lock);

    GST_INFO_OBJECT (self, "thread created");
  }

  alive = self->priv->alive;

  g_mutex_unlock (&self->priv->create_lock);

  return alive;
}

/**
 * gst_web_runner_new:
 * @canvases: The canvases the runner will have access to
 *
 * Create a new #GstWebRunner with the specified @canvases
 *
 * Returns: a new #GstWebRunner
 */
GstWebRunner *
gst_web_runner_new (const gchar *canvases)
{
  GstWebRunner *self;

  self = g_object_new (GST_TYPE_WEB_RUNNER, NULL);
  /* FIXME use a property canvases */
  /* FIXME create a new fundamental type GstWebStringList */
  self->priv->canvases = g_strdup (canvases);
  gst_object_ref_sink (self);

  return self;
}
