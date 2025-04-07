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
