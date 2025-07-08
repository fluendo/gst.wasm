/*
 * GStreamer - gst.wasm WebCodecsAudioDecoder source
 *
 * Copyright 2024 - 2025 Fluendo S.A.
 *  @author: Jorge Zapata <jzapata@fluendo.com>
 *  @author: César Fabián Orccón Chipana <jzapata@fluendo.com>
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
 * Some ideas:
 * Implement the GST_QUERY_ALLOCATOR (propose_allocation) to
 * use buffers that can be sent directly to the decoder (JS)
 * without having to copy the data
 *
 * Implement the negotiate_pool to ask for specific downstream
 * buffers, i.e GL
 *
 * Some interesting links:
 * https://github.com/emscripten-core/emscripten/issues/3515
 * https://emscripten.org/docs/api_reference/val.h.html
 * https://github.com/emscripten-core/emscripten/issues/4927
 * https://github.com/emscripten-core/emscripten/issues/7025
 * https://github.com/w3c/webcodecs/issues/88
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/gl/gl.h>
#include <gst/pbutils/pbutils.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <gst/web/gstwebutils.h>
#include <gst/web/gstwebvideoframe.h>

#include "gstwebcodecs.h"
#include "gstwebcodecsaudiodecoder.h"

using namespace emscripten;

#define GST_WEB_CODECS_AUDIO_DECODER_MAX_DEQUEUE 32

#define GST_WEB_CODECS_AUDIO_DECODER_FROM_JS                                  \
  reinterpret_cast<GstWebCodecsAudioDecoder *> (                              \
      val::module_property ("self").as<int> ())

#define GST_CAT_DEFAULT gst_web_codecs_audio_decoder_debug_category
GST_DEBUG_CATEGORY_STATIC (gst_web_codecs_audio_decoder_debug_category);

static gpointer parent_class = NULL;

typedef struct _GstWebCodecsAudioDecoderConfigureData
{
  GstWebCodecsAudioDecoder *self;
  GstCaps *caps;
  gboolean ret;
} GstWebCodecsAudioDecoderConfigureData;

typedef struct _GstWebCodecsAudioDecoderDecodeData
{
  GstWebCodecsAudioDecoder *self;
  GstBuffer *buffer;
} GstWebCodecsAudioDecoderDecodeData;

static bool
is_audio_format_planar (const std::string &format)
{
  return format.find ("-planar") != std::string::npos;
}

static inline guint32
get_total_allocation_size (val audio_data, int channels, bool planar)
{
  guint32 total_size = 0;

  if (planar) {
    for (int i = 0; i < channels; ++i) {
      val opts = val::object ();
      opts.set ("planeIndex", i);
      total_size +=
          audio_data.call<val> ("allocationSize", opts).as<guint32> ();
    }
  } else {
    val opts = val::object ();
    opts.set ("planeIndex", 0);
    total_size = audio_data.call<val> ("allocationSize", opts).as<guint32> ();
  }

  return total_size;
}

static GstAudioChannelPosition *
gst_web_codecs_audio_guess_channel_positions (gint channels)
{
  GstAudioChannelPosition *positions =
      g_new0 (GstAudioChannelPosition, channels);

  switch (channels) {
    case 1:
      positions[0] = GST_AUDIO_CHANNEL_POSITION_MONO;
      break;
    case 2:
      positions[0] = GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
      positions[1] = GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
      break;
    case 6:
      positions[0] = GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
      positions[1] = GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
      positions[2] = GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER;
      positions[3] = GST_AUDIO_CHANNEL_POSITION_LFE1;
      positions[4] = GST_AUDIO_CHANNEL_POSITION_REAR_LEFT;
      positions[5] = GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT;
      break;
    default:
      for (int i = 0; i < channels; i++)
        positions[i] = GST_AUDIO_CHANNEL_POSITION_NONE;
      break;
  }

  return positions;
}

static gboolean
gst_web_codecs_audio_decoder_get_format (
    GstWebCodecsAudioDecoder *self, val audio_data)
{
  const gchar *format_str;
  gint number_of_channels;
  gint sample_rate;
  gboolean planar;
  GstAudioFormat gst_format;
  GstAudioChannelPosition *positions;
  std::string format;

  format = audio_data["format"].as<std::string> ();
  format_str = format.c_str ();
  number_of_channels = audio_data["numberOfChannels"].as<int> ();
  sample_rate = audio_data["sampleRate"].as<int> ();

  if (number_of_channels <= 0 || sample_rate <= 0) {
    GST_ERROR_OBJECT (self, "Invalid audio format: channels=%d, rate=%d",
        number_of_channels, sample_rate);
    return FALSE;
  }

  planar = is_audio_format_planar (format.c_str ());
  if (!g_strcmp0 (format_str, "f32-planar") ||
      !g_strcmp0 (format_str, "f32")) {
    gst_format = GST_AUDIO_FORMAT_F32LE;
  } else if (!g_strcmp0 (format_str, "s16-planar") ||
             !g_strcmp0 (format_str, "s16")) {
    gst_format = GST_AUDIO_FORMAT_S16LE;
  } else if (!g_strcmp0 (format_str, "u8-planar") ||
             !g_strcmp0 (format_str, "u8")) {
    gst_format = GST_AUDIO_FORMAT_U8;
  } else {
    GST_ERROR_OBJECT (self, "Unsupported audio format: %s", format_str);
    return FALSE;
  }

  GST_DEBUG_OBJECT (self,
      "Negotiating audio: format=%s, planar=%d, channels=%d, rate=%d",
      format_str, planar, number_of_channels, sample_rate);

  positions =
      gst_web_codecs_audio_guess_channel_positions (number_of_channels);

  gst_audio_info_init (&self->output_info);
  gst_audio_info_set_format (&self->output_info, gst_format, sample_rate,
      number_of_channels, positions);
  g_free (positions);

  self->output_info.layout =
      planar ? GST_AUDIO_LAYOUT_NON_INTERLEAVED : GST_AUDIO_LAYOUT_INTERLEAVED;

  // Log the audio info values
  GST_DEBUG_OBJECT (self,
      "Audio info: format=%d, rate=%d, channels=%d, bpf=%d",
      self->output_info.finfo->format, self->output_info.rate,
      self->output_info.channels, self->output_info.bpf);
  for (gint i = 0; i < self->output_info.channels; i++) {
    GST_DEBUG_OBJECT (self, "    channel %d: position enum value = %d", i,
        self->output_info.position[i]);
  }

  if (self->output_caps)
    gst_caps_unref (self->output_caps);

  self->output_caps = gst_audio_info_to_caps (&self->output_info);
  if (!self->output_caps) {
    GST_ERROR_OBJECT (self, "Failed to create output caps");
    return FALSE;
  }

  GST_DEBUG_OBJECT (self, "Output caps: %" GST_PTR_FORMAT, self->output_caps);

  return gst_audio_decoder_negotiate (GST_AUDIO_DECODER (self));
}

static void
gst_web_codecs_audio_decoder_audio_data_to_buffer (
    GstWebCodecsAudioDecoder *self, val audio_data, GstBuffer *buffer,
    guint32 total_size)
{
  GstMapInfo map;
  int channels = audio_data["numberOfChannels"].as<int> ();
  std::string format = audio_data["format"].as<std::string> ();
  bool planar = is_audio_format_planar (format);

  if (!gst_buffer_map (buffer, &map, GST_MAP_WRITE)) {
    GST_ERROR_OBJECT (self, "Failed to map buffer");
    return;
  }

  g_assert (map.size == total_size);

  GST_DEBUG_OBJECT (self, "Copy AudioData to GstBuffer");
  if (planar) {
    guint8 *dst_ptr = map.data;
    for (int i = 0; i < channels; i++) {
      val opts = val::object ();
      opts.set ("planeIndex", i);
      guint32 plane_size =
          audio_data.call<val> ("allocationSize", opts).as<guint32> ();

      val dst_view = val (typed_memory_view (plane_size, dst_ptr));
      audio_data.call<val> ("copyTo", dst_view, opts);
      dst_ptr += plane_size;
    }
  } else {
    GST_DEBUG_OBJECT (self, "Copy fisrt plane");

    val opts = val::object ();
    opts.set ("planeIndex", 0);
    guint32 plane_size =
        audio_data.call<val> ("allocationSize", opts).as<guint32> ();
    g_assert (total_size == plane_size);
    val dst_view = val (typed_memory_view (total_size, map.data));
    audio_data.call<val> ("copyTo", dst_view, opts);
  }

  gst_buffer_unmap (buffer, &map);
  GST_DEBUG_OBJECT (self, "Copied AudioData to GstBuffer");
}

static void
gst_web_codecs_audio_decoder_on_output (val audio_data)
{
  GstWebCodecsAudioDecoder *self = GST_WEB_CODECS_AUDIO_DECODER_FROM_JS;
  GstAudioDecoder *dec = GST_AUDIO_DECODER (self);
  GstBuffer *buffer = nullptr;
  GstFlowReturn flow;

  GST_INFO_OBJECT (self, "AudioFrame received");

  // Estimate buffer size
  guint total_size = 0;
  int channels = audio_data["numberOfChannels"].as<int> ();
  int frames = audio_data["numberOfFrames"].as<int> ();
  std::string format = audio_data["format"].as<std::string> ();
  bool planar = is_audio_format_planar (format);

  GST_DEBUG_OBJECT (
      self, "AudioData shape: channels=%d, frames=%d", channels, frames);
  if (channels <= 0 || frames <= 0) {
    GST_ERROR_OBJECT (self,
        "AudioData has invalid shape: channels=%d, frames=%d", channels,
        frames);
    return;
  }

  // Lock the audio stream
  GST_AUDIO_DECODER_STREAM_LOCK (self);

  /* Configure the output */
  if (!self->output_caps) {
    self->need_negotiation = TRUE;
    if (!gst_web_codecs_audio_decoder_get_format (self, audio_data)) {
      GST_ERROR_OBJECT (self, "Failed to negotiate format");
      goto done;
    }
  }

  total_size = get_total_allocation_size (audio_data, channels, planar);
  GST_DEBUG_OBJECT (self, "Total size is %d", total_size);
  if (total_size == 0) {
    GST_ERROR_OBJECT (self, "Total size is 0");
    goto done;
  }

  buffer = gst_audio_decoder_allocate_output_buffer (dec, total_size);
  gst_buffer_add_audio_meta (
      buffer, &self->output_info, (total_size / self->output_info.bpf), NULL);
  if (!buffer) {
    GST_ERROR_OBJECT (self, "Failed to allocate output buffer");
    goto done;
  }

  gst_web_codecs_audio_decoder_audio_data_to_buffer (
      self, audio_data, buffer, total_size);

  flow = gst_audio_decoder_finish_frame (dec, buffer, 1);
  if (flow != GST_FLOW_OK) {
    GST_WARNING_OBJECT (
        self, "Failed to finish frame: %s", gst_flow_get_name (flow));
    gst_buffer_unref (buffer);
  }

