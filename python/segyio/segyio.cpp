#if defined(_DEBUG) && defined(_MSC_VER)
#  define _CRT_NOFORCE_MAINFEST 1
#  undef _DEBUG
#  include <Python.h>
#  include <bytesobject.h>
#  define _DEBUG 1
#else
#  include <Python.h>
#  include <bytesobject.h>
#endif

#include "segyio/segy.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

#if PY_MAJOR_VERSION >= 3
#define IS_PY3K
#endif

namespace {

struct autofd {
    explicit autofd( segy_file* p = NULL ) : fd( p ) {}
    operator segy_file*() const;
    operator bool() const;

    segy_file* fd;
};

autofd::operator segy_file*() const {
    if( this->fd ) return this->fd;

    PyErr_SetString( PyExc_IOError, "I/O operation on closed file" );
    return NULL;
}


autofd::operator bool() const { return this->fd; }

struct segyiofd {
    PyObject_HEAD
    autofd fd;
};

struct buffer_guard {
    explicit buffer_guard( Py_buffer& b ) : buffer( &b ) {}
    ~buffer_guard() { PyBuffer_Release( this->buffer ); }

    Py_buffer* buffer;
};

PyObject* TypeError( const char* msg ) {
    PyErr_SetString( PyExc_TypeError, msg );
    return NULL;
}

PyObject* ValueError( const char* msg ) {
    PyErr_SetString( PyExc_ValueError, msg );
    return NULL;
}

PyObject* IndexError( const char* msg ) {
    PyErr_SetString( PyExc_IndexError, msg );
    return NULL;
}

PyObject* BufferError( const char* msg ) {
    PyErr_SetString( PyExc_BufferError, msg );
    return NULL;
}

PyObject* RuntimeError( const char* msg ) {
    PyErr_SetString( PyExc_RuntimeError, msg );
    return NULL;
}

PyObject* IOError( const char* msg ) {
    PyErr_SetString( PyExc_IOError, msg );
    return NULL;
}

namespace fd {

int init( segyiofd* self, PyObject* args, PyObject* ) {
    char *filename = NULL;
    char *mode = NULL;
    int mode_len = 0;
    if( !PyArg_ParseTuple( args, "ss#", &filename, &mode, &mode_len ) )
        return -1;

    if( mode_len == 0 ) {
        PyErr_SetString( PyExc_ValueError, "Mode string must be non-empty" );
        return -1;
    }

    if( mode_len > 3 ) {
        PyErr_Format( PyExc_ValueError, "Invalid mode string '%s'", mode );
        return -1;
    }

    /* init can be called multiple times, which is treated as opening a new
     * file on the same object. That means the previous file handle must be
     * properly closed before the new file is set
     */
    segy_file* fd = segy_open( filename, mode );

    if( !fd && !strstr( "rb" "wb" "ab" "r+b" "w+b" "a+b", mode ) ) {
        PyErr_Format( PyExc_ValueError, "Invalid mode string '%s'", mode );
        return -1;
    }

    if( !fd ) {
        PyErr_Format( PyExc_IOError, "Unable to open file '%s'", filename );
        return -1;
    }

    if( self->fd.fd ) segy_close( self->fd.fd );
    self->fd.fd = fd;

    return 0;
}

void dealloc( segyiofd* self ) {
    if( self->fd ) segy_close( self->fd.fd );
    Py_TYPE( self )->tp_free( (PyObject*) self );
}

PyObject* close( segyiofd* self ) {
    errno = 0;

    /* multiple close() is a no-op */
    if( !self->fd ) return Py_BuildValue( "" );

    segy_close( self->fd );
    self->fd.fd = NULL;

    if( errno ) return PyErr_SetFromErrno( PyExc_IOError );

    return Py_BuildValue( "" );
}

PyObject* flush( segyiofd* self ) {
    errno = 0;

    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    segy_flush( self->fd, false );
    if( errno ) return PyErr_SetFromErrno( PyExc_IOError );

    return Py_BuildValue( "" );
}

PyObject* mmap( segyiofd* self ) {
    segy_file* fp = self->fd;

    if( !fp ) return NULL;

    const int err = segy_mmap( fp );

    if( err != SEGY_OK )
        Py_RETURN_FALSE;

    Py_RETURN_TRUE;
}

/*
 * No C++11, so no std::vector::data. single-alloc automatic heap buffer,
 * without resize
 */
struct heapbuffer {
    explicit heapbuffer( int sz ) : ptr( new( std::nothrow ) char[ sz ] ) {
        if( !this->ptr ) {
            PyErr_SetString( PyExc_MemoryError, "unable to alloc buffer" );
            return;
        }

        std::memset( this->ptr, 0, sz );
    }

    ~heapbuffer() { delete[] this->ptr; }

    operator char*()             { return this->ptr; }
    operator const char*() const { return this->ptr; }

