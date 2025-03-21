# 1.9.13
* Distribute wheels for Python 3.13
* Support for python 3.6, 3.7, 3.8 has been dropped, as it is end-of-life

# 1.9.12
* Distribute wheels for Python 3.12
* Distribute wheels for Apple ARM

# 1.9.11
* Fixed a bug where segyio used the Delay Recording Time without it's scaling
  factor when setting up the sample offsets on open.

# 1.9.10
* Distribute wheels for Python 3.11

# 1.9.9
* Packaging updates

# 1.9.8
* Packaging updates

# 1.9.7
* Packaging updates

# 1.9.6
* gather[] selects the right offsets
* Add python3.9 packages

# 1.9.5
* The 1.9.5 release drops pre-built packages for Python 2.7.
  It will probably still build fine, but no official wheels
  will be available, as pip drops support.

# 1.9.4
* Use Py_ssize_t for type-length arguments

# 1.9.3
* The group feature has been added to docs
* Some infrastructure and CI improvements

# 1.9.2
* segyio has learned to understand rev2 1-byte header words
* segyio now understand rev2 wide header word replacements
* segyio now detects traces with >65k samples
* Support for int24 formats
* Fixed a bug where f.trace[i,j] returned an array even when i,j are ints
* SEGY_INVALID_SORTING message has been improved
* SEGY_SIGNED_INTEGER_3_BYTE enum alias for symmetry with signed counterpart
* Minor build system and CI improvements

# 1.9.1
* Text headers with null-bytes are no longer truncated
* Slowness on sparse writes is pointed out in documentation
* The python package now respects CMAKE_INSTALL_PREFIX
* Multibuild is now cloned on travis, not a submodule
* tools.wrap now works in python3, by decoding the string
* A few C compiler warnings addressed
* collections.abc is imported a future-proof way in gather.py

# 1.9.0
* segyio has learned to work with with >32k samples per trace
* collections.abc is imported a future-proof way
* Support for python 3.4 has been dropped, as it is end-of-life
* Python 3.8 support

# 1.8.8
* Build and packaging fixes

# 1.8.7
* A new feature, group, has been added with experimental support (hence no
  minor release). This is intended for pre-stack files, and is useful for
  grouping up traces by shot gathers and similar
* Segyio has learned how to read subtraces, f.trace[trno, first:last]
* Source water depth is now correctly a 4-byte word
* Fixed a bug with header-assignments in pre-stack files
* A gallery has been added to the docs, for rich examples
* Various documentation improvements, examples, and spelling corrections
* The python build system is rewritten to use scikit-build
* The exported cmake targets are namespaced with segyio::
* segyio now requires CMake >= 3.5 (from 2.8.12) to build

# 1.8.6
* Fixed a bug where negative short header words on gcc were read wrong

# 1.8.5
* segyio now reads short attributes from little-endian files correctly

# 1.8.4
* The from_array function has been refactored, and speed is improved
* create works for non-4-byte numeric formats
* More numerical formats are accepted: 8-byte/64 bit double-precision IEEE
  float, 1, 2, and 4-byte signed and unsigned integers
* segyio no longer crashes on key misses in f.gather[int, :]
* The intended TypeError is raised when traces are accessed with None, instead
  of a misleading (and wrong) AttributeError
* sample-interval has gotten some new tests
* Minor behavorial improvements to C++ test-config
* Some stack-related bug fixes in the test suite
* The readme has seen some minor updates

# 1.8.3
* Check endianness variable properly

# 1.8.2
* Pass HOST_BIG_ENDIAN as preprocessor directive, not compiler option

# 1.8.1
* Fixed a docstring formatting issue
* Fixed a race condition in test suite

# 1.8.0
* segyio has learned the seismic unix file format, and can read and write SU
  files. This feature is found in the segyio.su module
* segyio supports least-significant-byte (LSB/little-endian) first formatted
  files
* Fixed a bug that caused writing headers in non-linear mode very slow,
  and wrong under some conditions
* segyio also considers files with descending line/offset numbers sorted, not
  just ascending
* The python file handle has learned the interpret method, which allows
  specifying structure directly, rather than inferring it from file metadata.
  This is particularly useful for files with structure, but broken headers
* A new family of from_array functions are added to the tools module,
  intended for quick-and-easy storing a 2/3/4D volume on disk
* Sorting defaults to inline in segyio.create if nothing is specified
* Text.__setitem__ used a wrong variable and didn't really work, but is now
  fixed
* Internal imports are tidied up
* The test suite in both Python and C++ have seen some minor improvements
* An experimental C++ interface is available for source builds, but is not
  considered stable - internals and interface can change with no prior notice.
  The experimental interface is enabled when segyio is built with
  -DEXPERIMENTAL=ON

# 1.7.1
* Fixed an issue where writing traces on memory-mapped files was a no-op
* Depth slices dimensions are always in fast-by-slow
* catb/catr has some new output formats
* Some docs entries updated and fixed

# 1.7.0
* segyio has learned to open non-ascii file paths on Windows
* Fixed an issue where segyio-catr would sometimes print the same trace twice
* segyio no longer incorrectly considers files where the
  inline/crossline/offset triple doesn't change sorted
* The test suite has seen some overhaul, making test cases more focused on the
  actual thing to test
* Checking sorting won't silently fail when the offset-word is broken
* `f.text` is now a Sequence, meaning it has slicing and other common array
  operations
* The makefiles no longer build both static and shared libraries, but rather
  respect the BUILD_SHARED_LIBS variable
* Python 3.7 support
* Various refactoring and internal improvements

# 1.6.0
* segyio has learned to handle int16 and int32 sample formats
* segyio has learned to write any array_likes with any float-convertible
  contents. `f.trace = np.linspace(0, 1, 0.01)` works as expected.
* segyio types have been extended to be more container-like, and conform to
  more protocols. Notably, headers and lines are properly dict_like, and traces
  are properly array_like
* `trace.ref` added, a write-back mode where changes to yielded trace data are
  written back to disk
* `trace.raw` has been improved, is faster and supports setitem and the
  container protocol
* Headers have learned to accept (seismic unix) keywords in `update`, so
  `header.update(cdpx=10)` works as expected
* `f.dtype` added, this is the dtype used in all segyio-provided numpy arrays,
  and reflects the underlying data format
* `f.readonly` added, to query if file handles are writable
* segyio gracefully handle non-contiguous and more array_likes in setitem
* The type requirements of `f.samples` has been relaxed, and it now is whatever
  numpy uses as default
* segyio now assumes IBM float if the format identifier is meaningless
* Some exception types are cleaned up, notably header key misses raise
  KeyError, not IndexError
* General speed and clarity improvements, some more exception guarantees
* segyio has learned to raise an error when provided with too short data traces
* segyio has started raising deprecation warnings when using to-be-removed
  features
* The python library has overall been simplified
* Lots of new documentation has been written, the sphinx docs structure has
  been overhauled

# 1.5.3
* segyio-catr has learned to handle no arguments, and as a results read the
  first trace header as intended
* segyio-catr has its help text and man page corrected
* segyio-crop has learned to say how many traces it copies
* segyio-crop has learned to warn when no traces are copied
* segyio-crop has learned not to overwrite its input file when source and dest
  is the same
* The makefiles understand the BUILD_DOC option, to build the python docs with
  sphinx
* Test suite has some speed and robustness improvements

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
