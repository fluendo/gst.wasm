c_code = executable_name + '.c'
html_code = executable_name + '-page.html'

cc = meson.get_compiler('c')

executable(executable_name,
    c_code,
    dependencies: [
      common_deps,
      dependency('gstaudioconvert'),
      dependency('gstlcevcdecoder'),
      dependency('gstopenal'),
      dependency('gstopengl'),
      dependency('gstreamer-emscripten-1.0'),
      dependency('gstvideoparsersbad'),
      gstisomp4_dep,
      gstlibav_dep,
      gstwebplugin_dep,
    ],
    link_args: common_link_args + [
      '-sEXPORTED_FUNCTIONS=_stop_pipeline,_main',
      '-lGL',
      '-sASYNCIFY',
      '-sASYNCIFY_STACK_SIZE=1048576',
      '-sOFFSCREENCANVAS_SUPPORT',
      '-sPROXY_TO_PTHREAD',
      '-sSTACK_SIZE=1048576'
    ],
    name_suffix: 'js',
    install: true,
    install_dir: install_dir
)

install_data(html_code, install_dir: install_dir)

custom_target('js',
  input: html_code,
  output: html_code,
  command: ['cp', '@INPUT@', '@OUTPUT@'],
  install: true,
  install_dir: install_dir)
