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

#ifndef __GST_WEB_TASK_H__
#define __GST_WEB_TASK_H__

G_BEGIN_DECLS

#define GST_TYPE_WEB_TASK (gst_web_task_get_type ())

typedef struct _GstWebTask GstWebTask;

GstWebTask *gst_web_task_new (
    GstTaskFunction func, gpointer user_data, GDestroyNotify notify);

G_END_DECLS

#endif /* __GST_WEB_TASK_H__ */
