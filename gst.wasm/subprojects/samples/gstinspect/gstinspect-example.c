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


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/emscripten/gstemscripten.h>
#include "gstinspect.h"


static void
register_elements () // FIXME: only inspects last declared plugin
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
  //gst_debug_set_threshold_from_string ("example:5", FALSE);
  register_elements ();
  char *my_argv[] = { "-a", NULL };

  int ret;
  
  /* TAKEN AND MODIFIED FROM UPSTREAM GSTREAMER - gstinspect.c*/
  /* gstinspect.c calls this function */
#if defined(G_OS_WIN32) && !defined(GST_CHECK_MAIN)
  argv = g_win32_get_command_line ();
#endif

#if defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE
  ret = gst_macos_main ((GstMainFunc) real_main, argc, argv, NULL);
#else
  ret = real_main (2, my_argv);
#endif

#if defined(G_OS_WIN32) && !defined(GST_CHECK_MAIN)
  g_strfreev (argv);
#endif

  return ret;
}
