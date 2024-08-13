
# Samples

The following is a list of provided samples:

1. **emhttpsrc-example**: Downloads the data from a text file `hello.txt` shared by the http server: `http://localhost:3000/hello.txt`,
and prints the contents on the text output of the webpage.
How to run this sample:
```
pushd _builddir/subprojects/gst-plugins-web/samples/emhttpsrc
cp /var/log/kern.log hello.txt
cp ../../../../../subprojects/gst-plugins-web/samples/emhttpsrc/bs-config.js .
lite-server
```

2. **webcanvasrsc-animation-example**: This example shows the webcanvasrsc element getting data from another canvas
which draws an animation.

3. **webcanvasrsc-webcam-example**: This example shows the webcanvasrsc element getting data from the webcam video frames drawn on an offscreen canvas.
