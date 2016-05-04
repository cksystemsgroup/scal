#!/bin/bash

if [[ $(uname -s) = "Darwin" ]]; then
  echo "Darwin install start"
  tools/make_deps.sh
  echo "Darwin install end"
fi

if [[ $(uname -s) = "Linux" ]]; then
  echo "Linux install start"
  # We want to compile with g++ 4.8 when rather than the default g++
  #sudo apt-get install -qq gcc-4.8 g++-4.8 
  #sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 90
  
  # alternative for the 2 line above
  sudo update-alternatives --config gcc

  # try to handle these pkgs via the .travis.yml file.
  #sudo apt-get --fix-missing install google-perftools libgoogle-perftools-dev cmake libgtest-dev
  tools/make_deps.sh
  echo "Linux install end"
fi