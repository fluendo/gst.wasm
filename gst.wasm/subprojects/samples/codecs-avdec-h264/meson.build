c_code = executable_name + '.c'
html_code = executable_name + '-page.html'

executable(executable_name,
    c_code,
    dependencies: [
      common_deps,
      gstisomp4_dep,
      gstlibav_dep,
      gstwebplugin_dep,
      dependency('gstreamer-emscripten-1.0'),
      dependency('gstopengl'),
      dependency('gstvideoconvertscale'),
    ],
    link_args: common_link_args + [
      '-sASYNCIFY',
      # This is giving problems when running the WebRunner at set_format (RDI-2850)
      '-sPROXY_TO_PTHREAD',
      '-lGL',
      '-sOFFSCREENCANVAS_SUPPORT'
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
