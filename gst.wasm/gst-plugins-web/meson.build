project(
  'gst-plugins-web',
  'c', 'cpp',
  version : '0.0.0',
  meson_version : '>= 0.56.0'
)

gst_dep = dependency('gstreamer-1.0')
gstcoreelements_dep = dependency('gstcoreelements')
gstapp_dep = dependency('gstapp')
gstaudio_dep = dependency('gstreamer-audio-1.0')
gstvideo_dep = dependency('gstreamer-video-1.0')
gstaudiotestsrc_dep = dependency('gstaudiotestsrc')
common_deps = [gst_dep, gstcoreelements_dep]

cdata = configuration_data()
cdata.set_quoted('VERSION', '1.22.0') # TODO
cdata.set_quoted('PACKAGE', 'gst-plugins-web')
cdata.set_quoted('PACKAGE_VERSION', '1.22.0') # TODO
cdata.set_quoted('PACKAGE_BUGREPORT', 'https://github.com/fluendo/gst.wasm/issues')
cdata.set_quoted('PACKAGE_NAME', 'GStreamer Plugins Web')
cdata.set_quoted('GST_PACKAGE_NAME', 'GStreamer Plugins Web')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://fluendo.com')
cdata.set_quoted('GST_LICENSE', 'UNKNOWN') # TODO

subdir('gst')
subdir('samples')
