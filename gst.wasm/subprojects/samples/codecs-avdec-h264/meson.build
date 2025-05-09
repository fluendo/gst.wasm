fs = import('fs')

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
      # This is giving problems when running the WebRunner at set_format
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

# This should be changed to something that works at compile time
html_data = configuration_data()
html_data.set('PAGE_NAME', html_code)
html_data.set('PAGE_CODE', fs.read(html_code).replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;').replace('\'', '&#39;').strip())
html_data.set('EXECUTABLE_NAME', executable_name + '.js')
html_data.set('CODE', fs.read(c_code).replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;').replace('\'', '&#39;').strip())

configure_file(
  input: '../template.html',
  output: executable_name + '.html',
  configuration: html_data,
  install: true,
  install_dir: install_dir
)
