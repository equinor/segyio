# SEGY IO

[![Travis](https://img.shields.io/travis/Statoil/segyio/master.svg?label=travis)](https://travis-ci.org/Statoil/segyio)
[![Appveyor](https://ci.appveyor.com/api/projects/status/2i5cr8ui2t9qbxk9?svg=true)](https://ci.appveyor.com/project/statoil-travis/segyio)
[![PyPI Updates](https://pyup.io/repos/github/Statoil/segyio/shield.svg)](https://pyup.io/repos/github/Statoil/segyio/)
[![Python 3](https://pyup.io/repos/github/Statoil/segyio/python-3-shield.svg)](https://pyup.io/repos/github/Statoil/segyio/)

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
 * xarray integration with netcdf_segy

## Project goals ##

Segyio does necessarily attempt to be the end-all of SEG-Y interactions;
rather, we aim to lower the barrier to interacting with SEG-Y files for
embedding, new applications or free-standing programs.

Additionally, the aim is not to support the full standard or all exotic (but
correctly) formatted files out there. Some assumptions are made, such as:

 * All traces in a file are assumed to be of the same sample size.

At this stage three different type of segy files can be read and written:

 * Post-stack 3D volumes, sorted with respect to two header words (generally INLINE and CROSSLINE)
 * Pre-stack 4D volumes, sorted with respect to three header words (generally INLINE, CROSSLINE, and OFFSET)
 * Unstructured data

The writing functionality in segyio is largely meant to *modify* or adapt
files. A file created from scratch is not necessarily a to-spec SEG-Y file, as
we only necessarily write the header fields segyio needs to make sense of the
geometry. It is still highly recommended that SEG-Y files are maintained and
written according to specification, but segyio does not mandate this.

## Getting started ##

When segyio is built and installed, you're ready to start programming! For
examples and documentation, check out the examples in the python/examples
directory.  If you're using python, pydoc is used, so fire up your favourite
python interpreter and type `help(segyio)` to get started.

### Requirements ###

To build and use segyio you need:
 * A C99 compatible C compiler (tested mostly on gcc and clang)
 * A C++ compiler for the python extension
 * [CMake](https://cmake.org/) version 2.8.8 or greater
 * [Python](https://www.python.org/) 2.7 or 3.x.

### Building ###

#### Users ####

To build and install segyio, perform the following actions in your console:

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
appropriate directory. Remember to update your $PATH! By default, only the
python bindings are built.

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

After building segyio you can run the tests with `ctest`, executed from the
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

## xarray integration ##

[Alan Richardson](https://github.com/ar4) has written a great little tool for
using [xarray](http://xarray.pydata.org/en/stable/) with segy files, which he
demos in this
[notebook](https://github.com/ar4/netcdf_segy/blob/master/notebooks/netcdf_segy.ipynb)

## Reproducing the test data ##

Small SEG Y formatted files are included in the repository for test purposes.
Phyiscally speaking the data is non-sensical, but it is reproducible by using
segyio. The tests file are located in the tests/test-data directory. To
reproduce the data file, build segyio and run the test program `make-file.py`,
`make-ps-file.py`, and `make-rotated-copies.py` as such:

```
python examples/make-file.py small.sgy 50 1 6 20 25
python examples/make-ps-file.py small-ps.sgy 10 1 5 1 4 1 3
python examples/make-rotated-copies.py small.sgy
```

If you have have small data files with a free license, feel free to submit it
to the project!


## Examples ##

### Python ###

Import useful libraries:

```python
import segyio
import numpy as np
from shutil import copyfile
```

Open segy file and inspect it:

```python
filename='name_of_your_file.sgy'
with segyio.open(filename, "r" ) as segyfile:

    # Memory map file for faster reading (especially if file is big...)
    segyfile.mmap()

    # Print binary header info
    print segyfile.bin
    print segyfile.bin[segyio.BinField.Traces]

    # Read headerword inline for trace 10
    print segyfile.header[10][segyio.TraceField.INLINE_3D]

    # Print inline and crossline axis
    print segyfile.xlines
    print segyfile.ilines
```

Read post-stack data cube contained in segy file:

```python
    # Read data along first xline
    data  = segyfile.xline[segyfile.xlines[1]]

    # Read data along last iline
    data  = segyfile.iline[segyfile.ilines[-1]]

    # Read data along 100th time slice
    data  = segyfile.depth_slice[100]

    # Read data cube
    data = segyio.tools.cube(filename)
```

Read pre-stack data cube contained in segy file:

```python
filename='name_of_your_prestack_file.sgy'
with segyio.open( filename, "r" ) as segyfile:

    # Print offsets
    print segyfile.offset

    # Read data along first iline and offset 100:  data [nxl x nt]
    data=segyfile.iline[0,100]

    # Read data along first iline and all offsets gath:  data [noff x nxl x nt]
    data=np.asarray([np.copy(x) for x in segyfile.iline[0:1,:]]

    # Read data along first 5 ilines and all offsets gath:  data [noff nil x nxl x nt]
    data=np.asarray([np.copy(x) for x in segyfile.iline[0:5,:]]

    # Read data along first xline and all offsets gath:  data [noff x nil x nt]
    data=np.asarray([np.copy(x) for x in segyfile.xline[0:1,:]])
```

Read and understand fairly 'unstructured' data (e.g., data sorted in common shot gathers):

```python
filename='name_of_your_prestack_file.sgy'
with segyio.open( filename, "r" ,ignore_geometry=True) as segyfile:
    segyfile.mmap()

    # Extract header word for all traces
    segyfile.attributes(segyio.TraceField.SourceX)[:]

    # Scatter plot sources and receivers color-coded on their number
    plt.figure()
    plt.scatter(segyfile.attributes(segyio.TraceField.SourceX)[:], segyfile.attributes(segyio.TraceField.SourceY)[:], c=segyfile.attributes(segyio.TraceField.NSummedTraces)[:],  edgecolor='none')
    plt.scatter(segyfile.attributes(segyio.TraceField.GroupX)[:],  segyfile.attributes(segyio.TraceField.GroupY)[:],  c=segyfile.attributes(segyio.TraceField.NStackedTraces)[:], edgecolor='none')
```

Write segy file using same header of another file but multiply data by *2

```python
input_file='name_of_your_input_file.sgy'
output_file='name_of_your_output_file.sgy'

copyfile(input_file, output_file)

with segyio.open( output_file, "r+" ) as src:

    # multiply data by 2
    for i in src.ilines:
        src.iline[i] = 2*src.iline[i]
```

Make segy file from sctrach

```python
spec = segyio.spec()
filename='name_of_your_file.sgy'

spec = segyio.spec()
file_out = 'test1.sgy'

spec.sorting = 2
spec.format  = 1
spec.samples = 30
spec.ilines  = np.arange(10)
spec.xlines  = np.arange(20)

with segyio.create(filename , spec) as f:

    # write the line itself to the file and the inline number in all this line's headers
    for ilno in spec.ilines:
        f.iline[ilno] = np.zeros((len(spec.xlines),spec.samples),dtype=np.single)+ilno
        f.header.iline[ilno] = { segyio.TraceField.INLINE_3D: ilno,
                                 segyio.TraceField.offset: 0
                               }

    # then do the same for xlines
    for xlno in spec.xlines:
        f.header.xline[xlno] = { segyio.TraceField.CROSSLINE_3D: xlno,
                                 segyio.TraceField.TRACE_SAMPLE_INTERVAL: 4000
                                }
```

Visualize data using sibling tool [SegyViewer](https://github.com/Statoil/segyviewer):

```python
from PyQt4.QtGui import QApplication
import segyviewlib
qapp = QApplication([])
l= segyviewlib.segyviewwidget.SegyViewWidget('filename.sgy')
l.show()
```

### MATLAB ###

```
filename='name_of_your_file.sgy'

% Inspect segy
Segy_struct=SegySpec(filename,189,193,1);

% Read headerword inline for each trace
Segy.get_header(filename,'Inline3D')

%Read data along first xline
data= Segy.readCrossLine(Segy_struct,Segy_struct.crossline_indexes(1));

%Read cube
data=Segy.get_cube(Segy_struct);

%Write segy, use same header but multiply data by *2
input_file='input_file.sgy';
output_file='output_file.sgy';
copyfile(input_file,output_file)
data = Segy.get_traces(input_file);
data1 = 2*data;
Segy.put_traces(output_file, data1);
```

## History ##
Segyio was initially written and is maintained by [Statoil
ASA](http://www.statoil.com/) as a free, simple, easy-to-use way of interacting
with seismic data that can be tailored to our needs, and as contribution to the
free software community.
