sources = [
  'gstsdl2.c', 'gstsdl2sink.c'
]

configure_file(output: 'config.h', configuration: cdata)

sdl2_dep = static_library('gstsdl2',
  sources,
  c_args: ['-DHAVE_CONFIG_H', '-sUSE_SDL=2', '-Wall', '-Werror'],
  link_args: ['-sUSE_SDL=2', '-lGL'],
  include_directories: [include_directories('.'), libinc],
  dependencies: [gstemscripten_dep, gstvideo_dep],
  install: true,
  install_dir: plugins_install_dir,
)

plugins += [sdl2_dep]
