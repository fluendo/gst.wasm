/*
 * GStreamer - gst.wasm codecs example
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

#include <gst/emscripten/gstemscripten.h>
#include <gst/web/gstwebutils.h>
#include <gst/web/gstwebvideoframe.h>

GST_DEBUG_CATEGORY_STATIC (example_dbg);
#define GST_CAT_DEFAULT example_dbg

static GstElement *pipeline;

static void
register_elements ()
{
  GST_ELEMENT_REGISTER_DECLARE (qtdemux);
  GST_ELEMENT_REGISTER_DECLARE (glimagesink);
  GST_PLUGIN_STATIC_DECLARE (web);
  GST_PLUGIN_STATIC_DECLARE (coreelements);
  GST_ELEMENT_REGISTER_DECLARE (openalsink);
  GST_ELEMENT_REGISTER_DECLARE (audioconvert);

  GST_PLUGIN_STATIC_REGISTER (web);
  GST_ELEMENT_REGISTER (glimagesink, NULL);
  GST_ELEMENT_REGISTER (qtdemux, NULL);
  GST_PLUGIN_STATIC_REGISTER (coreelements);
  GST_ELEMENT_REGISTER (openalsink, NULL);
  GST_ELEMENT_REGISTER (audioconvert, NULL);
}

#ifndef GSTWASM_CODECS_EXAMPLE_SRC
#define GSTWASM_CODECS_EXAMPLE_SRC "https://hbbtv-demo.fluendo.com/bbb.mp4"
#endif

static void
init_pipeline ()
{
  pipeline = gst_parse_launch (
      "webstreamsrc "
      "location=\"" GSTWASM_CODECS_EXAMPLE_SRC "\""
      " ! qtdemux name=demux multiqueue name=q"
#if 1 // <-- change to 0 to disable video
      " demux. ! video/x-h264 ! q. "
      " q. ! webcodecsviddech264sw ! webcanvassink"
#endif
#if 1 // <-- change to 0 to disable audio
      " demux. ! audio/mpeg ! q. "
      " q. ! webcodecsauddecaacsw ! audioconvert ! openalsink"
#endif
      ,
      NULL);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char **argv)
{
  gst_debug_set_default_threshold (1);
  gst_init (NULL, NULL);
  GST_DEBUG_CATEGORY_INIT (
      example_dbg, "example", 0, "webcodecs wasm example");
  gst_debug_set_threshold_from_string (
      "example:5, webcodecs*:3, videodecoder*:3", FALSE);

  gst_emscripten_init ();
  GST_INFO ("Registering elements");
  register_elements ();

  GST_INFO ("Initializing pipeline");
  init_pipeline ();

  return 0;
}
