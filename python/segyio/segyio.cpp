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
#include <sstream>
#include <stdexcept>

#if PY_MAJOR_VERSION >= 3
#define IS_PY3K
#endif

namespace {

std::string segy_errstr( int err ) {
    std::stringstream ss;

    switch( err ) {
        case SEGY_OK:                  return "segyio.ok";
        case SEGY_FOPEN_ERROR:         return "segyio.fopen";
        case SEGY_FSEEK_ERROR:         return "segyio.fseek";
        case SEGY_FREAD_ERROR:         return "segyio.fread";
        case SEGY_FWRITE_ERROR:        return "segyio.fwrite";
        case SEGY_INVALID_FIELD:       return "segyio.invalid.field";
        case SEGY_INVALID_SORTING:     return "segyio.invalid.sorting";
        case SEGY_MISSING_LINE_INDEX:  return "segyio.missing.lineindex";
        case SEGY_INVALID_OFFSETS:     return "segyio.invalid.offsets";
        case SEGY_TRACE_SIZE_MISMATCH: return "segyio.trace.size.mismatch";
        case SEGY_INVALID_ARGS:        return "segyio.invalid.args";
        case SEGY_MMAP_ERROR:          return "segyio.mmap.error";
        case SEGY_MMAP_INVALID:        return "segyio.mmap.invalid";
        case SEGY_READONLY:            return "segyio.readonly";
        case SEGY_NOTFOUND:            return "segyio.notfound";

        default:
            ss << "code " << err << "";
            return ss.str();
    }
}

template< typename T1 >
PyObject* TypeError( const char* msg, T1 t1 ) {
    return PyErr_Format( PyExc_TypeError, msg, t1 );
}

PyObject* ValueError( const char* msg ) {
    PyErr_SetString( PyExc_ValueError, msg );
    return NULL;
}

template< typename T1, typename T2 >
PyObject* ValueError( const char* msg, T1 t1, T2 t2 ) {
    return PyErr_Format( PyExc_ValueError, msg, t1, t2 );
}

PyObject* IndexError( const char* msg ) {
    PyErr_SetString( PyExc_IndexError, msg );
    return NULL;
}

template< typename T1, typename T2 >
PyObject* IndexError( const char* msg, T1 t1, T2 t2 ) {
    return PyErr_Format( PyExc_IndexError, msg, t1, t2 );
}

template< typename T1, typename T2, typename T3 >
PyObject* IndexError( const char* msg, T1 t1, T2 t2, T3 t3 ) {
    return PyErr_Format( PyExc_IndexError, msg, t1, t2, t3 );
}

PyObject* BufferError( const char* msg ) {
    PyErr_SetString( PyExc_BufferError, msg );
    return NULL;
}

PyObject* RuntimeError( const char* msg ) {
    PyErr_SetString( PyExc_RuntimeError, msg );
    return NULL;
}

PyObject* RuntimeError( int err ) {
    const std::string msg = "uncaught exception: " + segy_errstr( err );
    return RuntimeError( msg.c_str() );
}

PyObject* IOErrno() {
    return PyErr_SetFromErrno( PyExc_IOError );
}

PyObject* IOError( const char* msg ) {
    PyErr_SetString( PyExc_IOError, msg );
    return NULL;
}

template< typename T1, typename T2 >
PyObject* IOError( const char* msg, T1 t1, T2 t2 ) {
    return PyErr_Format( PyExc_IOError, msg, t1, t2 );
}

template< typename T1 >
PyObject* IOError( const char* msg, T1 t1 ) {
    return PyErr_Format( PyExc_IOError, msg, t1 );
}

template< typename T1, typename T2 >
PyObject* KeyError( const char* msg, T1 t1, T2 t2 ) {
    return PyErr_Format( PyExc_KeyError, msg, t1, t2 );
}

PyObject* Error( int err ) {
    /*
     * a default error handler. The fseek errors are sufficiently described
     * with errno, and all cases that raise fwrite and fread errors get
     * sufficient context from stack trace to be generalised with a better
     * message.
     *
     * Anything else falls through to a generic RuntimeError "uncaught
     * exception"
     */
    switch( err ) {
        case SEGY_FSEEK_ERROR: return IOErrno();
        case SEGY_FWRITE_ERROR: // fallthrough
        case SEGY_FREAD_ERROR: return IOError( "I/O operation failed, "
                                               "likely corrupted file" );
        case SEGY_READONLY:    return IOError( "file not open for writing. "
                                               "open with 'r+'" );
        default:               return RuntimeError( err );
    }
}

struct autofd {
    operator segy_file*() const;
    operator bool() const;
    void swap( autofd& other );
    void close();

