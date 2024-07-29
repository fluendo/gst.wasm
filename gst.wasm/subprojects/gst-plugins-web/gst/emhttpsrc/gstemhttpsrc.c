/*
 * GStreamer - GStreamer Emscripten HTTP source
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

#define GST_TYPE_EMHTTPSRC \
  (gst_em_http_src_get_type())
#define GST_EMHTTPSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_EMHTTPSRC,GstEmHttpSrc))

typedef struct _GstEmHttpSrcClass
{
  GstPushSrcClass parent_class;
} GstEmHttpSrcClass;

typedef struct _GstEmHttpSrc
{
  GstPushSrc element;

  gchar *uri;
  gboolean in_eos;
  gsize download_offset;
  gsize download_end;
  gsize resource_size;
} GstEmHttpSrc;

GType gst_em_http_src_get_type (void);

GST_DEBUG_CATEGORY_STATIC (gst_em_http_src_debug);
#define GST_CAT_DEFAULT gst_em_http_src_debug

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_MAX
};

#define PROP_LOCATION_DEFAULT NULL

static guint
gst_em_http_src_urihandler_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_em_http_src_urihandler_get_protocols (GType type)
{
  static const gchar *protocols[] = { "http", "https", NULL };

  return protocols;
}

static gboolean
gst_em_http_src_urihandler_set_uri (GstURIHandler * handler,
    const gchar * uri, GError ** error)
{
  GstEmHttpSrc *src = GST_EMHTTPSRC (handler);

  g_return_val_if_fail (GST_IS_URI_HANDLER (handler), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  GST_OBJECT_LOCK (src);

  if (src->uri != NULL) {
    GST_DEBUG_OBJECT (src,
        "URI already present as %s, updating to new URI %s", src->uri, uri);
    g_free (src->uri);
  }

  src->uri = g_strdup (uri);
  GST_OBJECT_UNLOCK (src);

  return TRUE;
}

static gchar *
gst_em_http_src_urihandler_get_uri (GstURIHandler * handler)
{
  gchar *ret;
  GstEmHttpSrc *src;

  g_return_val_if_fail (GST_IS_URI_HANDLER (handler), NULL);
  src = GST_EMHTTPSRC (handler);

  GST_OBJECT_LOCK (src);
  ret = g_strdup (src->uri);
  GST_OBJECT_UNLOCK (src);

  return ret;
}

static void
gst_em_http_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *uri_iface = (GstURIHandlerInterface *) g_iface;

  uri_iface->get_type = gst_em_http_src_urihandler_get_type;
  uri_iface->get_protocols = gst_em_http_src_urihandler_get_protocols;
  uri_iface->get_uri = gst_em_http_src_urihandler_get_uri;
  uri_iface->set_uri = gst_em_http_src_urihandler_set_uri;
}

#define gst_em_http_src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstEmHttpSrc, gst_em_http_src, GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_em_http_src_uri_handler_init));
GST_ELEMENT_REGISTER_DEFINE (emhttpsrc, "emhttpsrc",
    GST_RANK_SECONDARY, GST_TYPE_EMHTTPSRC);

static gboolean
gst_em_http_src_is_seekable (GstBaseSrc * bsrc)
{
  return TRUE;
}

#define GST_EMHTTPSRC_BYTES_CR " bytes "

static gsize
gst_em_http_src_parse_content_size_field (GstEmHttpSrc * src,
    emscripten_fetch_t * fetch)
{
  gsize length, ret = 0;

  length = emscripten_fetch_get_response_headers_length (fetch);
  if (length != 0) {
    char *h = g_malloc (length + 1);
    char **headers;
    char **header;

    emscripten_fetch_get_response_headers (fetch, h, length + 1);
    GST_LOG_OBJECT (src, "Received headers: %s", h);

    headers = emscripten_fetch_unpack_response_headers (h);
    for (header = headers; *header != NULL; header++) {
      char *ptr;

      if ((ptr = strstr (*header, "ontent-range"))
          && G_LIKELY (ptr > *header)
          && G_LIKELY (ptr[-1] == 'c' || ptr[-1] == 'C')) {

        if (G_LIKELY (header[1] != NULL)) {
          ptr = header[1];

          GST_LOG_OBJECT (src, "content-range value: [%s]", ptr);
          if (g_str_has_prefix (ptr, GST_EMHTTPSRC_BYTES_CR)) {
            ptr += sizeof (GST_EMHTTPSRC_BYTES_CR);
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

static GstBuffer *
gst_em_http_src_fetch_range (GstEmHttpSrc * src, gsize range_start,
    gsize range_end)
{
  gchar range_field[256];
  gchar *headers[] = { "Range", range_field, NULL };
  emscripten_fetch_attr_t attr;
  emscripten_fetch_t *fetch;
  GstBuffer *ret = NULL;
  gboolean request_again = FALSE;
  enum
  {
    GST_EMHTTPSRC_HTTP_OK = 200,
    GST_EMHTTPSRC_HTTP_PARTIAL_CONTENT = 206,
    GST_EMHTTPSRC_HTTP_RANGE_NOT_SATISFIABLE = 416,
  };

  g_return_val_if_fail (range_start < range_end, NULL);
  g_return_val_if_fail (src->uri != NULL, NULL);

  if (src->resource_size != 0 && range_end > src->resource_size) {
    range_end = src->resource_size;
  }

  if (src->download_end != 0 && range_end > src->download_end) {
    range_end = src->download_end;
  }

  snprintf (range_field, sizeof (range_field),
      "bytes=%" G_GSIZE_FORMAT "-%" G_GSIZE_FORMAT, range_start, range_end - 1);

  GST_DEBUG_OBJECT (src, "Requesting range '%s'", range_field);

  do {
    emscripten_fetch_attr_init (&attr);
    strcpy (attr.requestMethod, "GET");
    attr.attributes =
        EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS |
        EMSCRIPTEN_FETCH_REPLACE;
    attr.requestHeaders = (const char *const *) headers;
    fetch = emscripten_fetch (&attr, src->uri);

    switch (fetch->status) {
      case GST_EMHTTPSRC_HTTP_OK:
      case GST_EMHTTPSRC_HTTP_PARTIAL_CONTENT:
        GST_DEBUG_OBJECT (src, "Downloaded %" G_GINT64_FORMAT " bytes",
            fetch->numBytes);

        if (G_UNLIKELY (fetch->numBytes == 0)) {
          GST_ERROR_OBJECT (src, "Fetched zero size");
          break;
        }

        ret =
            gst_buffer_new_wrapped (g_memdup2 (fetch->data, fetch->numBytes),
            fetch->numBytes);
        request_again = FALSE;

        if (src->resource_size == 0) {
          src->resource_size =
              gst_em_http_src_parse_content_size_field (src, fetch);
          GST_INFO_OBJECT (src, "Resource size: %" G_GSIZE_FORMAT,
              src->resource_size);
        }

        if (range_end == src->resource_size
            /* This is for case when src->resource_size is not reported
             * by response headers */
            || fetch->numBytes != range_end - range_start) {
          src->in_eos = TRUE;
        }
        break;
      case GST_EMHTTPSRC_HTTP_RANGE_NOT_SATISFIABLE:
        if (request_again) {
          /* This is a weird case, but since it comes from the server, we must handle it.
           * Just decide it's an EOS. */
          request_again = FALSE;
          GST_DEBUG_OBJECT (src, "Range not accepted again");
          break;
        }

        snprintf (range_field, sizeof (range_field),
            "bytes=%" G_GSIZE_FORMAT "-", range_start);

        GST_DEBUG_OBJECT (src,
            "Range not accepted, fetching the rest of the resource (%s)",
            range_field);
        g_clear_pointer (&fetch, emscripten_fetch_close);
        request_again = TRUE;
        src->in_eos = TRUE;
        break;
      default:
        GST_ERROR_OBJECT (src, "Got error response from the server: %d",
            fetch->status);
        request_again = FALSE;
        break;
    }
  } while (request_again);

  emscripten_fetch_close (fetch);
  if (src->in_eos)
    GST_DEBUG_OBJECT (src, "EOS");

  return ret;
}

