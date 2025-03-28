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

#include "utils/h264.cpp"

static void
scan_video_decoders (GstPlugin *plugin)
{
  val vdecclass = val::global ("VideoDecoder");
  if (!vdecclass.as<bool> ()) {
    GST_ERROR ("No global VideoDecoder");
    return;
  }

  gst_web_codecs_utils_scan_video_h264_decoder (plugin, vdecclass);
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
