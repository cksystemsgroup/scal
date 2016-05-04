{
  'variables': {
    'default_cflags' : [
      '-Wall',
      '-Werror',
      '-ftls-model=initial-exec',
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
      '-fPIC',
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
  'xcode_settings': {},
  'target_defaults': {
      'configurations': {
        'Debug': {
          'cflags': [ '<@(default_cflags)', '-O0', '-gdwarf-2' ],
          'cflags_cc': [ '<@(default_cflags_cc)' ],
          'ldflags': [ '<@(default_ldflags)' ],
          'xcode_settings': {
            'CLANG_CXX_LANGUAGE_STANDARD': "c++11",
            'OTHER_CFLAGS': [ '<@(default_cflags)' , '-g', '-O0' ],
            'OTHER_LDFLAGS': [ '<@(default_ldflags)' ],
          },
          'defines': [ '__STDC_FORMAT_MACROS=1' ],
          'include_dirs': [
            '.',
            'src',
            '/usr/include',
            '/usr/local/include',
          ]
        },
        'Release': {
          'cflags': [ '<@(default_cflags)', '-O3' ],
          'cflags_cc': [ '<@(default_cflags_cc)' ],
          'ldflags': [ '<@(default_ldflags)' ],
          'xcode_settings': {
            'CLANG_CXX_LANGUAGE_STANDARD': "c++11",
            'OTHER_CFLAGS': [ '<@(default_cflags)' , '-g', '-O3'],
            'OTHER_LDFLAGS': [ '<@(default_ldflags)' ],
          },
          'defines': [ '__STDC_FORMAT_MACROS=1' ],
          'include_dirs': [
            '.',
            'src',
            '/usr/include',
            '/usr/local/include',
          ]
        },
      },
  },
}
