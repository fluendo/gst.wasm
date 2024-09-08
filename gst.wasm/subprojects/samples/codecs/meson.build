executable(executable_name,
    'codecs-example.c',
    dependencies: [
      common_deps,
      gstisomp4_dep,
      dependency('gstreamer-emscripten-1.0'),
      dependency('gstopengl'),
      dependency('gstsdl2')
    ],
    link_with: [
      gstweb_dep,
      gstweb_emhttpsrc_dep
    ],
    link_args: common_link_args + [
      '-lembind',
      '-sASYNCIFY',
      # This is giving problems when running the WebRunner at set_format
      '-sPROXY_TO_PTHREAD',
      # Required by emhttpsrc, this should be brought directly by the dep
      '-sFETCH=1',
      '-g3',
      '-lhtml5',
      '-lGL',
      '-sOFFSCREENCANVAS_SUPPORT', # To manipulate the gl context from a thread
    ],
    c_args: [
      '-g3',
      '-O0',
    ],
    name_suffix: 'html',
    install: true,
    install_dir: install_dir
)

custom_target('js',
  output: 'codecs-example.js',
  command: ['echo', 'installing js file'],
  install: true,
  install_dir: install_dir)
