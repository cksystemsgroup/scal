allQueues = ['ms'
             , 'lb'
             , 'fc'
             , 'wf-ppopp11'
#             , 'scal2random'
             , 'dq-1random'
#             , 'scalrr'
#             , 'scalid'
#             , 'scaltlrr'
             , 'uskfifo'
             , 'bskfifo'
#             , 'sq'
             , 'rd'
#             , 'dq1random'
#             , 'rp'
#             , 'bag'
#             , 'ed'
#             , 'dq2random'
 #            , 'dqid'
#             , 'dqrr'
#             , 'dqtlrr'
 #            , 'dqh1random'
 #            , 'dqh2random'
#             , 'scalperm'
             , 'dq-1rr'
             , 'dq-2rr'
             , 'dq-4rr'
#             , 'scal8rr'
#             , 'scal20rr'
#             , 'scal40rr'
#             , 'scal80rr'
             , 'dq-120rr'
             ]

queues = ['scal2random', 'ms', 'fc', 'scalrr', 'uskfifo', 'lb']

executables = { 
               'dq-1rr' : 'dq-partrr -partitions 1'
             , 'dq-2rr' : 'dq-partrr -partitions 2'
             , 'dq-4rr' : 'dq-partrr -partitions 4'
             , 'dq-8rr' : 'dq-partrr -partitions 8'
             , 'dq-20rr' : 'dq-partrr -partitions 20'
             , 'dq-40rr' : 'dq-partrr -partitions 40'
             , 'dq-80rr' : 'dq-partrr -partitions 80'
             , 'dq-120rr' : 'dq-partrr -partitions 120'
             , 'dq-1random' : 'dq-1random -nohw_random'
             , 'scal2random' : 'scal2random -nohw_random'
             }

#hasPartials = ['scal2random', 'scalrr', 'uskfifo', 'bskfifo', 'scal1random', 'scaltlrr', 'sq', 'rd']

pParameter = {
             'dq-1random' : 'p'
             , 'uskfifo' : 'k' 
             , 'bskfifo' : 'k'
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
             , 'dq-1rr' : 'p'
             , 'dq-2rr' : 'p'
             , 'dq-4rr' : 'p'
             , 'dq-8rr' : 'p'
             , 'dq-20rr' : 'p'
             , 'dq-40rr' : 'p'
             , 'dq-80rr' : 'p'
             , 'dq-120rr' : 'p'
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

