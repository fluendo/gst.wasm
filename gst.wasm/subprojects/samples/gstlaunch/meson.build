executable(executable_name,
    'gstlaunch-example.c',
    dependencies: [common_deps,
                   gstsdl2_dep,
                   dependency('gstopenal'),
                   dependency('gstcoreelements'),
                   dependency('gstreamer-emscripten-1.0'),
                   dependency('gstvideotestsrc'),
                   dependency('gstaudiotestsrc')],
    link_args: common_link_args + [
      '-sEXPORTED_FUNCTIONS=_init_pipeline,_main,cwrap',
      '-lhtml5',
      '-sASYNCIFY',
      '-lGL',
      '-sUSE_SDL=2',
      '--shell-file=@0@/html/index.html'.format(meson.current_source_dir())
    ],
    c_args: ['-DHAVE_CONFIG_H', '-sUSE_SDL=2', '-Wall', '-Werror'],
    name_suffix: 'html',
    install: true,
    install_dir: install_dir,
)

custom_target('js',
  output: 'gstlaunch-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
