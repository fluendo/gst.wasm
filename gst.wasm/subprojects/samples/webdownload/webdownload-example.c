/*
 * GStreamer - gst.wasm WebDownload example
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

#define DEFAULT_URL                                                           \
  "https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/"        \
  "BigBuckBunny.mp4"

#define GST_CAT_DEFAULT example_dbg
GST_DEBUG_CATEGORY_STATIC (example_dbg);

static GstElement *pipeline;

static void
register_elements ()
{
  GST_PLUGIN_STATIC_DECLARE (coreelements);
  GST_PLUGIN_STATIC_DECLARE (web);
  GST_PLUGIN_STATIC_DECLARE (isomp4);
  GST_PLUGIN_STATIC_DECLARE (debugutilsbad);

  GST_PLUGIN_STATIC_REGISTER (coreelements);
  GST_PLUGIN_STATIC_REGISTER (web);
  GST_PLUGIN_STATIC_REGISTER (isomp4);
  GST_PLUGIN_STATIC_REGISTER (debugutilsbad);
}

static void
init_pipeline ()
{
  pipeline =
      gst_parse_launch ("webstreamsrc location=" DEFAULT_URL " ! "
                        "qtdemux ! webcodecsviddech264sw ! webdownload ! "
                        "video/x-raw ! checksumsink",
          NULL);

  g_signal_connect (pipeline, "deep-notify",
      G_CALLBACK (gst_object_default_deep_notify), NULL);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char **argv)
{
  gst_debug_set_default_threshold (1);
  gst_init (NULL, NULL);
  gst_emscripten_init ();

  GST_DEBUG_CATEGORY_INIT (example_dbg, "example", 0, "webcodecs example");
  gst_debug_set_threshold_from_string ("example:5,webdownload:5", FALSE);

  GST_INFO ("Registering elements");
  register_elements ();

  GST_INFO ("Initializing pipeline");
  init_pipeline ();

  return 0;
}
