/* Copyright (C) Fluendo S.A. <support@fluendo.com> */
#include <gst/gst.h>
#include <emscripten.h>

GST_PLUGIN_STATIC_DECLARE (coreelements);
GST_PLUGIN_STATIC_DECLARE (app);
GST_PLUGIN_STATIC_DECLARE (audiotestsrc);

static void
glib_print_handler (const gchar *string)
{
  gchar *em_script = g_strdup_printf ("console.log('%s');", string);
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

  char *argv[] = { "test01", "--gst-debug", "audioplayer:6", NULL };
  int argc = sizeof (argv) / sizeof (argv[0]) - 1;
  char **nargv = calloc (sizeof (char *), argc + 1);

  int i;
  for (i = 0; i < argc; i++)
    nargv[i] = strdup (argv[i]);
  nargv[i] = NULL;

  // g_set_print_handler (glib_print_handler);
  g_set_printerr_handler (glib_print_handler);
  g_log_set_default_handler (glib_log_handler, NULL);

  g_message ("Running gst_init_check...");
  initialized = gst_init_check (&argc, &nargv, NULL);
  g_message ("initialized = %d", initialized);
  if (!initialized)
    return FALSE;

  /* gst_debug_set_threshold_from_string ("4", TRUE); */
  GST_PLUGIN_STATIC_REGISTER (coreelements);
  GST_PLUGIN_STATIC_REGISTER (app);
  GST_PLUGIN_STATIC_REGISTER (audiotestsrc);

  g_message ("Init done");
  return TRUE;
}
