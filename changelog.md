# 2017.05
* Requirements for the shape of the right hand side of `f.trace[:] =`
  expressions are relaxed and accepts more inputs
* C interface slightly cleaned up for C99 compliance
* C library can reason on arbitrary header words for offsets, not just 37

# 2017.04
* Examples in the readme
* Delay recording time (t0) is interpreted as milliseconds, not microseconds
* Some minor optimisations
* segy_mmap is more exception safe
* The applications warn if mmap fails
* Support for static analysis with cppcheck
* The statoil/pycmake repo is used for python integration in cmake
* Minor milli/microsecond bugfixes in mex bindings
* tools.wrap added for printing textual headers to screen or file

# 2017.03
* Float conversions (ibm <-> native) has been optimised and is much faster
* `segy_binheader_size` returns signed int like its friends
* `sample_interval` steps are now floats
* Multiple internal bug fixes
* Some buffer leaks are plugged
* segyio has learned to deal with files without good geometry.
  If `strict = False` is passed to segyio.open, and a file is without
  well-sorted inlines/crosslines, open will return a file handle, but with
  geometry-dependent modes disabled
* `file.samples` returns a list of samples, not number of samples
* Readme has been improved.
* `trace[int]` is more robust w.r.t. inputs
* A new mode has been added; gather. gather depends on a good geometry, and its
  getitem `[il, xl, slice(offsets)]` returns all offsets for an
  inline/crossline intersection

# 2017.02
* segyio has learned to deal with large files (>4G) on more platforms
* segyio can read quickly attributes (trace header words) over the full file
* python can tell the fast and slow directions apart
* Reading depth slices is much faster
* tools.collect for gathering read samples into a single numpy ndarray
* tools.cube for easily reading a full cube
* tools.native for fast third-party segy-to-native float conversion
* File opening in binary mode is now enforced
* Data types have been overhauled (prefer signed integers)
* Enumerations have been SEGY prefixed to reduce collisions
* Building shared libs can be switched on/off on cmake invocation
* Makefiles and CI overhauls

# 2017.01
* Matlab has learned about prestack files
* Reading traces in matlab no longer fails when not reading the whole file
* Matlab argument keys have been renamed

# 2016.11
* Fixed some condtions where a failed write would corrupt trace data
* Fixed a memory leak bug
* VERSION string added to python
* Experimental memory-mapped file support
* Line-oriented C functions are offset aware
* Python offset property exposes offset numbers, not just count
* Support for pre-stack files, with new subindexing syntax, line[n, offset]
* Improved python repl (shell, read-eval-print-loop) support
* The widgets have color- and layout selectors

# 2016.10
* Matlab tests can optionally be turned off
* The application Segyviewer is embeddable and provided by the segyview sub
  library
* libcwrap has been replaced fully by the python C api
* OS X and experimental Windows support
* A new sub mode for traces, raw, for eager reading of trace data
