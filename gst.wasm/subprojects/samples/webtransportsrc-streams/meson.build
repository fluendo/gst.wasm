c_code = executable_name + '.c'
html_code = executable_name + '-page.html'
webtransport_html_code = executable_name + '-ref.html'

executable(executable_name,
    c_code,
    dependencies: [
      common_deps,
      gstwebplugin_dep,
      dependency('gstrtp'),
      dependency('gstopengl'),
      gstlibav_dep,
      dependency('gstvideoconvertscale'),
      dependency('gstvideoparsersbad')
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
install_data(webtransport_html_code, install_dir: install_dir)

custom_target('js',
  input: webtransport_html_code,
  output: webtransport_html_code,
  command: ['cp', '@INPUT@', '@OUTPUT@'],
  install: true,
  install_dir: install_dir)

custom_target('wt',
  input: html_code,
  output: html_code,
  command: ['cp', '@INPUT@', '@OUTPUT@'],
  install: true,
  install_dir: install_dir)

description_title = 'webtransportsrc-streams'

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
