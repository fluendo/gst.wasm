sources = [
  'gstweb.c',
  'gstwebcanvassink.cpp',
  'gstwebcanvassrc.cpp',
  'gstwebdownload.c',
  'gstwebfetchsrc.c',
  'gstwebstreamsrc.cpp',
  'codecs/gstwebcodecs.cpp',
  'codecs/gstwebcodecsvideodecoder.cpp',
  'transport/gstwebtransportsrc.cpp',
  'transport/gstwebtransportstreamsrc.cpp'
]

# Codecs deps
codecs_deps = [gstvideo_dep, gstpbutils_dep, gstgl_dep]

gstweb_link_args = [
  '-sFETCH=1',
  '-lhtml5',
]
gstweb_lib_deps = common_deps + codecs_deps + gstweb_dep
gstweb_lib = static_library('gstweb',
  sources,
  install: true,
  dependencies: gstweb_lib_deps,
  include_directories: [configinc],
  c_args: gst_plugins_web_args,
  cpp_args: gst_plugins_web_args,
  link_args: gstweb_link_args
)

pkgconfig.generate(gstweb_lib, install_dir: plugins_pkgconfig_install_dir)
gstwebplugin_dep = declare_dependency(link_with : gstweb_lib,

  variables: {'full_path': gstweb_lib.full_path()},
  include_directories : [libinc],
  dependencies : gstweb_lib_deps,
  link_args : gstweb_link_args,
)
meson.override_dependency(gstweb_lib.name(), gstwebplugin_dep)
