sources = [
  'webstreamsrc-example.c'
]

executable('webstreamsrc-example',
  sources,
  install: true,
  install_dir: install_dir,
  dependencies: common_deps,
  include_directories: include_directories('.'),
  c_args: ['-DHAVE_CONFIG_H=1'],
  link_args: [
      '-sFETCH=1',
      # Required by webcanvassink, this should be brought directly by the dep
      '-lembind',
      '-lhtml5',
      '-sASYNCIFY',
  ],
  link_with: [
    gstweb_dep
  ],
  name_suffix: 'html'
)

custom_target('js',
  output: 'webstreamsrc-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
