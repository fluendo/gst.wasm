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
#include "gstwebcodecsvideodecoder.h"

using namespace emscripten;

#define WEBMEM 1
#define GST_WEB_CODECS_VIDEO_DECODER_MAX_DEQUEUE 8

#define GST_WEB_CODECS_VIDEO_DECODER_FROM_JS                                  \
  reinterpret_cast<GstWebCodecsVideoDecoder *> (                              \
      val::module_property ("self").as<int> ())

#define GST_CAT_DEFAULT gst_web_codecs_video_decoder_debug_category
GST_DEBUG_CATEGORY_STATIC (gst_web_codecs_video_decoder_debug_category);

static gpointer parent_class = NULL;

typedef struct _GstWebCodecsVideoDecoderConfigureData
{
  GstWebCodecsVideoDecoder *self;
  GstVideoCodecState *state;
  gboolean ret;
} GstWebCodecsVideoDecoderConfigureData;

typedef struct _GstWebCodecsVideoDecoderDecodeData
{
  GstWebCodecsVideoDecoder *self;
  GstVideoCodecFrame *frame;
} GstWebCodecsVideoDecoderDecodeData;

#ifndef WEBMEM
static void
gst_web_codecs_video_decoder_video_frame_to_codec_frame (
    GstWebCodecsVideoDecoder *self, val video_frame, GstVideoCodecFrame *frame)
{
  GstMapInfo map;
  val buffer_data;
  val options;

  GST_DEBUG_OBJECT (self, "Converting frame to C memory");
  /* destination buffer */
  /* map the buffer into an ArrayBuffer */
  gst_buffer_map (frame->output_buffer, &map, GST_MAP_WRITE);
  buffer_data = val (typed_memory_view (map.size, map.data));
  /* set the options */
  options = val::object ();
  /* Get the format from the output format */
  /* FIXME if the format is not of any rgb type, it complains and we must not
   * set it Also, we need to specify properly the layout of the planes. Check
   * allocationSize()
   */
  options.set ("format", std::string (self->output_format));
  video_frame.call<val> ("copyTo", buffer_data, options).await ();
  gst_buffer_unmap (frame->output_buffer, &map);
  GST_DEBUG_OBJECT (self, "Converted frame to C memory");
  video_frame.call<void> ("close");
}
#endif

static gboolean
gst_web_codecs_video_decoder_get_format (
    GstWebCodecsVideoDecoder *self, val video_frame)
{
  GstVideoFormat format;
  const gchar *vf_format;
  gint width;
  gint height;

  /* Get the video frame format */
  vf_format = video_frame["format"].as<std::string> ().c_str ();
  if (!g_strcmp0 (vf_format, "I420")) {
    format = GST_VIDEO_FORMAT_I420;
  } else if (!g_strcmp0 (vf_format, "I420A")) {
    format = GST_VIDEO_FORMAT_A420;
  } else if (!g_strcmp0 (vf_format, "I422")) {
    format = GST_VIDEO_FORMAT_Y42B;
  } else if (!g_strcmp0 (vf_format, "I444")) {
    format = GST_VIDEO_FORMAT_Y444;
  } else if (!g_strcmp0 (vf_format, "NV12")) {
    format = GST_VIDEO_FORMAT_NV12;
  } else if (!g_strcmp0 (vf_format, "RGBA")) {
    format = GST_VIDEO_FORMAT_RGBA;
  } else if (!g_strcmp0 (vf_format, "RGBX")) {
    format = GST_VIDEO_FORMAT_RGBx;
  } else if (!g_strcmp0 (vf_format, "BGRA")) {
    format = GST_VIDEO_FORMAT_BGRA;
  } else if (!g_strcmp0 (vf_format, "BGRX")) {
    format = GST_VIDEO_FORMAT_BGRx;
  } else {
    GST_ERROR_OBJECT (self, "Unsupported format %s", vf_format);
    goto done;
  }
//  self->output_format = g_strdup ("RGBA"); // g_strdup (vf_format);
//  format = GST_VIDEO_FORMAT_RGBA;          // FIXME, for now
  self->output_format = g_strdup (vf_format);
  width = video_frame["displayWidth"].as<int> ();
  height = video_frame["displayHeight"].as<int> ();

  GST_DEBUG_OBJECT (self, "width: %d, height: %d, format: %s",
      width, height, self->output_format);

  self->format = format;
  self->width = width;
  self->height = height;

//  ret = gst_video_decoder_negotiate (GST_VIDEO_DECODER (self));

done:
  return TRUE;
}

