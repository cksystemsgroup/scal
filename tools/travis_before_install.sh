#!/bin/bash

if [[ $(uname -s) = "Darwin" ]]; then
  echo "Darwin before install start"
  brew update
  brew outdated gflags || brew upgrade gflags
  brew outdated glog || brew upgrade glog
  echo "Darwin before install end"
fi

if [[ $(uname -s) = "Linux" ]]; then
  echo "Linux before install start"
  echo "Linux before install end"
fi