static GstFlowReturn
gst_em_http_src_create (GstPushSrc * psrc, GstBuffer ** outbuf)
{
  GstEmHttpSrc *src = GST_EMHTTPSRC (psrc);

  if (G_UNLIKELY (src->in_eos))
    return GST_FLOW_EOS;

  GST_OBJECT_LOCK (src);
  *outbuf =
      gst_em_http_src_fetch_range (src, src->download_offset,
      src->download_offset + 2048);

  if (G_UNLIKELY (*outbuf == NULL)) {
    GST_OBJECT_UNLOCK (src);
    return src->in_eos ? GST_FLOW_EOS : GST_FLOW_ERROR;
  }

  src->download_offset += gst_buffer_get_size (*outbuf);
  GST_OBJECT_UNLOCK (src);

  return GST_FLOW_OK;
}

static GstStateChangeReturn
gst_em_http_src_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstEmHttpSrc *src = GST_EMHTTPSRC (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (src->uri == NULL) {
        GST_ELEMENT_ERROR (element, RESOURCE, OPEN_READ, ("No URL set."),
            ("Missing URL"));
        return GST_STATE_CHANGE_FAILURE;
      }
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_OBJECT_LOCK (src);
      src->download_offset = 0;
      src->download_end = 0;
      src->resource_size = 0;
      src->in_eos = FALSE;
      GST_OBJECT_UNLOCK (src);
      break;
    default:
      break;
  }

  return ret;
}

