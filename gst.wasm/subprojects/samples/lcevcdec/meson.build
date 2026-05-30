c_code = executable_name + '.c'
html_code = executable_name + '-page.html'

cc = meson.get_compiler('c')

executable(executable_name,
    c_code,
    dependencies: [
      common_deps,
      gstisomp4_dep,
      gstlibav_dep,
      gstwebplugin_dep,
      dependency('gstreamer-emscripten-1.0'),
      dependency('gstopengl'),
      dependency('gstvideoparsersbad'),
      dependency('gstlcevcdecoder'),
    ],
    link_args: common_link_args + [
      '-sASYNCIFY',
      '-sPROXY_TO_PTHREAD',
      '-lGL',
      '-sOFFSCREENCANVAS_SUPPORT',
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
