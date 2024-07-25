/* Copyright (C) <2024> Fluendo S.A. <support@fluendo.com> */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "gstimagebitmapsrc.h"

#include <string.h>
#include <stdlib.h>
#include <emscripten/bind.h>
#include <emscripten/threading.h>

using namespace emscripten;

GST_DEBUG_CATEGORY_STATIC (image_bitmap_src_debug);
#define GST_CAT_DEFAULT image_bitmap_src_debug

// #define DEFAULT_PATTERN            GST_IMAGE_BITMAP_SRC_SMPTE
#define DEFAULT_IS_LIVE TRUE

enum
{
  PROP_0,
  PROP_ID,
  PROP_LAST
};

#define VTS_VIDEO_CAPS                                                        \
  GST_VIDEO_CAPS_MAKE (GST_VIDEO_FORMATS_ALL)                                 \
  ","                                                                         \
  "multiview-mode = { mono, left, right }"                                    \
  ";"

static GstStaticPadTemplate gst_image_bitmap_src_template =
    GST_STATIC_PAD_TEMPLATE (
        "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS (VTS_VIDEO_CAPS));

#define gst_image_bitmap_src_parent_class parent_class
G_DEFINE_TYPE (GstImageBitmapSrc, gst_image_bitmap_src, GST_TYPE_PUSH_SRC);
GST_ELEMENT_REGISTER_DEFINE (imagebitmapsrc, "imagebitmapsrc", GST_RANK_NONE,
    GST_TYPE_IMAGE_BITMAP_SRC);

static void gst_image_bitmap_src_set_property (
    GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_image_bitmap_src_get_property (
    GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static GstFlowReturn gst_image_bitmap_src_fill (
    GstPushSrc *psrc, GstBuffer *buffer);
static gboolean gst_image_bitmap_src_start (GstBaseSrc *basesrc);
static gboolean gst_image_bitmap_src_stop (GstBaseSrc *basesrc);

static void
gst_image_bitmap_src_class_init (GstImageBitmapSrcClass *klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpushsrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstpushsrc_class = (GstPushSrcClass *) klass;

  gobject_class->set_property = gst_image_bitmap_src_set_property;
  gobject_class->get_property = gst_image_bitmap_src_get_property;

  g_object_class_install_property (gobject_class, PROP_ID,
      g_param_spec_string ("id", "id", "id of canvas DOM element", NULL,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS)));

  gst_element_class_set_static_metadata (gstelement_class, "Video test source",
      "Source/Video", "Consumes data from an ImageBitmap",
      "Fluendo S.A. <engineering@fluendo.com>");

  gst_element_class_add_static_pad_template (
      gstelement_class, &gst_image_bitmap_src_template);

  gstbasesrc_class->start = gst_image_bitmap_src_start;
  gstbasesrc_class->stop = gst_image_bitmap_src_stop;
  gstpushsrc_class->fill = gst_image_bitmap_src_fill;
}

static void
gst_image_bitmap_src_init (GstImageBitmapSrc *src)
{
  /* we operate in time */
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (src), DEFAULT_IS_LIVE);
}

static void
gst_image_bitmap_src_set_property (
    GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  GstImageBitmapSrc *src = GST_IMAGE_BITMAP_SRC (object);

  switch (prop_id) {
    case PROP_ID:
      g_free (src->id);
      src->id = g_value_dup_string (value);
      break;
    default:
      break;
  }
}

static void
gst_image_bitmap_src_get_property (
    GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  GstImageBitmapSrc *src = GST_IMAGE_BITMAP_SRC (object);

  switch (prop_id) {
    case PROP_ID:
      g_value_set_string (value, src->id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_image_bitmap_src_get_bitmap_data (GstImageBitmapSrc *src)
{
  val document = val::global ("document");
  val canvas =
      document.call<val> ("getElementById", val (std::string (src->id)));
  val context2d = canvas.call<val> ("getContext", std::string ("2d"));
  // val image_data = context2d.call<val>("getImageData", 0, 0,
  // src->info.width, src->info.height);
  val image_data = context2d.call<val> ("getImageData", 0, 0, 300, 150);
  val data = image_data["data"];

  auto raw_data = vecFromJSArray<unsigned char> (data);
  guint length = data["length"].as<int> ();

  g_print ("length: %d\n", length);
}

static GstFlowReturn
gst_image_bitmap_src_fill (GstPushSrc *psrc, GstBuffer *buffer)
{
  GstImageBitmapSrc *src;
  GstClockTime next_time;
  GstVideoFrame frame;
  gconstpointer pal;
  gsize palsize;
  char r, g, b, a;

  src = GST_IMAGE_BITMAP_SRC (psrc);

  // TODO
  // if (G_UNLIKELY (GST_VIDEO_INFO_FORMAT (&src->info) ==
  //         GST_VIDEO_FORMAT_UNKNOWN))
  //   return GST_FLOW_NOT_NEGOTIATED;

  // if (!gst_video_frame_map (&frame, &src->info, buffer, GST_MAP_WRITE)) {
  //   GST_DEBUG_OBJECT (src, "invalid frame");
  //   return GST_FLOW_OK;
  // }

  emscripten_sync_run_in_main_runtime_thread_ (EM_FUNC_SIG_VI,
      (void *) (gst_image_bitmap_src_get_bitmap_data), src, NULL);

  // TODO: Calculate timestamps and fill buffer.
  GST_BUFFER_DTS (buffer) = GST_BUFFER_PTS (buffer) = GST_CLOCK_TIME_NONE;

  gst_object_sync_values (GST_OBJECT (psrc), GST_BUFFER_PTS (buffer));

  gst_video_frame_unmap (&frame);

  return GST_FLOW_OK;
}

static gboolean
gst_image_bitmap_src_start (GstBaseSrc *basesrc)
{
  GstImageBitmapSrc *src = GST_IMAGE_BITMAP_SRC (basesrc);

  GST_OBJECT_LOCK (src);
  gst_video_info_init (&src->info);
  GST_OBJECT_UNLOCK (src);

  return TRUE;
}

static gboolean
gst_image_bitmap_src_stop (GstBaseSrc *basesrc)
{
  GstImageBitmapSrc *src = GST_IMAGE_BITMAP_SRC (basesrc);
  guint i;

  return TRUE;
}

static gboolean
plugin_init (GstPlugin *plugin)
{
  GST_DEBUG_CATEGORY_INIT (
      image_bitmap_src_debug, "imagebitmapsrc", 0, "ImageBitmap Source");

  return GST_ELEMENT_REGISTER (imagebitmapsrc, plugin);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, imagebitmapsrc,
    "Creates a test video stream", plugin_init, VERSION, GST_LICENSE,
    GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