    char* ptr;

private:
    heapbuffer( const heapbuffer& );
};

PyObject* gettext( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    int index = 0;
    if( !PyArg_ParseTuple(args, "i", &index ) ) return NULL;

    heapbuffer buffer( segy_textheader_size() );
    if( !buffer ) return NULL;

    const int error = index == 0
              ? segy_read_textheader( fp, buffer )
              : segy_read_ext_textheader( fp, index - 1, buffer );

    if( error != SEGY_OK )
        return PyErr_Format( PyExc_Exception,
                             "Could not read text header: %s", strerror(errno));

    const size_t len = std::strlen( buffer );
    return PyBytes_FromStringAndSize( buffer, len );
}

PyObject* puttext( segyiofd* self, PyObject* args ) {
    int index;
    char* buffer;
    int size;

    if( !PyArg_ParseTuple(args, "is#", &index, &buffer, &size ) )
        return NULL;

    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    size = std::min( size, SEGY_TEXT_HEADER_SIZE );
    heapbuffer buf( SEGY_TEXT_HEADER_SIZE );
    if( !buf ) return NULL;
    std::copy( buffer, buffer + size, buf.ptr );

    const int err = segy_write_textheader( fp, index, buf );

    switch( err ) {
        case SEGY_OK:
            return Py_BuildValue("");

        case SEGY_FSEEK_ERROR:
        case SEGY_FWRITE_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        case SEGY_INVALID_ARGS:
            return IndexError( "text header index out of range" );

        default:
            return PyErr_Format( PyExc_RuntimeError,
                                 "unknown error code %d", err  );
    }
}

PyObject* getbin( segyiofd* self ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    char buffer[ SEGY_BINARY_HEADER_SIZE ] = {};

    const int err = segy_binheader( fp, buffer );

    switch( err ) {
        case SEGY_OK:
            return PyBytes_FromStringAndSize( buffer, sizeof( buffer ) );

        case SEGY_FSEEK_ERROR:
        case SEGY_FREAD_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        default:
            return PyErr_Format( PyExc_RuntimeError,
                                 "unknown error code %d", err  );
    }
}

PyObject* putbin( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    const char* buffer;
    int len;
    if( !PyArg_ParseTuple(args, "s#", &buffer, &len ) ) return NULL;

    if( len < SEGY_BINARY_HEADER_SIZE )
        return ValueError( "binary header too small" );

    const int err = segy_write_binheader( fp, buffer );

    switch( err ) {
        case SEGY_OK: return Py_BuildValue("");
        case SEGY_FSEEK_ERROR:
        case SEGY_FREAD_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        default:
            return PyErr_Format( PyExc_RuntimeError,
                                 "unknown error code %d", err  );
    }
}

PyObject* getth( segyiofd* self, PyObject *args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    int traceno;
    PyObject* bufferobj;
    long trace0;
    int trace_bsize;

    if( !PyArg_ParseTuple( args, "iOli", &traceno,
                                         &bufferobj,
                                         &trace0,
                                         &trace_bsize ) )
        return NULL;

    Py_buffer buffer;
    if( !PyObject_CheckBuffer( bufferobj ) )
        return TypeError( "expected buffer object" );

    if( PyObject_GetBuffer( bufferobj, &buffer,
        PyBUF_WRITEABLE | PyBUF_C_CONTIGUOUS ) )
        return BufferError( "buffer not contiguous-writeable" );

    buffer_guard g( buffer );

    if( buffer.len < SEGY_TRACE_HEADER_SIZE )
        return PyErr_Format( PyExc_ValueError, "trace header too small" );

    char* buf = (char*)buffer.buf;
    int err = segy_traceheader( fp, traceno, buf, trace0, trace_bsize );

    switch( err ) {
        case SEGY_OK:
            Py_INCREF( bufferobj );
            return bufferobj;

        case SEGY_FSEEK_ERROR:
        case SEGY_FREAD_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        default:
            return PyErr_Format( PyExc_RuntimeError,
                                 "unknown error code %d", err  );
    }
}

PyObject* putth( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    int traceno;
    Py_buffer buf;
    long trace0;
    int trace_bsize;

    if( !PyArg_ParseTuple( args, "is*li", &traceno,
                                          &buf,
                                          &trace0,
                                          &trace_bsize ) )
        return NULL;

    buffer_guard g( buf );

    if( buf.len < SEGY_TRACE_HEADER_SIZE )
        return ValueError( "trace header too small" );

    const char* buffer = (const char*)buf.buf;

    const int err = segy_write_traceheader( fp,
                                            traceno,
                                            buffer,
                                            trace0,
                                            trace_bsize );

    switch( err ) {
        case SEGY_OK: return Py_BuildValue("");
        case SEGY_FSEEK_ERROR:
        case SEGY_FWRITE_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        default:
            return PyErr_Format( PyExc_RuntimeError,
                                 "unknown error code %d", err  );
    }
}

PyObject* field_forall( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    PyObject* bufferobj;
    int start, stop, step;
    long trace0;
    int trace_bsize;
    int field;

    if( !PyArg_ParseTuple( args, "Oiiiili", &bufferobj,
                                            &start,
                                            &stop,
                                            &step,
                                            &field,
                                            &trace0,
                                            &trace_bsize ) )
        return NULL;

    if( step == 0 ) return ValueError( "slice step cannot be zero" );

    if( !PyObject_CheckBuffer( bufferobj ) )
        return TypeError( "expected buffer object" );

    Py_buffer buffer;
    if( PyObject_GetBuffer( bufferobj, &buffer,
        PyBUF_WRITEABLE | PyBUF_C_CONTIGUOUS ) )
        return BufferError( "buffer not contiguous-writeable" );

    buffer_guard g( buffer );

    const int err = segy_field_forall( fp,
                                       field,
                                       start,
                                       stop,
                                       step,
                                       (int*)buffer.buf,
                                       trace0,
                                       trace_bsize );

    switch( err ) {
        case SEGY_OK:
            Py_INCREF( bufferobj );
            return bufferobj;

        case SEGY_FSEEK_ERROR:
        case SEGY_FREAD_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        default:
            return PyErr_Format( PyExc_RuntimeError,
                                 "unknown error code %d", err  );
    }
}

PyObject* field_foreach( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    PyObject* buffer_out;
    Py_buffer indices;
    int field;
    long trace0;
    int trace_bsize;

    if( !PyArg_ParseTuple(args, "Os*ili", &buffer_out,
                                          &indices,
                                          &field,
                                          &trace0,
                                          &trace_bsize ) )
        return NULL;

    buffer_guard gind( indices );

    Py_buffer bufout;
    if( PyObject_GetBuffer( buffer_out, &bufout, PyBUF_FORMAT
                                               | PyBUF_C_CONTIGUOUS
                                               | PyBUF_WRITEABLE ) )
        return NULL;

    buffer_guard gbuf( bufout );

    const int len = indices.len / indices.itemsize;
    if( bufout.len / bufout.itemsize != len )
        return ValueError( "attributes array length != indices" );

    const int* ind = (int*)indices.buf;
    int* out = (int*)bufout.buf;
    for( int i = 0; i < len; ++i ) {
        int err = segy_field_forall( fp, field,
                                     ind[ i ],
                                     ind[ i ] + 1,
                                     1,
                                     out + i,
                                     trace0,
                                     trace_bsize );

        if( err != SEGY_OK )
            return PyErr_SetFromErrno( PyExc_IOError );
    }

    Py_INCREF( buffer_out );
    return buffer_out;
}

PyObject* metrics( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    const char* binary;
    int len = 0;
    if( !PyArg_ParseTuple(args, "s#", &binary, &len ) ) return NULL;

    if( len < SEGY_BINARY_HEADER_SIZE )
        return ValueError( "binary header too small" );

    long trace0 = segy_trace0( binary );
    int sample_count = segy_samples( binary );
    int format = segy_format( binary );
    int trace_bsize = segy_trace_bsize( sample_count );

    int trace_count = 0;
    const int err = segy_traces( fp, &trace_count, trace0, trace_bsize );

    switch( err )  {
        case SEGY_OK: break;

        case SEGY_FSEEK_ERROR:
        case SEGY_FWRITE_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        default:
            return PyErr_Format( PyExc_RuntimeError,
                                 "unknown error code %d", err  );
    }

    return Py_BuildValue( "{s:l, s:i, s:i, s:i, s:i}",
                            "trace0", trace0,
                            "sample_count", sample_count,
                            "format", format,
                            "trace_bsize", trace_bsize,
                            "trace_count", trace_count );
}


PyObject* cube_metrics( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    int il;
    int xl;
    int trace_count;
    long trace0;
    int trace_bsize;

    if( !PyArg_ParseTuple( args, "iiili", &il,
                                          &xl,
                                          &trace_count,
                                          &trace0,
                                          &trace_bsize ) )
        return NULL;

    int sorting = -1;
    int err = segy_sorting( fp, il,
                                xl,
                                SEGY_TR_OFFSET,
                                &sorting,
                                trace0,
                                trace_bsize);

    switch( err ) {
        case SEGY_OK: break;
        case SEGY_INVALID_FIELD:
            return IndexError( "wrong field value" );

        case SEGY_FSEEK_ERROR:
        case SEGY_FREAD_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        case SEGY_INVALID_SORTING:
            return RuntimeError( "unable to find sorting. corrupted file?" );

        default:
            return PyErr_Format( PyExc_RuntimeError,
                                 "unknown error code %d", err  );
    }

    int offset_count = -1;
    err = segy_offsets( fp, il,
                            xl,
                            trace_count,
                            &offset_count,
                            trace0,
                            trace_bsize);

    switch( err ) {
        case SEGY_OK: break;
        case SEGY_INVALID_FIELD:
            return IndexError( "wrong field value" );

        case SEGY_FSEEK_ERROR:
        case SEGY_FREAD_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        default:
            return PyErr_Format( PyExc_RuntimeError,
                                 "unknown error code %d", err  );
    }

    int xl_count = 0;
    int il_count = 0;
    if( trace_count == offset_count ) {
        /*
         * handle the case where there's only one trace in the file, as it
         * doesn't make sense to count 1 line from segyio's point of view
         *
         * TODO: hande inside lines_count?
         */
        il_count = xl_count = 1;
    }
    else {
        err = segy_lines_count( fp, il,
                                    xl,
                                    sorting,
                                    offset_count,
                                    &il_count,
                                    &xl_count,
                                    trace0,
                                    trace_bsize );

        switch( err ) {
            case SEGY_OK: break;
            case SEGY_INVALID_FIELD:
                return IndexError( "wrong field value" );

            case SEGY_FSEEK_ERROR:
            case SEGY_FREAD_ERROR:
                return PyErr_SetFromErrno( PyExc_IOError );

            default:
                return PyErr_Format( PyExc_RuntimeError,
                                     "unknown error code %d", err  );
        }
    }

    return Py_BuildValue( "{s:i, s:i, s:i, s:i, s:i, s:i, s:i}",
                          "sorting",      sorting,
                          "iline_field",  il,
                          "xline_field",  xl,
                          "offset_field", 37,
                          "offset_count", offset_count,
                          "iline_count",  il_count,
                          "xline_count",  xl_count );
}

long getitem( PyObject* dict, const char* key ) {
    return PyLong_AsLong( PyDict_GetItemString( dict, key ) );
}

PyObject* indices( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    PyObject* metrics;
    Py_buffer iline_out;
    Py_buffer xline_out;
    Py_buffer offset_out;

    if( !PyArg_ParseTuple( args, "O!w*w*w*", &PyDict_Type, &metrics,
                                             &iline_out,
                                             &xline_out,
                                             &offset_out ) )
        return NULL;

    buffer_guard gil( iline_out );
    buffer_guard xil( xline_out );
    buffer_guard oil( offset_out );

    const int iline_count  = getitem( metrics, "iline_count" );
    const int xline_count  = getitem( metrics, "xline_count" );
    const int offset_count = getitem( metrics, "offset_count" );

    if( iline_out.len < Py_ssize_t(iline_count * sizeof( int )) )
        return ValueError( "inline indices buffer too small" );

    if( xline_out.len < Py_ssize_t(xline_count * sizeof( int )) )
        return ValueError( "crossline indices buffer too small" );

    if( offset_out.len < Py_ssize_t(offset_count * sizeof( int )) )
        return ValueError( "offset indices buffer too small" );

    const int il_field     = getitem( metrics, "iline_field" );
    const int xl_field     = getitem( metrics, "xline_field" );
    const int offset_field = getitem( metrics, "offset_field" );
    const int sorting      = getitem( metrics, "sorting" );
    const long trace0      = getitem( metrics, "trace0" );
    const int trace_bsize  = getitem( metrics, "trace_bsize" );
    if( PyErr_Occurred() ) return NULL;

    int err = segy_inline_indices( fp, il_field,
                                       sorting,
                                       iline_count,
                                       xline_count,
                                       offset_count,
                                       (int*)iline_out.buf,
                                       trace0,
                                       trace_bsize);
    if( err != SEGY_OK ) goto error;

    err = segy_crossline_indices( fp, xl_field,
                                      sorting,
                                      iline_count,
                                      xline_count,
                                      offset_count,
                                      (int*)xline_out.buf,
                                      trace0,
                                      trace_bsize);
    if( err != SEGY_OK ) goto error;

    err = segy_offset_indices( fp, offset_field,
                                   offset_count,
                                   (int*)offset_out.buf,
                                   trace0,
                                   trace_bsize );
    if( err != SEGY_OK ) goto error;

    return Py_BuildValue( "" );

error:
    switch( err ) {
        case SEGY_FSEEK_ERROR:
        case SEGY_FREAD_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        case SEGY_INVALID_SORTING:
            return ValueError( "invalid sorting. corrupted file?" );

        default:
            return PyErr_Format( PyExc_RuntimeError,
                                 "unknown error code %d", err  );
    }
}

PyObject* gettr( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    PyObject* bufferobj;
    int start, length, step;
    long trace0;
    int trace_bsize;
    int format;
    int samples;

    if( !PyArg_ParseTuple(args, "Oiiiiili", &bufferobj,
                                            &start,
                                            &step,
                                            &length,
                                            &format,
                                            &samples,
                                            &trace0,
                                            &trace_bsize ) )
        return NULL;

    if( !PyObject_CheckBuffer( bufferobj ) )
        return TypeError( "expected buffer object" );

    Py_buffer buffer;
    if( PyObject_GetBuffer( bufferobj, &buffer,
        PyBUF_WRITEABLE | PyBUF_C_CONTIGUOUS ) )
        return BufferError( "buffer not contiguous-writeable" );

    buffer_guard g( buffer );

    const long long bufsize = (long long) length * samples;
    if( buffer.len < bufsize )
        return ValueError( "buffer too small" );

    int err = 0;
    float* buf = (float*)buffer.buf;

    for( int i = 0; err == 0 && i < length; ++i, buf += samples ) {
        err = segy_readtrace( fp, start + (i * step),
                                  buf,
                                  trace0,
                                  trace_bsize );
    }

    switch( err ) {
        case SEGY_OK: break;

        case SEGY_FSEEK_ERROR:
        case SEGY_FREAD_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        default:
           return PyErr_Format( PyExc_RuntimeError,
                                "unknown error code %d", err  );
    }

    err = segy_to_native( format, bufsize, (float*)buffer.buf );

    if( err != SEGY_OK )
        return RuntimeError( "unable to convert to native format" );

    Py_INCREF( bufferobj );
    return bufferobj;
}

PyObject* puttr( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    int trace_no;
    float* buffer;
    Py_ssize_t buflen;
    long trace0;
    int trace_bsize;
    int format;
    int samples;

    if( !PyArg_ParseTuple( args, "is#liii", &trace_no,
                                            &buffer, &buflen,
                                            &trace0, &trace_bsize,
                                            &format, &samples ) )
        return NULL;

    int err = segy_from_native( format, samples, buffer );

    if( err != SEGY_OK )
        return RuntimeError( "unable to convert to native format" );

    err = segy_writetrace( fp, trace_no,
                               buffer,
                               trace0, trace_bsize );

    const int conv_err = segy_to_native( format, samples, buffer );

    if( conv_err != SEGY_OK )
        return RuntimeError( "unable to preserve native float format" );

    switch( err ) {
        case SEGY_OK: return Py_BuildValue("");

        case SEGY_FSEEK_ERROR:
        case SEGY_FWRITE_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        default:
            return PyErr_Format( PyExc_RuntimeError,
                                 "unknown error code %d", err  );
    }
}

PyObject* getline( segyiofd* self, PyObject* args) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    int line_trace0;
    int line_length;
    int stride;
    int offsets;
    PyObject* bufferobj;
    long trace0;
    int trace_bsize;
    int format;
    int samples;

