
# Samples

The following is a list of provided samples:

1. **emhttpsrc**: downloads the data from a text file `hello.txt` shared by the http server: `http://localhost:3000/hello.txt`,
and prints the contents on the text output of the webpage.
How to run this sample:
```
pushd _builddir/subprojects/gst-plugins-web/samples/emhttpsrc
cp /var/log/kern.log hello.txt
cp ../../../../../subprojects/gst-plugins-web/samples/emhttpsrc/bs-config.js .
lite-server
```
