{
  'includes': [
    'common.gypi',
  ],
  'targets': [
    {
      'target_name': 'libscal',
      'type': 'static_library',
      'sources': [
        'src/util/atomic_value128.h',
        'src/util/atomic_value64_base.h',
        'src/util/atomic_value64_no_offset.h',
        'src/util/atomic_value64_offset.h',
        'src/util/atomic_value.h',
        'src/util/atomic_value_new.h',
        'src/util/allocation.h',
        'src/util/allocation.cc',
        'src/util/barrier.h',
        'src/util/bitmap.h',
        'src/util/malloc-compat.h',
        'src/util/operation_logger.h',
        'src/util/platform.h',
        'src/util/random.h',
        'src/util/random.cc',
        'src/util/threadlocals.h',
        'src/util/threadlocals.cc',
        'src/util/time.h',
        'src/util/workloads.h',
        'src/util/workloads.cc',
      ],
    },
    {
      'target_name': 'prodcon-base',
      'type': 'static_library',
      'sources': [
        'src/benchmark/common.cc',
        'src/benchmark/prodcon/prodcon.cc',
      ],
    },
    {
      'target_name': 'seqalt-base',
      'type': 'static_library',
      'sources': [
        'src/benchmark/common.cc',
        'src/benchmark/seqalt/seqalt.cc',
      ],
    },
    {
      'target_name': 'prodcon-ms',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:ms',
      ],
    },
    {
      'target_name': 'prodcon-treiber',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:treiber',
      ],
    },
    {
      'target_name': 'prodcon-kstack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:kstack',
      ],
    },
    {
      'target_name': 'prodcon-ll-kstack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:ll-kstack',
      ],
    },
    {
      'target_name': 'prodcon-dq-1random-ms',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:dq-1random-ms',
      ],
    },
    {
      'target_name': 'prodcon-dq-1random-treiber',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:dq-1random-treiber',
      ],
    },
    {
      'target_name': 'prodcon-fc',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:fc',
      ],
    },
    {
      'target_name': 'prodcon-rd',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:rd',
      ],
    },
    {
      'target_name': 'prodcon-us-kfifo',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:us-kfifo',
      ],
    },
    {
      'target_name': 'prodcon-ll-us-kfifo',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:ll-us-kfifo',
      ],
    },
    {
      'target_name': 'prodcon-ll-dq-ms',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:ll-dq-ms',
      ],
    },
    {
      'target_name': 'prodcon-ll-dq-treiber',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:ll-dq-treiber',
      ],
    },
    {
      'target_name': 'prodcon-lcrq',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:lcrq',
      ],
    },
    {
      'target_name': 'prodcon-hc-ts-cas-stack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:hc-ts-cas-stack',
      ],
    },
    {
      'target_name': 'prodcon-hc-ts-stutter-stack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:hc-ts-stutter-stack',
      ],
    },
    {
      'target_name': 'prodcon-hc-ts-interval-stack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:hc-ts-interval-stack',
      ],
    },
    {
      'target_name': 'prodcon-hc-ts-atomic-stack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:hc-ts-atomic-stack',
      ],
    },
    {
      'target_name': 'prodcon-hc-ts-hardware-stack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:hc-ts-hardware-stack',
      ],
    },
    {
      'target_name': 'prodcon-hc-ts-cas-queue',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:hc-ts-cas-queue',
      ],
    },
    {
      'target_name': 'prodcon-hc-ts-stutter-queue',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:hc-ts-stutter-queue',
      ],
    },
    {
      'target_name': 'prodcon-hc-ts-interval-queue',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:hc-ts-interval-queue',
      ],
    },
    {
      'target_name': 'prodcon-hc-ts-atomic-queue',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:hc-ts-atomic-queue',
      ],
    },
    {
      'target_name': 'prodcon-hc-ts-hardware-queue',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'prodcon-base',
        'glue.gypi:hc-ts-hardware-queue',
      ],
    },
    {
      'target_name': 'seqalt-ms',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:ms',
      ],
    },
    {
      'target_name': 'seqalt-treiber',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:treiber',
      ],
    },
    {
      'target_name': 'seqalt-kstack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:kstack',
      ],
    },
    {
      'target_name': 'seqalt-ll-kstack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:ll-kstack',
      ],
    },
    {
      'target_name': 'seqalt-dq-1random-ms',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:dq-1random-ms',
      ],
    },
    {
      'target_name': 'seqalt-dq-1random-treiber',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:dq-1random-treiber',
      ],
    },
    {
      'target_name': 'seqalt-fc',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:fc',
      ],
    },
    {
      'target_name': 'seqalt-rd',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:rd',
      ],
    },
    {
      'target_name': 'seqalt-us-kfifo',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:us-kfifo',
      ],
    },
    {
      'target_name': 'seqalt-ll-us-kfifo',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:ll-us-kfifo',
      ],
    },
    {
      'target_name': 'seqalt-ll-dq-ms',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:ll-dq-ms',
      ],
    },
    {
      'target_name': 'seqalt-ll-dq-treiber',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:ll-dq-treiber',
      ],
    },
    {
      'target_name': 'seqalt-hc-ts-cas-stack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:hc-ts-cas-stack',
      ],
    },
    {
      'target_name': 'seqalt-hc-ts-stutter-stack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:hc-ts-stutter-stack',
      ],
    },
    {
      'target_name': 'seqalt-hc-ts-interval-stack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:hc-ts-interval-stack',
      ],
    },
    {
      'target_name': 'seqalt-hc-ts-atomic-stack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:hc-ts-atomic-stack',
      ],
    },
    {
      'target_name': 'seqalt-hc-ts-hardware-stack',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:hc-ts-hardware-stack',
      ],
    },
    {
      'target_name': 'seqalt-hc-ts-cas-queue',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:hc-ts-cas-queue',
      ],
    },
    {
      'target_name': 'seqalt-hc-ts-stutter-queue',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:hc-ts-stutter-queue',
      ],
    },
    {
      'target_name': 'seqalt-hc-ts-interval-queue',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:hc-ts-interval-queue',
      ],
    },
    {
      'target_name': 'seqalt-hc-ts-atomic-queue',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:hc-ts-atomic-queue',
      ],
    },
    {
      'target_name': 'seqalt-hc-ts-hardware-queue',
      'type': 'executable',
      'libraries': [ '<@(default_libraries)' ],
      'dependencies': [
        'libscal',
        'seqalt-base',
        'glue.gypi:hc-ts-hardware-queue',
      ],
    },
  ]
}
