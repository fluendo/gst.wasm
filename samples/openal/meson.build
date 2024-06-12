executable('openal-example',
    'openal-example.c',
    install: false,
    dependencies: [common_deps, dependency('gstopenal')],
    link_args: [
      '-sPTHREAD_POOL_SIZE=32',
      '-sINITIAL_MEMORY=536870912'
    ],
    name_suffix: 'html'
)
