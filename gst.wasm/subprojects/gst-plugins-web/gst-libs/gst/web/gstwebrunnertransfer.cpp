/*
 * GStreamer - GStreamer WebRunner Transfer
 *
 * Copyright 2025 Fluendo S.A.
 *  @author: Jorge Zapata <jzapata@fluendo.com>
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

#include "gstwebrunnertransfer.h"

/* clang-format off */
EM_JS (void, gst_webrunner_transfer_recv_message, (val e), {
  let msgData = e.data;
  let cmd = msgData["gst_cmd"];
  let data = msgData["data"]; // move into scope
  if (cmd && cmd == "transferObject") {
    cb_name = data["cb_name"];
    cb_user_data = data["user_data"];
    obj = data["object"];
    sender_tid = data["sender_tid"];
    cb = Module[cb_name];
    cb (obj, cb_user_data);
  }
});

EM_JS (void, gst_webrunner_transfer_main_js_message, (val e), {
  let msgData = e.data;
  let cmd = msgData["gst_cmd"];
  let data = msgData["data"];
  if (cmd && cmd == "transferObject") {
    /* In this moment, the object is transfered to the main thread
     * Send it to the corresponding worker
     * TODO handle transferring to the main thread
     */
    var worker = PThread.pthreads[data["tid"]];
    var object = data["object"];
    worker.postMessage({
      gst_cmd: "transferObject",
      data: {
        cb_name: data["cb_name"],
        object: object,
        user_data: data["user_data"],
        tid: data["tid"],
        sender_tid: data["sender_tid"]
      }
    }, [object]);
  }
});

EM_JS (void, gst_webrunner_transfer_send_message, (val cb_name, val object, gint user_data, gint tid, gint sender_tid), {
  postMessage({
    gst_cmd: "transferObject",
    data: {
      cb_name: cb_name,
      object: object,
      user_data: user_data,
      tid: tid,
      sender_tid: sender_tid
    }
  }, [object]);
});
/* clang-format on */

void
gst_web_runner_transfer_init_recv (GstWebRunner *dst)
{
   static gsize once = 0;
   
   /* This function must be called from the RECEIVER thread */
   g_assert (pthread_self() == gst_web_runner_tid (dst));
   
  /* clang-format off */
  /* We register the function on the worker when a message comes from the main thread */
   if (dst->has_event_listener) {
      EM_ASM ({
	    addEventListener ("message", gst_webrunner_transfer_recv_message);
	 });
      dst->has_event_listener = true;
   }
   
   if (g_once_init_enter (&once)) {
      /* And another event listener when the mssages comes from the worker */
      /* TODO later instead of fetching the pthread id, use the actual GThread
       * pointer and keep a list similar to what Emscripten does for pthreads
       */
      MAIN_THREAD_EM_ASM ({
	    var worker = PThread.pthreads[$0];
	    worker.addEventListener ("message", gst_webrunner_transfer_main_js_message);
	 }, pthread_self());
      g_once_init_leave (&once, 1);
   }
   /* clang-format on */
}

static void
gst_web_transferable_transfer_finally (const gchar *cb_name, guintptr object, guintptr user_data, gint recv_tid)
{
  // This one is called:
  // From the thread of the SENDER (for sure)
  // object already represents a real object
   
  /* clang-format off */
  EM_ASM ({
    gst_webrunner_transfer_send_message (Emval.toValue ($0),
        Emval.toValue ($1), Emval.toValue ($2), $3, $4, $5);
    },
    val::u8string (cb_name).as_handle (), object, user_data, recv_tid,
    // sender tid
    pthread_self ()
  );
  /* clang-format on */
}

typedef struct {
   GstWebRunner *src;
   GstWebRunner *dst;
   val *object;
   GstWebRunnerCB when_ready;
   gpointer user_data;
   gint recv_tid;
} GstWebRunnerTransferContext;

static void
gst_web_runner_transfer_send (gpointer ptr)
{
  // This one is called:
  // From the thread of the SENDER (for sure)
  // object already represents a real object
   GstWebRunnerTransferContext *data = (GstWebRunnerTransferContext *)ptr;
   
   gst_web_transferable_transfer_finally ("gst_web_runner_transfer_recv",
					  // We can access object here, since this
					  // thread is it's owner (for now)
					  data->object->as_handle(),
					  data,
					  data->recv_tid
					  );
}

// Called from any thread
void gst_web_runner_transfer_object_async (
   GstWebRunner *src, GstWebRunner *dst, val *object,
   GstWebRunnerCB when_ready, gpointer user_data)
{
   GstWebRunnerTransferContext *data;

   data = g_new0 (GstWebRunnerTransferContext, 1);
   data->src_runner = src;
   data->dst_runner = dst;
   data->object = object;
   data->when_ready = when_ready;
   data->user_data = user_data;
   data->recv_tid = gst_web_runner_tid (dst);

   // NOTE: here we have a pointer to an object.
   // It probably makes sence to just send directly from
   // the sender to receiver and that's it.

   // NOTE 2: "message" handler should be registered on the receiver
   // beforehand
   
   // Go to the SENDER thread
   gst_web_runner_send_message_async (src->runner,
       gst_web_runner_transfer_send, data,
       g_free);   
}

static void
gst_web_runner_transfer_recv (val object, guintptr user_data)
{
  GstWebRunnerTransferContext *data = (GstWebRunnerTransferContext *)user_data;
  
  // This is called from the RECEIVER thread.
  // FIXME: object_name is redundant.
  // Now we need to call the when_transfered().

  GST_DEBUG_OBJECT (self, "Object of type (%s) have been transferred from thread %d",
		    object.typeOf ().as<std::string> ().c_str (), sender_tid);

  data->when_ready (data->user_data, object);
}

EMSCRIPTEN_BINDINGS (gst_web_transferable)
{
  function ("gst_web_runner_transfer_recv",
	    &gst_web_runner_transfer_recv);

}
