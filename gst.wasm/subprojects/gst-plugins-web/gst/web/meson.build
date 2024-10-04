sources = [
  'gstweb.c',
  'gstwebcanvassink.cpp',
  'codecs/gstwebcodecs.cpp',
  'codecs/gstwebcodecsvideodecoder.cpp'
]

# Codecs deps
codecs_deps = [gstvideo_dep, gstpbutils_dep, gstgl_dep]

gstweb_dep = static_library('gstweb',
  sources,
  install: true,
  dependencies: common_deps + codecs_deps + gstweb_dep,
  include_directories: [configinc],
  c_args: gst_plugins_web_args + [
    '-g3',
    '-O0'
  ],
  cpp_args: gst_plugins_web_args + [
    '-g3',
    '-O0'
  ],
  link_args: [
      '-lembind',
      '-sAUTO_NATIVE_LIBRARIES=1',
      '-sEXPORTED_RUNTIME_METHODS=ccall,cwrap',
      '-g3',
  ],
)
