import ah_config;
import shortest_path;
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
#queues = ['wf']
#queues = ['scal2random']
#threads = [1, 2, 4, 10, 20, 40] # ah_config.threadsB8
#threads = [1, 10, 20, 30, 40, 50, 60, 70, 80]
#threads = [45, 50]
#threads = [2]
#queues = ah_config.allQueues
queues = ['lb', 'ms', 'fc', 'wf', 'uskfifo', 'scal1rr']
#queues = ['scal2random']
threads = [1, 10, 20, 30, 40, 50, 60, 70, 80]
#threads = [80]
maxThreads = ah_config.maxThreadsB8


shortest_path.runShortestPath(queues = queues, threads = threads, maxThreads = maxThreads, directory = outputDir)
