project(
  'gst-wasm-samples',
  'c', 'cpp',
  version : '0.0.0',
  meson_version : '>= 0.56.0'
)

# Libraries
gst_dep = dependency('gstreamer-1.0')
gstaudio_dep = dependency('gstreamer-audio-1.0')
gstvideo_dep = dependency('gstreamer-video-1.0')

# Plugins
gstisomp4_dep = dependency('gstisomp4')
gstlibav_dep = dependency('gstlibav')
gstcoreelements_dep = dependency('gstcoreelements')
gstapp_dep = dependency('gstapp')
gstaudiotestsrc_dep = dependency('gstaudiotestsrc')

# Common to all samples
common_deps = [gst_dep, gstapp_dep, gstcoreelements_dep, gstaudio_dep, gstvideo_dep, gstaudiotestsrc_dep]

# Plugins relative to gst-plugins-web subproject
gstpluginsweb = subproject('gst-plugins-web')
gstsdl2_dep = gstpluginsweb.get_variable('gstsdl2_dep')
gstwebplugin_dep = gstpluginsweb.get_variable('gstwebplugin_dep')

examples = [
  'audio-decoder',
  'avdec+lcevc+aac',
  'codecs',
  'codecs-avdec-h264',
  'lcevcdec',
  'gl',
  'gstlaunch',
  'openal',
  'videotestsrc',
  'webcanvassrc',
  'webdownload',
  'webfetchsrc',
  'webstreamsrc',
  'webtransportsrc',
  'webtransportsrc-streams',
  'gstinspect',
  'dash',
]

common_link_args = [
  '-sPTHREAD_POOL_SIZE=32',
  '-sINITIAL_MEMORY=536870912',
]

foreach example : examples
  executable_name = '@0@-example'.format(example)
  install_dir = 'share/gst.wasm/samples/@0@'.format(executable_name)
  subdir(example)
endforeach

install_data(sources: 'ci/static/index.html', install_dir: 'share/gst.wasm/samples/')
install_data(sources: 'ci/static/enable-threads.js', install_dir: 'share/gst.wasm/samples/')
