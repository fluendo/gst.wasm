/* Copyright (C) Fluendo S.A. <support@fluendo.com> */

#include <gst/emscripten/gstemscripten.h>

GST_DEBUG_CATEGORY_STATIC (example_dbg);
#define GST_CAT_DEFAULT example_dbg

static GstElement *pipeline;

static void
register_elements ()
{
  GST_PLUGIN_STATIC_DECLARE (videotestsrc);
  GST_PLUGIN_STATIC_DECLARE (sdl2);

  GST_PLUGIN_STATIC_REGISTER (videotestsrc);
  GST_PLUGIN_STATIC_REGISTER (sdl2);
}

static void
init_pipeline ()
{
  pipeline = gst_parse_launch ("videotestsrc pattern=ball ! sdl2sink", NULL);
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
