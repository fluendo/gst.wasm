In order to start developing GStreamer elements for Web APIs or to integrate
them as part of your applications there are a few things you should know first.

JS values
=========
Use Emscripten's [val][val] to access the underlying JS system. Note that JS
values can not be sharead among threads because each thread has it's own
JS VM. This is opposite to how threads share C/C++ data as it is done in
`SharedArrayBuffer` Emscripten already setups for you.

If you need to share them you'll need to transfer them to the corresponding
`WebWorker`.


Transferring objects
--------------------
We have a mechanism to transfer JS objects from one WebWorker to another.
Call `gst_web_utils_message_new_request_object` for requesting a JS object
to be trasnferred to the calling thread. As it is a `GstMessage`, it is the
application or the parent's object who are in charge of processing the message
and call `gst_web_utils_element_process_request_object` to do the actual
transfer.

JS Callbacks
============
In JS, objects that can be listened to implement the
[EventTarget][event-target] interface.If you need to listen to an event
(i.e. [mouse events][mouse-events], [keyboard events][keyboard-events],
[element events][element-events] or [other events][other-events] in C code,
you can do so by doing:

```
static void
your_function_in_c (val event)
{

}

EMSCRIPTEN_BINDINGS(name) {
  function("your_function_in_js", &your_function_in_c);
}


object.call<void> ("addEventListener", std::string("eventname"),
      val::module_property("your_function_in_js"));
```


Threading
=========
Emscritpen threading model is well explained on their [documentation][pthreads].
In general, for each `pthread` a new [Web Worker][web-workers] is created.
The way Web Workers communicate with the main application or other threads is
through a message passing system which Emscripten owns and is setup on each
`pthread` creation process.

You can not easily modify such process. Take into account that several
[Web APIs][web-workers-supportes-apis] are usable from a Web Worker but you can
not access the DOM, as it belongs to the main thread.

Web Workers can have access to different objects from different Workers
including the main application thread, and this is done by transferring them.
The Web APIs standard has a definition of what [objects][transferrable-objects]
are transferrable.

GStreamer's threading model is complex but there are two well-known threads,
of course, other threads can exist and are cretaed by elements depending on
their needs. The two known threads are:

* Application thread: The thread that triggers a change of state of elements,
for example when you call `gst_element_set_state`
* Streaming thread: The thread used for sending buffers from one element to
another.

Emscripten has support for proxying the application main thread into a
Web Worker to not block the browser's main thread, which can cause several UI
responsiveness problems if the main thread is blocked.

It is recommended that all GStreamer apps run in a proxied main by using the
flag `-sPROXY_TO_PTHREAD`, unless you know how the threading model of your
application and pipeline is set up.

It is important to not take assumptions on GStreamer threads, a streaming
thread of one element might not be a streaming thread of another. A `queue`
element is a perfect example of such situation. The data received on a queue
`sink` pad is done on one thread and the data pushed from the `src` pad is
another.

Web Runner
----------
A [GstWebRunner][GstWebRunner] is a helper object that allows function calls to
be executed from the same thread. This is needed in case you need to manipulate
JS objects from different threads from the same element.

In case a `GstWebRunner` needs to be shared among elements use a `GstContext`
that wraps the runner that will be shared among the elements.

As an example, for sharing a `canvas` we followed a similar way to what the
OpenGL support does in GStreamer: every OpenGL related function is done in a
single thread and all GL related elements request the same `GstContext`.
This `GstContext` is a [GstWebCanvas].

Web Task
---------
A [GstWebTask][GstWebTask] and its associated default pool
[GstWebTaskPool][GstWebTaskPool] behave the same way as any other `GstTask`
with the difference that this tasks's threads are backed up by a
`GMainLoop`/`GMainContent` and therefore when paused, the control goes back
to Emscripten by doing an Emscripten's main loop iteration. This allows
Emsripten to continue processing the WebWorker message system.

This is specially helpful for tranfering objects from one thread to a streaming
thread.

Canvas
======
[Canvas][canvas] objects deserve a special mention. This is the only
transferrable object Emscripten does transfer if required.


[pthreads]: https://emscripten.org/docs/porting/pthreads.html
[web-workers]: https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API
[web-workers-supported-apis]: https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API#supported_web_apis
[transferrable-objects]: https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Transferable_objects#supported_objects
[val]: https://emscripten.org/docs/api_reference/val.h.html
[canvas]: https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API
[event-target]: https://developer.mozilla.org/en-US/docs/Web/API/EventTarget
[mouse-events]: https://developer.mozilla.org/en-US/docs/Web/API/Element#mouse_events
[keyboard-events]: https://developer.mozilla.org/en-US/docs/Web/API/Element#keyboard_events
[element-events]: https://developer.mozilla.org/en-US/docs/Web/API/HTMLElement#events
[other-events]: https://developer.mozilla.org/en-US/docs/Web/Events
[GstWebRunner]: https://github.com/fluendo/gst.wasm/blob/main/gst.wasm/subprojects/gst-plugins-web/gst-libs/gst/web/gstwebrunner.h
[GstWebCanvas]: https://github.com/fluendo/gst.wasm/blob/main/gst.wasm/subprojects/gst-plugins-web/gst-libs/gst/web/gstwebcanvas.h
[GstWebTask]: https://github.com/fluendo/gst.wasm/blob/main/gst.wasm/subprojects/gst-plugins-web/gst-libs/gst/web/gstwebtask.h
[GstWebTaskPool]: https://github.com/fluendo/gst.wasm/blob/main/gst.wasm/subprojects/gst-plugins-web/gst-libs/gst/web/gstwebtaskpool.h

Other advises
=============
Avoid using [emscripten::convertJSArrayToNumberVector()](https://emscripten.org/docs/api_reference/val.h.html#_CPPv4N10emscripten10emscripten3val28convertJSArrayToNumberVectorERK3val) to access the data of JS array: this function is very slow. We recommend using
```c++
GByteArray gst_web_utils_copy_data_from_js (const emscripten::val &data)
```
or
```c++
GstBuffer *gst_web_utils_js_array_to_buffer (const emscripten::val &data);
```
with
```
#include <gst/web/gstwebutils.h>
```
instead.
