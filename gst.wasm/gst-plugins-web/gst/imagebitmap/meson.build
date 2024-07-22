sources = [
  'gstimagebitmapsrc.cpp'
]

configure_file(output: 'config.h', configuration: cdata)

gstimagebitmapsrc_dep = static_library('gstimagebitmapsrc',
  sources,
  install: true,
  dependencies: common_deps + gstvideo_dep,
  include_directories: include_directories('.'),
  cpp_args: ['-DHAVE_CONFIG_H=1'],
  link_args: [
      '-lembind',
      '-sAUTO_NATIVE_LIBRARIES=1',
      '-sEXPORTED_RUNTIME_METHODS=ccall,cwrap',
      '-sPTHREAD_POOL_SIZE=4',
  ],
  # name_suffix: 'html'
)