    if( !PyArg_ParseTuple( args, "iiiiOliii", &line_trace0,
                                              &line_length,
                                              &stride,
                                              &offsets,
                                              &bufferobj,
                                              &trace0, &trace_bsize,
                                              &format, &samples ) )
        return NULL;

    if( !PyObject_CheckBuffer( bufferobj ) )
        return TypeError( "expected buffer object" );

    Py_buffer buffer;
    if( PyObject_GetBuffer( bufferobj, &buffer,
        PyBUF_WRITEABLE | PyBUF_C_CONTIGUOUS ) )
        return BufferError( "buffer not contiguous-writeable" );

    buffer_guard g( buffer );

    int err = segy_read_line( fp, line_trace0,
                                  line_length,
                                  stride,
                                  offsets,
                                  (float*)buffer.buf,
                                  trace0,
                                  trace_bsize);

    switch( err ) {
        case SEGY_OK: break;

        case SEGY_FSEEK_ERROR:
        case SEGY_FREAD_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        default:
           return PyErr_Format( PyExc_RuntimeError,
                                "unknown error code %d", err  );
    }

    err = segy_to_native( format, samples * line_length, (float*)buffer.buf );

    if( err != SEGY_OK )
        return RuntimeError( "unable to preserve native float format" );

    Py_INCREF( bufferobj );
    return bufferobj;
}

PyObject* getdepth( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    int depth;
    int count;
    int offsets;
    PyObject* bufferobj;
    long trace0;
    int trace_bsize;
    int format;
    int samples;

    if( !PyArg_ParseTuple( args, "iiiOliii", &depth,
                                             &count,
                                             &offsets,
                                             &bufferobj,
                                             &trace0, &trace_bsize,
                                             &format, &samples ) )
        return NULL;;

    if( !PyObject_CheckBuffer( bufferobj ) )
        return TypeError( "expected buffer object" );

    Py_buffer buffer;
    if( PyObject_GetBuffer( bufferobj, &buffer,
        PyBUF_WRITEABLE | PyBUF_C_CONTIGUOUS ) )
        return BufferError( "buffer not contiguous-writeable" );

    buffer_guard g( buffer );

    int trace_no = 0;
    int err = 0;
    float* buf = (float*)buffer.buf;

    for( ; err == 0 && trace_no < count; ++trace_no) {
        err = segy_readsubtr( fp,
                              trace_no * offsets,
                              depth,
                              depth + 1,
                              1,
                              buf++,
                              NULL,
                              trace0, trace_bsize);
    }

    switch( err ) {
        case SEGY_OK: break;

        case SEGY_FSEEK_ERROR:
        case SEGY_FREAD_ERROR:
            return PyErr_SetFromErrno( PyExc_IOError );

        default:
           return PyErr_Format( PyExc_RuntimeError,
                                "unknown error code %d", err  );
    }


    err = segy_to_native( format, count, (float*)buffer.buf );

    if( err != SEGY_OK )
        return RuntimeError( "unable to preserve native float format" );

    Py_INCREF( bufferobj );
    return bufferobj;
}

PyObject* getdt( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    float fallback;
    if( !PyArg_ParseTuple(args, "f", &fallback ) ) return NULL;

    float dt;
    int err = segy_sample_interval( fp, fallback, &dt );

    if( err == SEGY_OK )
        return PyFloat_FromDouble( dt );

    if( err != SEGY_FREAD_ERROR && err != SEGY_FSEEK_ERROR )
        return PyErr_Format( PyExc_RuntimeError,
                             "unknown error code %d", err  );

    /*
     * Figure out if the problem is reading the trace header
     * or the binary header
     */
    char buffer[ SEGY_BINARY_HEADER_SIZE ];
    err = segy_binheader( fp, buffer );

    if( err != SEGY_OK )
        return IOError( "unable to parse global binary header" );

    const long trace0 = segy_trace0( buffer );
    const int samples = segy_samples( buffer );
    const int trace_bsize = segy_trace_bsize( samples );
    err = segy_traceheader( fp, 0, buffer, trace0, trace_bsize );

    if( err != SEGY_OK )
        return IOError( "unable to read trace header (0)" );

    return PyErr_Format( PyExc_RuntimeError, "unknown error code %d", err  );
}



PyMethodDef methods [] = {
    { "close", (PyCFunction) fd::close, METH_VARARGS, "Close file." },
    { "flush", (PyCFunction) fd::flush, METH_VARARGS, "Flush file." },
    { "mmap",  (PyCFunction) fd::mmap,  METH_NOARGS,  "mmap file."  },

    { "gettext", (PyCFunction) fd::gettext, METH_VARARGS, "Get text header." },
    { "puttext", (PyCFunction) fd::puttext, METH_VARARGS, "Put text header." },

    { "getbin", (PyCFunction) fd::getbin, METH_VARARGS, "Get binary header." },
    { "putbin", (PyCFunction) fd::putbin, METH_VARARGS, "Put binary header." },

    { "getth", (PyCFunction) fd::getth, METH_VARARGS, "Get trace header." },
    { "putth", (PyCFunction) fd::putth, METH_VARARGS, "Put trace header." },

    { "field_forall",  (PyCFunction) fd::field_forall,  METH_VARARGS, "Field for-all."  },
    { "field_foreach", (PyCFunction) fd::field_foreach, METH_VARARGS, "Field for-each." },

    { "gettr", (PyCFunction) fd::gettr, METH_VARARGS, "Get trace." },
    { "puttr", (PyCFunction) fd::puttr, METH_VARARGS, "Put trace." },

    { "getline",  (PyCFunction) fd::getline,  METH_VARARGS, "Get line." },
    { "getdepth", (PyCFunction) fd::getdepth, METH_VARARGS, "Get depth." },

    { "getdt", (PyCFunction) fd::getdt, METH_VARARGS, "Get sample interval (dt)." },

    { "metrics",      (PyCFunction) fd::metrics,      METH_VARARGS, "Cube metrics."    },
    { "cube_metrics", (PyCFunction) fd::cube_metrics, METH_VARARGS, "Cube metrics."    },
    { "indices",      (PyCFunction) fd::indices,      METH_VARARGS, "Indices." },

    { NULL }
};

}

