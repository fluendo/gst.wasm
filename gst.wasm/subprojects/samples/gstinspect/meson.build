fs = import('fs')
c_code = executable_name + '.c'
html_code = executable_name + '-page.html'

include_directories('upstream-gstreamer')

executable(executable_name,
    'gstinspect-example.c',
    'upstream-gstreamer/gstinspect.c',
    dependencies: [common_deps, 
    dependency('gstweb'),
    dependency('gstsdl2'),
    ],
    link_args: common_link_args + [
      #'-sEXPORTED_FUNCTIONS=_init_pipeline,_main,cwrap',
      '-lhtml5',
      '-sASYNCIFY',
    ],
    #c_args: ['-DHAVE_CONFIG_H', '-Wall', '-Werror'],
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
#html_data.set('PAGE_CODE', fs.read(html_code).replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;').replace('\'', '&#39;').strip())
html_data.set('EXECUTABLE_NAME', executable_name + '.js')
html_data.set('CODE', fs.read(c_code).replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;').replace('\'', '&#39;').strip())

configure_file(
  input: '../template.html',
  output: executable_name + '.html',
  configuration: html_data,
  install: true,
  install_dir: install_dir
)
