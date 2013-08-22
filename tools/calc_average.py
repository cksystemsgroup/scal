import sys
import os
import shutil

def calculateAverage(filename, directories, outputDir):
  contents = []
  for directory in directories :
    inputFile = os.path.join(directory, filename)
    if not os.path.exists(inputFile):
      print('The file {filename} does not exist'.format(filename = inputFile))
      return
    f = open(inputFile, 'r')
    content = f.read()
    f.close()
    if content == '':
      print('The file {filename} is empty'.format(filename = inputFile))
      return

    contents.append(content.split(' '))

  numValues = len(contents[0])

  values = [0] * numValues

  for i in range(numValues):
    for content in contents:
      if len(content) < numValues:
        print("One of the files is invalid, not enough values: {filename}".format(filename=filename))
        return
      if content[i].isdigit():
        values[i] = values[i] + float(content[i])
    values[i] /= len(directories)

  result = ''
  for i in range(numValues):
    if contents[0][i].isdigit():
      result += str(values[i]) + ' '
    elif not '\n' in contents[0][i]:
      result += contents[0][i] + ' '

#  for value in values:
#    result += str(value) + ' '

  result += '\n'
  resultFile = open(os.path.join(outputDir, filename), 'w')

  resultFile.write(result)
  resultFile.close()

if len(sys.argv) < 3:
  print ("python calc_average.py <output_dir> [<directory>]")
 
else:

  outputDir = sys.argv[1]
  
  if os.path.exists(outputDir) :
    shutil.rmtree(outputDir)

  os.makedirs(outputDir)

  directories = []
  for i in range(2, len(sys.argv)):
    if os.path.isdir(sys.argv[i]):
      directories.append(sys.argv[i])
      print('{dir} added.'.format(dir = sys.argv[i]))
    else:
      print('{dir} is not a valid directory.'.format(dir = sys.argv[i]))

  files = os.listdir(directories[0])

  for f in files:

    calculateAverage(f, directories, outputDir)