PyTypeObject Segyiofd = {
    PyVarObject_HEAD_INIT( NULL, 0 )
    "_segyio.segyfd",               /* name */
    sizeof( segyiofd ),             /* basic size */
    0,                              /* tp_itemsize */
    (destructor)fd::dealloc,        /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_compare */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,             /* tp_flags */
    "segyio file descriptor",       /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    fd::methods,                    /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    (initproc)fd::init,             /* tp_init */
};

}


static segy_file* get_FILE_pointer_from_capsule( PyObject* self ) {
    if( !self ) {
        PyErr_SetString( PyExc_TypeError, "The object was not of type FILE, was NULL" );
        return NULL;
    }

    if( !PyObject_TypeCheck( self, &Segyiofd ) ) {
        PyErr_SetString( PyExc_TypeError, "The object was not of type FILE" );
        return NULL;
    }

    segy_file* fp = ((segyiofd*)self)->fd;
    if( !fp ) {
        PyErr_SetString( PyExc_IOError, "I/O operation invalid on closed file" );
        return NULL;
    }

    return fp;
}

// ------------- ERROR Handling -------------
struct error_args {
    int error;
    int errno_err;
    int field_1;
    int field_2;
    int field_count;
    const char *name;
};

static PyObject *py_handle_segy_error_(struct error_args args) {
    switch (args.error) {
        case SEGY_TRACE_SIZE_MISMATCH:
            return PyErr_Format(PyExc_RuntimeError,
                                "Number of traces is not consistent with file size. File may be corrupt.");

        case SEGY_INVALID_FIELD:
            if (args.field_count == 1) {
                return PyErr_Format(PyExc_IndexError, "Field value out of range: %d", args.field_1);
            } else {
                int inline_field = args.field_1;
                int crossline_field = args.field_2;
                return PyErr_Format(PyExc_IndexError, "Invalid inline (%d) or crossline (%d) field/byte offset. "
                        "Too large or between valid byte offsets.", inline_field, crossline_field);
            }
        case SEGY_INVALID_OFFSETS:
            return PyErr_Format(PyExc_RuntimeError, "Found more offsets than traces. File may be corrupt.");

        case SEGY_INVALID_SORTING:
            return PyErr_Format(PyExc_RuntimeError, "Unable to determine sorting. File may be corrupt.");

        case SEGY_INVALID_ARGS:
            return PyErr_Format(PyExc_RuntimeError, "Input arguments are invalid.");

        case SEGY_MISSING_LINE_INDEX:
            return PyErr_Format(PyExc_KeyError, "%s number %d does not exist.", args.name, args.field_1);

        default:
            errno = args.errno_err;
            return PyErr_SetFromErrno(PyExc_IOError);
    }
}

