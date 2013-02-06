allQueues = ['ms'
             , 'lb'
             , 'fc'
             , 'wf'
             , 'scal2random'
             , 'scal1random'
#             , 'scalrr'
#             , 'scalid'
#             , 'scaltlrr'
             , 'uskfifo'
             , 'bskfifo'
             , 'sq'
             , 'rd'
             , 'dq1random'
             , 'rp'
#             , 'bag'
             , 'ed'
#             , 'dq2random'
 #            , 'dqid'
#             , 'dqrr'
#             , 'dqtlrr'
 #            , 'dqh1random'
 #            , 'dqh2random'
#             , 'scalperm'
             , 'scal1rr'
             , 'scal2rr'
             , 'scal4rr'
#             , 'scal8rr'
#             , 'scal20rr'
#             , 'scal40rr'
#             , 'scal80rr'
             , 'scal120rr'
             ]

queues = ['scal2random', 'ms', 'fc', 'scalrr', 'uskfifo', 'lb']

executables = { 
               'scal1rr' : 'scalpartrr -partitions 1'
             , 'scal2rr' : 'scalpartrr -partitions 2'
             , 'scal4rr' : 'scalpartrr -partitions 4'
             , 'scal8rr' : 'scalpartrr -partitions 8'
             , 'scal20rr' : 'scalpartrr -partitions 20'
             , 'scal40rr' : 'scalpartrr -partitions 40'
             , 'scal80rr' : 'scalpartrr -partitions 80'
             , 'scal120rr' : 'scalpartrr -partitions 120'
             , 'scal1random' : 'scal1random -nohw_random'
             , 'scal2random' : 'scal2random -nohw_random'
             }

#hasPartials = ['scal2random', 'scalrr', 'uskfifo', 'bskfifo', 'scal1random', 'scaltlrr', 'sq', 'rd']

pParameter = {'scal2random': 'p'
             , 'scal1random' : 'p'
             , 'scalrr': 'p'
             , 'scalid': 'p'
             , 'scaltlrr' : 'p'
             , 'uskfifo' : 'k' 
             , 'bskfifo' : 'k'
             , 'sq' : 'k'
             , 'rd' : 'r'
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

