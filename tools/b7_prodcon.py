import ah_config;
import prodcon;
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

queues = ah_config.allQueues
#queues = ['wf']
#queues = ['scal2random']
#works = ah_config.allWorks
#works = [250, 500, 1000, 2000, 4000, 8000, 16000, 32000, 64000]
works = [250, 2000]
#works = [2000]
#threads = [1, 2, 4, 10, 20, 40] # ah_config.threadsB8
threads = [1, 4, 8, 12]
#threads = [45, 50]
#threads = [2]
maxThreads = ah_config.maxThreadsB7


prodcon.runProdcon(queues = queues, works = works, threads = threads, maxThreads = maxThreads, directory = outputDir, prefill = 0, performance = True)
