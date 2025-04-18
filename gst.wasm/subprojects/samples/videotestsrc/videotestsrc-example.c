 /*
 * GStreamer - gst.wasm videotestsrc example
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

#include <emscripten.h>
#include <gst/emscripten/gstemscripten.h>

#define GST_CAT_DEFAULT example_dbg
GST_DEBUG_CATEGORY_STATIC (example_dbg);

static GstElement *pipeline;

EM_JS(void, debuggg, (const char* str), {
  const msg = UTF8ToString(str);

  try {
    const outputBox = document.getElementById("debuggg");
    if (outputBox)
      outputBox.textContent += msg;
  } catch (e) {
  }
  
  console.log (msg);
});

static void
register_elements ()
{
  GST_PLUGIN_STATIC_DECLARE (videotestsrc);
  GST_PLUGIN_STATIC_DECLARE (sdl2);

  GST_PLUGIN_STATIC_REGISTER (videotestsrc);
  GST_PLUGIN_STATIC_REGISTER (sdl2);
}

#if DEBUG_GST_MSGS
GstBusSyncReply
sync_handler (GstBus * bus,
    GstMessage * message,
    gpointer user_data)
{
  if (thread_id != g_thread_self ())
    return GST_BUS_DROP;

  switch (GST_MESSAGE_TYPE (message))
  {
    case GST_MESSAGE_ERROR: {
      GError *err = NULL;
      gchar *dbg_info = NULL;

      gst_message_parse_error (message, &err, &dbg_info);
      gchar *msg = g_strdup_printf ("%s: ERROR: %s (%s)\n",
          GST_OBJECT_NAME (message->src), err->message,
          (dbg_info) ? dbg_info : "no debug info");

      debuggg (msg);
      
      g_error_free (err);
      g_free (dbg_info);
      g_free (msg);
      break;
    }

    case GST_MESSAGE_STATE_CHANGED: {
      GstState old_state, new_state;
      
      if (message->src != GST_OBJECT_CAST (pipeline))
        break;
      
      gst_message_parse_state_changed (message, &old_state, &new_state, NULL);
      gchar *msg = g_strdup_printf ("Pipeline state [%s]->[%s]\n",
          gst_element_state_get_name (old_state),
          gst_element_state_get_name (new_state));
      debuggg (msg);
      free (msg);
      break;
    }
    default:
      break;
  }
  
  return GST_BUS_DROP;
}
#endif

static void
init_pipeline ()
{
  pipeline = gst_parse_launch ("videotestsrc pattern=ball ! sdl2sink", NULL);
  debuggg ("create pipeline ok\n");

#if DEBUG_GST_MSGS
  {
    GstBus *bus;
    bus = gst_element_get_bus (pipeline);
    gst_bus_set_sync_handler (bus,
        sync_handler,
        NULL,
        NULL);
    gst_object_unref (bus);
  }
#endif
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char **argv)
{
  debuggg ("entered main ok\n");
  gst_init (NULL, NULL);
  debuggg ("gst init ok\n");
  gst_emscripten_init ();
  register_elements ();

  GST_DEBUG_CATEGORY_INIT (
      example_dbg, "example", 0, "videotestsrc wasm example");
  gst_debug_set_threshold_from_string ("1", FALSE);
  gst_debug_set_color_mode(GST_DEBUG_COLOR_MODE_OFF);

  init_pipeline ();

  return 0;
}
