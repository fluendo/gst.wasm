// src/data/examples.js

export const examplesData = [
  {
    id: "gstlaunch",
    pageName: "gstlaunch Example",
    descriptionTitle: "GStreamer Launch Example",
    descriptionContent:
      "This example demonstrates basic pipeline launching in Wasm.",
    streamUrl: null,
    executableName: "/gstlaunch-example/gstlaunch-example.js",
    code: `
  #include <gst/gst.h>
  int main(int argc, char *argv[]) {
    // gstlaunch code
    return 0;
  }`,
  },
  {
    id: "videotestsrc",
    pageName: "Video Test Source",
    descriptionTitle: "Video Test Source Example",
    descriptionContent: "Generates a test video stream using videotestsrc.",
    streamUrl: "https://example.com/stream",
    executableName: "/videotestsrc-example/videotestsrc-example.js",
    code: `
  #include <gst/gst.h>
  // videotestsrc code
  `,
  },
  // ... add the rest of your examples here
];
