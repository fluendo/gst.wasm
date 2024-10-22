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
#include <config.h>
#endif

#include <gst/gst.h>
#include "gstwebtask.h"
#include "gstwebtaskpool.h"

#define GET_TASK_STATE(t)                                                     \
  ((GstTaskState) g_atomic_int_get (&GST_TASK_STATE (t)))

typedef struct _GstWebTask
{
  GstTask base;
} GstWebTask;

typedef struct _GstWebTaskClass
{
  GstTaskClass base;
} GstWebTaskClass;

G_DEFINE_FINAL_TYPE (GstWebTask, gst_web_task, GST_TYPE_TASK)

static void
gst_web_task_execute (GstTask *task, GRecMutex *lock)
{
  GST_DEBUG_OBJECT (task, "Calling task function indefinitely");
  while (G_LIKELY (GET_TASK_STATE (task) == GST_TASK_STARTED)) {
    task->func (task->user_data);
  }
}

static void
gst_web_task_set_state (
    GstTask *task, GstTaskState old_state, GstTaskState new_state)
{
  /* Schedule again the task */
  if (old_state == GST_TASK_PAUSED && new_state == GST_TASK_STARTED) {
    GST_DEBUG_OBJECT (task, "Task was paused, needs to be resumed");
    GstTaskPool *pool;
    gpointer id;

    pool = gst_task_get_pool (task);
    id = gst_task_get_id (task);
    gst_web_task_pool_resume (pool, id);
    gst_object_unref (pool);
  }
}

static void
gst_web_task_init (GstWebTask *self)
{
}

static void
gst_web_task_class_init (GstWebTaskClass *klass)
{
  GstTaskClass *task_class = (GstTaskClass *) klass;

  /* TODO Set the default pool for this task class */
  task_class->execute = gst_web_task_execute;
  task_class->set_state = gst_web_task_set_state;
}

GstWebTask *
gst_web_task_new (
    GstTaskFunction func, gpointer user_data, GDestroyNotify notify)
{
  GstWebTask *self;

  g_return_val_if_fail (func != NULL, NULL);

  self = g_object_new (GST_TYPE_WEB_TASK, NULL);
  self->base.func = func;
  self->base.user_data = user_data;
  self->base.notify = notify;

  GST_DEBUG_OBJECT (self, "Created task %p", self);

  /* clear floating flag */
  gst_object_ref_sink (self);

  return self;
}
