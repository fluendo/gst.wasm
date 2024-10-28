
# Samples

The following is a list of provided samples:

1. **webfetchsrc-example**: Downloads the data from a text file `hello.txt` shared by the http server: `http://localhost:3000/hello.txt`,
and prints the contents on the text output of the webpage.
How to run this sample:
```
pushd _builddir/subprojects/samples/webfetchsrc
cp /var/log/kern.log hello.txt
cp ../../../../subprojects/samples/webfetchsrc/bs-config.js .
lite-server
```

2. **webcanvasrsc-animation-example**: This example shows the webcanvasrsc element getting data from another canvas
which draws an animation.

3. **webcanvasrsc-webcam-example**: This example shows the webcanvasrsc element getting data from the webcam video frames drawn on an offscreen canvas.

4. **webstreamsrc-example**: Downloads the data from a file `webstreamsrc-example.js` shared by the http server: `http://localhost:6931/webstreamsrc-example.js`,
and prints the contents on the text output of the webpage.
