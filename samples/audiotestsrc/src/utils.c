/* Copyright (C) Fluendo S.A. <support@fluendo.com> */
#include "utils.h"

const char *
gst_audio_info_to_audio_data_format (GstAudioInfo *ainfo)
{
  if (GST_AUDIO_FORMAT_INFO_IS_INTEGER (ainfo->finfo) &&
      !GST_AUDIO_FORMAT_INFO_IS_SIGNED (ainfo->finfo) &&
      GST_AUDIO_FORMAT_INFO_WIDTH (ainfo->finfo) == 8 &&
      ainfo->layout == GST_AUDIO_LAYOUT_INTERLEAVED)
    return "u8";

  if (GST_AUDIO_FORMAT_INFO_IS_INTEGER (ainfo->finfo) &&
      GST_AUDIO_FORMAT_INFO_IS_SIGNED (ainfo->finfo) &&
      GST_AUDIO_FORMAT_INFO_WIDTH (ainfo->finfo) == 16 &&
      ainfo->layout == GST_AUDIO_LAYOUT_INTERLEAVED)
    return "s16";

  if (GST_AUDIO_FORMAT_INFO_IS_INTEGER (ainfo->finfo) &&
      GST_AUDIO_FORMAT_INFO_IS_SIGNED (ainfo->finfo) &&
      GST_AUDIO_FORMAT_INFO_WIDTH (ainfo->finfo) == 32 &&
      ainfo->layout == GST_AUDIO_LAYOUT_INTERLEAVED)
    return "s32";

  if (GST_AUDIO_FORMAT_INFO_IS_FLOAT (ainfo->finfo) &&
      GST_AUDIO_FORMAT_INFO_WIDTH (ainfo->finfo) == 32 &&
      ainfo->layout == GST_AUDIO_LAYOUT_INTERLEAVED)
    return "f32";

  if (GST_AUDIO_FORMAT_INFO_IS_INTEGER (ainfo->finfo) &&
      !GST_AUDIO_FORMAT_INFO_IS_SIGNED (ainfo->finfo) &&
      GST_AUDIO_FORMAT_INFO_WIDTH (ainfo->finfo) == 8 &&
      ainfo->layout == GST_AUDIO_LAYOUT_NON_INTERLEAVED)
    return "u8-planar";

  if (GST_AUDIO_FORMAT_INFO_IS_INTEGER (ainfo->finfo) &&
      GST_AUDIO_FORMAT_INFO_IS_SIGNED (ainfo->finfo) &&
      GST_AUDIO_FORMAT_INFO_WIDTH (ainfo->finfo) == 16 &&
      ainfo->layout == GST_AUDIO_LAYOUT_NON_INTERLEAVED)
    return "s16-planar";

  if (GST_AUDIO_FORMAT_INFO_IS_INTEGER (ainfo->finfo) &&
      GST_AUDIO_FORMAT_INFO_IS_SIGNED (ainfo->finfo) &&
      GST_AUDIO_FORMAT_INFO_WIDTH (ainfo->finfo) == 32 &&
      ainfo->layout == GST_AUDIO_LAYOUT_NON_INTERLEAVED)
    return "s32-planar";

  if (GST_AUDIO_FORMAT_INFO_IS_FLOAT (ainfo->finfo) &&
      GST_AUDIO_FORMAT_INFO_WIDTH (ainfo->finfo) == 32 &&
      ainfo->layout == GST_AUDIO_LAYOUT_NON_INTERLEAVED)
    return "f32-planar";

  return NULL;
}