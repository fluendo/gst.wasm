fs = import('fs')

c_code = executable_name + '.c'
html_code = executable_name + '-page.html'

cc = meson.get_compiler('c')
lcevc_dec_api = cc.find_library('lcevc_dec_api')
lcevc_dec_core = cc.find_library('lcevc_dec_core')
lcevc_dec_core_sequencing = cc.find_library('lcevc_dec_core_sequencing')
lcevc_dec_api_utility = cc.find_library('lcevc_dec_api_utility')

executable(executable_name,
    c_code,
    dependencies: [
      common_deps,
      dependency('gstaudioconvert'),
      dependency('gstlcevcdecoder'),
      dependency('gstopenal'),
      dependency('gstopengl'),
      dependency('gstreamer-emscripten-1.0'),
      dependency('gstvideoconvertscale'),
      dependency('gstvideoparsersbad'),
      gstisomp4_dep,
      gstlibav_dep,
      gstwebplugin_dep,
      lcevc_dec_api,
      lcevc_dec_api_utility,
      lcevc_dec_core,
      lcevc_dec_core_sequencing,
    ],
    link_args: common_link_args + [
      '-lGL',
      '-sASYNCIFY',
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