    segy_file* fd;
};

autofd::operator segy_file*() const {
    if( this->fd ) return this->fd;

    IOError( "I/O operation on closed file" );
    return NULL;
}

autofd::operator bool() const { return this->fd; }

void autofd::swap( autofd& other ) { std::swap( this->fd, other.fd ); }
void autofd::close() {
    if( this->fd ) segy_close( this->fd );
    this->fd = NULL;
}

struct segyiofd {
    PyObject_HEAD
    autofd fd;
    long trace0;
    int trace_bsize;
    int tracecount;
    int samplecount;
    int format;
};

struct buffer_guard {
    /* automate Py_buffer handling.
     *
     * the python documentation does not mention any exception guarantees when
     * PyArg_ParseTuple, so assume that whenever the function fails, the buffer
     * object is either zero'd or untouched. That means checking if a
     * PyBuffer_Release should be called boils down to checking if underlying
     * buffer is a nullptr or not
     */
    buffer_guard() { Py_buffer b = {}; this->buffer = b; }
    explicit buffer_guard( const Py_buffer& b ) : buffer( b ) {}
    buffer_guard( PyObject* o, int flags = PyBUF_CONTIG_RO ) {
        Py_buffer b = {};
        this->buffer = b;

        if( !PyObject_CheckBuffer( o ) ) {
            TypeError( "'%s' does not expose buffer interface",
                       o->ob_type->tp_name );
            return;
        }

        const int cont = PyBUF_C_CONTIGUOUS;
        if( PyObject_GetBuffer( o, &this->buffer, flags | cont ) == 0 )
            return;

        if( (flags & PyBUF_WRITABLE) == PyBUF_WRITABLE )
            BufferError( "buffer must be contiguous and writable" );
        else
            BufferError( "buffer must be contiguous and readable" );
    }

    ~buffer_guard() { if( *this ) PyBuffer_Release( &this->buffer ); }

    operator bool() const  { return this->buffer.buf; }
    Py_ssize_t len() const { return this->buffer.len; }
    Py_buffer* operator&() { return &this->buffer;    }

    template< typename T >
    T*    buf() const { return static_cast< T* >( this->buffer.buf ); }
    char* buf() const { return this->buf< char >(); }

