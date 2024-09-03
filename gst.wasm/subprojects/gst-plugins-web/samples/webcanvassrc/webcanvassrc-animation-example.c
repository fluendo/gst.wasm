/*
 * GStreamer - GStreamer ImageBitmap Animation example.
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

static void
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
  const canvas2 = document.createElement ("canvas");

  const ctx2 = canvas2.getContext ("2d", { willReadFrequently : true });
  canvas2.id = "canvas2";
  canvas2.width = canvas1.width;
  canvas2.height = canvas1.height;
  canvas2.className = "emscripten";
  canvas2.style.backgroundColor = 'lightgray';

  // Add canvas to page.
  const span1 = document.createElement ('span');
  const span2 = document.createElement ('span');
  span1.textContent = 'GStreamer';
  span1.style.display = 'block';
  span1.style.textAlign = 'center';
  span2.textContent = 'Javascript';
  span2.style.display = 'block';
  span2.style.textAlign = 'center';

  canvas1.parentElement.prepend (span1);
  canvas1.parentElement.append (span2);
  canvas1.parentElement.append (canvas2);

  // Dumb animation.
  let x = 0;
  function animate (canvas, ctx, radius, speed, fillStyle)
  {
    ctx.clearRect (0, 0, canvas.width, canvas.height);
    ctx.beginPath ();
    ctx.arc (x, canvas.height / 2, radius, 0, Math.PI * 2);
    ctx.fill ();
    ctx.fillStyle = 'red';
    x = x > canvas.width ? 0 : x + speed;
    // clang-format off
    requestAnimationFrame (
        () => { animate (canvas, ctx, radius, speed, fillStyle) });
    // clang-format on
  };
  animate (canvas2, ctx2, 20, 2, 'red');
});

int
main (int argc, char **argv)
{
  add_new_canvas ();
  gst_init (NULL, NULL);
  gst_emscripten_init ();
  register_elements ();

  GST_DEBUG_CATEGORY_INIT (example_dbg, "example", 0, "ImageBitmap example");

  gst_debug_set_color_mode (GST_DEBUG_COLOR_MODE_OFF);
  gst_debug_set_threshold_from_string ("*:2,example:5,webcanvassrc:5", TRUE);

  init_pipeline ();

  return 0;
}
