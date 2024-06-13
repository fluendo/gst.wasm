/* Copyright (C) <2024> Fluendo S.A. <support@fluendo.com> */
#include <gst/gst.h>

GST_DEBUG_CATEGORY_STATIC (example_dbg);
#define GST_CAT_DEFAULT example_dbg

static struct
{
  GstElement *pipe;
} context;

static void init_pipeline ()
{
  GST_PLUGIN_STATIC_DECLARE(coreelements);
  GST_PLUGIN_STATIC_DECLARE(imagebitmapsrc);

  GST_PLUGIN_STATIC_REGISTER(coreelements);
  GST_PLUGIN_STATIC_REGISTER(imagebitmapsrc);

  context.pipe = gst_parse_launch ("imagebitmapsrc id=canvas ! fakesink", NULL);
  g_assert (context.pipe);
}

int main(int argc, char** argv) {  
  gst_init (NULL, NULL);
  GST_DEBUG_CATEGORY_INIT (example_dbg, "example",
      0, "Audio example");

  gst_debug_set_color_mode (GST_DEBUG_COLOR_MODE_OFF);
  gst_debug_set_threshold_from_string ("*:2,example:5", TRUE);

  GST_DEBUG ("Init pipeline");
  init_pipeline ();
  
  gst_element_set_state (context.pipe, GST_STATE_PLAYING);
  return 0;
}