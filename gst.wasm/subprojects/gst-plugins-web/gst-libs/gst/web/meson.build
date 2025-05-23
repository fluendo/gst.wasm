gstweb_sources = [
  'gstwebrunner.c',
  'gstwebtask.c',
  'gstwebtaskpool.c',
  'gstwebtransferable.cpp',
  'gstwebvideoframe.cpp',
  'gstwebutils.cpp',
  'gstwebcanvas.c'
]

gstweb_headers = [
  'gstwebrunner.h',
  'gstwebtransferable.h',
  'gstwebvideoframe.h',
  'gstwebutils.h',
  'gstwebcanvas.h'
]

link_args = [
  '-lembind',
  '-sAUTO_NATIVE_LIBRARIES=1',
  '-sEXPORTED_RUNTIME_METHODS=ccall,cwrap',
]

pkg_name = 'gstreamer-web-' + api_version

gstweb = library('gstweb-' + api_version,
  gstweb_sources,
  c_args : gst_plugins_web_args,
  cpp_args: gst_plugins_web_args,
  link_args: link_args,
  include_directories : [configinc, libinc],
  dependencies : [common_deps],
  version : libversion,
  install : true,
)

pkgconfig.generate(gstweb,
  libraries : [common_deps],
  variables : pkgconfig_variables,
  subdirs : pkgconfig_subdirs,
  name : pkg_name,
  description : 'GStreamer Web API library',
)

library_def = {'lib': gstweb}

install_headers(gstweb_headers, subdir : 'gstreamer-1.0/gst/web')

gstweb_dep = declare_dependency(link_with : gstweb,
  include_directories : [libinc],
  dependencies : [common_deps],
  link_args : [link_args]
)
