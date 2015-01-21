{
  'variables': {
    'default_cflags' : [
      '-Wall',
      '-Werror',
      '-std=c++11',
      '-m64',
      '-mcx16',
      '-ffast-math',
      '-fno-builtin-malloc',
      '-fno-builtin-calloc',
      '-fno-builtin-realloc',
      '-fno-builtin-free',
      '-fno-omit-frame-pointer',
      '-fno-rtti',
      '-fno-exceptions',
    ],
    'default_ldflags': [
      '-Wl,--as-needed',
    ],
    'default_libraries': [
      '-lpthread',
      '-lgflags',
    ],
  },
  'target_defaults': {
      'configurations': {
        'Debug': {
          'cflags': [ '<@(default_cflags)', '-O0', '-gdwarf-2' ],
          'ldflags': [ '<@(default_ldflags)' ],
          'include_dirs': [
            '.',
            'src',
          ]
        },
        'Release': {
          'cflags': [ '<@(default_cflags)', '-O3' ],
          'ldflags': [ '<@(default_ldflags)' ],
          'include_dirs': [
            '.',
            'src',
          ]
        },
      }
  },
}
