 /*
 * GStreamer - gst.wasm videotestsrc example
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

#include <gst/emscripten/gstemscripten.h>

static GstElement *pipeline;

static void
register_elements ()
{
  GST_PLUGIN_STATIC_DECLARE (videotestsrc);
  GST_PLUGIN_STATIC_DECLARE (sdl2);

  GST_PLUGIN_STATIC_REGISTER (videotestsrc);
  GST_PLUGIN_STATIC_REGISTER (sdl2);
}

EM_JS(void, debuggg, (const char* str), {
  const msg = UTF8ToString(str);

  try {
    const outputBox = document.getElementById("debuggg");
    if (outputBox)
      outputBox.textContent += msg;
  } catch (e) {
  }
  
  console.log (msg);
});

GType gst_video_test_src_get_type(void);
GType gst_sdl2_sink_get_type (void);

static void
init_pipeline ()
{
  pipeline = gst_pipeline_new (NULL);
  GstElement *videotestsrc = g_object_new (gst_video_test_src_get_type (), "pattern", 18, NULL);
  GstElement *sdl2sink = g_object_new (gst_sdl2_sink_get_type (), NULL);

  gst_bin_add_many (GST_BIN_CAST (pipeline),
      GST_ELEMENT_CAST (videotestsrc),
      GST_ELEMENT_CAST (sdl2sink), NULL);

  gst_element_link (videotestsrc, sdl2sink);
  
  debuggg ("create pipeline ok\n");
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char **argv)
{
  gst_init (NULL, NULL);
  gst_emscripten_init ();
  register_elements ();

  GST_DEBUG_CATEGORY_INIT (
      example_dbg, "example", 0, "videotestsrc wasm example");
  gst_debug_set_threshold_for_name ("example", 5);

  init_pipeline ();

  return 0;
}
