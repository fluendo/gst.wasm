/* Copyright (C) <2024> Fluendo S.A. <support@fluendo.com> */
#include <gst/gst.h>
#include <emscripten.h>

GST_DEBUG_CATEGORY_STATIC (example_dbg);
#define GST_CAT_DEFAULT example_dbg

static struct
{
  GstElement *pipe;
} context;

static void
fakesink_handoff_cb (
    GstElement *object, GstBuffer *buffer, GstPad *arg1, gpointer user_data)
{
  char *zero_terminated_data;
  GstMapInfo info;

  gst_buffer_map (buffer, &info, GST_MAP_READ);

  zero_terminated_data = g_malloc (info.size + 1);
  zero_terminated_data[info.size] = 0;
  memcpy (zero_terminated_data, info.data, info.size);

  g_print ("%s\n", zero_terminated_data);

  g_free (zero_terminated_data);
  gst_buffer_unmap (buffer, &info);
}

void
play ()
{
  gst_element_set_state (context.pipe, GST_STATE_PLAYING);
}

static void
init_pipeline ()
{
  GstElement *fakesink;

  GST_DEBUG ("Init pipeline");

  GST_PLUGIN_STATIC_DECLARE (coreelements);
  GST_PLUGIN_STATIC_DECLARE (emhttpsrc);

  GST_PLUGIN_STATIC_REGISTER (coreelements);
  GST_PLUGIN_STATIC_REGISTER (emhttpsrc);

  context.pipe = gst_parse_launch (
      "emhttpsrc location=\"http://localhost:3000/hello.txt\" ! fakesink "
      "name=fk signal-handoffs=true",
      NULL);
  g_assert (context.pipe);

  fakesink = gst_bin_get_by_name (GST_BIN (context.pipe), "fk");
  g_assert (fakesink);

  g_signal_connect (
      fakesink, "handoff", G_CALLBACK (fakesink_handoff_cb), NULL);

  gst_object_unref (fakesink);
}

int
main (int argc, char **argv)
{
  gst_init (NULL, NULL);
  GST_DEBUG_CATEGORY_INIT (example_dbg, "example", 0, "HTTPsrc example");

  gst_debug_set_color_mode (GST_DEBUG_COLOR_MODE_OFF);
  gst_debug_set_threshold_from_string ("*:2,emhttpsrc:8,example:5", TRUE);

  init_pipeline ();
  play ();
  return 0;
}