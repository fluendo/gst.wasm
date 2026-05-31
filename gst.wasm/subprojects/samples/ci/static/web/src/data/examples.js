export const examplesData = [
  {
    id: "codecs",
    pageName: "Codecs",
    descriptionTitle: "WebCodecs Audio + Video Playback",
    descriptionContent:
      "Demonstrates combined video and audio playback in gst.wasm using Web APIs. The pipeline fetches an MP4 stream, demuxes H.264 and AAC tracks, decodes them with webcodecsviddech264sw and webcodecsauddecaacsw, then renders video to a canvas and audio to OpenAL output.",
    streamUrl: null,
    executableName: "/codecs-example/codecs-example.js",
    code: "/samples/code/codecs-example.c",
  },
  {
    id: "codecs-avdec-h264",
    pageName: "H264 SW",
    descriptionTitle: "Software H.264 Decode with libav",
    descriptionContent:
      "Shows software H.264 decoding with avdec_h264 in gst.wasm. It downloads an MP4 file through webstreamsrc, demuxes it with qtdemux, decodes frames with libav, converts pixel format, and displays output in a browser canvas sink. Useful to compare CPU decoding against WebCodecs-based paths.",
    streamUrl: null,
    executableName: "/codecs-avdec-h264-example/codecs-avdec-h264-example.js",
    code: "/samples/code/codecs-avdec-h264-example.c",
  },
  {
    id: "lcevc+aac",
    pageName: "LCEVC+AAC",
    descriptionTitle: "LCEVC Video with AAC Audio",
    descriptionContent:
      "Presents an LCEVC plus AAC playback path. The sample demuxes MP4, decodes base H.264 and AAC streams, applies LCEVC enhancement, uses webdownload/webupload where needed, and sends synchronized video and audio to browser sinks. It is useful for evaluating enhanced video with standard audio playback.",
    streamUrl: null,
    executableName: "/lcevc+aac-example/lcevc+aac-example.js",
    code: "/samples/code/lcevc+aac-example.c",
  },
  {
    id: "lcevcdec",
    pageName: "LCEVC",
    descriptionTitle: "LCEVC Enhancement Decode",
    descriptionContent:
      "Highlights LCEVC video enhancement on top of an H.264 base stream. The pipeline fetches MP4 data, demuxes and parses video, decodes with avdec_h264, applies lcevcdec, uploads frames to the web path, and renders in a canvas sink. This example focuses on visual output and decoder integration flow.",
    streamUrl: null,
    executableName: "/lcevcdec-example/lcevcdec-example.js",
    code: "/samples/code/lcevcdec-example.c",
  },
  {
    id: "gl",
    pageName: "GL",
    descriptionTitle: "OpenGL Rendering Smoke Test",
    descriptionContent:
      "A minimal OpenGL rendering demo for gst.wasm. It creates synthetic frames with gltestsrc and displays them directly through glimagesink with sync disabled, making it easy to validate GPU-backed rendering in the browser runtime. Great for quick graphics bring-up and checking GL plugin registration.",
    streamUrl: null,
    executableName: "/gl-example/gl-example.js",
    code: "/samples/code/gl-example.c",
  },
  {
    id: "gstlaunch",
    pageName: "Launch",
    descriptionTitle: "Runtime Pipeline Launcher",
    descriptionContent:
      "Brings gst-launch style experimentation to the browser. The page reads a pipeline description from an input field, builds it at runtime with gst_parse_launch, and starts playback immediately. It is ideal for prototyping and debugging pipeline strings without rebuilding the sample.",
    streamUrl: null,
    executableName: "/gstlaunch-example/gstlaunch-example.js",
    code: "/samples/code/gstlaunch-example.c",
    inputEnabled: true,
    inputId: "pipeline",
    inputPlaceholder: "Enter a GStreamer pipeline",
    inputInitialValue: "videotestsrc ! sdl2sink",
    inputSubmitFunction: "_init_pipeline",
  },
  {
    id: "openal",
    pageName: "OpenAL",
    descriptionTitle: "OpenAL Audio Output Check",
    descriptionContent:
      "Simple audio output example using OpenAL in gst.wasm. It generates a tone with audiotestsrc and plays it through openalsink, providing a quick way to confirm audio plugin registration, pipeline state changes, and browser-side sound output. Useful as a baseline before testing complex media pipelines.",
    streamUrl: null,
    executableName: "/openal-example/openal-example.js",
    code: "/samples/code/openal-example.c",
  },
  {
    id: "videotestsrc",
    pageName: "Test Video",
    descriptionTitle: "Synthetic Video Test Pattern",
    descriptionContent:
      "Basic video playback sample that runs entirely from generated content. videotestsrc creates a moving test pattern, and sdl2sink renders it in the page, making this a fast sanity check for video pipeline setup, rendering path availability, and timing behavior in the wasm environment.",
    streamUrl: null,
    executableName: "/videotestsrc-example/videotestsrc-example.js",
    code: "/samples/code/videotestsrc-example.c",
  },
  {
    id: "webcanvassrc-animation",
    pageName: "Canvas Anim",
    descriptionTitle: "Canvas Animation Ingestion",
    descriptionContent:
      "Captures frames from a JavaScript-driven canvas animation and feeds them into GStreamer. webcanvassrc pulls pixel data from a secondary canvas, videoconvert adapts formats, and sdl2sink displays the result. This demonstrates bridging JS rendering loops with gst.wasm processing and presentation.",
    streamUrl: null,
    executableName: "/webcanvassrc-example/webcanvassrc-animation-example.js",
    code: "/samples/code/webcanvassrc-animation-example.c",
  },
  {
    id: "webcanvassrc-webcam",
    pageName: "Canvas Cam",
    descriptionTitle: "Webcam to Canvas Pipeline",
    descriptionContent:
      "Streams webcam frames into gst.wasm through an OffscreenCanvas. JavaScript acquires camera input, draws frames continuously, and webcanvassrc ingests that canvas into a GStreamer pipeline for conversion and display. This is a reference for combining browser capture APIs with gst.wasm elements.",
    streamUrl: null,
    executableName: "/webcanvassrc-example/webcanvassrc-webcam-example.js",
    code: "/samples/code/webcanvassrc-webcam-example.c",
  },
  {
    id: "webdownload",
    pageName: "Download",
    descriptionTitle: "Web Frame Download Bridge",
    descriptionContent:
      "Demonstrates moving video frames from the decoding path into GStreamer memory flow. The pipeline fetches MP4, demuxes, decodes H.264 with WebCodecs, downloads frames via webdownload, and verifies output with checksumsink. Helps explain interoperability between web-native and core elements.",
    streamUrl: null,
    executableName: "/webdownload-example/webdownload-example.js",
    code: "/samples/code/webdownload-example.c",
  },
  {
    id: "webstreamsrc",
    pageName: "Stream Src",
    descriptionTitle: "HTTP Stream Source Buffering",
    descriptionContent:
      "Shows how to fetch arbitrary web content through webstreamsrc and inspect arriving buffers in a fakesink handoff callback. It is a compact example of HTTP data acquisition, buffer mapping, and custom application-side processing in gst.wasm without requiring full audio or video playback chains.",
    streamUrl: null,
    executableName: "/webstreamsrc-example/webstreamsrc-example.js",
    code: "/samples/code/webstreamsrc-example.c",
  },
  {
    id: "gstinspect",
    pageName: "Inspect",
    descriptionTitle: "Plugin and Element Inspection",
    descriptionContent:
      "Runs a browser-friendly gst-inspect build to list plugins and element details inside gst.wasm. The sample initializes runtime plugins, disables unsupported terminal color behavior, and executes all-elements inspection mode. Useful for checking what is registered before constructing pipelines.",
    streamUrl: null,
    executableName: "/gstinspect-example/gstinspect-example.js",
    code: "/samples/code/gstinspect-example.c",
  },
  {
    id: "dash",
    pageName: "DASH",
    descriptionTitle: "MPEG-DASH Adaptive Playback",
    descriptionContent:
      "Plays MPEG-DASH content in gst.wasm using webstreamsrc and dashdemux, then decodes H.264 video with avdec_h264 and renders to a canvas sink. It includes presentation-delay and output size controls, making it a useful reference for adaptive streaming setup and browser playback with GStreamer components.",
    streamUrl: null,
    executableName: "/dash-example/dash-example.js",
    code: "/samples/code/dash-example.c",
  },
];
