/* Copyright (C) Fluendo S.A. <support@fluendo.com> */

#include <gst/gst.h>

GST_DEBUG_CATEGORY_STATIC (example_dbg);
#define GST_CAT_DEFAULT example_dbg

static GstElement *pipeline;

static void
register_elements ()
{
  GST_PLUGIN_STATIC_DECLARE (opengl);

  GST_PLUGIN_STATIC_REGISTER (opengl);
}

static void
init_pipeline ()
{
  pipeline = gst_parse_launch ("gltestsrc ! glimagesink sync=false", NULL);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char **argv)
{
  gst_debug_set_default_threshold (2);
  gst_init (NULL, NULL);
  GST_DEBUG_CATEGORY_INIT (
      example_dbg, "example", 0, "videotestsrc wasm example");
  gst_debug_set_threshold_from_string ("example:5, glwindow*:5", FALSE);

  GST_INFO ("Registering elements");
  register_elements ();

  GST_INFO ("Initializing pipeline");
  init_pipeline ();

  return 0;
}