static void
gst_web_codecs_video_decoder_on_output (val video_frame)
{
  GstWebCodecsVideoDecoder *self = GST_WEB_CODECS_VIDEO_DECODER_FROM_JS;
  GstBuffer *b;
  GstWebVideoFrame *memory;

  GST_INFO_OBJECT (self, "VideoFrame Received");

  if (!self->output_state) {
     // TODO: mutex ??
     gst_web_codecs_video_decoder_get_format (self, video_frame);
     self->need_negotiation = TRUE;
  }

  memory = gst_web_video_frame_wrap (video_frame, self->runner);
  g_assert (memory != NULL);
  b = gst_buffer_new ();
  gst_buffer_insert_memory (b, -1, GST_MEMORY_CAST (memory));

  g_async_queue_push (self->decoded_js, b);
}

static void
gst_web_codecs_video_decoder_on_error (val error)
{
  /* TODO handle this */
  GST_ERROR ("Error received");
}

static void
gst_web_codecs_video_decoder_on_dequeue (val event)
{
  GstWebCodecsVideoDecoder *self = GST_WEB_CODECS_VIDEO_DECODER_FROM_JS;
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

EMSCRIPTEN_BINDINGS (gst_web_codecs_video_decoder)
{
  function ("gst_web_codecs_video_decoder_on_output",
      &gst_web_codecs_video_decoder_on_output);
  function ("gst_web_codecs_video_decoder_on_error",
      &gst_web_codecs_video_decoder_on_error);
  function ("gst_web_codecs_video_decoder_on_dequeue",
      &gst_web_codecs_video_decoder_on_dequeue);
}

static void
gst_web_codecs_video_decoder_decode (gpointer data)
{
  GstWebCodecsVideoDecoderDecodeData *decode_data =
      (GstWebCodecsVideoDecoderDecodeData *) data;
  GstWebCodecsVideoDecoder *self = decode_data->self;
  GstVideoCodecFrame *frame = decode_data->frame;
  GstMapInfo map;
  val chunkclass = val::global ("EncodedVideoChunk");
  val options = val::object ();

  GST_DEBUG_OBJECT (self,
      "Decoding frame at %" GST_TIME_FORMAT " with duration %" GST_TIME_FORMAT,
      GST_TIME_ARGS (frame->pts), GST_TIME_ARGS (frame->duration));

  /* TODO use 'transfer' option in case we provide an allocator to avoid
   * the copy. For that, we need to create the memory in JS and give the
   * ownership to it.
   */
  if (GST_CLOCK_TIME_IS_VALID (frame->pts))
    options.set (
        "timestamp", static_cast<int> (GST_TIME_AS_MSECONDS (frame->pts)));
  if (GST_CLOCK_TIME_IS_VALID (frame->duration))
    options.set (
        "duration", static_cast<int> (GST_TIME_AS_MSECONDS (frame->duration)));
  options.set (
      "type", GST_VIDEO_CODEC_FRAME_IS_SYNC_POINT (frame) ? "key" : "delta");
  gst_buffer_map (frame->input_buffer, &map, GST_MAP_READ);
  val buffer_data = val (typed_memory_view (map.size, map.data));
  options.set ("data", buffer_data);

  val chunk = chunkclass.new_ (options);
  self->decoder.call<void> ("decode", chunk);
  gst_buffer_unmap (frame->input_buffer, &map);

  gst_video_codec_frame_unref (frame);
  GST_DEBUG_OBJECT (self, "Done decoding");
}

static void
gst_web_codecs_video_decoder_configure (gpointer data)
{
  GstWebCodecsVideoDecoderConfigureData *conf_data =
      (GstWebCodecsVideoDecoderConfigureData *) data;
  GstWebCodecsVideoDecoder *self = conf_data->self;
  GstVideoCodecState *state = conf_data->state;
  GstStructure *s;
  const GValue *codec_data_value = NULL;
  GstBuffer *codec_data = NULL;
  GstMapInfo map;
  gchar *mime_codec;
  val config = val::object ();

  mime_codec = gst_codec_utils_caps_get_mime_codec (state->caps);
  config.set ("codec", std::string (mime_codec));
  g_free (mime_codec);

  s = gst_caps_get_structure (state->caps, 0);
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

  GST_DEBUG_OBJECT (self, "Setting format");
  self->decoder.call<void> ("configure", config);
  gst_buffer_unmap (codec_data, &map);
}

static void
gst_web_codecs_video_decoder_ctor (gpointer data)
{
  GstWebCodecsVideoDecoder *self = GST_WEB_CODECS_VIDEO_DECODER (data);
  val vdecclass = val::global ("VideoDecoder");
  val mod = val::global ("Module");
  val options = val::object ();

  /* We keep self in the new thread Module */
  /* FIXME this will overwrite if other decoders/elements are handled
   * in the same thread
   */
  mod.set ("self", reinterpret_cast<int> (self));
  options.set ("output",
      val::module_property ("gst_web_codecs_video_decoder_on_output"));
  options.set (
      "error", val::module_property ("gst_web_codecs_video_decoder_on_error"));
  self->decoder = vdecclass.new_ (options);

  self->decoder.call<void> ("addEventListener", std::string ("dequeue"),
      val::module_property ("gst_web_codecs_video_decoder_on_dequeue"));
  GST_DEBUG_OBJECT (self, "decoder created successfully");
}

/* Called with the streaming lock taken */
static gboolean
gst_web_codecs_video_decoder_negotiate (GstVideoDecoder *decoder)
{
  GstWebCodecsVideoDecoder *self = GST_WEB_CODECS_VIDEO_DECODER (decoder);

  /* If we don't know the output format yet, skip this */
  if (!self->need_negotiation) {
     g_abort ();
    return TRUE;
  }

  self->need_negotiation = FALSE;

  if (self->output_state) {
    gst_video_codec_state_unref (self->output_state);
    self->output_state = NULL;
  }

  self->output_state = gst_video_decoder_set_output_state (
      decoder, self->format, self->width, self->height, self->input_state);

  /* FIXME this depends on the downstream negotiation */
  /* Set the memory type */
  self->output_state->caps =
      gst_video_info_to_caps (&self->output_state->info);
#if WEBMEM
  gst_caps_set_features_simple (self->output_state->caps,
      gst_caps_features_new (
          GST_CAPS_FEATURE_MEMORY_WEB_VIDEO_FRAME, (char *) NULL));
#endif
  return GST_VIDEO_DECODER_CLASS (parent_class)->negotiate (decoder);
}

static GstFlowReturn
gst_web_codecs_video_decoder_handle_frame (
    GstVideoDecoder *decoder, GstVideoCodecFrame *frame)
{
  GstWebCodecsVideoDecoder *self = GST_WEB_CODECS_VIDEO_DECODER (decoder);
  GstWebCodecsVideoDecoderDecodeData *decode_data;
  GstFlowReturn res = GST_FLOW_OK;

  GstBuffer *buffer;
  while (NULL !=
	 (buffer = (GstBuffer *)
	  g_async_queue_try_pop (self->decoded_js))) {
     GstFlowReturn flow;
     GstVideoCodecFrame *f;

     if (G_UNLIKELY (self->need_negotiation)) {
	if (!gst_video_decoder_negotiate (decoder)) {
	   res = GST_FLOW_NOT_NEGOTIATED;
	   break;
	}

	GST_ERROR_OBJECT (self, "Output caps are %" GST_PTR_FORMAT,
			  self->output_state->allocation_caps);
     }
     
     f = gst_video_decoder_get_oldest_frame (decoder);
     g_assert (f);
     g_assert (f->output_buffer == NULL);
     f->output_buffer = buffer;
     GST_DEBUG_OBJECT (self, "Will push frame with ts %" GST_TIME_FORMAT,
		       GST_TIME_ARGS (f->pts));
     
     GST_DEBUG_OBJECT (self, "Finishing frame %p", f);
     flow = gst_video_decoder_finish_frame (decoder, f);
     if (flow != GST_FLOW_OK) {
	g_error ("Flow error: %d", flow);
	res = flow;
	break;
     }
  }
  
  GST_DEBUG_OBJECT (decoder, "Handling frame");
  /* Wait until there is nothing pending to be to dequeued or there is a buffer
   */
//  GST_VIDEO_DECODER_STREAM_UNLOCK (self);
  g_mutex_lock (&self->dequeue_lock);
  while (self->dequeue_size >= GST_WEB_CODECS_VIDEO_DECODER_MAX_DEQUEUE) {
    GST_DEBUG_OBJECT (self, "Reached queue limit [%d/%d], waiting for dequeue",
        self->dequeue_size, GST_WEB_CODECS_VIDEO_DECODER_MAX_DEQUEUE);
    g_cond_wait (&self->dequeue_cond, &self->dequeue_lock);
  }
  self->dequeue_size++;
  g_mutex_unlock (&self->dequeue_lock);
//  GST_VIDEO_DECODER_STREAM_LOCK (self);

  /* We are ready to process data */
  GST_DEBUG_OBJECT (self, "Ready to process more data");
  decode_data = g_new (GstWebCodecsVideoDecoderDecodeData, 1);
  decode_data->self = self;
  decode_data->frame = frame;
  /* We can not keep the stream lock taken here and when the decoder outputs
   * frames, do it asynchronous
   */
//  runner = gst_web_canvas_get_runner (self->canvas);
  gst_web_runner_send_message_async (
      self->runner, gst_web_codecs_video_decoder_decode, decode_data, g_free);
//  gst_object_unref (GST_OBJECT (runner));
  GST_DEBUG_OBJECT (decoder, "Handle frame done");

  return res;
}

static gboolean
gst_web_codecs_video_decoder_set_format (
    GstVideoDecoder *decoder, GstVideoCodecState *state)
{
  GstWebCodecsVideoDecoder *self = GST_WEB_CODECS_VIDEO_DECODER (decoder);
  GstWebCodecsVideoDecoderConfigureData conf_data;

  GST_INFO_OBJECT (
      self, "Setting format with sink caps %" GST_PTR_FORMAT, state->caps);
#if 0
  /* If the output format IS GL, we need to request the context
   * and do everything on the GL context thread, otherwise create
   * our own thread. This is a limitation on how Emscripten works
   * tegarding emscripten::val and threads. The vals are not shared
   * among threads, are local.
   * We have an open issue for this case:
   * https://github.com/emscripten-core/emscripten/issues/22541
   *
   * In any case we'll have a situation, the video_frame belongs to the WebRunner
   * transferring the image to a GLTexture requires that both, the GL thread and the
   * WebRunner thread share the js video_frame ... we'll figure that out later.
   * There are several options to handle this
   */

  if (!gst_web_runner_run (self->runner, NULL)) {
    GST_ERROR_OBJECT (self, "Impossible to run the runner");
    return FALSE;
  }
#endif
  /* TODO Check if downstream OpenGL is supported, if so, request the GL
   * context */

  /* Call constructor */
  gst_web_runner_send_message (
      self->runner, gst_web_codecs_video_decoder_ctor, self);
  /* Configure */
  conf_data.self = self;
  conf_data.state = state;
  gst_web_runner_send_message (
      self->runner, gst_web_codecs_video_decoder_configure, &conf_data);

  /* Keep the input state available */
  if (self->input_state)
    gst_video_codec_state_unref (self->input_state);
  self->input_state = gst_video_codec_state_ref (state);


  return TRUE;
}

static gboolean
gst_web_codecs_video_decoder_flush (GstVideoDecoder *decoder)
{
  GstWebCodecsVideoDecoder *self = GST_WEB_CODECS_VIDEO_DECODER (decoder);

  GST_DEBUG_OBJECT (self, "Flushing");
  /* TODO Call flush */
  GST_DEBUG_OBJECT (self, "Flushed");

  return TRUE;
}

static gboolean
gst_web_codecs_video_decoder_open (GstVideoDecoder *decoder)
{
  GstWebCodecsVideoDecoder *self = GST_WEB_CODECS_VIDEO_DECODER (decoder);
  gboolean ret = FALSE;

#if 0
  if (!gst_web_utils_element_ensure_canvas (
          GST_ELEMENT (self), &self->canvas, NULL)) {
    GST_ERROR_OBJECT (self, "Failed requesting a WebCanvas context");
    goto done;
  }
#endif

  ret = TRUE;

done:
  return ret;
}

static gboolean
gst_web_codecs_video_decoder_start (GstVideoDecoder *decoder)
{
  GstWebCodecsVideoDecoder *self = GST_WEB_CODECS_VIDEO_DECODER (decoder);
  gboolean ret = FALSE;

  GST_DEBUG_OBJECT (self, "Start");
#if 0
  self->runner = gst_web_canvas_get_runner (self->canvas);
#else
  // Create runner with no canvas
  self->runner = gst_web_runner_new (NULL);
#endif
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
gst_web_codecs_video_decoder_stop (GstVideoDecoder *decoder)
{
  GstWebCodecsVideoDecoder *self = GST_WEB_CODECS_VIDEO_DECODER (decoder);

  GST_DEBUG_OBJECT (self, "Stop");
  /* TODO Call reset */

  if (self->output_format) {
    g_free (self->output_format);
    self->output_format = NULL;
  }

  if (self->input_state) {
    gst_video_codec_state_unref (self->input_state);
    self->input_state = NULL;
  }

  if (self->output_state) {
    gst_video_codec_state_unref (self->output_state);
    self->output_state = NULL;
  }

  GST_DEBUG_OBJECT (self, "Stopped");

  return TRUE;
}

static void
gst_web_codecs_video_decoder_set_context (
    GstElement *element, GstContext *context)
{
#if 0
  GstWebCodecsVideoDecoder *self = GST_WEB_CODECS_VIDEO_DECODER (element);

  gst_web_utils_element_set_context (element, context, &self->canvas);
#endif
}

G_GNUC_UNUSED static gboolean
gst_web_codecs_video_decoder_query (GstElement *element, GstQuery *query)
{
  GstWebCodecsVideoDecoder *self = GST_WEB_CODECS_VIDEO_DECODER (element);
  gboolean ret = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONTEXT:
#if 0
      ret = gst_web_utils_element_handle_context_query (
          element, query, self->canvas);
#endif
      break;
    default:
      break;
  }

  if (!ret)
    ret = GST_ELEMENT_CLASS (parent_class)->query (element, query);

  return ret;
}

static void
gst_web_codecs_video_decoder_finalize (GObject *object)
{
  GstWebCodecsVideoDecoder *self = GST_WEB_CODECS_VIDEO_DECODER (object);

  g_mutex_clear (&self->dequeue_lock);
  g_cond_clear (&self->dequeue_cond);
  if (self->canvas) {
    gst_object_unref (self->canvas);
    self->canvas = NULL;
  }

  GST_DEBUG_OBJECT (self, "End of finalize");
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_web_codecs_video_decoder_init (
    GstWebCodecsVideoDecoder *self, GstWebCodecsVideoDecoderClass g_class)
{
  gst_video_decoder_set_needs_sync_point (GST_VIDEO_DECODER (self), TRUE);
  g_mutex_init (&self->dequeue_lock);
  g_cond_init (&self->dequeue_cond);

  // TODO: free in finalize/dispose
  self->decoded_js = g_async_queue_new();
}

static void
gst_web_codecs_video_decoder_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);
  GstCaps *sink_caps;
  GstCaps *src_caps;
  GstCaps *copy_to_caps;
  GstCaps *gl_caps;
  GstCaps *video_frame_caps;
  GstPadTemplate *templ;

  sink_caps = (GstCaps *) g_type_get_qdata (
      G_TYPE_FROM_CLASS (g_class), gst_web_codecs_data_quark);
  /* This happens for the base class and abstract subclasses */
  if (!sink_caps)
    return;

  /* The caps accepted by the copyTo() */
  /* FIXME remove it once the VideoFrame based memory is ready */
  copy_to_caps = gst_caps_from_string ("video/x-raw, "
                                       "format = { (string) RGBA, (string) "
                                       "RGBx, (string) BGRA, (string) BGRx }");

  video_frame_caps = gst_caps_from_string (
      "video/x-raw"
#if WEBMEM
      "(" GST_CAPS_FEATURE_MEMORY_WEB_VIDEO_FRAME ")"
#endif
      ", "
      "format = { (string) RGBA, (string) RGBx, (string) BGRA, (string) BGRx, "
      "(string) I420, (string) A420, (string) Y42B, (string) Y444, (string) "
      "NV12 }");
  gl_caps = gst_caps_from_string (
      "video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY
      "), format = (string) RGBA, texture-target = (string) 2D");

  src_caps = gst_caps_merge (gl_caps, copy_to_caps);
  src_caps = gst_caps_merge (src_caps, video_frame_caps);
  /* Add pad templates */
  templ =
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps);
  gst_element_class_add_pad_template (element_class, templ);

  templ = gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS, src_caps);
  gst_element_class_add_pad_template (element_class, templ);
  gst_caps_unref (gl_caps);
}

