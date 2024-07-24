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
gstvideo_dep = dependency('gstreamer-video-1.0')
gstaudiotestsrc_dep = dependency('gstaudiotestsrc')
common_deps = [gst_dep, gstapp_dep, gstcoreelements_dep, gstaudio_dep, gstvideo_dep, gstaudiotestsrc_dep]


examples = [
  'audiotestsrc',
  'gstlaunch',
  'openal',
  'videotestsrc',
]

foreach example : examples
  executable_name = '@0@-example'.format(example)
  install_dir = 'share/gst.wasm/samples/@0@'.format(executable_name)
  subdir(example)
endforeach