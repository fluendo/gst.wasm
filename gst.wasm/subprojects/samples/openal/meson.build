executable(executable_name,
    'openal-example.c',
    dependencies: [common_deps, dependency('gstopenal')],
    link_args: [
      '-sPTHREAD_POOL_SIZE=32',
      '-sINITIAL_MEMORY=536870912',
      '-lopenal'
    ],
    name_suffix: 'html',
    install: true,
    install_dir: install_dir,
)

custom_target('js',
  output: 'openal-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