    Py_buffer buffer;
};

namespace fd {

int init( segyiofd* self, PyObject* args, PyObject* ) {
    char* filename = NULL;
    char* mode = NULL;
    int tracecount = 0;
    buffer_guard buffer;

    if( !PyArg_ParseTuple( args, "ss|iz*", &filename,
                                           &mode,
                                           &tracecount,
                                           &buffer ) )
        return -1;

    const char* binary = buffer.buf< const char >();

    if( binary && buffer.len() < SEGY_BINARY_HEADER_SIZE ) {
        ValueError( "internal: binary buffer too small, expected %i, was %zd",
                    SEGY_BINARY_HEADER_SIZE, buffer.len() );
        return -1;
    }

    if( std::strlen( mode ) == 0 ) {
        ValueError( "mode string must be non-empty" );
        return -1;
    }

    if( std::strlen( mode ) > 3 ) {
        ValueError( "invalid mode string '%s', good strings are %s",
                     mode, "'r' (read-only) and 'r+' (read-write)" );
        return -1;
    }

    struct unique : public autofd {
        explicit unique( segy_file* p ) { this->fd = p; }
        ~unique() { this->close(); }
    } fd( segy_open( filename, mode ) );

    if( !fd && !strstr( "rb" "wb" "ab" "r+b" "w+b" "a+b", mode ) ) {
        ValueError( "invalid mode string '%s', good strings are %s",
                mode, "'r' (read-only) and 'r+' (read-write)" );
        return -1;
    }

    if( !fd ) {
        IOErrno();
        return -1;
    }

    char bin[ SEGY_BINARY_HEADER_SIZE ] = {};

    if( !binary ) {
        const int err = segy_binheader( fd, bin );

        if( err ) {
            Error( err );
            return -1;
        }

        binary = bin;
    }

    const long trace0 = segy_trace0( binary );
    const int samplecount = segy_samples( binary );
    const int format = segy_format( binary );
    const int trace_bsize = segy_trace_bsize( samplecount );

    if( tracecount == 0 ) {
        const int err = segy_traces( fd, &tracecount, trace0, trace_bsize );
        switch( err ) {
            case SEGY_OK: break;

            case SEGY_FSEEK_ERROR:
                IOErrno();
                return -1;

            case SEGY_INVALID_ARGS:
                RuntimeError( "unable to count traces, "
                              "no data traces past headers" );
                return -1;

            case SEGY_TRACE_SIZE_MISMATCH:
                RuntimeError( "trace count inconsistent with file size, "
                              "trace lengths possibly of non-uniform" );
                return -1;

            default:
                Error( err );
                return -1;
        }
    }

    /*
     * init can be called multiple times, which is treated as opening a new
     * file on the same object. That means the previous file handle must be
     * properly closed before the new file is set
     */
    self->fd.swap( fd );

    self->trace0 = trace0;
    self->trace_bsize = trace_bsize;
    self->format = format;
    self->samplecount = samplecount;
    self->tracecount = tracecount;

    return 0;
}

void dealloc( segyiofd* self ) {
    self->fd.close();
    Py_TYPE( self )->tp_free( (PyObject*) self );
}

PyObject* close( segyiofd* self ) {
    /* multiple close() is a no-op */
    if( !self->fd ) return Py_BuildValue( "" );

    errno = 0;
    self->fd.close();

    if( errno ) return IOErrno();

    return Py_BuildValue( "" );
}

PyObject* flush( segyiofd* self ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    errno = 0;
    segy_flush( self->fd, false );
    if( errno ) return IOErrno();

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
    if( !PyArg_ParseTuple( args, "i", &index ) ) return NULL;

    heapbuffer buffer( segy_textheader_size() );
    if( !buffer ) return NULL;

    const int err = index == 0
              ? segy_read_textheader( fp, buffer )
              : segy_read_ext_textheader( fp, index - 1, buffer );

    if( err ) return Error( err );

    const size_t len = std::strlen( buffer );
    return PyByteArray_FromStringAndSize( buffer, len );
}

PyObject* puttext( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    int index;
    buffer_guard buffer;

    if( !PyArg_ParseTuple( args, "is*", &index, &buffer ) )
        return NULL;

    int size = std::min( int(buffer.len()), SEGY_TEXT_HEADER_SIZE );
    heapbuffer buf( SEGY_TEXT_HEADER_SIZE );
    if( !buf ) return NULL;

    const char* src = buffer.buf< const char >();
    std::copy( src, src + size, buf.ptr );

    const int err = segy_write_textheader( fp, index, buf );

    if( err ) return Error( err );

    return Py_BuildValue( "" );
}

PyObject* getbin( segyiofd* self ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    char buffer[ SEGY_BINARY_HEADER_SIZE ] = {};

    const int err = segy_binheader( fp, buffer );
    if( err ) return Error( err );

    return PyByteArray_FromStringAndSize( buffer, sizeof( buffer ) );
}

PyObject* putbin( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    buffer_guard buffer;
    if( !PyArg_ParseTuple(args, "s*", &buffer ) ) return NULL;

    if( buffer.len() < SEGY_BINARY_HEADER_SIZE )
        return ValueError( "internal: binary buffer too small, "
                           "expected %i, was %zd",
                           SEGY_BINARY_HEADER_SIZE, buffer.len() );

    const int err = segy_write_binheader( fp, buffer.buf< const char >() );

    if( err == SEGY_INVALID_ARGS )
        return IOError( "file not open for writing. open with 'r+'" );
    if( err ) return Error( err );

    return Py_BuildValue( "" );
}

PyObject* getth( segyiofd* self, PyObject *args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    int traceno;
    PyObject* bufferobj;

    if( !PyArg_ParseTuple( args, "iO", &traceno, &bufferobj ) ) return NULL;

    buffer_guard buffer( bufferobj, PyBUF_CONTIG );
    if( !buffer ) return NULL;

    if( buffer.len() < SEGY_TRACE_HEADER_SIZE )
        return ValueError( "internal: trace header buffer too small, "
                           "expected %i, was %zd",
                           SEGY_TRACE_HEADER_SIZE, buffer.len() );

    int err = segy_traceheader( fp, traceno,
                                    buffer.buf(),
                                    self->trace0,
                                    self->trace_bsize );

    switch( err ) {
        case SEGY_OK:
            Py_INCREF( bufferobj );
            return bufferobj;

        case SEGY_FREAD_ERROR:
            return IOError( "I/O operation failed on trace header %d",
                            traceno );

        default:
            return Error( err );
    }
}

PyObject* putth( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    int traceno;
    buffer_guard buf;
    if( !PyArg_ParseTuple( args, "is*", &traceno, &buf ) ) return NULL;

    if( buf.len() < SEGY_TRACE_HEADER_SIZE )
        return ValueError( "internal: trace header buffer too small, "
                           "expected %i, was %zd",
                           SEGY_TRACE_HEADER_SIZE, buf.len() );

    const char* buffer = buf.buf< const char >();

    const int err = segy_write_traceheader( fp,
                                            traceno,
                                            buffer,
                                            self->trace0,
                                            self->trace_bsize );

    switch( err ) {
        case SEGY_OK:
            return Py_BuildValue( "" );

        case SEGY_FWRITE_ERROR:
            return IOError( "I/O operation failed on trace header %d",
                            traceno );
        default:
            return Error( err );
    }
}

PyObject* field_forall( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    PyObject* bufferobj;
    int start, stop, step;
    int field;

    if( !PyArg_ParseTuple( args, "Oiiii", &bufferobj,
                                          &start,
                                          &stop,
                                          &step,
                                          &field ) )
        return NULL;

    if( step == 0 ) return ValueError( "slice step cannot be zero" );

    buffer_guard buffer( bufferobj, PyBUF_CONTIG );
    if( !buffer ) return NULL;

    const int err = segy_field_forall( fp,
                                       field,
                                       start,
                                       stop,
                                       step,
                                       buffer.buf< int >(),
                                       self->trace0,
                                       self->trace_bsize );

    if( err ) return Error( err );

    Py_INCREF( bufferobj );
    return bufferobj;
}

PyObject* field_foreach( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    PyObject* bufferobj;
    buffer_guard indices;
    int field;
    if( !PyArg_ParseTuple( args, "Os*i", &bufferobj, &indices, &field ) )
        return NULL;

    buffer_guard bufout( bufferobj, PyBUF_CONTIG );
    if( !bufout ) return NULL;

    if( bufout.len() != indices.len() )
        return ValueError( "internal: array size mismatch "
                           "(output %zd, indices %zd)",
                           bufout.len(), indices.len() );

    const int* ind = indices.buf< const int >();
    int* out = bufout.buf< int >();
    Py_ssize_t len = bufout.len() / sizeof(int);
    int err = 0;
    for( int i = 0; err == 0 && i < len; ++i ) {
        err = segy_field_forall( fp, field,
                                     ind[ i ],
                                     ind[ i ] + 1,
                                     1,
                                     out + i,
                                     self->trace0,
                                     self->trace_bsize );
    }

    if( err ) return Error( err );

    Py_INCREF( bufferobj );
    return bufferobj;
}

PyObject* metrics( segyiofd* self ) {
    static const int text = SEGY_TEXT_HEADER_SIZE;
    static const int bin  = SEGY_BINARY_HEADER_SIZE;
    const int ext = (self->trace0 - (text + bin)) / text;
    return Py_BuildValue( "{s:i, s:l, s:i, s:i, s:i, s:i}",
                          "tracecount",  self->tracecount,
                          "trace0",      self->trace0,
                          "trace_bsize", self->trace_bsize,
                          "samplecount", self->samplecount,
                          "format",      self->format,
                          "ext_headers", ext );
}

struct metrics_errmsg {
    int il, xl, of;
    PyObject* operator()( int err ) const {
        switch( err ) {
            case SEGY_INVALID_FIELD:
                return IndexError( "invalid iline, (%i), xline (%i), "
                                   "or offset (%i) field", il, xl, of );

            case SEGY_INVALID_SORTING:
                return RuntimeError( "unable to find sorting." );

            default:
                return Error( err );
        }
    }
};

PyObject* cube_metrics( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    int il;
    int xl;
    if( !PyArg_ParseTuple( args, "ii", &il, &xl ) ) return NULL;

    metrics_errmsg errmsg = { il, xl, SEGY_TR_OFFSET };

    int sorting = -1;
    int err = segy_sorting( fp, il,
                                xl,
                                SEGY_TR_OFFSET,
                                &sorting,
                                self->trace0,
                                self->trace_bsize );

    if( err ) return errmsg( err );

    int offset_count = -1;
    err = segy_offsets( fp, il,
                            xl,
                            self->tracecount,
                            &offset_count,
                            self->trace0,
                            self->trace_bsize );

    if( err ) return errmsg( err );

    int xl_count = 0;
    int il_count = 0;
    err = segy_lines_count( fp, il,
                                xl,
                                sorting,
                                offset_count,
                                &il_count,
                                &xl_count,
                                self->trace0,
                                self->trace_bsize );

    if( err == SEGY_NOTFOUND )
        return ValueError( "could not parse geometry, "
                           "open with strict=False" );

    if( err ) return errmsg( err );

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
    buffer_guard iline_out;
    buffer_guard xline_out;
    buffer_guard offset_out;

    if( !PyArg_ParseTuple( args, "O!w*w*w*", &PyDict_Type, &metrics,
                                             &iline_out,
                                             &xline_out,
                                             &offset_out ) )
        return NULL;

    const int iline_count  = getitem( metrics, "iline_count" );
    const int xline_count  = getitem( metrics, "xline_count" );
    const int offset_count = getitem( metrics, "offset_count" );

    if( iline_out.len() < Py_ssize_t(iline_count * sizeof( int )) )
        return ValueError( "internal: inline indices buffer too small, "
                           "expected %i, was %zd",
                           iline_count, iline_out.len() );

    if( xline_out.len() < Py_ssize_t(xline_count * sizeof( int )) )
        return ValueError( "internal: crossline indices buffer too small, "
                           "expected %i, was %zd",
                           xline_count, xline_out.len() );

    if( offset_out.len() < Py_ssize_t(offset_count * sizeof( int )) )
        return ValueError( "internal: offset indices buffer too small, "
                           "expected %i, was %zd",
                           offset_count, offset_out.len() );

    const int il_field     = getitem( metrics, "iline_field" );
    const int xl_field     = getitem( metrics, "xline_field" );
    const int offset_field = getitem( metrics, "offset_field" );
    const int sorting      = getitem( metrics, "sorting" );
    if( PyErr_Occurred() ) return NULL;

    metrics_errmsg errmsg = { il_field, xl_field, SEGY_TR_OFFSET };

    int err = segy_inline_indices( fp, il_field,
                                       sorting,
                                       iline_count,
                                       xline_count,
                                       offset_count,
                                       iline_out.buf< int >(),
                                       self->trace0,
                                       self->trace_bsize );
    if( err ) return errmsg( err );

    err = segy_crossline_indices( fp, xl_field,
                                      sorting,
                                      iline_count,
                                      xline_count,
                                      offset_count,
                                      xline_out.buf< int >(),
                                      self->trace0,
                                      self->trace_bsize );
    if( err ) return errmsg( err );

    err = segy_offset_indices( fp, offset_field,
                                   offset_count,
                                   offset_out.buf< int >(),
                                   self->trace0,
                                   self->trace_bsize );
    if( err ) return errmsg( err );

    return Py_BuildValue( "" );
}

PyObject* gettr( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    PyObject* bufferobj;
    int start, length, step;

    if( !PyArg_ParseTuple( args, "Oiii", &bufferobj, &start, &step, &length ) )
        return NULL;

    buffer_guard buffer( bufferobj, PyBUF_CONTIG );
    if( !buffer) return NULL;

    const int samples = self->samplecount;
    const long long bufsize = (long long) length * samples;
    const long trace0 = self->trace0;
    const int trace_bsize = self->trace_bsize;

    if( buffer.len() < bufsize )
        return ValueError( "internal: data trace buffer too small, "
                           "expected %zi, was %zd",
                            bufsize, buffer.len() );

    int err = 0;
    int i = 0;
    float* buf = buffer.buf< float >();
    for( ; err == 0 && i < length; ++i, buf += samples ) {
        err = segy_readtrace( fp, start + (i * step),
                                  buf,
                                  trace0,
                                  trace_bsize );
    }

    if( err == SEGY_FREAD_ERROR )
        return IOError( "I/O operation failed on data trace %d", i );

    if( err ) return Error( err );

    segy_to_native( self->format, bufsize, buffer.buf< float >() );

    Py_INCREF( bufferobj );
    return bufferobj;
}

PyObject* puttr( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    int traceno;
    float* buffer;
    Py_ssize_t buflen;

    if( !PyArg_ParseTuple( args, "is#", &traceno, &buffer, &buflen ) )
        return NULL;

    segy_from_native( self->format, self->samplecount, buffer );

    int err = segy_writetrace( fp, traceno,
                                   buffer,
                                   self->trace0,
                                   self->trace_bsize );

    segy_to_native( self->format, self->samplecount, buffer );

    switch( err ) {
        case SEGY_OK:
            return Py_BuildValue("");

        case SEGY_FREAD_ERROR:
            return IOError( "I/O operation failed on data trace %d", traceno );

        default:
            return Error( err );
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

    if( !PyArg_ParseTuple( args, "iiiiO", &line_trace0,
                                          &line_length,
                                          &stride,
                                          &offsets,
                                          &bufferobj ) )
        return NULL;

    buffer_guard buffer( bufferobj, PyBUF_CONTIG );
    if( !buffer ) return NULL;

    int err = segy_read_line( fp, line_trace0,
                                  line_length,
                                  stride,
                                  offsets,
                                  buffer.buf< float >(),
                                  self->trace0,
                                  self->trace_bsize);
    if( err ) return Error( err );

    segy_to_native( self->format,
                    self->samplecount * line_length,
                    buffer.buf< float >() );

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

    if( !PyArg_ParseTuple( args, "iiiO", &depth,
                                         &count,
                                         &offsets,
                                         &bufferobj ) )
        return NULL;

    buffer_guard buffer( bufferobj, PyBUF_CONTIG );
    if( !buffer ) return NULL;

    int traceno = 0;
    int err = 0;
    float* buf = buffer.buf< float >();

    const long trace0 = self->trace0;
    const int trace_bsize = self->trace_bsize;

    for( ; err == 0 && traceno < count; ++traceno) {
        err = segy_readsubtr( fp,
                              traceno * offsets,
                              depth,
                              depth + 1,
                              1,
                              buf++,
                              NULL,
                              trace0, trace_bsize);
    }

    if( err == SEGY_FREAD_ERROR )
        return IOError( "I/O operation failed on data trace %d at depth %d",
                        traceno, depth );

    if( err ) return Error( err );

    segy_to_native( self->format, count, buffer.buf< float >() );

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
        return Error( err );

    /*
     * Figure out if the problem is reading the trace header
     * or the binary header
     */
    char buffer[ SEGY_BINARY_HEADER_SIZE ];
    err = segy_binheader( fp, buffer );

    if( err )
        return IOError( "I/O operation failed on binary header, "
                        "likely corrupted file" );

    err = segy_traceheader( fp, 0, buffer, self->trace0, self->trace_bsize );

    if( err == SEGY_FREAD_ERROR )
        return IOError( "I/O operation failed on trace header 0, "
                        "likely corrupted file" );

    return Error( err );
}

PyObject* rotation( segyiofd* self, PyObject* args ) {
    segy_file* fp = self->fd;
    if( !fp ) return NULL;

    int line_length;
    int stride;
    int offsets;
    buffer_guard linenos;

    if( !PyArg_ParseTuple( args, "iiis*", &line_length,
                                          &stride,
                                          &offsets,
                                          &linenos ) )
        return NULL;

    float rotation;
    int err = segy_rotation_cw( fp, line_length,
                                    stride,
                                    offsets,
                                    linenos.buf< const int >(),
                                    linenos.len() / sizeof( int ),
                                    &rotation,
                                    self->trace0,
                                    self->trace_bsize );

    if( err ) return Error( err );

    return PyFloat_FromDouble( rotation );
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

    { "getdt",    (PyCFunction) fd::getdt, METH_VARARGS,    "Get sample interval (dt)." },
    { "rotation", (PyCFunction) fd::rotation, METH_VARARGS, "Get clockwise rotation."   },

    { "metrics",      (PyCFunction) fd::metrics,      METH_NOARGS,  "Metrics."         },
    { "cube_metrics", (PyCFunction) fd::cube_metrics, METH_VARARGS, "Cube metrics."    },
    { "indices",      (PyCFunction) fd::indices,      METH_VARARGS, "Indices."         },

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

PyObject* binsize( PyObject* ) {
    return PyLong_FromLong( segy_binheader_size() );
}

PyObject* thsize( PyObject* ) {
    return PyLong_FromLong( SEGY_TRACE_HEADER_SIZE );
}

PyObject* textsize(PyObject* ) {
    return PyLong_FromLong( SEGY_TEXT_HEADER_SIZE );
}

PyObject* trbsize( PyObject*, PyObject* args ) {
    int sample_count;
    if( !PyArg_ParseTuple( args, "i", &sample_count ) ) return NULL;
    return PyLong_FromLong( segy_trace_bsize( sample_count ) );
}

PyObject* getfield( PyObject*, PyObject *args ) {
    buffer_guard buffer;
    int field;

    if( !PyArg_ParseTuple( args, "s*i", &buffer, &field ) ) return NULL;

    if( buffer.len() != SEGY_BINARY_HEADER_SIZE &&
        buffer.len() != SEGY_TRACE_HEADER_SIZE )
        return BufferError( "buffer too small" );

    int value = 0;
    int err = buffer.len() == segy_binheader_size()
            ? segy_get_bfield( buffer.buf< const char >(), field, &value )
            : segy_get_field(  buffer.buf< const char >(), field, &value )
            ;

    switch( err ) {
        case SEGY_OK:            return PyLong_FromLong( value );
        case SEGY_INVALID_FIELD: return IndexError( "invalid field value" );
        default:                 return Error( err );
    }
}

PyObject* putfield( PyObject*, PyObject *args ) {

    buffer_guard buffer;
    int field;
    int value;
    if( !PyArg_ParseTuple( args, "w*ii", &buffer, &field, &value ) )
        return NULL;

    if( buffer.len() != SEGY_BINARY_HEADER_SIZE &&
        buffer.len() != SEGY_TRACE_HEADER_SIZE )
        return BufferError( "buffer too small" );

    int err = buffer.len() == segy_binheader_size()
            ? segy_set_bfield( buffer.buf< char >(), field, value )
            : segy_set_field(  buffer.buf< char >(), field, value )
            ;

    switch( err ) {
        case SEGY_OK:            return PyLong_FromLong( value );
        case SEGY_INVALID_FIELD: return IndexError( "invalid field value" );
        default:                 return Error( err );
    }
}

PyObject* line_metrics( PyObject*, PyObject *args) {
    SEGY_SORTING sorting;
    int trace_count;
    int inline_count;
    int crossline_count;
    int offset_count;

    if( !PyArg_ParseTuple( args, "iiiii", &sorting,
                                          &trace_count,
                                          &inline_count,
                                          &crossline_count,
                                          &offset_count ) )
        return NULL;

    int iline_length = segy_inline_length( crossline_count );
    int xline_length = segy_crossline_length( inline_count );

    int iline_stride = 0;
    int err = segy_inline_stride( sorting, inline_count, &iline_stride );

    // only check first call since the only error that can occur is
    // SEGY_INVALID_SORTING
    if( err == SEGY_INVALID_SORTING )
        return ValueError( "internal: invalid sorting." );

    if( err )
        return Error( err );

    int xline_stride;
    segy_crossline_stride( sorting, crossline_count, &xline_stride );

    return Py_BuildValue( "{s:i, s:i, s:i, s:i}",
                          "xline_length", xline_length,
                          "xline_stride", xline_stride,
                          "iline_length", iline_length,
                          "iline_stride", iline_stride );
}

PyObject* fread_trace0( PyObject* , PyObject* args ) {
    int lineno;
    int other_line_length;
    int stride;
    int offsets;
    int* indices;
    int indiceslen;
    char* linetype;

    if( !PyArg_ParseTuple( args, "iiiis#s", &lineno,
                                            &other_line_length,
                                            &stride,
                                            &offsets,
                                            &indices,
                                            &indiceslen,
                                            &linetype ) )
        return NULL;

    int trace_no = 0;
    int err = segy_line_trace0( lineno,
                                other_line_length,
                                stride,
                                offsets,
                                indices, indiceslen / sizeof( int ),
                                &trace_no );

    if( err == SEGY_MISSING_LINE_INDEX )
        return KeyError( "no such %s %d", linetype, lineno );

    if( err )
        return Error( err );

    return PyLong_FromLong( trace_no );
}

PyObject* format( PyObject* , PyObject* args ) {
    PyObject *out;
    int format;

    if( !PyArg_ParseTuple( args, "Oi", &out, &format ) ) return NULL;

    buffer_guard buffer( out, PyBUF_CONTIG );;

    const int len = buffer.len() / sizeof( float );
    segy_to_native( format, len, buffer.buf< float >() );

    Py_INCREF( out );
    return out;
}

PyMethodDef SegyMethods[] = {
    { "binsize",  (PyCFunction) binsize,  METH_NOARGS, "Size of the binary header." },
    { "thsize",   (PyCFunction) thsize,   METH_NOARGS, "Size of the trace header."  },
    { "textsize", (PyCFunction) textsize, METH_NOARGS, "Size of the text header."   },

    { "trace_bsize", (PyCFunction) trbsize, METH_VARARGS, "Size of a trace (in bytes)." },

    { "getfield", (PyCFunction) getfield, METH_VARARGS, "Get a header field." },
    { "putfield", (PyCFunction) putfield, METH_VARARGS, "Put a header field." },

    { "line_metrics", (PyCFunction) line_metrics,  METH_VARARGS, "Find the length and stride of lines." },
    { "fread_trace0", (PyCFunction) fread_trace0,  METH_VARARGS, "Find trace0 of a line."               },
    { "native",       (PyCFunction) format,        METH_VARARGS, "Convert to native float."             },

    { NULL }
};

}

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
