/*
 * GStreamer - gst.wasm audiotestsrc example
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

GST_PLUGIN_STATIC_DECLARE (app);
GST_PLUGIN_STATIC_DECLARE (audiotestsrc);
GST_PLUGIN_STATIC_DECLARE (coreelements);

static void
glib_print_handler (const gchar *string)
{
  gchar *em_script = g_strdup_printf ("console.log('%s')", string);
  emscripten_run_script (em_script);
  g_free (em_script);
}

static void
glib_log_handler (const gchar *log_domain, GLogLevelFlags log_level,
    const gchar *message, gpointer user_data)
{
  gchar *em_script =
      g_strdup_printf ("console.log('glib(%d): %s')", log_level, message);
  emscripten_run_script (em_script);
  g_free (em_script);
}

int
init (void)
{
  static gboolean initialized = FALSE;

  if (initialized)
    return TRUE;

  char *argv[] = { "test01", "--gst-debug", "audiotestsrc:6", NULL };
  int argc = sizeof (argv) / sizeof (argv[0]) - 1;
  char **nargv = calloc (sizeof (char *), argc + 1);

  int i;
  for (i = 0; i < argc; i++)
    nargv[i] = strdup (argv[i]);
  nargv[i] = NULL;

  g_set_print_handler (glib_print_handler);
  g_set_printerr_handler (glib_print_handler);
  g_log_set_default_handler (glib_log_handler, NULL);

  initialized = gst_init_check (&argc, &nargv, NULL);
  if (!initialized)
    return FALSE;

  /* gst_debug_set_threshold_from_string ("4", TRUE); */
  GST_PLUGIN_STATIC_REGISTER (coreelements);
  GST_PLUGIN_STATIC_REGISTER (app);
  GST_PLUGIN_STATIC_REGISTER (audiotestsrc);

  return TRUE;
}
