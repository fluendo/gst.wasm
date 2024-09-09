project(
  'gst-plugins-web',
  'c', 'cpp',
  version : '0.0.0',
  meson_version : '>= 0.56.0'
)

gst_dep = dependency('gstreamer-1.0')
gstcoreelements_dep = dependency('gstcoreelements')
gstapp_dep = dependency('gstapp')
gstaudio_dep = dependency('gstreamer-audio-1.0')
gstvideo_dep = dependency('gstreamer-video-1.0')
gstaudiotestsrc_dep = dependency('gstaudiotestsrc')
common_deps = [gst_dep, gstcoreelements_dep]

cdata = configuration_data()
cdata.set_quoted('VERSION', '1.22.0') # TODO
cdata.set_quoted('PACKAGE', 'gst-plugins-web')
cdata.set_quoted('PACKAGE_VERSION', '1.22.0') # TODO
cdata.set_quoted('PACKAGE_BUGREPORT', 'https://github.com/fluendo/gst.wasm/issues')
cdata.set_quoted('PACKAGE_NAME', 'GStreamer Plugins Web')
cdata.set_quoted('GST_PACKAGE_NAME', 'GStreamer Plugins Web')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://fluendo.com')
cdata.set_quoted('GST_LICENSE', 'UNKNOWN') # TODO

plugins = []
subdir('gst')

pkgconfig = import('pkgconfig')
plugins_install_dir = join_paths(get_option('libdir'), 'gstreamer-1.0')
plugins_pkgconfig_install_dir = join_paths(plugins_install_dir, 'pkgconfig')
# plugin_names = []
# gst_plugins = []
foreach plugin: plugins
  pkgconfig.generate(plugin, install_dir: plugins_pkgconfig_install_dir)
  dep = declare_dependency(link_with: plugin, variables: {'full_path': plugin.full_path()})
  meson.override_dependency(plugin.name(), dep)
  # gst_plugins += [dep]
  # if plugin.name().startswith('gst')
  #   plugin_names += [plugin.name().substring(3)]
  # else
  #   plugin_names += [plugin.name()]
  # endif
endforeach
