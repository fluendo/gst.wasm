/*
 * GStreamer - gst.wasm gst-inspect example
 *
 * Copyright 2025 Fluendo S.A.
 * @author: Pablo García Sancho <pgarcia@fluendo.com>
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

#define GST_API_VERSION "1.0"
#define GST_PACKAGE_ORIGIN "gst.wasm"
#define GETTEXT_PACKAGE "gst.wasm"

#include <gst/emscripten/gstemscripten.h>

#define main gst_inspect_main
#include "gst-inspect.c"
#undef main

static void
register_elements ()
{
 GST_PLUGIN_STATIC_DECLARE (web);
 GST_PLUGIN_STATIC_REGISTER (web);

 GST_PLUGIN_STATIC_DECLARE (sdl2);
 GST_PLUGIN_STATIC_REGISTER (sdl2);
}


int
main (int argc, char *argv[])
{
  gst_init (NULL, NULL);
  GST_DEBUG_CATEGORY_STATIC (example_dbg);
  GST_DEBUG_CATEGORY_INIT (
    example_dbg, "example", 0, "gstinspect wasm example");
  register_elements ();
  
  //Sorting breaks silently and colors ANSI codes are not supported in wasm
  char *my_argv[] = { NULL, "-a", "--sort=none", "--no-colors"};
  int ret = gst_inspect_main (4, my_argv);
  
  return ret;
}
