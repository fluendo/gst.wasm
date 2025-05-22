/*
 * GStreamer - gst.wasm WebTransportSrc example
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

#include <gst/gst.h>

// gst-launch-1.0 -v videotestsrc is-live=true do-timestamp=true ! openh264enc ! quinnwtserversink address=127.0.0.1 port=4443 certificate-file=ext/moq-rs/dev/localhost.crt private-key-file=ext/moq-rs/dev/localhost.key
// emrun _builddir/subprojects/samples/webtransportsrc-streams/webtransportsrc-streams-example.html --browser chrome

GST_DEBUG_CATEGORY_STATIC (example_dbg);
#define GST_CAT_DEFAULT example_dbg

static GstElement *pipeline;

static void
register_elements ()
{
  GST_PLUGIN_STATIC_DECLARE (coreelements);
  GST_PLUGIN_STATIC_DECLARE (web);
  GST_PLUGIN_STATIC_DECLARE (rtp);
  GST_PLUGIN_STATIC_DECLARE (opengl);
  GST_ELEMENT_REGISTER_DECLARE (h264parse);
  GST_ELEMENT_REGISTER_DECLARE (videoconvert);
  GST_PLUGIN_STATIC_DECLARE (libav);

  GST_PLUGIN_STATIC_REGISTER (libav);
  GST_PLUGIN_STATIC_REGISTER (coreelements);
  GST_PLUGIN_STATIC_REGISTER (web);
  GST_PLUGIN_STATIC_REGISTER (rtp);
  GST_PLUGIN_STATIC_REGISTER (opengl);
  GST_ELEMENT_REGISTER (videoconvert, NULL);
  GST_ELEMENT_REGISTER (h264parse, NULL);
}

static void
init_pipeline ()
{
  GstStateChangeReturn ret;
  pipeline = gst_parse_launch (
      "webtransportsrc location=\"https://127.0.0.1:4443\" "
      "name=src "
      "src.unidi_0 ! "
      "queue ! "
      "video/x-h264, width=320, height=240, format=I420, framerate=30/1 ! "
      "h264parse ! avdec_h264 ! videoconvert ! webcanvassink",
      NULL);
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  GST_INFO ("Pipeline state: %s", gst_element_state_change_return_get_name(ret));
}

int
main (int argc, char **argv)
{
  gst_debug_set_default_threshold (1);
  gst_init (NULL, NULL);
  GST_DEBUG_CATEGORY_INIT (example_dbg, "example", 0, "GstWASM example debug");
  gst_debug_set_threshold_from_string (
      "example:5, webtransport*:7, webtransferable*:7,2", FALSE);

  GST_INFO ("Registering elements");
  register_elements ();

  GST_INFO ("Initializing pipeline");
  init_pipeline ();

  return 0;
}
