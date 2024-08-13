webcanvassrc_example_common_link_args = [
  '-lembind',
  '-sAUTO_NATIVE_LIBRARIES=1',
  '-sASYNCIFY',
]

executable('webcanvassrc-animation-example',
  'webcanvassrc-animation-example.c',
  dependencies: [
    common_deps,
    dependency('gstsdl2'),
    dependency('gstvideoconvertscale'),
    dependency('gstreamer-emscripten-1.0')
  ],
  link_with: gstwebcanvassrc_dep,
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
  dependencies: [
    common_deps,
    dependency('gstsdl2'),
    dependency('gstvideoconvertscale'),
    dependency('gstreamer-emscripten-1.0')
  ],
  link_with: gstwebcanvassrc_dep,
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
