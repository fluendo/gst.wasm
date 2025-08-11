/*
 * Gst.WASM
 * Copyright 2024-2025 Fluendo S.A.
 *  @author: Alexander Slobodeniuk <aslobodeniuk@fluendo.com>
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
  GST_ELEMENT_REGISTER_DECLARE (capsfilter);
  GST_ELEMENT_REGISTER_DECLARE (qtdemux);
  GST_ELEMENT_REGISTER_DECLARE (
      glimagesink); /* FIXME: Should not be needed. (RDI-2866) */
  GST_ELEMENT_REGISTER_DECLARE (web_canvas_sink);
  GST_ELEMENT_REGISTER_DECLARE (web_stream_src);
  GST_ELEMENT_REGISTER_DECLARE (videoconvert);
  GST_ELEMENT_REGISTER_DECLARE (dashdemux);
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
  GST_ELEMENT_REGISTER (dashdemux, NULL);
  GST_ELEMENT_REGISTER (capsfilter, NULL);
}

#ifndef GSTWASM_DASH_EXAMPLE_SRC
#define GSTWASM_DASH_EXAMPLE_SRC "https://cmafref.akamaized.net/cmaf/live-ull/2006350/akambr/out.mpd"
#endif

#ifndef CANVAS_WIDTH
#define CANVAS_WIDTH 640
#endif

#ifndef CANVAS_HEIGHT
#define CANVAS_HEIGHT 480
#endif

static void
init_pipeline ()
{
  pipeline = gst_parse_launch (
      "webstreamsrc "
      "location=\"" GSTWASM_DASH_EXAMPLE_SRC "\" ! "
      "dashdemux presentation-delay=2s "
      "max-video-width=" G_STRINGIFY (CANVAS_WIDTH) " max-video-height=" G_STRINGIFY (CANVAS_HEIGHT) " ! "
      "video/quicktime ! qtdemux ! queue ! "
      "avdec_h264 qos=false max-threads=4 ! videoconvert ! webcanvassink",
      NULL);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char **argv)
{
  gst_debug_set_default_threshold (1);
  gst_debug_set_colored (FALSE);

  gst_init (NULL, NULL);
  GST_DEBUG_CATEGORY_INIT (
      example_dbg, "example", 0, "SW h264 decoder wasm example");
  gst_debug_set_threshold_from_string ("2", FALSE);

  gst_emscripten_init ();
  GST_INFO ("Registering elements");
  register_elements ();

  GST_INFO ("Initializing pipeline");
  init_pipeline ();

  return 0;
}
