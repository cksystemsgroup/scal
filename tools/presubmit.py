#!/usr/bin/env python2

# Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
# for details. All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.

import multiprocessing
import os
from os.path import abspath, join, dirname, basename, exists
import sys

from presubmit_v8 import CppLintWorker, ENABLED_LINT_RULES

SOURCE_EXTENSIONS = [ ".h", ".c", ".cc"]

def GitGetChangedFiles():
  f = os.popen("git diff --name-only --cached")
  files = []
  for line in f:
    files.append(line.strip())
  f.close()
  return files

def Main():
  workspace = abspath(join(dirname(sys.argv[0]), '..'))
  files = [f for f in GitGetChangedFiles() if os.path.splitext(f)[1] in SOURCE_EXTENSIONS] 
  if len(files) == 0:
    print("No changed code files detected. Skipping cpplint check.")
    sys.exit(0)

  filt = '-,' + ",".join(['+' + n for n in ENABLED_LINT_RULES])
  command = ['cpplint.py', '--filter', filt]
  local_cpplint = join(workspace, "tools", "cpplint.py")
  if exists(local_cpplint):
    command = ['python', local_cpplint, '--filter', filt]

  commands = join([command + [file] for file in files])
  count = multiprocessing.cpu_count()
  pool = multiprocessing.Pool(count)
  try:
    results = pool.map_async(CppLintWorker, commands).get(999999)
  except KeyboardInterrupt:
    print ("\nCaught KeyboardInterrupt, terminating workers.")
    sys.exit(1)

  total_errors = sum(results)
  print ("Total errors found: %d" % total_errors)
  sys.exit(total_errors == 0)

if __name__ == '__main__':
  Main()

