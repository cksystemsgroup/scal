#!/usr/bin/env python
#
# Copyright 2012 the V8 project authors. All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of Google Inc. nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os
from os.path import abspath, join, dirname, basename, exists
import re
import sys
import subprocess
import multiprocessing
from subprocess import PIPE

# Disabled LINT rules and reason.
# build/include_what_you_use: Started giving false positives for variables
#  named "string" and "map" assuming that you needed to include STL headers.

ENABLED_LINT_RULES = """
build/class
build/deprecated
build/endif_comment
build/forward_decl
build/include_order
build/printf_format
build/storage_class
legal/copyright
readability/boost
readability/braces
readability/casting
readability/check
readability/constructors
readability/fn_size
readability/function
readability/multiline_comment
readability/multiline_string
readability/streams
readability/todo
readability/utf8
runtime/arrays
runtime/casting
runtime/deprecated_fn
runtime/explicit
runtime/int
runtime/memset
runtime/mutex
runtime/nonconf
runtime/printf
runtime/printf_format
runtime/references
runtime/rtti
runtime/sizeof
runtime/string
runtime/virtual
runtime/vlog
whitespace/blank_line
whitespace/braces
whitespace/comma
whitespace/comments
whitespace/ending_newline
whitespace/indent
whitespace/labels
whitespace/line_length
whitespace/newline
whitespace/operators
whitespace/parens
whitespace/tab
whitespace/todo
""".split()


LINT_OUTPUT_PATTERN = re.compile(r'^.+[:(]\d+[:)]|^Done processing')


def CppLintWorker(command):
  try:
    process = subprocess.Popen(command, stderr=subprocess.PIPE)
    process.wait()
    out_lines = ""
    error_count = -1
    while True:
      out_line = process.stderr.readline()
      if out_line == '' and process.poll() != None:
        if error_count == -1:
          print ("Failed to process %s" % command.pop())
          return 1
        break
      m = LINT_OUTPUT_PATTERN.match(out_line)
      if m:
        out_lines += out_line
        error_count += 1
    sys.stdout.write(out_lines)
    return error_count
  except KeyboardInterrupt:
    process.kill()
  except:
    print('Error running cpplint.py. Please make sure you have depot_tools' +
          ' in your $PATH. Lint check skipped.')
    process.kill()