static PyObject *py_handle_segy_error(int error, int errno_err) {
    struct error_args args;
    args.error = error;
    args.errno_err = errno_err;
    args.field_1 = 0;
    args.field_2 = 0;
    args.field_count = 0;
    args.name = "";
    return py_handle_segy_error_(args);
}

static PyObject *py_handle_segy_error_with_fields(int error, int errno_err, int field_1, int field_2, int field_count) {
    struct error_args args;
    args.error = error;
    args.errno_err = errno_err;
    args.field_1 = field_1;
    args.field_2 = field_2;
    args.field_count = field_count;
    args.name = "";
    return py_handle_segy_error_(args);
}

static PyObject *py_handle_segy_error_with_index_and_name(int error, int errno_err, int index, const char *name) {
    struct error_args args;
    args.error = error;
    args.errno_err = errno_err;
    args.field_1 = index;
    args.field_2 = 0;
    args.field_count = 1;
    args.name = name;
    return py_handle_segy_error_(args);
}

// ------------ Text Header -------------

static PyObject *py_textheader_size(PyObject *self) {
    return Py_BuildValue("i", SEGY_TEXT_HEADER_SIZE);
}

// ------------ Binary and Trace Header ------------
static char *get_header_pointer_from_capsule(PyObject *capsule, int *length) {
    if( PyBytes_Check( capsule ) ) {
        Py_ssize_t len = PyBytes_Size( capsule );
        if( len < SEGY_BINARY_HEADER_SIZE ) {
            PyErr_SetString( PyExc_TypeError, "binary header too small" );
            return NULL;
        }

        if( length ) *length = len;
        return PyBytes_AsString( capsule );
    }

    if( PyByteArray_Check( capsule ) ) {
        Py_ssize_t len = PyByteArray_Size( capsule );
        if( len < SEGY_TRACE_HEADER_SIZE ) {
            PyErr_SetString( PyExc_TypeError, "trace header too small" );
            return NULL;
        }

        if( length ) *length = len;
        return PyByteArray_AsString( capsule );
    }

    PyErr_SetString(PyExc_TypeError, "The object was not a header type");
    return NULL;
}


