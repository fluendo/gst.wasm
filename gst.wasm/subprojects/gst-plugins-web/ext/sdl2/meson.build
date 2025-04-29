sources = [
  'gstsdl2sink.c'
]

configure_file(output: 'config.h', configuration: cdata)

sdl2_lib = static_library('gstsdl2',
  sources,
  c_args: ['-DHAVE_CONFIG_H', '-sUSE_SDL=2', '-Wall', '-Werror'],
  link_args: ['-sUSE_SDL=2', '-lGL'],
  include_directories: [include_directories('.'), libinc],
  dependencies: [gstemscripten_dep, gstvideo_dep],
  install: true,
  install_dir: plugins_install_dir,
)

pkgconfig.generate(sdl2_lib, install_dir: plugins_pkgconfig_install_dir)
sdl2_dep = declare_dependency(link_with : sdl2_lib,

  variables: {'full_path': sdl2_lib.full_path()},
  include_directories : [libinc],
  dependencies: [gstemscripten_dep, gstvideo_dep],
)
meson.override_dependency(sdl2_lib.name(), sdl2_dep)