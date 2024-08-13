examples = [
  'webcanvassrc',
  'emhttpsrc',
]

foreach example : examples
  executable_name = '@0@-example'.format(example)
  install_dir = 'share/gst.wasm/gst-plugins-web/samples/@0@'.format(executable_name)
  subdir(example)
endforeach
