fs = import('fs')

#FIXME: this is the ugliest thing I've ever seen. This is the consequence of gst-inspect-1.0 being a tool and not a library
#Maybe converting GStreamer to a subproject?
sources_dir = join_paths(get_option('prefix'), '..', '..', 'sources')
gst_basics_source_dir = join_paths(sources_dir, 'local', 'gstreamer-1.0', 'subprojects', 'gstreamer')
gstinspect_source_dir = join_paths(gst_basics_source_dir, 'tools')
gstreamer_includes = include_directories(gstinspect_source_dir, gst_basics_source_dir)

executable(executable_name,
    'gstinspect-example.c',
    dependencies: [common_deps,
    dependency('gstweb'),
    dependency('gstsdl2'),
    ],
    include_directories: [gstreamer_includes],
    link_args: common_link_args + [
      '-lhtml5',
      '-sASYNCIFY',
    ],
    c_args: ['-Wall', '-Werror', '-Wno-deprecated-declarations', '-Wno-unused-function'],
    name_suffix: 'js',
    install: true,
    install_dir: install_dir,
)
