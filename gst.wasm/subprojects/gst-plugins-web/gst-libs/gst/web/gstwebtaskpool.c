/*
 * GStreamer - gst.wasm WebTaskPool source
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
#include <config.h>
#endif

#include <gst/gst.h>
#include "gstwebtaskpool.h"

typedef struct _GstWebTaskPool
{
  GstTaskPool base;
} GstWebTaskPool;

typedef struct _GstWebTaskPoolClass
{
  GstTaskPoolClass base;
} GstWebTaskPoolClass;

typedef struct _GstWebTaskPoolData
{
  GMainLoop *loop;
  GMainContext *ctx;
  GThread *thread;
  GstTaskPoolFunction func;
  gpointer user_data;
  gboolean created;
  GCond create_cond;
  GMutex create_lock;
} GstWebTaskPoolData;

G_DEFINE_FINAL_TYPE (GstWebTaskPool, gst_web_task_pool, GST_TYPE_TASK_POOL)

static gboolean
gst_web_task_pool_run (GstWebTaskPoolData *data)
{
  GstTaskPoolFunction func;
  gpointer user_data;

  func = data->func;
  user_data = data->user_data;
  func (user_data);
  return FALSE;
}

static gpointer
gst_web_task_pool_main_loop (GstWebTaskPoolData *data)
{
  /* Notify the caller that the thread is ready */
  g_mutex_lock (&data->create_lock);
  data->created = TRUE;
  g_cond_signal (&data->create_cond);
  g_mutex_unlock (&data->create_lock);

  g_main_loop_run (data->loop);
  return NULL;
}

static gpointer
gst_web_task_pool_push (GstTaskPool *pool, GstTaskPoolFunction func,
    gpointer user_data, GError **error)
{
  GstWebTaskPoolData *data;

  data = g_new (GstWebTaskPoolData, 1);
  data->ctx = g_main_context_new ();
  data->loop = g_main_loop_new (data->ctx, FALSE);
  data->func = func;
  data->user_data = user_data;
  data->created = FALSE;
  g_cond_init (&data->create_cond);
  g_mutex_init (&data->create_lock);

  g_mutex_lock (&data->create_lock);
  data->thread =
      g_thread_new (NULL, (GThreadFunc) gst_web_task_pool_main_loop, data);
  /* Wait until the thread is created */
  while (!data->created)
    g_cond_wait (&data->create_cond, &data->create_lock);
  g_mutex_unlock (&data->create_lock);

  g_main_context_invoke (data->ctx, (GSourceFunc) gst_web_task_pool_run, data);

  return data;
}

static void
gst_web_task_pool_join (GstTaskPool *pool, gpointer id)
{
  /* TODO what to do here? */
  /* g_main_loop_quit () */
}

static void
gst_web_task_pool_dispose_handle (GstTaskPool *pool, gpointer id)
{
  GstWebTaskPoolData *data = id;
  g_cond_clear (&data->create_cond);
  g_mutex_clear (&data->create_lock);
  g_free (data);
}

static void
gst_web_task_pool_init (GstWebTaskPool *self)
{
}

static void
gst_web_task_pool_class_init (GstWebTaskPoolClass *klass)
{
  GstTaskPoolClass *task_pool_class = (GstTaskPoolClass *) klass;

  task_pool_class->push = gst_web_task_pool_push;
  task_pool_class->join = gst_web_task_pool_join;
  task_pool_class->dispose_handle = gst_web_task_pool_dispose_handle;
}

GstWebTaskPool *
gst_web_task_pool_new (void)
{
  GstWebTaskPool *self;

  self = g_object_new (GST_TYPE_WEB_TASK_POOL, NULL);

  /* clear floating flag */
  gst_object_ref_sink (self);

  return self;
}

void
gst_web_task_pool_resume (GstTaskPool *pool, gpointer id)
{
  GstWebTaskPoolData *data = id;

  GST_DEBUG_OBJECT (pool, "About to resume on id %p", id);
  g_main_context_invoke (data->ctx, (GSourceFunc) gst_web_task_pool_run, data);
}
