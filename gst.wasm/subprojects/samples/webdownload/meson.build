c_code = executable_name + '.c'
html_code = executable_name + '-page.html'

executable(executable_name,
    'webdownload-example.c',
    dependencies: [
      common_deps,
      gstisomp4_dep,
      gstwebplugin_dep,
      dependency('gstreamer-emscripten-1.0'),
      dependency('gstdebugutilsbad')
    ],
    link_args: common_link_args + [
      '-sASYNCIFY',
      '-sASYNCIFY_STACK_SIZE=1048576',
      # This is giving problems when running the WebRunner at set_format (RDI-2850)
      '-sPROXY_TO_PTHREAD',
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
