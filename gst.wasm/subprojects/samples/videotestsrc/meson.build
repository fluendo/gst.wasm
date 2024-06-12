executable('videotestsrc-example',
    'videotestsrc-example.c',
    install: true,
    dependencies: [common_deps,
                   dependency('gstsdl2'),
                   dependency('gstreamer-emscripten-1.0'),
                   dependency('gstvideotestsrc')],
    link_args: [
      '-sPTHREAD_POOL_SIZE=32',
      '-sINITIAL_MEMORY=536870912',
      '-lhtml5',
      '-sASYNCIFY',
      '-lGL'
    ],
    name_suffix: 'html'
)

custom_target('js',
  output: 'videotestsrc-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: 'bin')
