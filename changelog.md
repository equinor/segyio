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