static gboolean
gst_em_http_src_get_size (GstBaseSrc * bsrc, guint64 * size)
{
  GstEmHttpSrc *src = GST_EMHTTPSRC (bsrc);
  gboolean ret = FALSE;

  GST_OBJECT_LOCK (src);
  if (src->resource_size == 0)
    goto done;

  *size = src->resource_size;

  ret = TRUE;
done:
  GST_OBJECT_UNLOCK (src);

  return ret;
}

static gboolean
gst_em_http_src_do_seek (GstBaseSrc * bsrc, GstSegment * segment)
{
  GstEmHttpSrc *src = GST_EMHTTPSRC (bsrc);

  g_return_val_if_fail (gst_em_http_src_is_seekable (bsrc), FALSE);
  g_return_val_if_fail (GST_CLOCK_TIME_IS_VALID (segment->start), FALSE);

  if (segment->format != GST_FORMAT_BYTES) {
    GST_ERROR_OBJECT (src, "Only bytes format is supported for seeking");
    return FALSE;
  }

  GST_OBJECT_LOCK (src);
  src->download_offset = segment->start;
  src->download_end = GST_CLOCK_TIME_IS_VALID (segment->stop) ?
      segment->stop : 0;
  GST_OBJECT_UNLOCK (src);

  return TRUE;
}

static void
gst_em_http_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstEmHttpSrc *src = GST_EMHTTPSRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      gst_em_http_src_urihandler_set_uri (GST_URI_HANDLER (src),
          g_value_get_string (value), NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_em_http_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstEmHttpSrc *src = GST_EMHTTPSRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      g_value_take_string (value,
          gst_em_http_src_urihandler_get_uri (GST_URI_HANDLER (src)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_em_http_src_finalize (GObject * obj)
{
  GstEmHttpSrc *src = GST_EMHTTPSRC (obj);

  g_free (src->uri);

  G_OBJECT_CLASS (gst_em_http_src_parent_class)->finalize (obj);
}

static void
gst_em_http_src_init (GstEmHttpSrc * src)
{
}

static void
gst_em_http_src_class_init (GstEmHttpSrcClass * klass)
{
  static GstStaticPadTemplate srcpadtemplate = GST_STATIC_PAD_TEMPLATE ("src",
      GST_PAD_SRC,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS_ANY);

  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBaseSrcClass *basesrc_class = (GstBaseSrcClass *) klass;
  GstPushSrcClass *pushsrc_class = (GstPushSrcClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_em_http_src_debug, "emhttpsrc",
      0, "HTTP Client Source using Emscripten wget");

  element_class->change_state = gst_em_http_src_change_state;

  pushsrc_class->create = gst_em_http_src_create;

  basesrc_class->is_seekable = gst_em_http_src_is_seekable;
  basesrc_class->get_size = gst_em_http_src_get_size;
  basesrc_class->do_seek = gst_em_http_src_do_seek;

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&srcpadtemplate));

  gobject_class->set_property = gst_em_http_src_set_property;
  gobject_class->get_property = gst_em_http_src_get_property;
  gobject_class->finalize = gst_em_http_src_finalize;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "Location", "URI of resource to read",
          PROP_LOCATION_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (element_class,
      "HTTP Client Source using Emscripten wget",
      "Source/Network",
      "Receiver data as a client over a network via HTTP using Emscripten",
      "Alexander Slobodeniuk <aslobodeniuk@fluendo.com>");
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return GST_ELEMENT_REGISTER (emhttpsrc, plugin);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    emhttpsrc,
    "emhttpsrc element",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
