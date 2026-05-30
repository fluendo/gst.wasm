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
  'codecs',
  'codecs-avdec-h264',
  'lcevc+aac',
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

sample_code_output_dir = join_paths(
  meson.current_source_dir(),
  'ci',
  'static',
  'web',
  'public',
  'samples',
  'code',
)
sample_code_files = [
  'codecs/codecs-example.c',
  'codecs-avdec-h264/codecs-avdec-h264-example.c',
  'lcevc+aac/lcevc+aac-example.c',
  'lcevcdec/lcevcdec-example.c',
  'gl/gl-example.c',
  'gstlaunch/gstlaunch-example.c',
  'openal/openal-example.c',
  'videotestsrc/videotestsrc-example.c',
  'webcanvassrc/webcanvassrc-animation-example.c',
  'webcanvassrc/webcanvassrc-webcam-example.c',
  'webdownload/webdownload-example.c',
  'webfetchsrc/webfetchsrc-example.c',
  'webstreamsrc/webstreamsrc-example.c',
  'webtransportsrc/webtransportsrc-example.c',
  'webtransportsrc-streams/webtransportsrc-streams-example.c',
  'gstinspect/gstinspect-example.c',
  'dash/dash-example.c',
]

run_command('mkdir', '-p', sample_code_output_dir, check: true)
foreach sample_code_file : sample_code_files
  run_command(
    'cp',
    join_paths(meson.current_source_dir(), sample_code_file),
    sample_code_output_dir,
    check: true,
  )
endforeach

install_subdir('ci/static/web/dist',
  install_dir: 'share/gst.wasm/samples/',
  strip_directory: true
)
