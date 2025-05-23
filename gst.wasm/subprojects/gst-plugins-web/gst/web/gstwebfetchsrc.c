/*
 * GStreamer - gst.wasm Web Fetch HTTP source
 *
 * Copyright 2024 Fluendo S.A.
 *  @author: Alexander Slobodeniuk <aslobodeniuk@fluendo.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/base/gstpushsrc.h>
#include <emscripten/fetch.h>
#include <stdio.h>
#include <string.h>

static void gst_web_fetch_src_uri_handler_init (
    gpointer g_iface, gpointer iface_data);

#define GST_TYPE_WEB_FETCH_SRC (gst_web_fetch_src_get_type ())
#define GST_CAT_DEFAULT gst_web_fetch_src_debug
#define parent_class gst_web_fetch_src_parent_class

#define GST_WEB_FETCH_SRC_BYTES_CR " bytes "
#define CHUNK_SIZE 1048576

#define PROP_LOCATION_DEFAULT NULL

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_MAX
};

typedef struct _GstWebFetchSrc
{
  GstPushSrc element;

  gchar *uri;
  gboolean in_eos;
  gsize download_offset;
  gsize download_end;
  gsize resource_size;
} GstWebFetchSrc;

G_DECLARE_FINAL_TYPE (
    GstWebFetchSrc, gst_web_fetch_src, GST, WEB_FETCH_SRC, GstPushSrc)
G_DEFINE_TYPE_WITH_CODE (GstWebFetchSrc, gst_web_fetch_src, GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (
        GST_TYPE_URI_HANDLER, gst_web_fetch_src_uri_handler_init));
GST_ELEMENT_REGISTER_DEFINE (
    web_fetch_src, "webfetchsrc", GST_RANK_SECONDARY, GST_TYPE_WEB_FETCH_SRC);
GST_DEBUG_CATEGORY_STATIC (gst_web_fetch_src_debug);

static guint
gst_web_fetch_src_urihandler_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_web_fetch_src_urihandler_get_protocols (GType type)
{
  static const gchar *protocols[] = { "http", "https", NULL };

  return protocols;
}

static gboolean
gst_web_fetch_src_urihandler_set_uri (
    GstURIHandler *handler, const gchar *uri, GError **error)
{
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (handler);

  g_return_val_if_fail (GST_IS_URI_HANDLER (handler), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  GST_OBJECT_LOCK (self);

  if (self->uri != NULL) {
    GST_DEBUG_OBJECT (self,
        "URI already present as %s, updating to new URI %s", self->uri, uri);
    g_free (self->uri);
  }

  self->uri = g_strdup (uri);
  GST_OBJECT_UNLOCK (self);

  return TRUE;
}

static gchar *
gst_web_fetch_src_urihandler_get_uri (GstURIHandler *handler)
{
  gchar *ret;
  GstWebFetchSrc *self;

  g_return_val_if_fail (GST_IS_URI_HANDLER (handler), NULL);
  self = GST_WEB_FETCH_SRC (handler);

  GST_OBJECT_LOCK (self);
  ret = g_strdup (self->uri);
  GST_OBJECT_UNLOCK (self);

  return ret;
}

static void
gst_web_fetch_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *uri_iface = (GstURIHandlerInterface *) g_iface;

  uri_iface->get_type = gst_web_fetch_src_urihandler_get_type;
  uri_iface->get_protocols = gst_web_fetch_src_urihandler_get_protocols;
  uri_iface->get_uri = gst_web_fetch_src_urihandler_get_uri;
  uri_iface->set_uri = gst_web_fetch_src_urihandler_set_uri;
}

static void
gst_web_fetch_src_init (GstWebFetchSrc *src)
{
}

static gsize
gst_web_fetch_src_parse_content_size_field (
    GstWebFetchSrc *self, emscripten_fetch_t *fetch)
{
  gsize length, ret = 0;

  length = emscripten_fetch_get_response_headers_length (fetch);
  if (length != 0) {
    char *h = g_malloc (length + 1);
    char **headers;
    char **header;

    emscripten_fetch_get_response_headers (fetch, h, length + 1);
    GST_LOG_OBJECT (self, "Received headers: %s", h);

    headers = emscripten_fetch_unpack_response_headers (h);
    for (header = headers; *header != NULL; header++) {
      char *ptr;

      if ((ptr = strstr (*header, "ontent-range")) &&
          G_LIKELY (ptr > *header) &&
          G_LIKELY (ptr[-1] == 'c' || ptr[-1] == 'C')) {

        if (G_LIKELY (header[1] != NULL)) {
          ptr = header[1];

          GST_LOG_OBJECT (self, "content-range value: [%s]", ptr);
          if (g_str_has_prefix (ptr, GST_WEB_FETCH_SRC_BYTES_CR)) {
            ptr += sizeof (GST_WEB_FETCH_SRC_BYTES_CR);
            ptr = strchr (ptr, '/');
            if (ptr && ptr[0]) {
              ret = atoll (&ptr[1]);
              break;
            }
          }
        }
      }
    }

    emscripten_fetch_free_unpacked_response_headers (headers);
    g_free (h);
  } else {
    g_warn_if_reached ();
  }

  return ret;
}

/* Called with the GST_OBJECT_LOCK taken */
static GstBuffer *
gst_web_fetch_src_fetch_range (
    GstWebFetchSrc *self, gchar *uri, gsize range_start, gsize range_end)
{
  gchar range_field[256];
  gchar *headers[] = { "Range", range_field, NULL };
  emscripten_fetch_attr_t attr;
  emscripten_fetch_t *fetch;
  GstBuffer *ret = NULL;
  gboolean request_again = FALSE;
  enum
  {
    GST_WEB_FETCH_SRC_HTTP_OK = 200,
    GST_WEB_FETCH_SRC_HTTP_PARTIAL_CONTENT = 206,
    GST_WEB_FETCH_SRC_HTTP_RANGE_NOT_SATISFIABLE = 416,
  };

  g_return_val_if_fail (range_start < range_end, NULL);
  g_return_val_if_fail (uri != NULL, NULL);

  if (self->resource_size != 0 && range_end > self->resource_size) {
    range_end = self->resource_size;
  }

  if (self->download_end != 0 && range_end > self->download_end) {
    range_end = self->download_end;
  }

  snprintf (range_field, sizeof (range_field),
      "bytes=%" G_GSIZE_FORMAT "-%" G_GSIZE_FORMAT, range_start,
      range_end - 1);

  GST_DEBUG_OBJECT (self, "Requesting range '%s'", range_field);

  GST_OBJECT_UNLOCK (self);
  do {
    emscripten_fetch_attr_init (&attr);
    strcpy (attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY |
                      EMSCRIPTEN_FETCH_SYNCHRONOUS | EMSCRIPTEN_FETCH_REPLACE;
    attr.requestHeaders = (const char *const *) headers;
    fetch = emscripten_fetch (&attr, uri);

    switch (fetch->status) {
      case GST_WEB_FETCH_SRC_HTTP_OK:
      case GST_WEB_FETCH_SRC_HTTP_PARTIAL_CONTENT:
        GST_DEBUG_OBJECT (
            self, "Downloaded %" G_GINT64_FORMAT " bytes", fetch->numBytes);

        if (G_UNLIKELY (fetch->numBytes == 0)) {
          GST_ERROR_OBJECT (self, "Fetched zero size");
          break;
        }

        ret = gst_buffer_new_wrapped (
            g_memdup2 (fetch->data, fetch->numBytes), fetch->numBytes);
        request_again = FALSE;

        GST_OBJECT_LOCK (self);
        if (self->resource_size == 0) {
          self->resource_size =
              gst_web_fetch_src_parse_content_size_field (self, fetch);
          GST_INFO_OBJECT (
              self, "Resource size: %" G_GSIZE_FORMAT, self->resource_size);
        }

        if (range_end == self->resource_size
            /* This is for case when self->resource_size is not reported
             * by response headers */
            || fetch->numBytes != range_end - range_start) {
          self->in_eos = TRUE;
        }
        GST_OBJECT_UNLOCK (self);
        break;
      case GST_WEB_FETCH_SRC_HTTP_RANGE_NOT_SATISFIABLE:
        if (request_again) {
          /* This is a weird case, but since it comes from the server, we must
           * handle it. Just decide it's an EOS. */
          request_again = FALSE;
          GST_DEBUG_OBJECT (self, "Range not accepted again");
          break;
        }

        snprintf (range_field, sizeof (range_field),
            "bytes=%" G_GSIZE_FORMAT "-", range_start);

        GST_DEBUG_OBJECT (self,
            "Range not accepted, fetching the rest of the resource (%s)",
            range_field);
        g_clear_pointer (&fetch, emscripten_fetch_close);
        request_again = TRUE;
        GST_OBJECT_LOCK (self);
        self->in_eos = TRUE;
        GST_OBJECT_UNLOCK (self);
        break;
      default:
        GST_ERROR_OBJECT (
            self, "Got error response from the server: %d", fetch->status);
        request_again = FALSE;
        break;
    }
  } while (request_again);

  emscripten_fetch_close (fetch);

  GST_OBJECT_LOCK (self);
  if (self->in_eos)
    GST_DEBUG_OBJECT (self, "EOS");
  GST_OBJECT_UNLOCK (self);

  return ret;
}

static GstFlowReturn
gst_web_fetch_src_create (GstPushSrc *psrc, GstBuffer **outbuf)
{
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (psrc);
  GstFlowReturn ret = GST_FLOW_OK;
  gchar *uri;

  GST_OBJECT_LOCK (self);
  if (G_UNLIKELY (self->in_eos)) {
    ret = GST_FLOW_EOS;
    goto done;
  }

  /* We don't want the download to happen while blocking the streaming thread
   */
  GST_PAD_STREAM_UNLOCK (GST_BASE_SRC (self)->srcpad);
  /* Simple workaround to avoid replacing uri and not having it locked */
  uri = g_strdup (self->uri);
  *outbuf = gst_web_fetch_src_fetch_range (
      self, uri, self->download_offset, self->download_offset + CHUNK_SIZE);
  g_free (uri);

  GST_PAD_STREAM_LOCK (GST_BASE_SRC (self)->srcpad);
  if (G_UNLIKELY (*outbuf == NULL)) {
    ret = self->in_eos ? GST_FLOW_EOS : GST_FLOW_ERROR;
  }

  self->download_offset += gst_buffer_get_size (*outbuf);

done:
  GST_OBJECT_UNLOCK (self);
  return ret;
}

static GstStateChangeReturn
gst_web_fetch_src_change_state (GstElement *element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (element);

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

static gboolean
gst_web_fetch_src_get_size (GstBaseSrc *bsrc, guint64 *size)
{
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (bsrc);
  gboolean ret = FALSE;

  GST_OBJECT_LOCK (self);
  if (self->resource_size == 0)
    goto done;

  *size = self->resource_size;

  ret = TRUE;
done:
  GST_OBJECT_UNLOCK (self);

  return ret;
}

static gboolean
gst_web_fetch_src_is_seekable (GstBaseSrc *bsrc)
{
  return TRUE;
}

static gboolean
gst_web_fetch_src_do_seek (GstBaseSrc *bsrc, GstSegment *segment)
{
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (bsrc);

  g_return_val_if_fail (gst_web_fetch_src_is_seekable (bsrc), FALSE);
  g_return_val_if_fail (GST_CLOCK_TIME_IS_VALID (segment->start), FALSE);

  if (segment->format != GST_FORMAT_BYTES) {
    GST_ERROR_OBJECT (self, "Only bytes format is supported for seeking");
    return FALSE;
  }

  GST_OBJECT_LOCK (self);
  self->download_offset = segment->start;
  self->download_end =
      GST_CLOCK_TIME_IS_VALID (segment->stop) ? segment->stop : 0;
  GST_OBJECT_UNLOCK (self);

  return TRUE;
}

static void
gst_web_fetch_src_set_property (
    GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (object);

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
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (object);

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
  GstWebFetchSrc *self = GST_WEB_FETCH_SRC (obj);

  g_free (self->uri);

  G_OBJECT_CLASS (gst_web_fetch_src_parent_class)->finalize (obj);
}

static void
gst_web_fetch_src_class_init (GstWebFetchSrcClass *klass)
{
  static GstStaticPadTemplate srcpadtemplate = GST_STATIC_PAD_TEMPLATE (
      "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBaseSrcClass *basesrc_class = (GstBaseSrcClass *) klass;
  GstPushSrcClass *pushsrc_class = (GstPushSrcClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_web_fetch_src_debug, "webfetchsrc", 0,
      "HTTP Client Source using Web Fetch API");

  // parent_class = g_type_class_peek_parent (klass);

  element_class->change_state = gst_web_fetch_src_change_state;

  pushsrc_class->create = gst_web_fetch_src_create;

  basesrc_class->is_seekable = gst_web_fetch_src_is_seekable;
  basesrc_class->get_size = gst_web_fetch_src_get_size;
  basesrc_class->do_seek = gst_web_fetch_src_do_seek;

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
