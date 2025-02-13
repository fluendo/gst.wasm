/*
 * Gst.WASM
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

GST_DEBUG_CATEGORY_STATIC (example_dbg);
#define GST_CAT_DEFAULT example_dbg

static GstElement *pipeline;

static void
register_elements ()
{
  GST_PLUGIN_STATIC_DECLARE (coreelements);
  GST_PLUGIN_STATIC_DECLARE (web);
  GST_PLUGIN_STATIC_DECLARE (opengl);
  GST_PLUGIN_STATIC_DECLARE (isomp4);
  GST_PLUGIN_STATIC_DECLARE (compositor);
  GST_PLUGIN_STATIC_DECLARE (videoconvertscale);
  GST_PLUGIN_STATIC_DECLARE (videotestsrc);

  GST_PLUGIN_STATIC_REGISTER (coreelements);
  GST_PLUGIN_STATIC_REGISTER (web);
  GST_PLUGIN_STATIC_REGISTER (opengl);
  GST_PLUGIN_STATIC_REGISTER (isomp4);
  GST_PLUGIN_STATIC_REGISTER (compositor);
  GST_PLUGIN_STATIC_REGISTER (videoconvertscale);
  GST_PLUGIN_STATIC_REGISTER (videotestsrc);
}

static void
init_pipeline ()
{
#define H264SRC(loc) " webstreamsrc "                                \
      "location=\"" loc "\" ! "                                      \
      " qtdemux ! "                                                  \
      " webcodecsviddech264sw "
  
  pipeline = gst_parse_launch (
      " compositor name=mix "
      " sink_1::width=300 sink_1::height=200 "
      " sink_1::xpos=0 sink_1::ypos=0 sink_1::zorder=1 sink_0::zorder=0"
      " ! queue ! webcanvassink "
      H264SRC ("https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4")
      " ! mix.sink_0 "
      " videotestsrc "
//      H264SRC ("http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ElephantsDream.mp4")
      " ! mix.sink_1 "
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
      "2", FALSE);

  gst_emscripten_init ();
  GST_INFO ("Registering elements");
  register_elements ();

  GST_INFO ("Initializing pipeline");
  init_pipeline ();

  return 0;
}
