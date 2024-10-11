/*
 * GStreamer - GStreamer Web Streams HTTP source
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

#include <emscripten.h>
#include <gst/base/gstpushsrc.h>
#include <stdio.h>
#include <string.h>

static void gst_web_stream_src_uri_handler_init (
    gpointer g_iface, gpointer iface_data);

#define GST_TYPE_WEB_STREAM_SRC (gst_web_stream_src_get_type ())
#define GST_CAT_DEFAULT gst_web_stream_src_debug
#define parent_class gst_web_stream_src_parent_class

#define GST_WEB_STREAM_SRC_BYTES_CR " bytes "
#define CHUNK_SIZE 1048576

#define PROP_LOCATION_DEFAULT NULL

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_MAX
};

typedef struct _GstWebStreamSrc
{
  GstPushSrc element;

  gchar *uri;
  gboolean in_eos;
  gchar *fetch_error;
  GQueue *q;
  gsize queue_max_size;
  gsize accumulated_data_size;
  GCond qcond;
  GThread *fetch_thread;
} GstWebStreamSrc;

G_DECLARE_FINAL_TYPE (
    GstWebStreamSrc, gst_web_stream_src, GST, WEB_STREAM_SRC, GstPushSrc)
G_DEFINE_TYPE_WITH_CODE (GstWebStreamSrc, gst_web_stream_src,
    GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (
        GST_TYPE_URI_HANDLER, gst_web_stream_src_uri_handler_init));
GST_ELEMENT_REGISTER_DEFINE (web_stream_src, "webstreamsrc",
    GST_RANK_SECONDARY, GST_TYPE_WEB_STREAM_SRC);
GST_DEBUG_CATEGORY_STATIC (gst_web_stream_src_debug);

void
gst_web_stream_src_chunk (void *thiz, uint8_t *chunk, size_t length)
{
  GstWebStreamSrc *self = (GstWebStreamSrc *) thiz;

  GST_DEBUG_OBJECT (self, "Received chunk of size: %zu", length);

  GST_OBJECT_LOCK (self);
  while (self->accumulated_data_size + length > self->queue_max_size &&
         GST_STATE_NEXT (self) >= GST_STATE_PAUSED) {
    /* In case of que queue max size is less then the amount accumulated
     * there's no way out: to prevent from this we will have to extend the
     * queue max size. */
    if (length > self->queue_max_size) {
      GST_WARNING_OBJECT (self,
          "Will have to extend the queue max size to %" G_GSIZE_FORMAT,
          length);
      self->queue_max_size = length;
    }

    GST_DEBUG_OBJECT (self,
        "Not enough space in the queue:"
        "(%" G_GSIZE_FORMAT ") --> (%" G_GSIZE_FORMAT "/%" G_GSIZE_FORMAT
        ") bytes."
        " Sleeping..",
        length, self->accumulated_data_size, self->queue_max_size);
    g_cond_wait (&self->qcond, GST_OBJECT_GET_LOCK (self));
  }

  if (GST_STATE_NEXT (self) < GST_STATE_PAUSED) {
    GST_DEBUG_OBJECT (self, "Element is stopping, stop fetching");
    // TODO: need to return something to quit the JS loop
    goto done;
  }

  self->accumulated_data_size += length;
  g_queue_push_tail (
      self->q, gst_buffer_new_wrapped (g_memdup2 (chunk, length), length));

  GST_DEBUG_OBJECT (self,
      "Pushed buffer of size %" G_GSIZE_FORMAT
      " to the queue. Now it's of (%" G_GSIZE_FORMAT "/%" G_GSIZE_FORMAT
      ") bytes",
      length, self->accumulated_data_size, self->queue_max_size);

done:
  g_cond_signal (&self->qcond);
  GST_OBJECT_UNLOCK (self);
}

