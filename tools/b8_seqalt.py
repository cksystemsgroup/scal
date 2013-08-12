import ah_config;
import seqalt;
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
queues = ['ah', 'ahstack']
works = [250, 2000]
threads = [1, 10, 20, 30, 40, 50, 60, 70, 80]
maxThreads = ah_config.maxThreadsB8


seqalt.runSeqalt(queues = queues, works = works, threads = threads, maxThreads = maxThreads, directory = outputDir, prefill = 0, performance = True)

