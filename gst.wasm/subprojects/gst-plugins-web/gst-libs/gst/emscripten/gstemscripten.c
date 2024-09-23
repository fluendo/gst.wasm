/* GStreamer Emscripten helper
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include <emscripten.h>
#include <emscripten/threading.h>

GST_DEBUG_CATEGORY_STATIC (gstemscripten_dbg);
#define GST_CAT_DEFAULT gstemscripten_dbg

static struct 
{
  /* FIXME: fix and use GMainContext instead!!! */
  GSourceFunc just_one_func;
  gpointer just_one_data;
} gst_emscripten_ui_context;

void
gst_emscripten_ui_attach_callback (GSourceFunc cb, gpointer data, GDestroyNotify notify)
{
  if (gst_emscripten_ui_context.just_one_func) {
    g_error ("Only one handler is supported currently, fix GMainContext!!");
  }

  GST_DEBUG_CATEGORY_INIT (gstemscripten_dbg, "emscripten",
      0, "emscripten helper");

  GST_DEBUG ("Attach cb (%p), data (%p), notify (%p)", cb, data, notify);
  gst_emscripten_ui_context.just_one_func = cb;
  gst_emscripten_ui_context.just_one_data = data;
}

void
gst_emscripten_ui_remove_callback (GSourceFunc cb, gpointer data)
{
  g_assert (gst_emscripten_ui_context.just_one_func == cb);
  g_assert (gst_emscripten_ui_context.just_one_data == data);

  gst_emscripten_ui_context.just_one_func = NULL;
  gst_emscripten_ui_context.just_one_data = NULL;
}

static void
gst_emscripten_ui_main_loop_iteration ()
{
  if (gst_emscripten_ui_context.just_one_func == NULL) {
    return;
  }
  
  GST_LOG ("UI dispatch");
  
  /* FIXME: ugly hack to workaround broken GMainContext */
  gst_emscripten_ui_context.just_one_func
      (gst_emscripten_ui_context.just_one_data);

  GST_LOG ("UI dispatched");
}

void
gst_emscripten_init ()
{
  emscripten_set_main_loop (gst_emscripten_ui_main_loop_iteration, 0, FALSE);
}

void
gst_emscripten_deinit ()
{
  emscripten_set_main_loop (NULL, 0, FALSE);
}
