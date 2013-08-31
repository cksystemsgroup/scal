allQueues = [ 'bskfifo'
             , 'ebstack'
             , 'fc'
             , 'kstack'
             , 'lb'
             , 'ms'
             , 'rd'
             , 'scal1random'
             , 'scal2random'
             , 'scal1rr'
             , 'scal2rr'
             , 'scal120rr'
             , 'sq'
             , 'tsstack'
             , 'tstack'
             , 'uskfifo'
             , 'wf'
             , 'tsstutterlist' 
             , 'tsstutterarray'
             , 'tsatomiclist'  
             , 'tsatomicarray' 
             , 'tshwlist'      
             , 'tshwarray'     
             , 'tshw2ts'     
             , 'tsqueuestutter' 
             , 'tsqueueatomic' 
             , 'tsqueuehw' 
             , 'tsqueue2ts' 
             , 'tsdequestutter' 
             , 'tsdequeatomic' 
             , 'tsdequehw' 
             , 'tsdequehw2ts' 
             , 'tsdequeshw' 
             , 'tsdequeshw2ts' 
             , 'tsdequeqhw' 
             , 'tsdequeqhw2ts' 
             ]

executables = { 
               'scal1rr' : 'dq-partrr -partitions 1'
             , 'scal2rr' : 'dq-partrr -partitions 2'
             , 'scal120rr' : 'dq-partrr -partitions 120'
             , 'scal1random' : 'dq-1random -nohw_random'
             , 'scal2random' : 'dq-2random -nohw_random'
             , 'wf' : 'wf-ppopp11'
             , 'ebstack' : 'ebstack -delay 2000'
             , 'tsstutterlist'   : 'tsstack -stutter_clock -list -noinit_threshold'
             , 'tsstutterarray'  : 'tsstack -stutter_clock -array -noinit_threshold'
             , 'tsatomiclist'    : 'tsstack -atomic_clock -list -init_threshold'
             , 'tsatomicarray'   : 'tsstack -atomic_clock -array -init_threshold'
             , 'tshwlist'        : 'tsstack -hw_clock -list -init_threshold'
             , 'tshwarray'       : 'tsstack -hw_clock -array -init_threshold'
             , 'tshw2ts'       : 'tsstack -hw_clock -2ts -init_threshold -delay 3000'
             , 'tsqueuestutter'  : 'tsqueue -stutter_clock -list'
             , 'tsqueueatomic'   : 'tsqueue -atomic_clock -list'
             , 'tsqueuehw'       : 'tsqueue -hw_clock -list'
             , 'tsqueue2ts'       : 'tsqueue -hw_clock -2ts -delay 3000'
             , 'tsdequestutter'  : 'tsdeque -list -stutter_clock -init_threshold'
             , 'tsdequeatomic'   : 'tsdeque -list -atomic_clock -init_threshold'
             , 'tsdequehw'       : 'tsdeque -list -hw_clock -init_threshold'
             , 'tsdequehw2ts'       : 'tsdeque -2ts -hw_clock -init_threshold'
             , 'tsdequeshw'       : 'tsdeque -list -hw_clock -init_threshold'
             , 'tsdequeshw2ts'       : 'tsdeque -2ts -hw_clock -init_threshold'
             , 'tsdequeqhw'       : 'tsdeque -list -hw_clock -init_threshold'
             , 'tsdequeqhw2ts'       : 'tsdeque -2ts -hw_clock -init_threshold'
             }

#hasPartials = ['scal2random', 'scalrr', 'uskfifo', 'bskfifo', 'scal1random', 'scaltlrr', 'sq', 'rd']

pParameter = {'scal2random': 'p'
             , 'scal1random' : 'p'
             , 'scalrr': 'p'
             , 'scalid': 'p'
             , 'scaltlrr' : 'p'
             , 'uskfifo' : 'k' 
             , 'bskfifo' : 'k'
             , 'kstack' : 'k'
             , 'sq' : 'k'
             , 'rd' : 'quasi_factor'
             , 'dq1random': 'p'
             , 'dq2random': 'p'
             , 'dqid': 'p'
             , 'dqrr': 'p'
             , 'dqtlrr': 'p'
             , 'dqh1random' : 'p'
             , 'dqh2random' : 'p'
             , 'scalperm' : 'p'
             , 'scal1rr' : 'p'
             , 'scal2rr' : 'p'
             , 'scal4rr' : 'p'
             , 'scal8rr' : 'p'
             , 'scal20rr' : 'p'
             , 'scal40rr' : 'p'
             , 'scal80rr' : 'p'
             , 'scal120rr' : 'p'
             }

works = [0, 1000, 2000, 4000, 8000, 16000, 32000, 64000]
allWorks = [0, 1000, 2000, 4000, 8000, 16000, 32000, 64000]

threads = [1, 2, 12, 24]

threadsB6 = [1, 2, 4, 8]
threadsB7 = [1, 2, 12, 24]
threadsB8 = [1, 2, 10, 20, 40, 80]

maxThreads=24
maxThreadsB6= 8
maxThreadsB8=80
maxThreadsB7=24

# The @ indicates the beginning of the real template. Everything before
# the @ will be cut away. This is necessary because all templates need to
# have the same placeholders.
templates = {
  'prodcon': '@../prodcon-{exe} -producers {thread} ' 
  + '-consumers {thread} -operations 10000 -c {work} -prealloc_size 500m '
  + '{partials_param} {partials} {perfParam} -noset_rt_priority '
  + '> {filename}'

  , 'enq': '@../prodcon-{exe} -producers {thread} -consumers 0 '
  + '-operations 10000 -c {work} {partials_param} {partials} '
  + '{perfParam} -noset_rt_priority -prealloc_size 250m > {filename}'

  , 'infprod': '@../prodcon-{exe} -producers {thread} -consumers {thread} '
  + '-operations 150000 -measure_at 10000 -c {work} '
  + '{partials_param} {partials} '
  + '{perfParam} -noset_rt_priority -prealloc_size 1g > {filename}'

  , 'deq': '@../prodcon-{exe} -producers {thread} -consumers {thread} '
  + '-operations 10000 -c {work} {partials_param} {partials} -barrier '
  + '{perfParam} -noset_rt_priority -prealloc_size 500m > {filename}'

  , 'seqalt' : '@../seqalt-{exe} -allow_empty_returns -threads {thread} '
  + '-elements 10000 -c {work} {partials_param} {partials} {perfParam} '
  + '-noset_rt_priority -prealloc_size 500m > {filename}'

  , 'shortest-path' : '{work}@../shortest-path-{exe} -threads {thread} '
  + '-height 100 -width 10000 {partials_param} {partials} '
  + '-noset_rt_priority -prealloc_size 1g > {filename}' 
}

filenameTemplates = {
    'prodcon': '@{directory}{queue}-t{thread}{partials_param}{partials}'
    + '-c{work}.txt'

    , 'enq': '@{directory}{queue}-t{thread}{partials_param}{partials}'
    + '-c{work}.txt'

    , 'infprod': '@{directory}{queue}-t{thread}{partials_param}{partials}'
    + '-c{work}.txt'

    , 'deq': '@{directory}{queue}-t{thread}{partials_param}{partials}'
    + '-c{work}.txt'

    , 'seqalt': '@{directory}{queue}-t{thread}{partials_param}{partials}'
    + '-c{work}.txt'

    , 'shortest-path': '{work}@{directory}{queue}-t{thread}'
    + '{partials_param}{partials}.txt'
    }
