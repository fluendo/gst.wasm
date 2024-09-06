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
    name_suffix: 'html',
    install: true,
    install_dir: install_dir,
)

custom_target('js',
  output: 'gl-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