void
gst_web_stream_src_eos (void *thiz)
{
  GstWebStreamSrc *self = (GstWebStreamSrc *) thiz;

  GST_DEBUG_OBJECT (self, "EOS");
  GST_OBJECT_LOCK (self);
  self->in_eos = TRUE;
  g_cond_signal (&self->qcond);
  GST_OBJECT_UNLOCK (self);
}

void
gst_web_stream_src_error (void *thiz, const char *msg)
{
  GstWebStreamSrc *self = (GstWebStreamSrc *) thiz;

  GST_ERROR_OBJECT (self, "Download failed: %s", msg);
  GST_OBJECT_LOCK (self);

  g_free (self->fetch_error);
  self->fetch_error = g_strdup (msg);
  g_cond_signal (&self->qcond);
  GST_OBJECT_UNLOCK (self);
}

EM_JS(void, gst_web_stream_fetch, (void *thiz, const char* url), {
  const fetchUrl = UTF8ToString (url);

  // Fetch data using the Streams API
    fetch(fetchUrl)
        .then(response => response.body)
        .then(rs => {
    const reader = rs.getReader ();

              return new ReadableStream({
                    async start(controller) {
                      while (true) {
                        const { done, value } = await reader.read();
                        
                        // When no more data needs to be consumed, break the reading
                        if (done) {
      gst_web_stream_src_eos (thiz);
      break;
                        }

                        gst_web_stream_src_chunk(thiz, value, value.length);
              // TODO: quit the loop if the plugin is stopping
                      }
                      reader.releaseLock();
                    }
})
})
        // Create a new response out of the stream
        .then(rs => new Response(rs))
        .then(response => response.blob())
        .catch(fetchError => {
  gst_web_stream_src_error (thiz, fetchError);
        });
});

static guint
gst_web_stream_src_urihandler_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_web_stream_src_urihandler_get_protocols (GType type)
{
  static const gchar *protocols[] = { "http", "https", NULL };

  return protocols;
}

static gboolean
gst_web_stream_src_urihandler_set_uri (
    GstURIHandler *handler, const gchar *uri, GError **error)
{
  GstWebStreamSrc *self = GST_WEB_STREAM_SRC (handler);

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
gst_web_stream_src_urihandler_get_uri (GstURIHandler *handler)
{
  gchar *ret;
  GstWebStreamSrc *self;

  g_return_val_if_fail (GST_IS_URI_HANDLER (handler), NULL);
  self = GST_WEB_STREAM_SRC (handler);

  GST_OBJECT_LOCK (self);
  ret = g_strdup (self->uri);
  GST_OBJECT_UNLOCK (self);

  return ret;
}

static void
gst_web_stream_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *uri_iface = (GstURIHandlerInterface *) g_iface;

  uri_iface->get_type = gst_web_stream_src_urihandler_get_type;
  uri_iface->get_protocols = gst_web_stream_src_urihandler_get_protocols;
  uri_iface->get_uri = gst_web_stream_src_urihandler_get_uri;
  uri_iface->set_uri = gst_web_stream_src_urihandler_set_uri;
}

static void
gst_web_stream_src_init (GstWebStreamSrc *src)
{
  g_cond_init (&src->qcond);
  src->q = g_queue_new ();
}

static gpointer
gst_web_stream_fetch_thread (gpointer data)
{
  GstWebStreamSrc *self = GST_WEB_STREAM_SRC (data);

  gst_web_stream_fetch (self, self->uri);
  return NULL;
}

