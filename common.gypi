{
  'variables': {
    'default_cflags' : [
      '-Wall',
      '-Werror',
      '-m64',
      '-mcx16',
      '-mtune=native',
      '-ffast-math',
      '-fno-builtin-malloc',
      '-fno-builtin-calloc',
      '-fno-builtin-realloc',
      '-fno-builtin-free',
      '-fno-omit-frame-pointer',
      '-fno-exceptions',
    ],
    'default_cflags_cc' : [
      '-std=c++11',
      '-fno-rtti',
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
          'cflags_cc': [ '<@(default_cflags_cc)' ],
          'ldflags': [ '<@(default_ldflags)' ],
          'defines': [ '__STDC_FORMAT_MACROS=1' ],
          'include_dirs': [
            '.',
            'src',
          ]
        },
        'Release': {
          'cflags': [ '<@(default_cflags)', '-O3' ],
          'cflags_cc': [ '<@(default_cflags_cc)' ],
          'defines': [ '__STDC_FORMAT_MACROS=1' ],
          'ldflags': [ '<@(default_ldflags)' ],
          'include_dirs': [
            '.',
            'src',
          ]
        },
      }
  },
}
