executable(executable_name,
    'videotestsrc-example.c',
    dependencies: [common_deps,
                   dependency('gstsdl2'),
                   dependency('gstreamer-emscripten-1.0'),
                   dependency('gstvideotestsrc')],
    link_args: common_link_args + [
      '-lhtml5',
      '-sASYNCIFY',
      '-lGL'
    ],
    name_suffix: 'html',
    install: true,
    install_dir: install_dir,
)

custom_target('js',
  output: 'videotestsrc-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
