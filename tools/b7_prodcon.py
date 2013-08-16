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

#if os.path.exists(outputDir) :
#  shutil.rmtree(outputDir)
if not os.path.exists(outputDir):
  os.makedirs(outputDir)

#queues = ah_config.allQueues
queues = [ 'ahstack']
works = [250, 2000]
threads = [1, 2, 4, 6, 8, 10, 12]
maxThreads = ah_config.maxThreadsB7


prodcon.runProdcon(queues = queues, works = works, threads = threads, maxThreads = maxThreads, directory = outputDir, prefill = 0, performance = True)
