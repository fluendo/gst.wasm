fs = import('fs')
c_code = executable_name + '.c'
html_code = executable_name + '-page.html'

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

description_title = 'gst-inspect'

custom_target(
  executable_name + '.html',
  input: [
    '../template.html',
    html_code,
    c_code
  ],
  output: executable_name + '.html',
  command: [
    html_generator,
    '@INPUT0@',
    '@OUTPUT@',
    'PAGE_NAME', description_title,
    'DESCRIPTION_TITLE',  description_title,
    'PAGE_CODE', '@INPUT1@',
    'CODE', '@INPUT2@',
    'EXECUTABLE_NAME', executable_name + '.js'
  ],
  install: true,
  install_dir: install_dir,
)
