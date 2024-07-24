sources = [
  'imagebitmap-example.c'
]

executable('imagebitmap-example',
  sources,
  dependencies: common_deps,
  link_with: gstimagebitmapsrc_dep,
  include_directories: include_directories('.'),
  cpp_args: ['-DHAVE_CONFIG_H=1'],
  link_args: [
      '-lembind',
      '-sAUTO_NATIVE_LIBRARIES=1',
      '-sEXPORTED_RUNTIME_METHODS=ccall,cwrap',
      '-sPTHREAD_POOL_SIZE=4',
  ],
  name_suffix: 'html',
  install: true,
  install_dir: install_dir,
)

custom_target('js',
  output: 'imagebitmap-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)