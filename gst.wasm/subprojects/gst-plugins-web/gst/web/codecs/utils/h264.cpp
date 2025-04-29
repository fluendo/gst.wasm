/*
 * GStreamer
 * Copyright (C) 2024 Jorge Zapata <jzapata@fluendo.com>
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

/* FIXME the following gst_codec_utils* functions should go to gstpbutils */
/* In GStreamer the H264 flags (or constrained flags) are embedded into the
 * profile field of the GstCaps' GstStructure
 */
static const gchar *gst_codec_utils_h264_profiles[] = { "constrained-baseline",
  "baseline", "main", "extended", "constrained-high", "progressive-high",
  "high", "high-10-intra", "progressive-high-10", "high-10",
  "high-4:2:2-intra", "high-4:2:2", "high-4:4:4-intra", "high-4:4:4",
  "cavlc-4:4:4-intra", "multiview-high", "stereo-high",
  "scalable-constrained-baseline", "scalable-baseline", "scalable-high-intra",
  "scalable-constrained-high", "scalable-high", NULL };

typedef enum _GstCodecUtilsH264Profile
{
  GST_CODEC_UTILS_H264_PROFILE_CONSTRAINED_BASELINE,
  GST_CODEC_UTILS_H264_PROFILE_BASELINE,
  GST_CODEC_UTILS_H264_PROFILE_MAIN,
  GST_CODEC_UTILS_H264_PROFILE_EXTENDED,
  GST_CODEC_UTILS_H264_PROFILE_CONSTRAINED_HIGH,
  GST_CODEC_UTILS_H264_PROFILE_PROGRESSIVE_HIGH,
  GST_CODEC_UTILS_H264_PROFILE_HIGH,
  GST_CODEC_UTILS_H264_PROFILE_HIGH_10_INTRA,
  GST_CODEC_UTILS_H264_PROFILE_PROGRESSIVE_HIGH_10,
  GST_CODEC_UTILS_H264_PROFILE_HIGH_10,
  GST_CODEC_UTILS_H264_PROFILE_HIGH_422_INTRA,
  GST_CODEC_UTILS_H264_PROFILE_HIGH_422,
  GST_CODEC_UTILS_H264_PROFILE_HIGH_444_INTRA,
  GST_CODEC_UTILS_H264_PROFILE_HIGH_444,
  GST_CODEC_UTILS_H264_PROFILE_CAVLC_444_INTRA,
  GST_CODEC_UTILS_H264_PROFILE_MULTIVIEW_HIGH,
  GST_CODEC_UTILS_H264_PROFILE_STEREO_HIGH,
  GST_CODEC_UTILS_H264_PROFILE_SCALABLE_CONSTRAINED_BASELINE,
  GST_CODEC_UTILS_H264_PROFILE_SCALABLE_BASELINE,
  GST_CODEC_UTILS_H264_PROFILE_SCALABLE_HIGH_INTRA,
  GST_CODEC_UTILS_H264_PROFILE_SCALABLE_CONSTRAINED_HIGH,
  GST_CODEC_UTILS_H264_PROFILE_SCALABLE_HIGH
} GstCodecUtilsH264Profile;

#define GST_CODEC_UTILS_H264_PROFILES                                         \
  GST_CODEC_UTILS_H264_PROFILE_SCALABLE_HIGH

static const gchar *gst_codec_utils_h264_levels[] = { "1", "1b", "1.1", "1.2",
  "1.3", "2", "2.1", "2.2", "3", "3.1", "3.2", "4", "4.1", "4.2", "5", "5.1",
  "5.2", "6", "6.1", "6.2", NULL };

