#!/usr/bin/env bash
#
# FLUENDO S.A.
# Copyright (C) <2024>  <support@fluendo.com>
#
# Script to prepare gstreamer emscripten wasm environment
# This script is a copy of a CI workflow, please, keep them in-sync.

set -e
git clone https://github.com/fluendo/gst.wasm -b fix-gstreamer
(
    cd gst.wasm
    git clone https://github.com/fluendo/cerbero cerbero-src -b gst.wasm --depth=1
    echo "All repos are cloned. Let's build:"
    ./cerbero-src/cerbero-uninstalled -c cerbero/gst.wasm.cbc bootstrap
    ./cerbero-src/cerbero-uninstalled -c cerbero/gst.wasm.cbc build gst-plugins-base-1.0 gst-plugins-good-1.0 gst-plugins-bad-1.0
    echo "...All components are built."
)
echo "Creating the bundle & cleanup:"
tar -cvJf gst.wasm.tar.xz -C gst.wasm/cerbero/build/gst.wasm_linux_web_wasm32/dist/web_wasm32/ -c .
rm -fr gst.wasm
echo "Done:"
ls -l gst.wasm.tar.xz
