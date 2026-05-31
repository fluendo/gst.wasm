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
      '-sEXPORTED_FUNCTIONS=_stop_pipeline,_main',
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
