# Scal

The Scal framework provides implementations and benchmarks for concurrent
objects.  The benchmarks show the trade-off between concurrent data structure
semantics and scalability.

Homepage: http://scal.cs.uni-salzburg.at

## Dependencies

* pkg-config
* cmake
* [googletest](https://code.google.com/p/googletest "googletest")
* [gflags](https://code.google.com/p/gflags/ "gflags")
* [google-perftools](https://code.google.com/p/gperftools/ "google-perftools")

On Ubuntu (&ge; 12.04) based systems:

    sudo apt-get install build-essential autoconf libtool google-perftools libgoogle-perftools-dev cmake libgtest-dev
    wget https://gflags.googlecode.com/files/libgflags0_2.0-1_amd64.deb
    wget https://gflags.googlecode.com/files/libgflags-dev_2.0-1_amd64.deb
    sudo dpkg -i libgflags0_2.0-1_amd64.deb
    sudo dpkg -i libgflags-dev_2.0-1_amd64.deb

A similar script is executed on our [continuous integration test.](https://drone.io/github.com/cksystemsgroup/scal/admin)

## Building [![Build Status](https://drone.io/github.com/cksystemsgroup/scal/status.png)](https://drone.io/github.com/cksystemsgroup/scal/latest)

This is as easy as

    tools/get_gyp.sh
    build/gyp/gyp --depth-. scal.gyp
    make
    BUILDTYPE=Release make

The debug and release builds reside in `out/`.

Additional data files, such as graph files, are available as submodule

    git submodule init
    git submodule update data

The resulting files reside in `data/`.

## Examples

### Producer/consumer

The most common parameters are:
* consumers: Number of consuming threads
* producers: Number of producing threads
* c: The computational workload (iterative pi calculation) between two data
  structure operations
* operations: The number of put/enqueue operations the should be performed by a
  producer

The following runs the Michael-Scott queue in a producer/consumer benchmark:

    ./prodcon-ms -producers=15 -consumers=15 -operations=100000 -c=250

And the same for the bounded-size k-FIFO queue:

    ./prodcon-bskfifo -producers=15 -consumers=15 -operations=100000 -c=250

And for Distributed Queue with a 1-random balancer:

    ./prodcon-dq-1random -producers=15 -consumers=15 -operations=100000 -c=250


Try `./prodcon-<data_structure> --help` to see the full list of available parameters.

## License

Copyright (c) 2012-2013, the Scal Project Authors.
All rights reserved. Please see the AUTHORS file for details.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies, 
either expressed or implied, of the Scal Project.
