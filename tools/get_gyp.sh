#!/bin/bash

# Copyright (c) 2014, the Mipster Project Authors.  All rights reserved.
# Please see the AUTHORS file for details.  Use of this source code is governed
# by a BSD license that can be found in the LICENSE file.

echo "cloning gyp... "
git clone https://chromium.googlesource.com/external/gyp.git build/gyp
if [ $? -eq 0 ]; then
  echo  "done cloning gyp"
else
  exit $?
fi
