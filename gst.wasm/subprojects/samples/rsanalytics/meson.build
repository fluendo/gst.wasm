fs = import('fs')

gstrsanalytics_dep = dependency('gstrsanalytics', required: false)

if gstrsanalytics_dep.found()
  static_plugins_c = custom_target('rsanalytics-static-plugins',
    output: 'gst-wasm-static-plugins.c',
    command: [
      find_program(join_paths(meson.current_source_dir(), '..', 'generate-static-plugins.py')),
      '@OUTPUT@',
      'rsanalytics',
    ],
  )

  c_code = executable_name + '.c'
  html_code = executable_name + '-page.html'

  rsanalytics_example = executable(executable_name,
    c_code,
    static_plugins_c,
    dependencies: [
      gst_dep,
      dependency('gstreamer-emscripten-1.0'),
      gstrsanalytics_dep,
    ],
    link_args: common_link_args + ['-lhtml5'],
    c_args: ['-Wall', '-Werror'],
    name_suffix: 'js',
    install: true,
    install_dir: install_dir,
  )

  custom_target('page',
    input: html_code,
    output: html_code,
    command: ['cp', '@INPUT@', '@OUTPUT@'],
    install: true,
    install_dir: install_dir,
  )

  html_data = configuration_data()
  html_data.set('PAGE_NAME', html_code)
  html_data.set('EXECUTABLE_NAME', executable_name + '.js')
  html_data.set('PAGE_CODE', fs.read(html_code).replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;').replace('\'', '&#39;').strip())
  html_data.set('CODE', fs.read(c_code).replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;').replace('\'', '&#39;').strip())

  configure_file(
    input: '../template.html',
    output: executable_name + '.html',
    configuration: html_data,
    install: true,
    install_dir: install_dir,
  )

  node = find_program('node', required: false)
  if node.found()
    test('rsanalytics', node,
      args: [rsanalytics_example.full_path()],
      timeout: 60,
    )
  endif
endif
