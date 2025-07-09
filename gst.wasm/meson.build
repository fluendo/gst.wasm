project(
  'gst.wasm',
  version : '0.0.0',
  meson_version : '>= 1.4.1',
)

error(get_option('examples'))

subproject('gst-plugins-web')
if not get_option('examples').disabled()
  subproject('samples')
endif
