output_dir = 'audiotestsrc'

configure_file(input : 'html/scripts/index.js', output : 'index.js',
               configuration : configuration_data())

sources = [
  'audioplayer.cpp',
  'audiotestsrc-example.c',
  'utils.c',
]

executable('audiotestsrc-example',
  sources,
  dependencies: common_deps,
  include_directories: include_directories('.'),
  link_args: common_link_args + [
      '-lembind',
      '-sAUTO_NATIVE_LIBRARIES=1',
      '-sEXPORTED_FUNCTIONS=_init', # defined in audiotestsrc-example.c
      '-sEXPORTED_RUNTIME_METHODS=ccall,cwrap',
      '--extern-post-js=@0@/html/scripts/index.js'.format(meson.current_source_dir()),
      '--shell-file=@0@/html/index.html'.format(meson.current_source_dir()),
  ],
  name_suffix: 'html',
  install: true,
  install_dir: install_dir,
)

custom_target('js',
  output: 'audiotestsrc-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
