sources = [
  'webstreamsrc-example.c'
]

executable('webstreamsrc-example',
  sources,
  install: true,
  install_dir: install_dir,
  dependencies: common_deps + gstwebplugin_dep,
  include_directories: include_directories('.'),
  c_args: ['-DHAVE_CONFIG_H=1'],
  link_args: [
      '-sASYNCIFY',
  ],
  name_suffix: 'html'
)

custom_target('js',
  output: 'webstreamsrc-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
