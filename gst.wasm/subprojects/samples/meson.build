project(
  'gst-wasm-samples',
  'c', 'cpp',
  version : '0.0.0',
  meson_version : '>= 0.56.0'
)

gst_dep = dependency('gstreamer-1.0')
gstcoreelements_dep = dependency('gstcoreelements')
gstapp_dep = dependency('gstapp')
gstaudio_dep = dependency('gstreamer-audio-1.0')
gstaudiotestsrc_dep = dependency('gstaudiotestsrc')
common_deps = [gst_dep, gstapp_dep, gstcoreelements_dep, gstaudio_dep, gstaudiotestsrc_dep]

subdir('audiotestsrc')
subdir('videotestsrc')
subdir('openal')
