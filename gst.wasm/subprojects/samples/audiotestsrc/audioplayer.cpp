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

#include <iostream>
#include <string>
#include <emscripten/bind.h>
#include <emscripten/threading.h>
#include <gst/gst.h>
#include <gst/audio/audio-info.h>
#include "utils.h"

using namespace emscripten;

#define GST_CAT_DEFAULT (audioplayer_debug)
GST_DEBUG_CATEGORY (audioplayer_debug);

class AudioPlayer;
static void output_sample (AudioPlayer *decoder);

class AudioPlayer
{
private:
  val m_parent;
  GstElement *pipe;
  GstElement *sink;

public:
  GAsyncQueue *queue;

  AudioPlayer (val parent, std::string pipeline_desc) : m_parent (parent)
  {
    GST_DEBUG_CATEGORY_INIT (
        GST_CAT_DEFAULT, "audioplayer", 0, "audiotestsrc player");

    char *full_pipeline_desc = g_strdup_printf (
        "%s! appsink name=sink emit-signals=true", pipeline_desc.c_str ());

    queue = g_async_queue_new_full ((GDestroyNotify) gst_sample_unref);
    pipe = gst_parse_launch (full_pipeline_desc, NULL);
    g_free (full_pipeline_desc);

    g_assert (pipe != NULL);
    sink = gst_bin_get_by_name (GST_BIN (pipe), "sink");
    g_assert (sink != NULL);

    g_signal_connect (sink, "new-sample",
        G_CALLBACK (AudioPlayer::_appsink_new_sample_cb), this);
  }

  void
  Play ()
  {
    gst_element_set_state (pipe, GST_STATE_PLAYING);
  }

  static GstFlowReturn
  _appsink_new_sample_cb (GstElement *appsink, AudioPlayer *decoder)
  {
    GstSample *sample = NULL;
    GstBuffer *buffer;

    GST_LOG ("Has sample");
    g_signal_emit_by_name (appsink, "pull-sample", &sample);

    buffer = gst_sample_get_buffer (sample);
    GST_LOG ("buffer of size %" G_GSIZE_FORMAT ", ts = %" GST_TIME_FORMAT
             ", duration = %" GST_TIME_FORMAT,
        gst_buffer_get_size (buffer), GST_TIME_ARGS (GST_BUFFER_PTS (buffer)),
        GST_TIME_ARGS (GST_BUFFER_DURATION (buffer)));
    g_async_queue_push (decoder->queue, sample);

    GST_LOG ("Pushing to the main thread");
    // FIXME: Should run asynchronously.
    emscripten_sync_run_in_main_runtime_thread_ (
        EM_FUNC_SIG_VI, (void *) (output_sample), decoder);
    GST_LOG ("Main thread done");
    return GST_FLOW_OK;
  }

  void
  NewSampleCb (GstSample *sample)
  {
    GstAudioInfo ainfo;
    GstBuffer *buf = gst_sample_get_buffer (sample);
    GstCaps *caps = gst_sample_get_caps (sample);

    g_assert (gst_audio_info_from_caps (&ainfo, caps));

    val audio_data_klass = val::global ("AudioData");
    if (!audio_data_klass.as<bool> ()) {
      GST_ERROR ("AudioData does not exist in JS engine.");
      return;
    }

    // Convert appsink's GstSample to JS AudioData.
    val init = val::object ();
    const char *fmt = gst_audio_info_to_audio_data_format (&ainfo);
    init.set ("format", val (fmt));
    init.set ("sampleRate", val (ainfo.rate));
    init.set ("numberOfFrames", val (1)); // TODO
    init.set ("numberOfChannels", val (ainfo.channels));
    init.set ("timestamp", val ((int) (GST_BUFFER_PTS (buf) / GST_USECOND)));

    guint n_samples = gst_buffer_get_size (buf) / GST_AUDIO_INFO_BPF (&ainfo);

    val typed_array_klass =
        val::global ("Uint8Array"); // TODO. Depends on GstAudioFormatInfo.
    if (!typed_array_klass.as<bool> ()) {
      GST_ERROR ("Uint8Array does not exist in JS engine.");
      return;
    }

    GstMapInfo info;
    gst_buffer_map (buf, &info, GST_MAP_READ);

    // FIXME: how many times are we copying the raw audio samples here???
    init.set (
        "data", typed_array_klass.new_ (val::array ((const char *) info.data,
                    (const char *) info.data + info.size)));
    gst_buffer_unmap (buf, &info);

    val audio_data = audio_data_klass.new_ (init);

    // Should be called on Main Thread.
    m_parent.call<val> ("onNewSample", audio_data, val (n_samples));
  }
};

static void
output_sample (AudioPlayer *decoder)
{
  GstSample *sample;

  GST_LOG ("Handle sample on main thread");
  sample = (GstSample *) g_async_queue_try_pop (decoder->queue);
  if (!sample) {
    GST_ERROR ("No sample found.");
    return;
  }
  GST_LOG ("Have sample of %" G_GSIZE_FORMAT " bytes",
      gst_buffer_get_size (gst_sample_get_buffer (sample)));
  decoder->NewSampleCb (sample);
}

EMSCRIPTEN_BINDINGS (my_class_example2)
{
  class_<AudioPlayer> ("AudioPlayer")
      .constructor<val, std::string> ()
      .function ("play", &AudioPlayer::Play);
}
