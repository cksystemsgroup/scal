{
  'includes': [
    'common.gypi',
  ],
  'targets': [
    {
      'target_name': 'ms',
      'type': 'static_library',
      'sources': [
        'src/benchmark/std_glue/glue_ms_queue.cc'
      ],
    },
    {
      'target_name': 'treiber',
      'type': 'static_library',
      'sources': [
        'src/benchmark/std_glue/glue_treiber_stack.cc'
      ],
    },
    {
      'target_name': 'kstack',
      'type': 'static_library',
      'sources': [
        'src/benchmark/std_glue/glue_kstack.cc'
      ],
    },
  ]
}