done:
  GST_AUDIO_DECODER_STREAM_UNLOCK (self);
}

static void
gst_web_codecs_audio_decoder_on_error (val error)
{
  GST_ERROR ("Error: %s", error.call<std::string> ("toString").c_str ());
}

static void
gst_web_codecs_audio_decoder_on_dequeue (val event)
{
  GstWebCodecsAudioDecoder *self = GST_WEB_CODECS_AUDIO_DECODER_FROM_JS;
  gint dequeue_size;

  dequeue_size = self->decoder["decodeQueueSize"].as<int> ();

  GST_INFO_OBJECT (
      self, "Dequeue received with current size %d", dequeue_size);
  g_mutex_lock (&self->dequeue_lock);
  self->dequeue_size = dequeue_size;
  g_cond_signal (&self->dequeue_cond);
  g_mutex_unlock (&self->dequeue_lock);
  GST_DEBUG_OBJECT (self, "Handle frame notified");
}

EMSCRIPTEN_BINDINGS (gst_web_codecs_audio_decoder)
{
  function ("gst_web_codecs_audio_decoder_on_output",
      &gst_web_codecs_audio_decoder_on_output);
  function ("gst_web_codecs_audio_decoder_on_error",
      &gst_web_codecs_audio_decoder_on_error);
  function ("gst_web_codecs_audio_decoder_on_dequeue",
      &gst_web_codecs_audio_decoder_on_dequeue);
}

