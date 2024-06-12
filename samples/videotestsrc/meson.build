executable('videotestsrc-example',
    'videotestsrc-example.c',
    install: false,
    dependencies: [common_deps,
                   dependency('gstreamer-video-1.0'),
                   dependency('gstvideotestsrc')],
    c_args: ['-sUSE_SDL=2'],
    link_args: [
      '-sPTHREAD_POOL_SIZE=32',
      '-sINITIAL_MEMORY=536870912',
      '-sUSE_SDL=2'
    ],
    name_suffix: 'html'
)
