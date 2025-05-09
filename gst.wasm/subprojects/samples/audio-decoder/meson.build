executable(executable_name,
    'audio-decoder-example.c',
    dependencies: [
      common_deps,
      gstisomp4_dep,
      dependency('gstreamer-emscripten-1.0'),
      dependency('gstopenal'),
      dependency('gstaudioconvert'),
      dependency('gstweb'),
    ],
    link_args: common_link_args + [
      '-sASYNCIFY',
      '-sPROXY_TO_PTHREAD',
    ],
    name_suffix: 'html',
    install: true,
    install_dir: install_dir,
)

custom_target('js',
  output: 'audio-decoder-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
