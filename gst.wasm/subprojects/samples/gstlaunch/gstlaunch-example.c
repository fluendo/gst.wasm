/*
 * GStreamer - gst.wasm gstlaunch example
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
#include <gst/gst.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <gst/emscripten/gstemscripten.h>

#define GST_CAT_DEFAULT example_dbg
#define PIPELINE_INPUT_ID "pipeline"

GST_DEBUG_CATEGORY_STATIC (example_dbg);

static GstElement *pipeline;

static void
register_elements ()
{
  GST_PLUGIN_STATIC_DECLARE (audiotestsrc);
  GST_PLUGIN_STATIC_DECLARE (coreelements);
  GST_PLUGIN_STATIC_DECLARE (openal);
  GST_PLUGIN_STATIC_DECLARE (sdl2);
  GST_PLUGIN_STATIC_DECLARE (videotestsrc);

  GST_PLUGIN_STATIC_REGISTER (audiotestsrc);
  GST_PLUGIN_STATIC_REGISTER (coreelements);
  GST_PLUGIN_STATIC_REGISTER (openal);
  GST_PLUGIN_STATIC_REGISTER (sdl2);
  GST_PLUGIN_STATIC_REGISTER (videotestsrc);
}

void
init_pipeline ()
{
  if (pipeline) {
    gst_element_set_state (pipeline, GST_STATE_NULL);
    g_clear_pointer (&pipeline, gst_object_unref);
  }

  const char *value = emscripten_run_script_string (
      "document.getElementById('" PIPELINE_INPUT_ID "').value");
  GST_INFO ("pipeline: %s", value);

  pipeline = gst_parse_launch (value, NULL);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char **argv)
{
  gst_init (NULL, NULL);
  gst_emscripten_init ();
  register_elements ();

  GST_DEBUG_CATEGORY_INIT (
      example_dbg, "example", 0, "gstlaunch wasm example");
  gst_debug_set_threshold_from_string ("example:5, *:3", TRUE);

  EM_ASM (
      { Module._init_pipeline = Module.cwrap ('init_pipeline', null, []); });

  return 0;
}
