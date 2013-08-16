import ah_config;
import seq_enq_par_deq;
import os.path;
import shutil;
import sys;

directory = 'tmp/'

if (len(sys.argv) >= 2):
  directory = sys.argv[1]

if (not directory.endswith('/')) :
  directory = directory + '/'

outputDir = directory
if (not directory.startswith('/')) :
  outputDir = '/mnt/local_homes/ahaas/{directory}'.format(directory = directory)

if os.path.exists(outputDir) :
  shutil.rmtree(outputDir)

os.makedirs(outputDir)


#queues = [
#               'scal1rr'
#             , 'scal2rr'
#             , 'scal4rr'
#             , 'scal8rr'
#             , 'scal20rr'
#             , 'scal40rr'
#             , 'scal80rr'
#             , 'scalrr'
#             ]

#queues = ah_config.allQueues
queues = ['lb', 'ms', 'fc', 'wf', 'uskfifo', 'scal1rr']
#queues = ['scal2random']
#works = [250, 2000]
works = [0]
#threads = [1, 10, 20, 30, 40, 50, 60, 70, 80]
threads = [80]
maxThreads = ah_config.maxThreadsB8


seq_enq_par_deq.runProdcon(queues = queues, works = works, threads = threads, maxThreads = maxThreads, directory = outputDir, prefill = 0)
