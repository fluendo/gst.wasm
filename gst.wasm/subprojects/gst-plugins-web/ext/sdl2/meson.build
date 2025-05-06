sources = [
  'gstsdl2sink.c'
]

configure_file(output: 'config.h', configuration: cdata)

sdl2_lib_link_args = ['-sUSE_SDL=2', '-lGL']
sdl2_lib = static_library('gstsdl2',
  sources,
  c_args: ['-DHAVE_CONFIG_H', '-sUSE_SDL=2', '-Wall', '-Werror'],
  link_args: sdl2_lib_link_args,
  include_directories: [include_directories('.'), libinc],
  dependencies: [gstemscripten_dep, gstvideo_dep],
  install: true,
  install_dir: plugins_install_dir,
)

pkgconfig.generate(sdl2_lib, install_dir: plugins_pkgconfig_install_dir)
gstsdl2_dep = declare_dependency(link_with : sdl2_lib,
  variables: {'full_path': sdl2_lib.full_path()},
  link_args: sdl2_lib_link_args,
)
meson.override_dependency(sdl2_lib.name(), gstsdl2_dep)