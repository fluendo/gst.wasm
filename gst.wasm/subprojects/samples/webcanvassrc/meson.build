webcanvassrc_example_common_link_args = [
  '-sASYNCIFY',
  # Required by webfetchsrc, this should be brought directly by the dep
  '-sFETCH=1',
]
webcanvassrc_example_common_deps = [
    common_deps,
    dependency('gstvideoconvertscale'),
    dependency('gstreamer-emscripten-1.0'),
]

executable('webcanvassrc-animation-example',
  'webcanvassrc-animation-example.c',
  dependencies: webcanvassrc_example_common_deps + gstwebplugin_dep + gstsdl2_dep,
  include_directories: include_directories('.'),
  cpp_args: ['-DHAVE_CONFIG_H=1'],
  link_args: webcanvassrc_example_common_link_args,
  name_suffix: 'html',
  install: true,
  install_dir: install_dir,
)
custom_target('webcanvassrc-animation-example-js',
  output: 'webcanvassrc-animation-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)

executable('webcanvassrc-webcam-example',
  'webcanvassrc-webcam-example.c',
  dependencies: webcanvassrc_example_common_deps + gstwebplugin_dep + gstsdl2_dep,
  include_directories: include_directories('.'),
  cpp_args: ['-DHAVE_CONFIG_H=1'],
  link_args: webcanvassrc_example_common_link_args + [
    '-sEXPORTED_FUNCTIONS=_init_pipeline,_main,cwrap',
    '-lhtml5',
    '-sINITIAL_MEMORY=536870912'
  ],
  name_suffix: 'html',
  install: true,
  install_dir: install_dir,
)
custom_target('webcanvassrc-webcam-example-js',
  output: 'webcanvassrc-webcam-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
