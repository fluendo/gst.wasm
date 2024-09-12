sources = [
  'emhttpsrc-example.c'
]

executable('index',
  sources,
  install: true,
  install_dir: install_dir,
  dependencies: common_deps + gstweb_emhttpsrc_dep,
  include_directories: include_directories('.'),
  c_args: ['-DHAVE_CONFIG_H=1'],
  link_args: [
      '-sPTHREAD_POOL_SIZE=4',
      '-sFETCH=1',
  ],
  name_suffix: 'html'
)

custom_target('js',
  output: 'index.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
