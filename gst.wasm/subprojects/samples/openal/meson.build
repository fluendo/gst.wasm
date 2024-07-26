executable(executable_name,
    'openal-example.c',
    dependencies: [common_deps, dependency('gstopenal')],
    link_args: common_link_args,
    name_suffix: 'html',
    install: true,
    install_dir: install_dir,
)

custom_target('js',
  output: 'openal-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
