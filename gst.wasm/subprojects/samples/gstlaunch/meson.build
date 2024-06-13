executable('gstlaunch',
    'gstlaunch.c',
    install: true,
    dependencies: [common_deps,
                   dependency('gstsdl2'),
                   dependency('gstopenal'),
                   dependency('gstcoreelements'),
                   dependency('gstreamer-emscripten-1.0'),
                   dependency('gstvideotestsrc'),
                   dependency('gstaudiotestsrc')],
    link_args: [
      '-sEXPORTED_FUNCTIONS=_init_pipeline,_main,cwrap',
      '-sPTHREAD_POOL_SIZE=32',
      '-sINITIAL_MEMORY=536870912',
      '-lhtml5',
      '-sASYNCIFY',
      '-lGL',
      '--shell-file=@0@/index.html'.format(meson.current_source_dir())
    ],
    name_suffix: 'html'
)

custom_target('js',
  output: 'gstlaunch.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: 'bin')
