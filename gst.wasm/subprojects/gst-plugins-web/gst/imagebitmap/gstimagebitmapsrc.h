/* Copyright (C) <2024> Fluendo S.A. <support@fluendo.com> */
#ifndef __GST_IMAGE_BITMAP_SRC_H__
#define __GST_IMAGE_BITMAP_SRC_H__

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

G_BEGIN_DECLS

#define GST_TYPE_IMAGE_BITMAP_SRC (gst_image_bitmap_src_get_type())
G_DECLARE_FINAL_TYPE (GstImageBitmapSrc, gst_image_bitmap_src, GST, IMAGE_BITMAP_SRC,
    GstPushSrc)

/**
 * GstImageBitmapSrc:
 *
 * Opaque data structure.
 */
struct _GstImageBitmapSrc {
  GstPushSrc element;

  /*< private >*/
  GstVideoInfo info; /* protected by the object or stream lock */
  gchar *id;
};

GST_ELEMENT_REGISTER_DECLARE (imagebitmapsrc);

G_END_DECLS

#endif /* __GST_IMAGE_BITMAP_SRC_H__ */