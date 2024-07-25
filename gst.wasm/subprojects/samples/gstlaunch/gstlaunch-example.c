/* Copyright (C) Fluendo S.A. <support@fluendo.com> */

#include <gst/gst.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <gst/emscripten/gstemscripten.h>

GST_DEBUG_CATEGORY_STATIC (example_dbg);
#define GST_CAT_DEFAULT example_dbg

static struct
{
  GstElement *pipe;
} context;

static gboolean init;

void
init_pipeline ()
{
  if (!init) {
    init = TRUE;
    gst_init (NULL, NULL);

    GST_DEBUG_CATEGORY_INIT (
        example_dbg, "example", 0, "videotestsrc wasm example");
    gst_debug_set_threshold_from_string ("example:5, *:3", TRUE);

    GST_PLUGIN_STATIC_DECLARE (videotestsrc);
    GST_PLUGIN_STATIC_DECLARE (audiotestsrc);
    GST_PLUGIN_STATIC_DECLARE (sdl2);
    GST_PLUGIN_STATIC_DECLARE (openal);
    GST_PLUGIN_STATIC_DECLARE (coreelements);

    GST_PLUGIN_STATIC_REGISTER (videotestsrc);
    GST_PLUGIN_STATIC_REGISTER (audiotestsrc);
    GST_PLUGIN_STATIC_REGISTER (sdl2);
    GST_PLUGIN_STATIC_REGISTER (openal);
    GST_PLUGIN_STATIC_REGISTER (coreelements);

    gst_emscripten_init ();
  }

  if (context.pipe) {
    gst_element_set_state (context.pipe, GST_STATE_NULL);
    g_clear_pointer (&context.pipe, gst_object_unref);
  }

  const char *value = emscripten_run_script_string (
      "document.getElementById('myInput').value");
  GST_INFO ("pipeline: %s", value);

  context.pipe = gst_parse_launch (value, NULL);
  gst_element_set_state (context.pipe, GST_STATE_PLAYING);
}

int
main (int argc, char **argv)
{
  EM_ASM (
      { Module._init_pipeline = Module.cwrap ('init_pipeline', null, []); });

  return 0;
}
