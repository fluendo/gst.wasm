/*
 * GStreamer - GStreamer ImageBitmap Webcam example
 *
 * Copyright 2024 Fluendo S.A.
 *  @author: Cesar Fabian Orccon Chipana <forccon@fluendo.com>
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
#include <gst/emscripten/gstemscripten.h>

GST_DEBUG_CATEGORY_STATIC (example_dbg);
#define GST_CAT_DEFAULT example_dbg

static GstElement *pipeline;

static void
register_elements ()
{
  GST_PLUGIN_STATIC_DECLARE (coreelements);
  GST_PLUGIN_STATIC_DECLARE (webcanvassrc);
  GST_PLUGIN_STATIC_DECLARE (sdl2);
  GST_PLUGIN_STATIC_DECLARE (videoconvertscale);
  GST_PLUGIN_STATIC_DECLARE (coreelements);

  GST_PLUGIN_STATIC_REGISTER (coreelements);
  GST_PLUGIN_STATIC_REGISTER (webcanvassrc);
  GST_PLUGIN_STATIC_REGISTER (sdl2);
  GST_PLUGIN_STATIC_REGISTER (videoconvertscale);
  GST_PLUGIN_STATIC_REGISTER (coreelements);
}

void
init_pipeline ()
{
  GST_DEBUG ("Init pipeline");

  pipeline = gst_parse_launch (
      "webcanvassrc id=canvas2 ! videoconvert ! sdl2sink", NULL);
  g_assert (pipeline);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

EM_JS (void, add_new_canvas, (), {
  const canvas1 = document.getElementById ("canvas");
  const canvas2 = window.canvas2 =
      new OffscreenCanvas (canvas1.width, canvas1.height);
  const ctx2 = canvas2.getContext ("2d", { willReadFrequently : true });

  if (window.MediaStreamTrackProcessor) {
    let started = false;
    // clang-format off
    (async () => {
    // clang-format off
      const stream =
          await navigator.mediaDevices.getUserMedia ({ video : true });
      const track = stream.getVideoTracks ()[0];
      const processor = new MediaStreamTrackProcessor (track);
      const reader = processor.readable.getReader ();
      readFrame ();
      function readFrame ()
      {
        // clang-format off
        reader.read ().then (({ done, value }) => {
          // clang-format on
          canvas2.width = value.displayWidth;
          canvas2.height = value.displayHeight;

          if (!started) {
            canvas1.width = canvas2.width;
            canvas1.height = canvas2.height;
            started = true;
            _init_pipeline ();
          }

          ctx2.drawImage (value, 0, 0);
          value.close ();

          if (!done)
            readFrame ();
        });
      }
    }) ();
  } else {
    alert ('Browser does not support `MediaStreamTrackProcessor`.');
  }
});

int
main (int argc, char **argv)
{
  EM_ASM (
      { Module._init_pipeline = Module.cwrap ('init_pipeline', null, []); });

  add_new_canvas ();

  gst_init (NULL, NULL);
  gst_emscripten_init ();
  register_elements ();

  GST_DEBUG_CATEGORY_INIT (example_dbg, "example", 0, "ImageBitmap example");

  gst_debug_set_color_mode (GST_DEBUG_COLOR_MODE_OFF);
  gst_debug_set_threshold_from_string ("*:2,example:5,webcanvassrc:5", TRUE);

  return 0;
}