static void
gst_web_codecs_video_decoder_class_init (
    GstWebCodecsVideoDecoderClass *klass, gpointer klass_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoDecoderClass *video_decoder_class = GST_VIDEO_DECODER_CLASS (klass);

  gobject_class->finalize = gst_web_codecs_video_decoder_finalize;
  element_class->set_context = gst_web_codecs_video_decoder_set_context;
//  element_class->query = gst_web_codecs_video_decoder_query;
  gst_element_class_set_static_metadata (element_class,
      "WebCodecs base video decoder", "Codec/Decoder/Video",
      "decode streams using WebCodecs API",
      "Fluendo S.A. <engineering@fluendo.com>");

  video_decoder_class->open =
      GST_DEBUG_FUNCPTR (gst_web_codecs_video_decoder_open);
  video_decoder_class->start =
      GST_DEBUG_FUNCPTR (gst_web_codecs_video_decoder_start);
  video_decoder_class->stop =
      GST_DEBUG_FUNCPTR (gst_web_codecs_video_decoder_stop);
  video_decoder_class->flush =
      GST_DEBUG_FUNCPTR (gst_web_codecs_video_decoder_flush);
  video_decoder_class->set_format =
      GST_DEBUG_FUNCPTR (gst_web_codecs_video_decoder_set_format);
  video_decoder_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_web_codecs_video_decoder_handle_frame);
  video_decoder_class->negotiate =
      GST_DEBUG_FUNCPTR (gst_web_codecs_video_decoder_negotiate);

  parent_class = g_type_class_peek_parent (klass);
}

GType
gst_web_codecs_video_decoder_get_type (void)
{
  static gsize type = 0;

  if (g_once_init_enter (&type)) {
    GType _type;
    static const GTypeInfo info = { sizeof (GstWebCodecsVideoDecoderClass),
      (GBaseInitFunc) gst_web_codecs_video_decoder_base_init, NULL,
      (GClassInitFunc) gst_web_codecs_video_decoder_class_init, NULL, NULL,
      sizeof (GstWebCodecsVideoDecoder), 0,
      (GInstanceInitFunc) gst_web_codecs_video_decoder_init, NULL };

    _type = g_type_register_static (GST_TYPE_VIDEO_DECODER,
        "GstWebCodecsVideoDecoder", &info, G_TYPE_FLAG_NONE);

    GST_DEBUG_CATEGORY_INIT (gst_web_codecs_video_decoder_debug_category,
        "webcodecsviddec", 0, "WebCodecs Video Decoder");

    g_once_init_leave (&type, _type);
  }
  return type;
}
