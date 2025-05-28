fs = import('fs')
c_code = executable_name + '.c'
html_code = executable_name + '-page.html'

#FIXME: this is the ugliest thing I've ever seen. This is the consequence of gst-inspect-1.0 being a tool and not a library
#Maybe converting GStreamer to a subproject?
gstinspect_source_dir = '../../../../build/gst.wasm_web_wasm32/sources/local/gstreamer-1.0/subprojects/gstreamer/tools'
gst_basics_source_dir = '../../../../build/gst.wasm_web_wasm32/sources/local/gstreamer-1.0/subprojects/gstreamer'
gstreamer_includes = include_directories(gstinspect_source_dir, gst_basics_source_dir)

executable(executable_name,
    'gstinspect-example.c',
    dependencies: [common_deps, 
    dependency('gstweb'),
    dependency('gstsdl2'),
    ],
    include_directories: [gstreamer_includes],
    link_args: common_link_args + [
      '-lhtml5',
      '-sASYNCIFY',
    ],
    c_args: ['-Wall', '-Werror', '-Wno-deprecated-declarations'],
    name_suffix: 'html',
    install: true,
    install_dir: install_dir,
)

custom_target('js',
  input: html_code,
  output: html_code,
  command: ['cp', '@INPUT@', '@OUTPUT@'],
  install: true,
  install_dir: install_dir)

# This should be changed to something that works at compile time
html_data = configuration_data()
html_data.set('PAGE_NAME', html_code)
html_data.set('EXECUTABLE_NAME', executable_name + '.js')
html_data.set('CODE', fs.read(c_code).replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;').replace('\'', '&#39;').strip())

configure_file(
  input: '../template.html',
  output: executable_name + '.html',
  configuration: html_data,
  install: true,
  install_dir: install_dir
)
