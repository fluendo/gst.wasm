executable(executable_name,
    'gstlaunch-example.c',
    dependencies: [common_deps,
                   dependency('gstsdl2'),
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
      '--shell-file=@0@/html/index.html'.format(meson.current_source_dir())
    ],
    name_suffix: 'html',
    install: true,
    install_dir: install_dir,
)

custom_target('js',
  output: 'gstlaunch-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
