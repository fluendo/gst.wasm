/*
 * GStreamer - gst.wasm WebCodecsAudioDecoder source
 *
 * Copyright 2024 - 2025 Fluendo S.A.
 *  @author: Jorge Zapata <jzapata@fluendo.com>
 *  @author: César Fabián Orccón Chipana <jzapata@fluendo.com>
 *
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

#ifndef __GST_WEB_CODECS_AUDIO_DECODER_H__
#define __GST_WEB_CODECS_AUDIO_DECODER_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/audio/gstaudiodecoder.h>
#include <emscripten/bind.h>
#include <string.h>
#include <gst/web/gstwebcanvas.h>
#include <gst/web/gstwebrunner.h>

G_BEGIN_DECLS

#define GST_TYPE_WEB_CODECS_AUDIO_DECODER                                     \
  (gst_web_codecs_audio_decoder_get_type ())
#define GST_WEB_CODECS_AUDIO_DECODER(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_CAST (                                               \
      (obj), GST_TYPE_WEB_CODECS_AUDIO_DECODER, GstWebCodecsAudioDecoder))
#define GST_WEB_CODECS_AUDIO_DECODER_CLASS(klass)                             \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_WEB_CODECS_AUDIO_DECODER,       \
      GstWebCodecsAudioDecoderClass))
#define GST_IS_WEB_CODECS_AUDIO_DECODER(obj)                                  \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_WEB_CODECS_AUDIO_DECODER))
#define GST_IS_WEB_CODECS_AUDIO_DECODER_CLASS(klass)                          \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_WEB_CODECS_AUDIO_DECODER))

typedef struct _GstWebCodecsAudioDecoder GstWebCodecsAudioDecoder;
typedef struct _GstWebCodecsAudioDecoderClass GstWebCodecsAudioDecoderClass;

/**
 * GstWebCodecsAudioDecoder:
 *
 * Opaque object data structure.
 */
struct _GstWebCodecsAudioDecoder
{
  GstAudioDecoder base;

  GstCaps *input_caps;
  GstCaps *output_caps;
  GstAudioInfo output_info;

  /* TODO: Move this to a prv struct */
  /* TODO: No need for a canvas here */
  GstWebCanvas *canvas;
  gboolean need_negotiation;

  emscripten::val decoder;
  /* Amount of the output frames pending to be dequeued */
  gint dequeue_size;
  GMutex dequeue_lock;
  GCond dequeue_cond;
};

struct _GstWebCodecsAudioDecoderClass
{
  GstAudioDecoderClass base;
};

GType gst_web_codecs_audio_decoder_get_type (void);

G_END_DECLS

#endif /* __GST_WEB_CODECS_AUDIO_DECODER_H__ */
