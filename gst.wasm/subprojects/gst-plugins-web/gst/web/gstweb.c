/*
 * GStreamer - gst.wasm GstWeb source
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

#include <gst/web/gstwebutils.h>
#include <gst/web/gstwebvideoframe.h>

#include "gstweb.h"
#include "gstwebfetchsrc.h"
#include "gstwebstreamsrc.h"
#include "gstwebcanvassink.h"
#include "gstwebcanvassrc.h"
#include "gstwebdownload.h"
#include "gstwebupload.h"

#include "codecs/gstwebcodecs.h"
#include "transport/gstwebtransportsrc.h"

static gboolean
plugin_init (GstPlugin *plugin)
{
  gst_web_utils_init ();
  gst_web_video_frame_init ();
  gst_web_codecs_init (plugin);

  gst_element_register_web_canvas_sink (plugin);
  gst_element_register_web_canvas_src (plugin);
  gst_element_register_web_fetch_src (plugin);
  gst_element_register_web_stream_src (plugin);
  gst_element_register_web_transport_src (plugin);
  gst_element_register_web_download (plugin);
  gst_element_register_web_upload (plugin);

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, web,
    "web related elements", plugin_init, VERSION, GST_LICENSE,
    GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