typedef enum _GstCodecUtilsH264Level
{
  GST_CODEC_UTILS_H264_LEVEL_1,
  GST_CODEC_UTILS_H264_LEVEL_1_b,
  GST_CODEC_UTILS_H264_LEVEL_1_1,
  GST_CODEC_UTILS_H264_LEVEL_1_2,
  GST_CODEC_UTILS_H264_LEVEL_1_3,
  GST_CODEC_UTILS_H264_LEVEL_2,
  GST_CODEC_UTILS_H264_LEVEL_2_1,
  GST_CODEC_UTILS_H264_LEVEL_2_2,
  GST_CODEC_UTILS_H264_LEVEL_3,
  GST_CODEC_UTILS_H264_LEVEL_3_1,
  GST_CODEC_UTILS_H264_LEVEL_3_2,
  GST_CODEC_UTILS_H264_LEVEL_4,
  GST_CODEC_UTILS_H264_LEVEL_4_1,
  GST_CODEC_UTILS_H264_LEVEL_4_2,
  GST_CODEC_UTILS_H264_LEVEL_5,
  GST_CODEC_UTILS_H264_LEVEL_5_1,
  GST_CODEC_UTILS_H264_LEVEL_5_2,
  GST_CODEC_UTILS_H264_LEVEL_6,
  GST_CODEC_UTILS_H264_LEVEL_6_1,
  GST_CODEC_UTILS_H264_LEVEL_6_2
} GstCodecUtilsH264Level;

#define GST_CODEC_UTILS_H264_LEVELS GST_CODEC_UTILS_H264_LEVEL_6_2

void
gst_codec_utils_h264_set_level_and_profile (
    guint8 *sps, guint len, const gchar *level, const gchar *profile)
{
  if (len < 3) {
    GST_ERROR ("Wrong length (%d) for a SPS", len);
    return;
  }

  if (!g_strcmp0 (profile, "constrained-baseline")) {
    sps[0] = 66;
    sps[1] = 0x40;
  } else if (!g_strcmp0 (profile, "baseline")) {
    sps[0] = 66;
  } else if (!g_strcmp0 (profile, "main")) {
    sps[0] = 77;
  } else if (!g_strcmp0 (profile, "extended")) {
    sps[0] = 88;
  }

#if 0
  csf1 = (sps[1] & 0x40) >> 6;
  csf3 = (sps[1] & 0x10) >> 4;
  csf4 = (sps[1] & 0x08) >> 3;
  csf5 = (sps[1] & 0x04) >> 2;

  switch (sps[0]) {
    case 100:
      if (csf4) {
        if (csf5)
          profile = "constrained-high";
        else
          profile = "progressive-high";
      } else
        profile = "high";
      break;
    case 110:
      if (csf3)
        profile = "high-10-intra";
      else if (csf4)
        profile = "progressive-high-10";
      else
        profile = "high-10";
      break;
    case 122:
      if (csf3)
        profile = "high-4:2:2-intra";
      else
        profile = "high-4:2:2";
      break;
    case 244:
      if (csf3)
        profile = "high-4:4:4-intra";
      else
        profile = "high-4:4:4";
      break;
    case 44:
      profile = "cavlc-4:4:4-intra";
      break;
    case 118:
      profile = "multiview-high";
      break;
    case 128:
      profile = "stereo-high";
      break;
    case 83:
      if (csf5)
        profile = "scalable-constrained-baseline";
      else
        profile = "scalable-baseline";
      break;
    case 86:
      if (csf3)
        profile = "scalable-high-intra";
      else if (csf5)
        profile = "scalable-constrained-high";
      else
        profile = "scalable-high";
      break;
    default:
      return NULL;
  }

  return profile;
#endif

  /* FIXME probably, there are faster (and smarter) ways to handle this */
  if (!g_strcmp0 (level, "1")) {
    sps[2] = 10;
  } else if (!g_strcmp0 (level, "1.b")) {
    if (sps[1] & 0x10)
      sps[2] = 11;
    else
      sps[2] = 9;
  } else if (!g_strcmp0 (level, "1.1")) {
    sps[2] = 11;
  } else if (!g_strcmp0 (level, "1.2")) {
    sps[2] = 12;
  } else if (!g_strcmp0 (level, "1.3")) {
    sps[2] = 13;
  } else if (!g_strcmp0 (level, "2")) {
    sps[2] = 20;
  } else if (!g_strcmp0 (level, "2.1")) {
    sps[2] = 21;
  } else if (!g_strcmp0 (level, "2.2")) {
    sps[2] = 22;
  } else if (!g_strcmp0 (level, "3")) {
    sps[2] = 30;
  } else if (!g_strcmp0 (level, "3.1")) {
    sps[2] = 31;
  } else if (!g_strcmp0 (level, "3.2")) {
    sps[2] = 32;
  } else if (!g_strcmp0 (level, "4")) {
    sps[2] = 40;
  } else if (!g_strcmp0 (level, "4.1")) {
    sps[2] = 41;
  } else if (!g_strcmp0 (level, "4.2")) {
    sps[2] = 42;
  } else if (!g_strcmp0 (level, "5")) {
    sps[2] = 50;
  } else if (!g_strcmp0 (level, "5.1")) {
    sps[2] = 51;
  } else if (!g_strcmp0 (level, "5.2")) {
    sps[2] = 51;
  } else if (!g_strcmp0 (level, "6")) {
    sps[2] = 60;
  } else if (!g_strcmp0 (level, "6.1")) {
    sps[2] = 61;
  } else if (!g_strcmp0 (level, "6.2")) {
    sps[2] = 62;
  }
}