static PyObject *py_get_field(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *header_capsule = NULL;
    int field;

    PyArg_ParseTuple(args, "Oi", &header_capsule, &field);

    int length = 0;
    char *header = get_header_pointer_from_capsule(header_capsule, &length);

    if (PyErr_Occurred()) { return NULL; }

    int value;
    int error;
    if (length == segy_binheader_size()) {
        error = segy_get_bfield(header, field, &value);
    } else {
        error = segy_get_field(header, field, &value);
    }

    if (error == 0) {
        return Py_BuildValue("i", value);
    } else {
        return py_handle_segy_error_with_fields(error, errno, field, 0, 1);
    }
}

static PyObject *py_set_field(PyObject *self, PyObject *args) {
    errno = 0;
    PyObject *header_capsule = NULL;
    int field;
    int value;

    PyArg_ParseTuple(args, "Oii", &header_capsule, &field, &value);

    int length = 0;
    char *header = get_header_pointer_from_capsule(header_capsule, &length);

    if (PyErr_Occurred()) { return NULL; }

    int error;
    if (length == segy_binheader_size()) {
        error = segy_set_bfield(header, field, value);
    } else {
        error = segy_set_field(header, field, value);
    }

    if (error == 0) {
        return Py_BuildValue("");
    } else {
        return py_handle_segy_error_with_fields(error, errno, field, 0, 1);
    }
}

