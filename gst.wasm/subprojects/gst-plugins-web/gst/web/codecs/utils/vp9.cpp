static const gchar *gst_codec_utils_vp9_profiles[] = { "1", "2", "3" };
static const guint gst_codec_utils_vp9_bit_depths[] = {8, 10, 12};
static const guint gst_codec_utils_vp9_luma_depths[] = {8, 10, 12};


static void
foreach_vp9_codec (GstPlugin *plugin, val vdecclass, gboolean is_hw)
{
  GstCaps *all_caps;
  guint p, b, l;
  gboolean supported = FALSE;
  const gchar *codec_name = is_hw ? "VP9HW" : "VP9SW";

  all_caps = gst_caps_new_empty ();
  for (p = 0; p < G_N_ELEMENTS (gst_codec_utils_vp9_profiles); p++) {
    for (b = 0; b < G_N_ELEMENTS (gst_codec_utils_vp9_bit_depths); b++) {
      for (l = 0; l < G_N_ELEMENTS (gst_codec_utils_vp9_luma_depths); l++) {
        GstCaps *caps;
        gchar *mime_codec;
        gboolean mime_codec_supported;

        caps = gst_caps_new_simple ("video/x-vp9",
          "profile", G_TYPE_STRING, gst_codec_utils_vp9_profiles[p],
          "bit-depth-chroma", G_TYPE_UINT, gst_codec_utils_vp9_bit_depths[b],
          "bit-depth-luma", G_TYPE_UINT, gst_codec_utils_vp9_luma_depths[l],
          nullptr);

        mime_codec = gst_codec_utils_caps_get_mime_codec (caps);
        // g_print ("caps: %s\n", gst_caps_to_string (caps));
        // g_print ("mime_codec: %s\n", mime_codec);
        mime_codec_supported = video_decoder_is_config_supported (vdecclass,
            mime_codec, is_hw);
        supported |= mime_codec_supported;

        if (mime_codec_supported)
          all_caps = gst_caps_merge (all_caps, caps);

        g_free (mime_codec);
      }
    }
  }

  if (supported)
    register_video_decoder (plugin, codec_name, NULL, gst_caps_ref (all_caps), is_hw);
}