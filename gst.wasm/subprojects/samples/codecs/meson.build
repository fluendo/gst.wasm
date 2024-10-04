fs = import('fs')

c_code = executable_name + '.c'
html_code = executable_name + '-page.html'

executable(executable_name,
    'codecs-example.c',
    dependencies: [
      common_deps,
      gstisomp4_dep,
      dependency('gstreamer-emscripten-1.0'),
      dependency('gstopengl')
    ],
    link_with: [
      gstweb_dep,
      gstweb_emhttpsrc_dep
    ],
    link_args: common_link_args + [
      '-lembind',
      '-sASYNCIFY',
      # This is giving problems when running the WebRunner at set_format
      '-sPROXY_TO_PTHREAD',
      # Required by emhttpsrc, this should be brought directly by the dep
      '-sFETCH=1',
      '-g3',
      '-lhtml5',
      '-lGL',
      '-sOFFSCREENCANVAS_SUPPORT', # To manipulate the gl context from a thread
    ],
    c_args: [
      '-g3',
      '-O0',
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
