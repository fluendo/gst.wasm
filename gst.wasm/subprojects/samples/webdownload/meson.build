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

description_title = 'webdownload'

custom_target(executable_name + '.html',
  input: [ '../template.html', html_code, c_code ],
  output: executable_name + '.html',
  command: [
    html_generator,
    '@INPUT0@',
    '@OUTPUT@',
    'PAGE_NAME',
    html_code,
    'DESCRIPTION_TITLE',
    description_title,
    'PAGE_CODE',
    '@INPUT1@',
    'CODE',
    '@INPUT2@',
    'EXECUTABLE_NAME',
    executable_name + '.js',
  ],
  install: true,
  install_dir: install_dir,
)