// ------------ Binary Header -------------
static PyObject *py_binheader_size(PyObject *self) {
    return Py_BuildValue("i", segy_binheader_size());
}

static PyObject *py_empty_binaryhdr(PyObject *self) {
    char buffer[ SEGY_BINARY_HEADER_SIZE ] = {};
    return PyBytes_FromStringAndSize( buffer, sizeof( buffer ) );
}

// -------------- Trace Headers ----------
static PyObject *py_empty_trace_header(PyObject *self) {
    char buffer[ SEGY_TRACE_HEADER_SIZE ] = {};
    return PyByteArray_FromStringAndSize( buffer, sizeof( buffer ) );
}

static PyObject *py_trace_bsize(PyObject *self, PyObject *args) {
    errno = 0;
    int sample_count;

    PyArg_ParseTuple(args, "i", &sample_count);

    int byte_count = segy_trace_bsize(sample_count);

    return Py_BuildValue("i", byte_count);
}

static PyObject *py_init_line_metrics(PyObject *self, PyObject *args) {
    errno = 0;
    SEGY_SORTING sorting;
    int trace_count;
    int inline_count;
    int crossline_count;
    int offset_count;

    PyArg_ParseTuple(args, "iiiii", &sorting, &trace_count, &inline_count, &crossline_count, &offset_count);

    int iline_length = segy_inline_length(crossline_count);

    int xline_length = segy_crossline_length(inline_count);

    int iline_stride;
    int error = segy_inline_stride(sorting, inline_count, &iline_stride);
    //Only check first call since the only error that can occur is SEGY_INVALID_SORTING
    if( error ) { return py_handle_segy_error( error, errno ); }

    int xline_stride;
    segy_crossline_stride(sorting, crossline_count, &xline_stride);

    PyObject *dict = PyDict_New();
    PyDict_SetItemString(dict, "xline_length", Py_BuildValue("i", xline_length));
    PyDict_SetItemString(dict, "xline_stride", Py_BuildValue("i", xline_stride));
    PyDict_SetItemString(dict, "iline_length", Py_BuildValue("i", iline_length));
    PyDict_SetItemString(dict, "iline_stride", Py_BuildValue("i", iline_stride));

    return Py_BuildValue("O", dict);
}

static PyObject *py_fread_trace0(PyObject *self, PyObject *args) {
    errno = 0;
    int lineno;
    int other_line_length;
    int stride;
    int offsets;
    PyObject *indices_object;
    char *type_name;

    PyArg_ParseTuple(args, "iiiiOs", &lineno, &other_line_length, &stride, &offsets, &indices_object, &type_name);

    Py_buffer buffer;
    if (!PyObject_CheckBuffer(indices_object)) {
        PyErr_Format(PyExc_TypeError, "The destination for %s is not a buffer object", type_name);
        return NULL;
    }
    PyObject_GetBuffer(indices_object, &buffer, PyBUF_FORMAT | PyBUF_C_CONTIGUOUS);

    int trace_no;
    int linenos_sz = PyObject_Length(indices_object);
    int error = segy_line_trace0(lineno, other_line_length, stride, offsets, (int*)buffer.buf, linenos_sz, &trace_no);
    PyBuffer_Release( &buffer );

    if (error != 0) {
        return py_handle_segy_error_with_index_and_name(error, errno, lineno, type_name);
    }

    return Py_BuildValue("i", trace_no);
}

