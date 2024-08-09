sources = [
  'gstemhttpsrc.c'
]

configure_file(output: 'config.h', configuration: cdata)

emhttpsrc_dep = static_library('gstemhttpsrc',
  sources,
  install: true,
  dependencies: common_deps,
  include_directories: include_directories('.'),
  c_args: ['-DHAVE_CONFIG_H'],
  link_args: [
    '-sPTHREAD_POOL_SIZE=4',
    '-sFETCH=1'
  ],
)

