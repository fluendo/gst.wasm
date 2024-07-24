/* Copyright (C) Fluendo S.A. <support@fluendo.com> */
#ifndef __AUDIOTESTSRC_SAMPLE_UTILS_H__
#define __AUDIOTESTSRC_SAMPLE_UTILS_H__

#include <glib.h>
#include <gst/audio/audio-info.h>

G_BEGIN_DECLS

const char *gst_audio_info_to_audio_data_format (GstAudioInfo *ainfo);

G_END_DECLS

#endif /* __AUDIOTESTSRC_SAMPLE_UTILS_H__ */