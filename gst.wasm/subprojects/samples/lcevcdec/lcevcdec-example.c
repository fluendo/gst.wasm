/*
 * GStreamer - gst.wasm LCEVC decoder example
 *
 * Copyright 2025 Fluendo S.A.
 *  @author: César Fabián Orccón Chipana <forccon@fluendo.com>
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

#define GSTWASM_LCEVCDEC_EXAMPLE_SRC                                          \
  "http://localhost:6931/sample.mp4"

static GstElement *pipeline;

static void
register_elements ()
{
  GST_PLUGIN_STATIC_DECLARE (coreelements);
  GST_PLUGIN_STATIC_DECLARE (libav);
  GST_PLUGIN_STATIC_DECLARE (web);
  GST_ELEMENT_REGISTER_DECLARE (
      glimagesink); /* FIXME: Should not be needed. */
  GST_ELEMENT_REGISTER_DECLARE (h264parse);
  GST_ELEMENT_REGISTER_DECLARE (lcevcdec);
  GST_ELEMENT_REGISTER_DECLARE (qtdemux);
  GST_ELEMENT_REGISTER_DECLARE (videoconvert);
  GST_ELEMENT_REGISTER_DECLARE (openalsink);
  GST_ELEMENT_REGISTER_DECLARE (audioconvert);

  GST_PLUGIN_STATIC_REGISTER (coreelements);
  GST_PLUGIN_STATIC_REGISTER (libav);
  GST_PLUGIN_STATIC_REGISTER (web);
  GST_ELEMENT_REGISTER (glimagesink, NULL);
  GST_ELEMENT_REGISTER (h264parse, NULL);
  GST_ELEMENT_REGISTER (lcevcdec, NULL);
  GST_ELEMENT_REGISTER (qtdemux, NULL);
  GST_ELEMENT_REGISTER (videoconvert, NULL);
  GST_ELEMENT_REGISTER (openalsink, NULL);
  GST_ELEMENT_REGISTER (audioconvert, NULL);
}

static void
init_pipeline ()
{
  pipeline = gst_parse_launch (
#if 1
      "webstreamsrc "
      "location=\"" GSTWASM_LCEVCDEC_EXAMPLE_SRC "\" ! qtdemux name=m ! multiqueue max-size-buffers=100 name=mm "

      //            " ! h264parse ! avdec_h264 max-threads=4 ! lcevcdec ! videoconvert ! webcanvassink"

      " ! h264parse ! webcodecsviddech264sw ! queue ! videoconvert ! lcevcdec ! videoconvert ! queue ! webcanvassink"
      
      //      " mm. ! audio/mpeg ! webcodecsauddecaacsw ! audioconvert ! openalsink async=false"
      ,
#endif

#if 0
      "webstreamsrc "
      "location=\"https://commondatastorage.googleapis.com/"
      "gtv-videos-bucket/sample/BigBuckBunny.mp4\" ! "
      "qtdemux ! "
      "audio/mpeg ! webcodecsauddecaacsw ! audioconvert ! openalsink",
#endif
      
      NULL);

  g_usleep (G_TIME_SPAN_SECOND*3);
  
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char **argv)
{
  //  gst_debug_set_default_threshold (3);
  gst_init (NULL, NULL);
  GST_DEBUG_CATEGORY_INIT (example_dbg, "example", 0, "lcevcdec wasm example");

  gst_emscripten_init ();
  GST_INFO ("Registering elements");
  register_elements ();

  gst_debug_set_threshold_from_string (
      "*webcodecsauddec*:6, *audiodecoder*:6",
      FALSE);

  GST_INFO ("Initializing pipeline");
  init_pipeline ();

  return 0;
}