static void
gst_web_codecs_audio_decoder_decode (gpointer data)
{
  GstWebCodecsAudioDecoderDecodeData *decode_data =
      (GstWebCodecsAudioDecoderDecodeData *) data;
  GstWebCodecsAudioDecoder *self = decode_data->self;
  GstBuffer *buf = decode_data->buffer;
  GstMapInfo map;
  val chunkclass = val::global ("EncodedAudioChunk");
  val options = val::object ();

  GST_DEBUG_OBJECT (self,
      "Decoding frame at %" GST_TIME_FORMAT " with duration %" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_PTS (buf)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (buf)));

  /* TODO use 'transfer' option in case we provide an allocator to avoid
   * the copy. For that, we need to create the memory in JS and give the
   * ownership to it.
   */
  if (GST_CLOCK_TIME_IS_VALID (GST_BUFFER_PTS (buf)))
    options.set ("timestamp",
        static_cast<int> (GST_TIME_AS_MSECONDS (GST_BUFFER_PTS (buf))));
  else
    GST_DEBUG_OBJECT (self, "PTS invalid");

  if (GST_CLOCK_TIME_IS_VALID (GST_BUFFER_DURATION (buf)))
    options.set ("duration",
        static_cast<int> (GST_TIME_AS_MSECONDS (GST_BUFFER_DURATION (buf))));
  else
    GST_DEBUG_OBJECT (self, "duration invalid");

  options.set ("type", "key"); // TODO: Everythin is a keyframe?
  gst_buffer_map (buf, &map, GST_MAP_READ);
  val buffer_data = val (typed_memory_view (map.size, map.data));
  options.set ("data", buffer_data);

  GST_LOG_OBJECT (self, "EncodedAudioChunk options: %s",
      val::global ("JSON")
          .call<val> ("stringify", options)
          .as<std::string> ()
          .c_str ());

  val chunk = chunkclass.new_ (options);

  self->decoder.call<void> ("decode", chunk);
  gst_buffer_unmap (buf, &map);

  GST_DEBUG_OBJECT (self, "Done decoding");
}

