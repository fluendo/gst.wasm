sources = [
  'gstwebcanvassrc.cpp'
]

configure_file(output: 'config.h', configuration: cdata)

gstwebcanvassrc_dep = static_library('gstwebcanvassrc',
  sources,
  install: true,
  dependencies: common_deps + gstvideo_dep,
  include_directories: include_directories('.'),
  cpp_args: ['-DHAVE_CONFIG_H=1'],
  link_args: [
      '-lembind',
      '-sAUTO_NATIVE_LIBRARIES=1',
      '-sEXPORTED_RUNTIME_METHODS=ccall,cwrap',
  ],
)
