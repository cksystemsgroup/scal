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
    {
      'target_name': 'll-kstack',
      'type': 'static_library',
      'cflags': [ '-DLOCALLY_LINEARIZABLE' ],
      'sources': [
        'src/benchmark/std_glue/glue_ll_kstack.cc'
      ],
    },
    {
      'target_name': 'dq-1random-ms',
      'type': 'static_library',
      'sources': [
        'src/benchmark/std_glue/glue_dq_1random_ms.cc'
      ],
    },
    {
      'target_name': 'dq-1random-treiber',
      'type': 'static_library',
      'sources': [
        'src/benchmark/std_glue/glue_dq_1random_treiber.cc'
      ],
    },
    {
      'target_name': 'lru-dq',
      'type': 'static_library',
      'sources': [
        'src/benchmark/std_glue/glue_lru_dq.cc'
      ],
    },
    {
      'target_name': 'fc',
      'type': 'static_library',
      'sources': [
        'src/benchmark/std_glue/glue_fc_queue.cc'
      ],
    },
    {
      'target_name': 'rd',
      'type': 'static_library',
      'sources': [
        'src/benchmark/std_glue/glue_rd_queue.cc'
      ],
    },
    {
      'target_name': 'us-kfifo',
      'type': 'static_library',
      'sources': [
        'src/benchmark/std_glue/glue_uskfifo.cc'
      ],
    },
    {
      'target_name': 'bs-kfifo',
      'type': 'static_library',
      'sources': [
        'src/benchmark/std_glue/glue_bskfifo.cc'
      ],
    },
    {
      'target_name': 'll-us-kfifo',
      'type': 'static_library',
      'cflags': [ '-DLOCALLY_LINEARIZABLE' ],
      'sources': [
        'src/benchmark/std_glue/glue_uskfifo.cc'
      ],
    },
    {
      'target_name': 'll-dq-ms',
      'type': 'static_library',
      'defines': [ 'GET_TRY_LOCAL_FIRST' ],
      'sources': [
        'src/benchmark/std_glue/glue_ll_dq_ms.cc'
      ],
    },
    {
      'target_name': 'll-dq-treiber',
      'type': 'static_library',
      'defines': [ 'GET_TRY_LOCAL_FIRST' ],
      'sources': [
        'src/benchmark/std_glue/glue_ll_dq_treiber.cc'
      ],
    },
    {
      'target_name': 'll-dq-ms-nonlinempty',
      'type': 'static_library',
      'defines': [
        'BACKEND_MS_QUEUE',
        'BALANCER_LL',
        'NON_LINEARIZABLE_EMPTY'
      ],
      'sources': [
        'src/benchmark/std_glue/glue_dq.cc'
      ],
    },
    {
      'target_name': 'll-dq-treiber-nonlinempty',
      'type': 'static_library',
      'defines': [
        'BACKEND_TREIBER',
        'BALANCER_LL',
        'NON_LINEARIZABLE_EMPTY'
      ],
      'sources': [
        'src/benchmark/std_glue/glue_dq.cc'
      ],
    },
    {
      'target_name': 'll-dyn-dq-ms-nonlinempty',
      'type': 'static_library',
      'defines': [
        'BACKEND_MS_QUEUE',
        'NON_LINEARIZABLE_EMPTY'
      ],
      'sources': [
        'src/benchmark/std_glue/glue_dyn_dq.cc'
      ],
    },
    {
      'target_name': 'll-dyn-dq-treiber-nonlinempty',
      'type': 'static_library',
      'defines': [
        'BACKEND_TREIBER',
        'NON_LINEARIZABLE_EMPTY'
      ],
      'sources': [
        'src/benchmark/std_glue/glue_dyn_dq.cc'
      ],
    },
    {
      'target_name': 'll-dyn-dq-ms',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_ll_dyn_dq_ms.cc'
      ],
    },
    {
      'target_name': 'll-dyn-dq-treiber',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_ll_dyn_dq_treiber.cc'
      ],
    },
    {
      'target_name': 'lcrq',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_lcrq.cc'
      ],
      'dependencies': [
        'upstream.gypi:lcrq-base',
      ],
    },
    {
      'target_name': 'hc-ts-cas-stack',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_hardcoded_ts_cas_stack.cc'
      ],
    },
    {
      'target_name': 'hc-ts-stutter-stack',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_hardcoded_ts_stutter_stack.cc'
      ],
    },
    {
      'target_name': 'hc-ts-interval-stack',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_hardcoded_ts_interval_stack.cc'
      ],
    },
    {
      'target_name': 'hc-ts-atomic-stack',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_hardcoded_ts_atomic_stack.cc'
      ],
    },
    {
      'target_name': 'hc-ts-hardware-stack',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_hardcoded_ts_hardware_stack.cc'
      ],
    },
    {
      'target_name': 'hc-ts-cas-queue',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_hardcoded_ts_cas_queue.cc'
      ],
    },
    {
      'target_name': 'hc-ts-stutter-queue',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_hardcoded_ts_stutter_queue.cc'
      ],
    },
    {
      'target_name': 'hc-ts-interval-queue',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_hardcoded_ts_interval_queue.cc'
      ],
    },
    {
      'target_name': 'hc-ts-atomic-queue',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_hardcoded_ts_atomic_queue.cc'
      ],
    },
    {
      'target_name': 'hc-ts-hardware-queue',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_hardcoded_ts_hardware_queue.cc'
      ],
    },
    {
      'target_name': 'dts-queue',
      'type': 'static_library',
      'sources': [
        'src/benchmark/std_glue/glue_dts_queue.cc'
      ],
    },
    {
      'target_name': 'lts-queue',
      'type': 'static_library',
      'sources': [
        'src/benchmark/std_glue/glue_lts_queue.cc'
      ],
    },
    {
      'target_name': 'ts-cas-deque',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_ts_cas_deque.cc'
      ],
    },
    {
      'target_name': 'ts-stutter-deque',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_ts_stutter_deque.cc'
      ],
    },
    {
      'target_name': 'ts-interval-deque',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_ts_interval_deque.cc'
      ],
    },
    {
      'target_name': 'ts-atomic-deque',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_ts_atomic_deque.cc'
      ],
    },
    {
      'target_name': 'ts-hardware-deque',
      'type': 'static_library',
      'cflags': [ ],
      'sources': [
        'src/benchmark/std_glue/glue_ts_hardware_deque.cc'
      ],
    },
  ]
}