static void
gst_web_codecs_audio_decoder_configure (gpointer data)
{
  GstWebCodecsAudioDecoderConfigureData *conf_data =
      (GstWebCodecsAudioDecoderConfigureData *) data;
  GstWebCodecsAudioDecoder *self = conf_data->self;
  GstStructure *s;
  const GValue *codec_data_value = NULL;
  GstBuffer *codec_data = NULL;
  GstMapInfo map;
  gchar *mime_codec;
  gint channels, rate;
  val config = val::object ();

  mime_codec = gst_codec_utils_caps_get_mime_codec (conf_data->caps);
  config.set ("codec", std::string (mime_codec));
  g_free (mime_codec);

  s = gst_caps_get_structure (conf_data->caps, 0);
  if (!gst_structure_get_int (s, "channels", &channels)) {
    GST_ERROR_OBJECT (self, "Caps do not have channels");
    conf_data->ret = FALSE;
    return;
  }

  s = gst_caps_get_structure (conf_data->caps, 0);
  if (!gst_structure_get_int (s, "rate", &rate)) {
    GST_ERROR_OBJECT (self, "Caps do not have rate");
    conf_data->ret = FALSE;
    return;
  }
  codec_data_value = gst_structure_get_value (s, "codec_data");
  if (!codec_data_value) {
    GST_ERROR_OBJECT (self, "Caps do not have codec_data");
    conf_data->ret = FALSE;
    return;
  }

  codec_data = gst_value_get_buffer (codec_data_value);
  if (!gst_buffer_map (codec_data, &map, GST_MAP_READ)) {
    GST_ERROR_OBJECT (self, "Impossible to map the buffer");
    conf_data->ret = FALSE;
    return;
  }

  val codec_data_js = val (typed_memory_view (map.size, map.data));
  config.set ("description", codec_data_js);
  config.set ("numberOfChannels", channels);
  config.set ("sampleRate", rate);

  GST_DEBUG_OBJECT (self, "Setting format with config: %s",
      val::global ("JSON")
          .call<val> ("stringify", config)
          .as<std::string> ()
          .c_str ());
  self->decoder.call<void> ("configure", config);
  gst_buffer_unmap (codec_data, &map);
}

