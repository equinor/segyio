# Planned breaking changes for segyio 2

This documents describe the planned breaking changes for segyio >= 2.

## libsegyio
### removal of linkage of ebcdic2ascii and friends, and ibm2ieee

The conversion functions are linkable today, but the symbols are not exposed in
any headers. Giving these functions public linkage (for testing purposes) was a
mistake, but hopefully no users will be affected as the symbols never appears in
the header file. Affected functions:

- `ebcdic2ascii`
- `ascii2ebcdic`
- `ibm2ieee`
- `ieee2ibm`
- `segy_seek`
- `segy_ftell`

## python
### accessing closed files raises ValueError

Calling methods on closed files should not raise `IOError`, but `ValueError`,
in order to be uniform with python's own file object.

Most users shouldn't (or wouldn't) use this error for control flow or recovery,
but in order to account for it, the change is postponed.

### f.text no longer implicitly resolves to f.text[0]

The implicit accessing of the mandatory text header is inconsistent and error
prone, and won't work in segyio2. In segyio2, the only way to grab the
mandatory text header is `f.text[0]`

### str(f.text[n]) removed

The implicit string conversion should be either tied to fmt or other explicit
methods, instead of implicitly resolving into a newline-interleaved
80-linewidth string

### f.text[n] always returns 3200 bytes buffer object
Currently, the returned buffer assumes the contents are somewhat string-like,
and has some wonky behaviour when there are 0-bytes in the text headers, and
not proper whitespace paddings. In segyio2, this behaviour will be reviewed for
consistency, so that 0-bytes-in-buffers are handled gracefully, and textheaders
can be indexed into reliably and safely.
