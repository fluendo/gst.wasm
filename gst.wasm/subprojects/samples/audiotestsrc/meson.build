output_dir = 'audiotestsrc'

configure_file(input : 'src/js/index.js', output : 'index.js',
               configuration : configuration_data())

sources = [
  'src/audioplayer.cpp',
  'src/init.c',
  'src/utils.c',
]

executable('index',
  sources,
  install: false,
  dependencies: common_deps,
  include_directories: include_directories('src'),
  link_args: [
      '-lembind',
      '-sAUTO_NATIVE_LIBRARIES=1',
      '-sEXPORTED_FUNCTIONS=_init', # defined in src/init.c
      '-sEXPORTED_RUNTIME_METHODS=ccall,cwrap',
      '-sPTHREAD_POOL_SIZE=4',
      '--extern-post-js=@0@/src/js/index.js'.format(meson.current_source_dir()),
      '--shell-file=@0@/index.html'.format(meson.current_source_dir())
  ],
  name_suffix: 'html'
)