static void
gst_web_codecs_audio_decoder_ctor (gpointer data)
{
  GstWebCodecsAudioDecoder *self = GST_WEB_CODECS_AUDIO_DECODER (data);
  val vdecclass = val::global ("AudioDecoder");
  val mod = val::global ("Module");
  val options = val::object ();

  /* We keep self in the new thread Module */
  /* FIXME this will overwrite if other decoders/elements are handled
   * in the same thread
   */
  mod.set ("self", reinterpret_cast<int> (self));
  options.set ("output",
      val::module_property ("gst_web_codecs_audio_decoder_on_output"));
  options.set (
      "error", val::module_property ("gst_web_codecs_audio_decoder_on_error"));
  self->decoder = vdecclass.new_ (options);

  self->decoder.call<void> ("addEventListener", std::string ("dequeue"),
      val::module_property ("gst_web_codecs_audio_decoder_on_dequeue"));
  GST_DEBUG_OBJECT (self, "decoder created successfully");
}

/* Called with the streaming lock taken */
static gboolean
gst_web_codecs_audio_decoder_negotiate (GstAudioDecoder *decoder)
{
  GstWebCodecsAudioDecoder *self = GST_WEB_CODECS_AUDIO_DECODER (decoder);

  // GstCaps *caps = gst_pad_get_allowed_caps (GST_AUDIO_DECODER_SRC_PAD
  // (decoder));

  /* If we don't know the output format yet, skip this */
  if (!self->need_negotiation) {
    GST_DEBUG_OBJECT (decoder, "Negotiation not needed");
    return TRUE;
  }

  self->need_negotiation = FALSE;

  if (!self->output_caps) {
    GST_ERROR_OBJECT (decoder, "Output caps NULL");
    return FALSE;
  }

  if (!gst_audio_decoder_set_output_caps (decoder, self->output_caps)) {
    GST_ERROR_OBJECT (decoder, "Cannot set output caps");
    return FALSE;
  }

  if (!gst_audio_decoder_set_output_format (decoder, &self->output_info)) {
    GST_ERROR_OBJECT (decoder, "Failed to set output format");
    return FALSE;
  }
  // TODO: Implement GST_CAPS_FEATURE_MEMORY_WEB_AUDIO_DATA?
  gboolean ret = GST_AUDIO_DECODER_CLASS (parent_class)->negotiate (decoder);

  GST_DEBUG_OBJECT (decoder, "Negotiation ret: %d", ret);

  return ret;
}

static void
gst_web_codecs_audio_decoder_decode_data_free (
    GstWebCodecsAudioDecoderDecodeData *data)
{
  gst_buffer_unref (data->buffer);
  g_free (data);
}

