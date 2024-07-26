/* Copyright (C) Fluendo S.A. <support@fluendo.com> */

#include <stdio.h>

#include <emscripten.h>
#include <emscripten/threading.h>

#include <gst/gst.h>

GST_DEBUG_CATEGORY_STATIC (example_dbg);
#define GST_CAT_DEFAULT example_dbg

static GstElement *pipeline;

static void
register_elements ()
{
  GST_PLUGIN_STATIC_DECLARE (audiotestsrc);
  GST_PLUGIN_STATIC_DECLARE (openal);

  GST_PLUGIN_STATIC_REGISTER (audiotestsrc);
  GST_PLUGIN_STATIC_REGISTER (openal);
}

void
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