static GstFlowReturn
gst_web_stream_src_create (GstPushSrc *psrc, GstBuffer **outbuf)
{
  GstWebStreamSrc *self = GST_WEB_STREAM_SRC (psrc);

  if (G_UNLIKELY (self->in_eos))
    return GST_FLOW_EOS;

  GST_OBJECT_LOCK (self);

  if (!self->fetch_thread) {
    gchar *thr_name =
        g_strdup_printf ("%s_fetch_thread", GST_OBJECT_NAME (self));
    self->fetch_thread =
        g_thread_new (thr_name, gst_web_stream_fetch_thread, self);
  }

  while (g_queue_get_length (self->q) == 0) {
    GST_DEBUG_OBJECT (self, "Queue is empty, wait for a buffer");
    g_cond_wait (&self->qcond, GST_OBJECT_GET_LOCK (self));
  }

  if (self->fetch_error) {
    gchar *err = self->fetch_error;
    self->fetch_error = NULL;
    GST_OBJECT_UNLOCK (self);
    GST_ELEMENT_ERROR (self, RESOURCE, FAILED,
        ("Fetch failed: %s", err), (NULL));
    g_free (err);
    return GST_FLOW_ERROR;
  }

  *outbuf = (GstBuffer *) g_queue_pop_head (self->q);
  if (G_UNLIKELY (*outbuf == NULL)) {
    GST_OBJECT_UNLOCK (self);
    return self->in_eos ? GST_FLOW_EOS : GST_FLOW_ERROR;
  }

  self->accumulated_data_size -= gst_buffer_get_size (*outbuf);

  /* self->download_offset += gst_buffer_get_size (*outbuf); */
  GST_OBJECT_UNLOCK (self);

  return GST_FLOW_OK;
}

static GstStateChangeReturn
gst_web_stream_src_change_state (
    GstElement *element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstWebStreamSrc *self = GST_WEB_STREAM_SRC (element);

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
      g_cond_signal (&self->qcond);
      g_thread_join (self->fetch_thread);
      self->in_eos = FALSE;
      g_queue_clear_full (self->q, (GDestroyNotify) gst_buffer_unref);
      GST_OBJECT_UNLOCK (self);
      break;
    default:
      break;
  }

  return ret;
}

static gboolean
gst_web_stream_src_is_seekable (GstBaseSrc *bsrc)
{
  return FALSE;
}

static void
gst_web_stream_src_set_property (
    GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  GstWebStreamSrc *self = GST_WEB_STREAM_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      gst_web_stream_src_urihandler_set_uri (
          GST_URI_HANDLER (self), g_value_get_string (value), NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_web_stream_src_get_property (
    GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  GstWebStreamSrc *self = GST_WEB_STREAM_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      g_value_take_string (value,
          gst_web_stream_src_urihandler_get_uri (GST_URI_HANDLER (self)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_web_stream_src_finalize (GObject *obj)
{
  GstWebStreamSrc *self = GST_WEB_STREAM_SRC (obj);

  g_free (self->uri);
  g_cond_clear (&self->qcond);
  g_queue_free (self->q);

  G_OBJECT_CLASS (gst_web_stream_src_parent_class)->finalize (obj);
}

static void
gst_web_stream_src_class_init (GstWebStreamSrcClass *klass)
{
  static GstStaticPadTemplate srcpadtemplate = GST_STATIC_PAD_TEMPLATE (
      "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBaseSrcClass *basesrc_class = (GstBaseSrcClass *) klass;
  GstPushSrcClass *pushsrc_class = (GstPushSrcClass *) klass;

  GST_DEBUG_CATEGORY_INIT (gst_web_stream_src_debug, "webstreamsrc", 0,
      "HTTP Client Source using Web Streams API");

  element_class->change_state = gst_web_stream_src_change_state;

  pushsrc_class->create = gst_web_stream_src_create;

  basesrc_class->is_seekable = gst_web_stream_src_is_seekable;

  gst_element_class_add_pad_template (
      element_class, gst_static_pad_template_get (&srcpadtemplate));

  gobject_class->set_property = gst_web_stream_src_set_property;
  gobject_class->get_property = gst_web_stream_src_get_property;
  gobject_class->finalize = gst_web_stream_src_finalize;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "Location", "URI of resource to read",
          PROP_LOCATION_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (element_class,
      "HTTP Client Source using Web Streams API", "Source/Network",
      "Receiver data as a client over a network via HTTP using Web Streams "
      "API",
      "Alexander Slobodeniuk <aslobodeniuk@fluendo.com>");
}
