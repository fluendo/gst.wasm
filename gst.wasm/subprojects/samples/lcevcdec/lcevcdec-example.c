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
  "https://d3mfda3gpj3dw1.cloudfront.net/vn9s0p86SVbJorX6/"                   \
  "lcevc_vn9s0p86SVbJorX6_2.mp4"

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

  GST_PLUGIN_STATIC_REGISTER (coreelements);
  GST_PLUGIN_STATIC_REGISTER (libav);
  GST_PLUGIN_STATIC_REGISTER (web);
  GST_ELEMENT_REGISTER (glimagesink, NULL);
  GST_ELEMENT_REGISTER (h264parse, NULL);
  GST_ELEMENT_REGISTER (lcevcdec, NULL);
  GST_ELEMENT_REGISTER (qtdemux, NULL);
}

static void
init_pipeline ()
{
  pipeline = gst_parse_launch (
      "webstreamsrc "
      "location=\"" GSTWASM_LCEVCDEC_EXAMPLE_SRC "\" ! qtdemux ! h264parse ! "
      "avdec_h264 ! lcevcdec ! webupload ! webcanvassink",
      NULL);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char **argv)
{
  gst_debug_set_default_threshold (1);
  gst_init (NULL, NULL);
  GST_DEBUG_CATEGORY_INIT (example_dbg, "example", 0, "lcevcdec wasm example");

  gst_emscripten_init ();
  GST_INFO ("Registering elements");
  register_elements ();

  GST_INFO ("Initializing pipeline");
  init_pipeline ();

  return 0;
}