static GstFlowReturn
gst_web_codecs_audio_decoder_handle_frame (
    GstAudioDecoder *decoder, GstBuffer *buffer)
{
  GstWebCodecsAudioDecoder *self = GST_WEB_CODECS_AUDIO_DECODER (decoder);
  GstWebCodecsAudioDecoderDecodeData *decode_data;
  GstFlowReturn res = GST_FLOW_OK;

  GST_DEBUG_OBJECT (self,
      "Handling frame with buffer at %" GST_TIME_FORMAT
      " with duration %" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_PTS (buffer)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (buffer)));

  /* Wait until there is nothing pending to be to dequeued or there is a buffer
   */
  GST_AUDIO_DECODER_STREAM_UNLOCK (self);
  g_mutex_lock (&self->dequeue_lock);
  while (self->dequeue_size >= GST_WEB_CODECS_AUDIO_DECODER_MAX_DEQUEUE) {
    GST_DEBUG_OBJECT (self, "Reached queue limit [%d/%d], waiting for dequeue",
        self->dequeue_size, GST_WEB_CODECS_AUDIO_DECODER_MAX_DEQUEUE);
    g_cond_wait (&self->dequeue_cond, &self->dequeue_lock);
  }
  self->dequeue_size++;
  g_mutex_unlock (&self->dequeue_lock);
  GST_AUDIO_DECODER_STREAM_LOCK (self);

  GST_DEBUG_OBJECT (self, "Ready to process more data");
  decode_data = g_new (GstWebCodecsAudioDecoderDecodeData, 1);
  decode_data->self = self;
  decode_data->buffer = gst_buffer_ref (buffer);

  gst_web_runner_send_message_async (self->runner,
      gst_web_codecs_audio_decoder_decode, decode_data,
      (GDestroyNotify) gst_web_codecs_audio_decoder_decode_data_free);

  GST_DEBUG_OBJECT (decoder, "Handle frame done");

  return res;
}

static gboolean
gst_web_codecs_audio_decoder_set_format (
    GstAudioDecoder *decoder, GstCaps *caps)
{
  GstWebCodecsAudioDecoder *self = GST_WEB_CODECS_AUDIO_DECODER (decoder);
  GstWebCodecsAudioDecoderConfigureData conf_data;

  GST_INFO_OBJECT (
      self, "Setting format with sink caps %" GST_PTR_FORMAT, caps);

  gst_web_runner_send_message (
      self->runner, gst_web_codecs_audio_decoder_ctor, self);
  /* Configure */
  conf_data.self = self;
  conf_data.caps = caps;
  gst_web_runner_send_message (
      self->runner, gst_web_codecs_audio_decoder_configure, &conf_data);

  /* Keep the input state available */
  if (self->input_caps)
    gst_caps_unref (self->input_caps);
  self->input_caps = gst_caps_ref (caps);

  return TRUE;
}

static void
gst_web_codecs_audio_decoder_flush (GstAudioDecoder *decoder, gboolean hard)
{
  GstWebCodecsAudioDecoder *self = GST_WEB_CODECS_AUDIO_DECODER (decoder);

  GST_DEBUG_OBJECT (self, "Flushing");
  // TODO: Implement.
  GST_DEBUG_OBJECT (self, "Flushed");
}

static gboolean
gst_web_codecs_audio_decoder_start (GstAudioDecoder *decoder)
{
  GstWebCodecsAudioDecoder *self = GST_WEB_CODECS_AUDIO_DECODER (decoder);
  gboolean ret = FALSE;

  GST_DEBUG_OBJECT (self, "Start");
  self->runner = gst_web_runner_new (NULL);
  if (!gst_web_runner_run (self->runner, NULL)) {
    GST_ERROR_OBJECT (self, "Impossible to run the runner");
    goto done;
  }
  GST_DEBUG_OBJECT (self, "Started");
  ret = TRUE;

done:
  return ret;
}

static gboolean
gst_web_codecs_audio_decoder_stop (GstAudioDecoder *decoder)
{
  GstWebCodecsAudioDecoder *self = GST_WEB_CODECS_AUDIO_DECODER (decoder);

  GST_DEBUG_OBJECT (self, "Stop");
  /* TODO Call reset */

  gst_object_unref (self->runner);
  g_clear_pointer (&self->input_caps, gst_caps_unref);
  g_clear_pointer (&self->output_caps, gst_caps_unref);

  GST_DEBUG_OBJECT (self, "Stopped");

  return TRUE;
}

