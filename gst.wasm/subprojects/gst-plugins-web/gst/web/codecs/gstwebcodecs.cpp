/*
 * GStreamer - gst.wasm WebCodecs source
 *
 * Copyright 2024 Fluendo S.A.
 * @author: Jorge Zapata <jzapata@fluendo.com>
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

/*
 * This plugin should be something similar to what gstamc is, a wrapper on top
 * of JS WebCodecs API to register video decoders and encoders
 *
 * Some useful links
 * https://github.com/w3c/webcodecs/ the main repository for WebCodecs API
 * https://www.rfc-editor.org/rfc/rfc6381 for understanding the string
 * definition of codecs and profiles https://github.com/w3c/webcodecs/issues/88
 * Useful discussion for Decoder -> Canvas
 * https://developer.mozilla.org/en-US/docs/Web/API/VideoDecoder/isConfigSupported_static#examples
 * https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html#using-val-to-transliterate-javascript-to-c
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <emscripten/bind.h>

#include "gstwebcodecs.h"
#include "gstwebcodecsvideodecoder.h"

using namespace emscripten;

GQuark gst_web_codecs_data_quark = 0;

GST_DEBUG_CATEGORY_STATIC (web_codecs_debug);
#define GST_CAT_DEFAULT web_codecs_debug

static const gchar *accelerations[] = { "prefer-software", "prefer-hardware",
  NULL };

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

static gchar *
create_type_name (const gchar *parent_name, const gchar *codec_name)
{
  gchar *type_name;

  type_name = g_strdup_printf ("%s%s", parent_name, codec_name);
  GST_DEBUG ("Using type name '%s'", type_name);
  return type_name;
}

static gchar *
create_element_name (GType parent_type, const gchar *codec_name)
{
  const gchar *prefix = NULL;
  gchar *element_name;
  gint i, k;
  gint codec_name_len = strlen (codec_name);
  gint prefix_len;

  if (parent_type == gst_web_codecs_video_decoder_get_type ())
    prefix = "webcodecsviddec";

  if (!prefix) {
    GST_ERROR ("No prefix, therefore wrong type");
    return NULL;
  }

  prefix_len = strlen (prefix);
  element_name = g_new0 (gchar, prefix_len + strlen (codec_name) + 1);
  memcpy (element_name, prefix, prefix_len);

  for (i = 0, k = 0; i < codec_name_len; i++) {
    if (g_ascii_isalnum (codec_name[i])) {
      element_name[prefix_len + k++] = g_ascii_tolower (codec_name[i]);
    }
    /* Skip all non-alnum chars */
  }

  GST_DEBUG ("Using element name '%s'", element_name);
  return element_name;
}

static gboolean
register_video_decoder (
    GstPlugin *plugin, const gchar *codec_name, GstCaps *caps, gboolean is_hw)
{
  GType type, subtype;
  GTypeQuery type_query;
  GTypeInfo type_info = {
    0,
  };
  GTypeFlags type_flags = G_TYPE_FLAG_FINAL;
  gchar *type_name, *element_name;
  guint rank;
  gboolean ret = TRUE;

  GST_DEBUG ("Registering codec %" GST_PTR_FORMAT, caps);

  type = gst_web_codecs_video_decoder_get_type ();
  g_type_query (type, &type_query);
  memset (&type_info, 0, sizeof (type_info));
  type_info.class_size = type_query.class_size;
  type_info.instance_size = type_query.instance_size;
  type_name = create_type_name (type_query.type_name, codec_name);

  if (g_type_from_name (type_name) != G_TYPE_INVALID) {
    GST_ERROR (
        "Type '%s' already exists for codec '%s'", type_name, codec_name);
    ret = FALSE;
    goto existing_type;
  }

  subtype = g_type_register_static (type, type_name, &type_info, type_flags);
  g_type_set_qdata (subtype, gst_web_codecs_data_quark, gst_caps_ref (caps));

  element_name = create_element_name (type, codec_name);
  rank = is_hw ? GST_RANK_PRIMARY : GST_RANK_SECONDARY;
  if (!gst_element_register (plugin, element_name, rank, subtype)) {
    GST_ERROR ("Cannot register element '%s'", element_name);
    ret = FALSE;
    goto failed_registration;
  }

  GST_INFO ("Element '%s' registered successfully", element_name);

failed_registration:
  g_free (element_name);

existing_type:
  g_free (type_name);

  gst_caps_unref (caps);
  return ret;
}

static gboolean
video_decoder_is_config_supported (
    val vdecclass, gchar *codec, gboolean hwaccel)
{
  const gchar *acceleration = accelerations[hwaccel ? 1 : 0];
  gboolean is_supported;
  val config = val::object ();

  GST_LOG ("Checking for support of codec %s, hardwareAcceleration: %s", codec,
      acceleration);
  config.set ("codec", std::string (codec));
  config.set ("hardwareAcceleration", std::string (acceleration));
  val result = vdecclass.call<val> ("isConfigSupported", config).await ();
  is_supported = result["supported"].as<bool> ();
  GST_LOG ("Decoder for codec '%s' with hw-acceleration '%s' is %ssupported",
      codec, acceleration, is_supported ? "" : "not ");

  return is_supported;
}

static void
scan_video_h264_decoder (GstPlugin *plugin, val vdecclass)
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
            video_decoder_is_config_supported (vdecclass, mime_codec, i);
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

static void
scan_video_decoders (GstPlugin *plugin)
{
  val vdecclass = val::global ("VideoDecoder");
  if (!vdecclass.as<bool> ()) {
    GST_ERROR ("No global VideoDecoder");
    return;
  }

  scan_video_h264_decoder (plugin, vdecclass);
}

gboolean
gst_web_codecs_init (GstPlugin *plugin)
{
  GST_DEBUG_CATEGORY_INIT (web_codecs_debug, "webcodecs", 0, "WebCodecs");

  gst_web_codecs_data_quark =
      g_quark_from_static_string ("gst-web-codecs-data");
  scan_video_decoders (plugin);

  return TRUE;
}
