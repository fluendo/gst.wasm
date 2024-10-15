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

#include <gst/gst.h>

GST_DEBUG_CATEGORY_STATIC (example_dbg);
#define GST_CAT_DEFAULT example_dbg

static GstElement *pipeline;

static void
register_elements ()
{
  GST_PLUGIN_STATIC_DECLARE (coreelements);
  GST_PLUGIN_STATIC_DECLARE (web);

  GST_PLUGIN_STATIC_REGISTER (coreelements);
  GST_PLUGIN_STATIC_REGISTER (web);
}

static void
init_pipeline ()
{
#if 0
  pipeline = gst_parse_launch ("webtransportsrc "
                               //"location=\"https://localhost:4443\" ! "
                               "location=\"https://localhost:4443\" name=src"
                               " src.rbidi_0 ! "
                               //"location=\"https://relay.quic.video\" ! "
                               "fakesink dump=true",
      NULL);
#endif
  pipeline =
      gst_parse_launch ("webtransportsrc location=\"https://127.0.0.1:4443\" "
                        "server-certificate-hashes=<(string)"
                        "\"b958e2cf0a2903098e229c576e84f65a72c4c69c57e7244de4e"
                        "c1954a793b972\"> name=src "
                        " src.datagram ! fakesink dump=true",
          NULL);
  // pipeline = gst_parse_launch ("webtransportsrc
  // location=\"https://127.0.0.1:4443\"
  // server-certificate-hashes=<(string)\"b958e2cf0a2903098e229c576e84f65a72c4c69c57e7244de4ec1954a793b972\">
  // name=src"
  //                              " src.unidi_0 ! fakesink dump=true", NULL);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char **argv)
{
  gst_debug_set_default_threshold (1);
  gst_init (NULL, NULL);
  GST_DEBUG_CATEGORY_INIT (example_dbg, "example", 0, "GstWASM example debug");
  gst_debug_set_threshold_from_string ("example:5, webtransport*:5", FALSE);

  GST_INFO ("Registering elements");
  register_elements ();

  GST_INFO ("Initializing pipeline");
  init_pipeline ();

  return 0;
}
