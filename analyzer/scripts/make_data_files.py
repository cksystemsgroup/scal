import os;
import sys;

def makeDataFiles(directory):

  if (not os.path.exists(directory)) or not os.path.isdir(directory):
    print ("The directory {directory} does not exist.".format(directory = directory))
    return

  filenames = os.listdir(directory)
  queues = {}
  
  for filename in filenames:

    if os.path.isfile(os.path.join(directory, filename)) :
      parts = filename.replace('.txt', '').split('-c')
     
      if len(parts) >= 2 :
        if not parts[0] in queues:
          queues[parts[0]] = []

        queues[parts[0]].append((int(parts[1]), filename))

  for queue in queues:
    queues[queue] = sorted(queues[queue])

    resultFilename = os.path.join(directory, queue + '-all.txt')

    print (resultFilename)
    if os.path.exists(resultFilename) :
      os.remove(resultFilename)

    resultFile = open(resultFilename, 'w')
    
    for parts in queues[queue]:
      data = open(os.path.join(directory, parts[1]), 'r')
      resultFile.write("" + str(parts[0]) + " " + data.read())

    resultFile.close()

if len(sys.argv) < 2:
  print ("python make_data_files.py <directory>")
else :
  makeDataFiles(sys.argv[1])
