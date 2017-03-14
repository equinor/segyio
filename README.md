# SEGY IO ![build status](https://travis-ci.org/Statoil/SegyIO.svg?branch=master "TravisCI Build Status")

## Introduction ##

Segyio is a small LGPL licensed C library for easy interaction with SEG Y
formatted seismic data, with language bindings for Python and Matlab. Segyio is
an attempt to create an easy-to-use, embeddable, community-oriented library for
seismic applications. Features are added as they are needed; suggestions and
contributions of all kinds are very welcome.

## Feature summary ##
 * A low-level C interface with few assumptions; easy to bind to other
   languages.
 * Read and write binary and textual headers.
 * Read and write traces, trace headers.
 * Easy to use and native-feeling python interface with numpy integration.

## Project goals ##

Segyio does necessarily attempt to be the end-all of SEG-Y interactions;
rather, we aim to lower the barrier to interacting with SEG-Y files for
embedding, new applications or free-standing programs.

Additionally, the aim is not to support the full standard or all exotic (but
correctly) formatted files out there. Some assumptions are made, such as:

 * All traces in a file are assumed to be of the same sample size.
 * It is assumed all lines have the same number of traces.

The writing functionality in Segyio is largely meant to *modify* or adapt
files. A file created from scratch is not necessarily a to-spec SEG-Y file, as
we only necessarily write the header fields segyio needs to make sense of the
geometry. It is still highly recommended that SEG-Y files are maintained and
written according to specification, but segyio does not mandate this.

## Getting started ##

When Segyio is built and installed, you're ready to start programming! For
examples and documentation, check out the examples in the python/examples
directory.  If you're using python, pydoc is used, so fire up your favourite
python interpreter and type `help(segyio)` to get started.

### Requirements ###

To build and use Segyio you need:
 * A C99 compatible C compiler (tested mostly on gcc and clang)
 * [CMake](https://cmake.org/) version 2.8.8 or greater
 * [Python](https://www.python.org/) 2.7 or 3.x.

### Building ###

#### Users ####

To build and install Segyio, perform the following actions in your console:

```
git clone https://github.com/Statoil/segyio
cd segyio
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
make
make install
```

Make install must be done as root for a system install; if you want to install
in your home directory, add `-DCMAKE_INSTALL_PREFIX=~/` or some other
appropriate directory. Remember to update your $PATH!

##### Matlab support #####

To build the matlab bindings, invoke CMake with the option `-DBUILD_MEX=ON`. In
some environments the Matlab binaries are in a non-standard location, in which
case you need to help CMake find the matlab binaries by passing
`-DMATLAB_ROOT=/path/to/matlab`.

#### Developers ####

It's recommended to build in debug mode to get more warnings and to embed debug
symbols in the objects. Substituting `Debug` for `Release` in the
`CMAKE_BUILD_TYPE` is plenty.

Tests are located in the language/tests directories, and it's highly
recommended that new features added are demonstrated for correctness and
contract by adding a test. Feel free to use the tests already written as a
guide.

After building Segyio you can run the tests with `ctest`, executed from the
build directory.

Please note that to run the python examples you need to let your environment know
where to find the segyio python library. On linux (bash) this is accomplished
by being in the build directory and executing:
`export PYTHONPATH=$PWD/python:$PYTHONPATH`

## Contributing ##

We welcome all kinds of contributions, including code, bug reports, issues,
feature requests and documentation. The preferred way of submitting a
contribution is to either make an
[issue](https://github.com/Statoil/SegyIO/issues) on github or by forking the
project on github and making a pull request.

## Reproducing the test data ##

Small SEG Y formatted files are included in the repository for test purposes.
Phyiscally speaking the data is non-sensical, but it is reproducible by using
Segyio. The tests file are located in the tests/test-data directory. To
reproduce the data file, build Segyio and run the test program `make-file.py`
and `make-ps-file.py` as such:

```
python examples/make-file.py small.sgy 50 1 6 20 25
python examples/make-ps-file.py small-ps.sgy 10 1 5 1 4 1 3
```

If you have have small data files with a free license, feel free to submit it
to the project!

## History ##
Segyio was initially written and is maintained by [Statoil
ASA](http://www.statoil.com/) as a free, simple, easy-to-use way of interacting
with seismic data that can be tailored to our needs, and as contribution to the
free software community.
