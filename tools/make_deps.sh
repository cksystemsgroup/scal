#!/bin/bash

echo "updating dependencies for scal"
echo ""
wget https://gflags.googlecode.com/files/libgflags0_2.0-1_amd64.deb
sudo dpkg -i libgflags0_2.0-1_amd64.deb
wget https://gflags.googlecode.com/files/libgflags-dev_2.0-1_amd64.deb
sudo dpkg -i libgflags-dev_2.0-1_amd64.deb

echo "gyp... "
echo ""

if [[ -d build/gyp ]]; then
  cd build/gyp
  git pull
else
  mkdir -p build/gyp
  git clone https://chromium.googlesource.com/external/gyp build/gyp
fi

if [ $? -eq 0 ]; then
  echo ""
  echo "gyp... done"
else
  exit $?
fi
