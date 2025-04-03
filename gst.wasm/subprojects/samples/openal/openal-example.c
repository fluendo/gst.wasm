 /*
 * GStreamer - gst.wasm OpenAL example
 *
 * Copyright 2024 Fluendo S.A.
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

#include <stdio.h>
#include <emscripten.h>
#include <emscripten/threading.h>
#include <gst/gst.h>

#define GST_CAT_DEFAULT example_dbg
GST_DEBUG_CATEGORY_STATIC (example_dbg);

static GstElement *pipeline;

static void
register_elements ()
{
  GST_PLUGIN_STATIC_DECLARE (audiotestsrc);
  GST_PLUGIN_STATIC_DECLARE (openal);

  GST_PLUGIN_STATIC_REGISTER (audiotestsrc);
  GST_PLUGIN_STATIC_REGISTER (openal);
}

static void
init_pipeline ()
{
  GST_DEBUG ("Init pipeline");
  pipeline = gst_parse_launch ("audiotestsrc ! openalsink", NULL);
  g_assert (pipeline);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char **argv)
{
  gst_init (NULL, NULL);
  register_elements ();

  GST_DEBUG_CATEGORY_INIT (example_dbg, "example", 0, "Audio example");

  gst_debug_set_color_mode (GST_DEBUG_COLOR_MODE_OFF);
  gst_debug_set_threshold_from_string ("*:2,example:5", TRUE);

  init_pipeline ();

  return 0;
}