const gchar *
gst_codec_utils_h264_get_nth_profile (guint n)
{
  g_return_val_if_fail (n < GST_CODEC_UTILS_H264_PROFILES, NULL);
  return gst_codec_utils_h264_profiles[n];
}

const gchar *
gst_codec_utils_h264_get_nth_level (guint n)
{
  g_return_val_if_fail (n < GST_CODEC_UTILS_H264_LEVELS, NULL);
  return gst_codec_utils_h264_levels[n];
}

static void
gst_web_codecs_utils_scan_video_h264_decoder (GstPlugin *plugin, val vdecclass)
{
  const gchar *codec_names[] = { "H264SW", "H264HW" };
  gint i;

  /* Check hw or not hw */
  for (i = 0; i < 2; i++) {
    GstCaps *caps;
    gboolean supported = FALSE;
    gint profile;

    caps = gst_caps_new_empty_simple ("video/x-h264");
    /* Get all profiles */
    for (profile = 0; profile < GST_CODEC_UTILS_H264_PROFILES; profile++) {
      const gchar *profile_str;
      gint level;

      profile_str = gst_codec_utils_h264_get_nth_profile (profile);
      /* Get all levels */
      for (level = 0; level < GST_CODEC_UTILS_H264_LEVELS; level++) {
        guint8 sps[3] = {
          0,
        };
        gchar *mime_codec;
        const gchar *level_str;
        gboolean conf_supported;

        level_str = gst_codec_utils_h264_get_nth_level (level);
        /* TODO avoid asking for the same level */

        /* Transform to a codec string */
        GST_LOG ("Checking for H.264 with profile: %s and level: %s",
            profile_str, level_str);
        gst_codec_utils_h264_set_level_and_profile (
            sps, 3, level_str, profile_str);
        mime_codec =
            g_strdup_printf ("avc1.%02X%02X%02X", sps[0], sps[1], sps[2]);
        /* Validate */
        conf_supported =
            is_config_supported (vdecclass, val::null (), mime_codec, i);
        supported |= conf_supported;
        /* Merge profile and level into the caps */
        g_free (mime_codec);
      }
    }

    if (!supported) {
      /* If no h264 is valid, don't do anything */
      GST_WARNING ("No H.264 decoder found for %s", codec_names[i]);
    } else {
      GST_INFO ("H.264 decoder found for %s", codec_names[i]);
      /* TODO If valid append to the caps alternatives */
      register_video_decoder (plugin, codec_names[i], gst_caps_ref (caps), i);
    }

    gst_caps_unref (caps);
  }
}
