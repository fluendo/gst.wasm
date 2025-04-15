/*
 * Gst.WASM
 * Copyright 2024-2025 Fluendo S.A.
 *  @author: Jorge Zapata <jzapata@fluendo.com>
 *  @author: Marek Olejnik <molejnik@fluendo.com>
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
  GST_ELEMENT_REGISTER_DECLARE (queue);
  GST_ELEMENT_REGISTER_DECLARE (qtdemux);
  GST_ELEMENT_REGISTER_DECLARE (glimagesink);
  GST_ELEMENT_REGISTER_DECLARE (web_canvas_sink);
  GST_ELEMENT_REGISTER_DECLARE (web_stream_src);
  GST_ELEMENT_REGISTER_DECLARE (videoconvert);
  GST_PLUGIN_STATIC_DECLARE (libav);

  gst_web_utils_init ();
  gst_web_video_frame_init ();

  GST_PLUGIN_STATIC_REGISTER (libav);
  GST_ELEMENT_REGISTER (web_canvas_sink, NULL);
  GST_ELEMENT_REGISTER (web_stream_src, NULL);
  GST_ELEMENT_REGISTER (glimagesink, NULL);
  GST_ELEMENT_REGISTER (qtdemux, NULL);
  GST_ELEMENT_REGISTER (queue, NULL);
  GST_ELEMENT_REGISTER (videoconvert, NULL);
}

static void
init_pipeline ()
{
  pipeline = gst_parse_launch (
      "webstreamsrc "
      "location=\"https://commondatastorage.googleapis.com/"
      "gtv-videos-bucket/sample/BigBuckBunny.mp4\" ! "
      "qtdemux ! "
      "avdec_h264 qos=false ! videoconvert ! queue ! webcanvassink",
      NULL);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char **argv)
{
  gst_debug_set_default_threshold (1);
  gst_init (NULL, NULL);
  GST_DEBUG_CATEGORY_INIT (
      example_dbg, "example", 0, "SW h264 decoder wasm example");
  gst_debug_set_threshold_from_string (
      "*:3, example:5, videodecoder*:3", FALSE);

  gst_emscripten_init ();
  GST_INFO ("Registering elements");
  register_elements ();

  GST_INFO ("Initializing pipeline");
  init_pipeline ();

  return 0;
}
