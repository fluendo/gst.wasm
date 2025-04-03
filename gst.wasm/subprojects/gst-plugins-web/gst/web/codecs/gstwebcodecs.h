/*
 * GStreamer - gst.wasm WebCodecs source
 *
 * Copyright 2024 Fluendo S.A.
 * @author: Jorge Zapata <jzapata@fluendo.com>
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

#ifndef __GST_WEB_CODECS_H__
#define __GST_WEB_CODECS_H__

#include <gst/gst.h>

G_BEGIN_DECLS

gboolean gst_web_codecs_init (GstPlugin *plugin);

extern GQuark gst_web_codecs_data_quark;

G_END_DECLS

#endif