static PyObject * py_format(PyObject *self, PyObject *args) {
    PyObject *out;
    int format;

    PyArg_ParseTuple( args, "Oi", &out, &format );

    Py_buffer buffer;
    PyObject_GetBuffer( out, &buffer,
                        PyBUF_FORMAT | PyBUF_C_CONTIGUOUS | PyBUF_WRITEABLE );

    int err = segy_to_native( format, buffer.len / buffer.itemsize, (float*)buffer.buf );

    PyBuffer_Release( &buffer );

    if( err != SEGY_OK ) {
        PyErr_SetString( PyExc_RuntimeError, "Unable to convert to native float." );
        return NULL;
    }

    Py_IncRef( out );
    return out;
}

static PyObject* py_rotation(PyObject *self, PyObject* args) {
    PyObject* file = NULL;
    int line_length;
    int stride;
    int offsets;
    PyObject* linenos;
    long trace0;
    int trace_bsize;

    PyArg_ParseTuple( args, "OiiiOli", &file,
                                       &line_length,
                                       &stride,
                                       &offsets,
                                       &linenos,
                                       &trace0,
                                       &trace_bsize );

    segy_file* fp = get_FILE_pointer_from_capsule( file );
    if( PyErr_Occurred() ) { return NULL; }

    if ( !PyObject_CheckBuffer( linenos ) ) {
        PyErr_SetString(PyExc_TypeError, "The linenos object is not a correct buffer object");
        return NULL;
    }

    Py_buffer buffer;
    PyObject_GetBuffer(linenos, &buffer, PyBUF_FORMAT | PyBUF_C_CONTIGUOUS);
    int linenos_sz = PyObject_Length( linenos );

    errno = 0;
    float rotation;
    int err = segy_rotation_cw( fp, line_length,
                                    stride,
                                    offsets,
                                    (const int*)buffer.buf,
                                    linenos_sz,
                                    &rotation,
                                    trace0,
                                    trace_bsize );
    int errn = errno;
    PyBuffer_Release( &buffer );

    if( err != 0 ) return py_handle_segy_error_with_index_and_name( err, errn, 0, "Inline" );
    return PyFloat_FromDouble( rotation );
}


/*  define functions in module */
static PyMethodDef SegyMethods[] = {
        {"binheader_size",     (PyCFunction) py_binheader_size,     METH_NOARGS,  "Return the size of the binary header."},
        {"textheader_size",    (PyCFunction) py_textheader_size,    METH_NOARGS,  "Return the size of the text header."},

        {"empty_binaryheader", (PyCFunction) py_empty_binaryhdr,    METH_NOARGS,  "Create empty binary header for a segy file."},

        {"empty_traceheader",  (PyCFunction) py_empty_trace_header, METH_NOARGS,  "Create empty trace header for a segy file."},

        {"trace_bsize",        (PyCFunction) py_trace_bsize,        METH_VARARGS, "Returns the number of bytes in a trace."},
        {"get_field",          (PyCFunction) py_get_field,          METH_VARARGS, "Get a header field."},
        {"set_field",          (PyCFunction) py_set_field,          METH_VARARGS, "Set a header field."},

        {"init_line_metrics",  (PyCFunction) py_init_line_metrics,  METH_VARARGS, "Find the length and stride of inline and crossline."},
        {"fread_trace0",       (PyCFunction) py_fread_trace0,       METH_VARARGS, "Find trace0 of a line."},
        {"native",             (PyCFunction) py_format,             METH_VARARGS, "Convert to native float."},
        {"rotation",           (PyCFunction) py_rotation,           METH_VARARGS, "Find survey clock-wise rotation in radians"},
        {NULL, NULL, 0, NULL}
};

/* module initialization */
#ifdef IS_PY3K
static struct PyModuleDef segyio_module = {
        PyModuleDef_HEAD_INIT,
        "_segyio",   /* name of module */
        NULL, /* module documentation, may be NULL */
        -1,  /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
        SegyMethods
};

PyMODINIT_FUNC
PyInit__segyio(void) {

    Segyiofd.tp_new = PyType_GenericNew;
    if( PyType_Ready( &Segyiofd ) < 0 ) return NULL;

    PyObject* m = PyModule_Create(&segyio_module);

    if( !m ) return NULL;

    Py_INCREF( &Segyiofd );
    PyModule_AddObject( m, "segyiofd", (PyObject*)&Segyiofd );

    return m;
}
#else
PyMODINIT_FUNC
init_segyio(void) {
    Segyiofd.tp_new = PyType_GenericNew;
    if( PyType_Ready( &Segyiofd ) < 0 ) return;

    PyObject* m = Py_InitModule("_segyio", SegyMethods);

    Py_INCREF( &Segyiofd );
    PyModule_AddObject( m, "segyiofd", (PyObject*)&Segyiofd );
}
#endif
