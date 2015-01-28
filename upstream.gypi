{
  'includes': [
    'common.gypi',
  ],
  'targets': [
    {
      'target_name': 'lcrq-base',
      'type': 'static_library',
      'include_dirs+': [
        'src/datastructures/upstream/sim-synch1.4'
      ],
      'sources': [
        'src/datastructures/upstream/lcrq/lcrq.c'
      ],
    },
  ]
}
