executable(executable_name,
    'gstinspect.c',
    dependencies: [common_deps, 
    dependency('gstreamer-emscripten-1.0'),
    dependency('gstaudioconvert'),
    dependency('gstweb'),
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
  output: 'gstinspect.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
