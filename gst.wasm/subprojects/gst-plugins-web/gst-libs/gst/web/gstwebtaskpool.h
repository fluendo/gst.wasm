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

#ifndef __GST_WEB_TASK_POOL_H__
#define __GST_WEB_TASK_POOL_H__

G_BEGIN_DECLS

#define GST_TYPE_WEB_TASK_POOL (gst_web_task_pool_get_type ())

typedef struct _GstWebTaskPool GstWebTaskPool;

GstWebTaskPool *gst_web_task_pool_new (void);
void gst_web_task_pool_resume (GstTaskPool *pool, gpointer id);

G_END_DECLS

#endif /* __GST_WEB_TASK_POOL_H__ */