static void
gst_web_codecs_audio_decoder_finalize (GObject *object)
{
  GstWebCodecsAudioDecoder *self = GST_WEB_CODECS_AUDIO_DECODER (object);

  g_mutex_clear (&self->dequeue_lock);
  g_cond_clear (&self->dequeue_cond);

  GST_DEBUG_OBJECT (self, "End of finalize");
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_web_codecs_audio_decoder_init (
    GstWebCodecsAudioDecoder *self, GstWebCodecsAudioDecoderClass g_class)
{
  g_mutex_init (&self->dequeue_lock);
  g_cond_init (&self->dequeue_cond);
}

static void
gst_web_codecs_audio_decoder_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);
  GstCaps *sink_caps;
  GstCaps *src_caps;
  GstPadTemplate *templ;

  sink_caps = (GstCaps *) g_type_get_qdata (
      G_TYPE_FROM_CLASS (g_class), gst_web_codecs_data_quark);
  /* This happens for the base class and abstract subclasses */
  if (!sink_caps)
    return;

  // TODO: Use more precise src_caps.
  // TODO: Support audio/x-raw(memory:WebCodecsAudioData), too?
  src_caps = gst_caps_from_string ("audio/x-raw");

  templ =
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps);
  gst_element_class_add_pad_template (element_class, templ);

  templ = gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS, src_caps);
  gst_element_class_add_pad_template (element_class, templ);
}

static void
gst_web_codecs_audio_decoder_class_init (
    GstWebCodecsAudioDecoderClass *klass, gpointer klass_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstAudioDecoderClass *audio_decoder_class = GST_AUDIO_DECODER_CLASS (klass);

  gobject_class->finalize = gst_web_codecs_audio_decoder_finalize;
  gst_element_class_set_static_metadata (element_class,
      "WebCodecs base audio decoder", "Codec/Decoder/Audio",
      "decode streams using WebCodecs API",
      "Fluendo S.A. <engineering@fluendo.com>");

  audio_decoder_class->start =
      GST_DEBUG_FUNCPTR (gst_web_codecs_audio_decoder_start);
  audio_decoder_class->stop =
      GST_DEBUG_FUNCPTR (gst_web_codecs_audio_decoder_stop);
  audio_decoder_class->flush =
      GST_DEBUG_FUNCPTR (gst_web_codecs_audio_decoder_flush);
  audio_decoder_class->set_format =
      GST_DEBUG_FUNCPTR (gst_web_codecs_audio_decoder_set_format);
  audio_decoder_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_web_codecs_audio_decoder_handle_frame);
  audio_decoder_class->negotiate =
      GST_DEBUG_FUNCPTR (gst_web_codecs_audio_decoder_negotiate);

  parent_class = g_type_class_peek_parent (klass);
}

GType
gst_web_codecs_audio_decoder_get_type (void)
{
  static gsize type = 0;

  if (g_once_init_enter (&type)) {
    GType _type;
    static const GTypeInfo info = { sizeof (GstWebCodecsAudioDecoderClass),
      (GBaseInitFunc) gst_web_codecs_audio_decoder_base_init, NULL,
      (GClassInitFunc) gst_web_codecs_audio_decoder_class_init, NULL, NULL,
      sizeof (GstWebCodecsAudioDecoder), 0,
      (GInstanceInitFunc) gst_web_codecs_audio_decoder_init, NULL };

    _type = g_type_register_static (GST_TYPE_AUDIO_DECODER,
        "GstWebCodecsAudioDecoder", &info, G_TYPE_FLAG_NONE);

    GST_DEBUG_CATEGORY_INIT (gst_web_codecs_audio_decoder_debug_category,
        "webcodecsauddec", 0, "WebCodecs Video Decoder");

    g_once_init_leave (&type, _type);
  }
  return type;
}
