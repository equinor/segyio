# 1.5.2
* `open` and `create` handle anything string-convertible as filename argument
* pytest replaces unittest, both as library and test driver
* segyio-crop now respects the byte-offset arguments, instead of just ignoring
  them and using 189 & 193 from SEG-Y revision 1
* Some errors in readme and documentation is cleared up
* Fixed a bug in create that would trigger 16-bit integer overflow, effectively
  breaking any file with more than 65k traces.

# 1.5.0
* A bug making an external text header disappear has been fixed
* The python extension has been changed to use C++ features, simplifying code
  and dropping the use of capsules
* segyio-cath sets non-zero status code on failures
* Application testing is moved from python to cmake, giving a large speedup
* The IndexError message when accessing headers out-of-range has been improved
* Some work has been moved from python into the extension
* Error messages in python have received an overhaul
* Errors produced when memory-mapping files are made consistent with fstream
  sourced errors

# 1.4.0
* segyio has learned how to resample a file (`segyio.tools.resample`). This
  function does not actually touch the data traces, but rewrites the header
  fields and attributes required to persistently change sample rate.
  Interpolation of data traces must be done manually, but for a strict
  reinterpretation this function is sufficient
* segyio has learned to read enough structure from a file to create a new file
  with the same dimensions and lines (`segyio.tools.metadata`)
* segyio has learned to create unstructured files (only traces, no inlines or
  crosslines)
* `f.text[0] =` requires `bytes` convertability. This catches some errors that
  were previously fatal or silent
* `f.text` broken internal buffer allocations fixed
* `f.text[n] =` supports strings longer or shorter than 3200 bytes by
  truncating or padding, respectively
* Fixed a bug where a particular length of mode strings caused errors
* `segyio.open('w')` raises an exception, instead of silently truncating the
  file and failing later when the file size does not match the expected.
* `segy_traces` now fails if `trace0` is outside domain, instead of silently
  returning garbage
* Return correct size for dirty, newly-created files. This means carefully
  created new files can be `mmap`d earlier
* Methods on closed files always raise exceptions
* `mmap` support improved - all C functions are `mmap` aware.
* The file is now closed after a successful `mmap` call
* `str.format` used for string interpolation over the `%` operator
* Several potential issues found by static analysis, such as non-initialised
  temporaries, divide-by-zero code paths, and leak-errors (in process teardown)
  addressed, to reduce noise and improve safety
* Error message on failure in `segyio.tools.dt` improved
* Error message on unparsable global binary header improved
  changes for the next major release
* Catch2 is introduced to test the core C library, replacing the old
  `test/segy.c` family of tests
* Contract for `segy_traces` clarified in documentation
* Docstrings improved for `depth_slice` and `segyio.create`
* A new document, `breaking-changes.md`, lists planned deprecations and API
* The readme has gotten a makeover, with better structure, an index, and more
  examples
* `setup.py` requires setuptools >= 28. A rather recent setuptools was always a
  requirement, but not codified
* scan-build (clang analysis) enabled on Travis

# 1.3.9
* Fix OS X wheel packaging.

# 1.3.8
* Automate python ast analysis with bandit on travis
* The installed python extension is built without rpath
* The numpy minimum requirement is handled in setup.py
* The python installation layout can be configured via cmake
  e.g. -DPYTHON_INSTALL_LAYOUT=deb

# 1.3.7
* Makefiles can turn off version detection from git from env or via args

# 1.3.6
* Applications no longer spuriously ignore arguments
* All assertClose calls in tests have non-zero epsilon

# 1.3.5
* make install respects DESTDIR, also for python files

# 1.3.4
* Reading a slice in gather mode is significantly faster
* Use ftello when available to support large files on systems where long is
  32-bit
* The python extension is changed to use C++; a C++ compiler is now required,
  not just optional, when building the python extension
* Many internal and infrastructure improvements
* The python library is built with setuptools - still integrated with cmake.
  Users building from source can still do cmake && make
* Git tag is now authority on version numbers, as opposed to the version string
  recorded in the cmake file.
* General building and infrastructure improvements

# 1.3.3
* Infrastructure fixes

# 1.3.2
* Add test for segy-cath
* Fix memory double-free error in subtr functions

# 1.3.1
* Fix a typo in segyio-crop --version
* Some building improvements

# 1.3.0
* segyio is now meant to be used as proper versions, not trunk checkouts.
  changelogs from now on will be written when new versions are released, not on
  a monthly basis.
* Minor typo fixes in segyio-cath help
* Applications now come with man pages
* `header` modes handle negative indexing
* `header.update` handle any key-value iterable
* New application: segyio-catr for printing trace headers
* New application: segyio-catb for printing the binary header
* New application: segyio-crop for copying sub cubes

# 2017.06
* seismic unix-style aliases are available for python in the segyio.su
  namespace
* segyio has learned how to calculate the rotation of the cube
  (segy_rotation_cw and segyio.tools.rotation)
* The python header objects behave more as dicts as expected
* The new program segyio-cath is added, a cat-like program that concatenate
  text headers.
* Infrastructure improvements
* segyio for python is available via pypi (pip)
* segyio is now meant to be consumed with binary downloads of versions, but
  with a rapid release cycle. Releases within a major release will be backwards
  compatible
* Shared linking of the python extension is considered deprecated on Windows.

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
