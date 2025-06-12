from cerbero.config import Platform, Architecture, Distro, DistroVersion

# The target information
target_platform = Platform.WEB
target_distro = Distro.EMSCRIPTEN
target_arch = Architecture.WASM32

# The home dir defines
external_recipes = {
    'gst.wasm': (os.path.join(path, 'recipes'), 1),
}

gst_url = 'https://github.com/fluendo/gstreamer.git'
gst_commit = 'fluendo/gst.wasm'

recipes_remotes = {
    'gstreamer-1.0': {'fluendo': gst_url },
    'gst-plugins-base-1.0': {'fluendo': gst_url },
    'gst-plugins-good-1.0': {'fluendo': gst_url },
    'gst-plugins-bad-1.0': {'fluendo': gst_url },
    'gst-plugins-ugly-1.0': {'fluendo': gst_url },
    'gst-libav-1.0': {'fluendo': gst_url }
}

recipes_commits = {
    'gstreamer-1.0': gst_commit,
    'gst-plugins-base-1.0': gst_commit,
    'gst-plugins-good-1.0': gst_commit,
    'gst-plugins-bad-1.0': gst_commit,
    'gst-plugins-ugly-1.0': gst_commit,
    'gst-libav-1.0': gst_commit
}

# FIXME: remove this once all variables are correctly configured in web.config
host = "wasm32-unknown-linux"

folder_name = '{}_{}_{}'.format(folder_prefix, target_platform, target_arch)
home_dir = os.path.join(path, folder_name)
emscripten_path = os.path.join(home_dir, "emsdk", "upstream", "emscripten")
env['PATH'] = f'{emscripten_path}:{env["PATH"]}'
