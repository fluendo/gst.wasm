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

#ifndef __AUDIOTESTSRC_SAMPLE_UTILS_H__
#define __AUDIOTESTSRC_SAMPLE_UTILS_H__

#include <glib.h>
#include <gst/audio/audio-info.h>

G_BEGIN_DECLS

const char *gst_audio_info_to_audio_data_format (GstAudioInfo *ainfo);

G_END_DECLS

#endif /* __AUDIOTESTSRC_SAMPLE_UTILS_H__ */