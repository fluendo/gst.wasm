#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/base/gstpushsrc.h>

typedef struct _GstWebTransportSrc
{
  GstPushSrc base;
} GstWebTransportSrc;

G_DECLARE_FINAL_TYPE (
    WebFetchSrc, gst_web_fetch_src, GST, WEB_FETCH_SRC, GstPushSrc)
G_DEFINE_TYPE_WITH_CODE (WebFetchSrc, gst_web_fetch_src, GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (
        GST_TYPE_URI_HANDLER, gst_web_fetch_src_uri_handler_init));
GST_ELEMENT_REGISTER_DEFINE (
    web_fetch_src, "webfetchsrc", GST_RANK_SECONDARY, GST_TYPE_WEB_FETCH_SRC);

GST_DEBUG_CATEGORY_STATIC (gst_web_fetch_src_debug);

static void
gst_web_fetch_src_init (WebFetchSrc *src)
{
}

static GstFlowReturn
gst_web_fetch_src_create (GstPushSrc *psrc, GstBuffer **outbuf)
{
  WebFetchSrc *self = GST_WEB_FETCH_SRC (psrc);
  GstFlowReturn ret = GST_FLOW_OK;
  gchar *uri;

  GST_OBJECT_LOCK (self);
  GST_OBJECT_UNLOCK (self);
  return ret;
}

static GstStateChangeReturn
gst_web_fetch_src_change_state (GstElement *element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  WebFetchSrc *self = GST_WEB_FETCH_SRC (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (self->uri == NULL) {
        GST_ELEMENT_ERROR (
            element, RESOURCE, OPEN_READ, ("No URL set."), ("Missing URL"));
        return GST_STATE_CHANGE_FAILURE;
      }
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_OBJECT_LOCK (self);
      self->download_offset = 0;
      self->download_end = 0;
      self->resource_size = 0;
      self->in_eos = FALSE;
      GST_OBJECT_UNLOCK (self);
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_web_fetch_src_set_property (
    GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  WebFetchSrc *self = GST_WEB_FETCH_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      gst_web_fetch_src_urihandler_set_uri (
          GST_URI_HANDLER (self), g_value_get_string (value), NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_web_fetch_src_get_property (
    GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  WebFetchSrc *self = GST_WEB_FETCH_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      g_value_take_string (value,
          gst_web_fetch_src_urihandler_get_uri (GST_URI_HANDLER (self)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_web_fetch_src_finalize (GObject *obj)
{
  WebFetchSrc *self = GST_WEB_FETCH_SRC (obj);

  g_free (self->uri);

  G_OBJECT_CLASS (gst_web_fetch_src_parent_class)->finalize (obj);
}

static void
gst_web_fetch_src_class_init (WebFetchSrcClass *klass)
{

  /* const transport = new WebTransport(url);

    // The connection can be used once ready fulfills
    await transport.ready;
    return transport;
    Get the number of streams, unidirectional, bidirectional and datagrams
  */
  static GstStaticPadTemplate srcpadtemplate = GST_STATIC_PAD_TEMPLATE (
      "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBaseSrcClass *basesrc_class = (GstBaseSrcClass *) klass;
  GstPushSrcClass *pushsrc_class = (GstPushSrcClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_web_fetch_src_debug, "webfetchsrc", 0,
      "HTTP Client Source using Web Fetch API");

  element_class->change_state = gst_web_fetch_src_change_state;

  pushsrc_class->create = gst_web_fetch_src_create;
  gst_element_class_add_pad_template (
      element_class, gst_static_pad_template_get (&srcpadtemplate));

  gobject_class->set_property = gst_web_fetch_src_set_property;
  gobject_class->get_property = gst_web_fetch_src_get_property;
  gobject_class->finalize = gst_web_fetch_src_finalize;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "Location", "URI of resource to read",
          PROP_LOCATION_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (element_class,
      "HTTP Client Source using Web Fetch API", "Source/Network",
      "Receiver data as a client over a network via HTTP using Web Fetch API",
      "Alexander Slobodeniuk <aslobodeniuk@fluendo.com>");
}
