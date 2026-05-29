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

description_title = executable_name.replace('-example', '')
description_content = ''
stream_url = ''

custom_target(
  output: executable_name + '.html',
  input: [
    '../template.html',
    html_code,
    c_code
  ],
  command: [
    html_generator,
    '@INPUT0@',
    '@OUTPUT@',
    'PAGE_NAME', html_code,
    'DESCRIPTION_TITLE', description_title,
    'DESCRIPTION_CONTENT', description_content,
    'STREAM_URL', stream_url,
    'PAGE_CODE', '@INPUT1@',
    'CODE', '@INPUT2@',
    'EXECUTABLE_NAME', executable_name + '.js',
  ],
  install: true,
  install_dir: install_dir,
)
