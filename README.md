<p align="center">
  <img src="artwork/gst.wasm.svg" width="100" height="100" align="center"/>
</p>

___

`gst.wasm` is a GStreamer port to WebAssembly. Its main goal is to implement the support of WebAssembly
in GStreamer (and its dependencies) and bring the changes upstream. This repository is a placeholder to
keep up-to-date information required on how to build, develop, and deploy GStreamer in a browser or in
a `node.js` runtime environment until all the information is finally merged into the corresponding
projects.

## The stack
We use the standard libraries and tools used on GStreamer. The gray boxes are the dependencies and the
green ones are new projects that will become part of mainstream GStreamer.

```mermaid
block-beta
  columns 3
  gst["gst-plugins-web"]:1
  space
  js["js-gstreamer-bindings"]:1
  browser["Browser/Node.js"]:3
  Emscripten:3
  GStreamer:3
  GLib Orc Cerbero
  libffi space meson
  style browser fill:#90e2f7,stroke:#4a7580;
  style gst fill:#cee741,stroke:#728024;
  style js fill:#cee741,stroke:#728024;
  style Emscripten fill:#90e2f7,stroke:#4a7580;
  style GStreamer fill:#e6e6e6,stroke:#808080;
  style GLib fill:#e6e6e6,stroke:#808080;
  style Orc fill:#e6e6e6,stroke:#808080;
  style Cerbero fill:#e6e6e6,stroke:#808080;
  style libffi fill:#e6e6e6,stroke:#808080;
  style meson fill:#e6e6e6,stroke:#808080;
```

## Dependencies
GStreamer has plenty of dependencies, especially at the plugin level, but our goal is focused on the
core ones, as shown in the gray boxes of the [above diagram](#the-stack)

### libffi
### GLib
### GStreamer
### Cerbero

## Usage

## Development
