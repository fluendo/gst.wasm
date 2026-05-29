c_code = executable_name + '.c'
html_code = executable_name + '-page.html'

executable(executable_name,
    'gl-example.c',
    dependencies: [common_deps,
                   dependency('gstopengl')],
    link_args: common_link_args + [
      '-lhtml5',
      '-lGL',
      '-sOFFSCREENCANVAS_SUPPORT', # To manipulate the gl context from a thread
      '-sPROXY_TO_PTHREAD', # To avoid deadlock between main thread access like stdout from a thread while main thread is blocked
    ],
    name_suffix: 'js',
    install: true,
    install_dir: install_dir,
)

install_data(html_code, install_dir: install_dir)

custom_target('js',
  input: html_code,
  output: html_code,
  command: ['cp', '@INPUT@', '@OUTPUT@'],
  install: true,
  install_dir: install_dir)

description_title = 'gl'

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
