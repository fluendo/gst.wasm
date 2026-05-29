c_code = executable_name + '.c'
html_code = executable_name + '-page.html'

executable(executable_name,
    c_code,
    dependencies: [
      common_deps,
      gstisomp4_dep,
      dependency('gstreamer-emscripten-1.0'),
      dependency('gstopengl'),
      dependency('gstweb'),
      dependency('gstopenal'),
      dependency('gstaudioconvert'),
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

description_title = 'codecs'

custom_target(
  executable_name + '.html',
  input: [
    '../template.html',
    html_code,
    c_code,
  ],
  output: executable_name + '.html',
  command: [
    html_generator,
    '@INPUT0@',
    '@OUTPUT@',
    'PAGE_NAME', description_title,
    'DESCRIPTION_TITLE', description_title,
    'PAGE_CODE', '@INPUT1@',
    'CODE', '@INPUT2@',
    'EXECUTABLE_NAME', executable_name + '.js',
  ],
  install: true,
  install_dir: install_dir,
)
