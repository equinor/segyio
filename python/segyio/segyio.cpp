#define PY_SSIZE_T_CLEAN
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

#include <segyio/segy.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#if PY_MAJOR_VERSION >= 3
#define IS_PY3K
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define IS_GCC
#endif
#if defined(__clang__)
#define IS_CLANG
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

template< typename... Args >
PyObject* ValueError( const char* msg, Args... args ) {
    return PyErr_Format( PyExc_ValueError, msg, args... );
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

template< typename... Args >
PyObject* RuntimeError( const char* msg, Args... args ) {
    return PyErr_Format( PyExc_RuntimeError, msg, args... );
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

template< typename T1 >
PyObject* KeyError( const char* msg, T1 t1 ) {
    return PyErr_Format( PyExc_KeyError, msg, t1 );
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
        case SEGY_DS_FLUSH_ERROR:
                               return IOError( "Datasource flush failed" );
        case SEGY_DS_CLOSE_ERROR:
                               return IOError( "Datasource close failed" );
        default:               return RuntimeError( err );
    }
}


namespace ds {

int py_read( segy_datasource* self, void* buffer, size_t size ) {
    int err = SEGY_DS_READ_ERROR;
    PyObject* stream = (PyObject*)self->stream;

    PyObject* result = PyObject_CallMethod( stream, "read", "n", size );
    if( !result ) {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        return err;
    }
    const char* data = PyBytes_AsString( result );
    if( data ) {
        Py_ssize_t result_size = PyBytes_Size( result );
        if( static_cast<size_t>( result_size ) == size ) {
            memcpy( buffer, data, size );
            err = SEGY_OK;
        }
    }

    Py_DECREF( result );
    return err;
}

int py_write( segy_datasource* self, const void* buffer, size_t size ) {
    int err = SEGY_DS_WRITE_ERROR;
    PyObject* stream = (PyObject*)self->stream;

    PyObject* result = PyObject_CallMethod( stream, "write", "y#", buffer, size );
    if( !result ) {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        return err;
    }

    Py_ssize_t result_size = PyLong_AsSsize_t( result );
    if( static_cast<size_t>( result_size ) == size ) {
        err = SEGY_OK;
    }
    Py_DECREF( result );
    return err;
}

int py_seek( segy_datasource* self, long long offset, int whence ) {
    PyObject* stream = (PyObject*)self->stream;
    PyObject* result = PyObject_CallMethod( stream, "seek", "Li", offset, whence );
    if( !result ) {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        return SEGY_DS_SEEK_ERROR;
    }
    Py_DECREF( result );
    return SEGY_OK;
}

int py_tell( segy_datasource* self, long long* pos ) {
    PyObject* stream = (PyObject*)self->stream;
    PyObject* result = PyObject_CallMethod( stream, "tell", NULL );
    if( !result ) {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        return SEGY_DS_ERROR;
    }
    *pos = PyLong_AsLongLong( result );
    Py_DECREF( result );
    return SEGY_OK;
}

int py_size( segy_datasource* self, long long* out ) {
    long long original_tell;
    int err = py_tell( self, &original_tell );
    if( err != SEGY_OK ) return err;

    err = py_seek( self, 0, SEEK_END );
    if( err != SEGY_OK ) return err;

    err = py_tell( self, out );
    if( err != SEGY_OK ) return err;

    err = py_seek( self, original_tell, SEEK_SET );
    if( err != SEGY_OK ) return err;
    return SEGY_OK;
}

int py_flush( segy_datasource* self ) {
    PyObject* stream = (PyObject*)self->stream;
    PyObject* result = PyObject_CallMethod( stream, "flush", NULL );
    if( !result ) {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        return SEGY_DS_FLUSH_ERROR;
    }
    Py_DECREF( result );
    return SEGY_OK;
}

int py_close( segy_datasource* self ) {
    PyObject* stream = (PyObject*)self->stream;
    PyObject* result = PyObject_CallMethod( stream, "close", NULL );
    if( !result ) {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        return SEGY_DS_CLOSE_ERROR;
    }
    Py_DECREF( result );
    Py_DECREF( stream );
    return SEGY_OK;
}

int py_set_writable( segy_datasource* self ) {
    PyObject* stream = (PyObject*)self->stream;
    PyObject* result = PyObject_CallMethod( stream, "writable", NULL );
    if( !result ) {
        if( PyErr_Occurred() ) {
            PyErr_Print();
        }
        return SEGY_DS_ERROR;
    }
    self->writable = PyObject_IsTrue( result );
    Py_DECREF( result );
    return SEGY_OK;
}

static void init_traceheader_mapping(
    segy_header_mapping* dst_mapping,
    const uint8_t* src_name_map,
    const segy_entry_definition* src_entry_definition_map,
    const char* name
) {
    memcpy( dst_mapping->name, name, 8 );
    memcpy(
        dst_mapping->name_to_offset,
        src_name_map,
        sizeof( dst_mapping->name_to_offset )
    );
    memcpy(
        dst_mapping->offset_to_entry_definition,
        src_entry_definition_map,
        sizeof( dst_mapping->offset_to_entry_definition )
    );
}

segy_datasource* create_py_stream_datasource(
    PyObject* py_stream, bool minimize_requests_number
) {
    segy_datasource* ds = (segy_datasource*)malloc( sizeof( segy_datasource ) );
    if( !ds ) return NULL;

    /* current requirements on file-like-object stream: read, write, seek, tell,
     * flush, close, writable
     */
    ds->stream = py_stream;

    ds->read = py_read;
    ds->write = py_write;
    ds->seek = py_seek;
    ds->tell = py_tell;
    ds->size = py_size;
    ds->flush = py_flush;
    ds->close = py_close;

    // writable is set only on init, assuming stream does not change it during
    // operation
    const int err = py_set_writable( ds );
    if( err ) return NULL;

    ds->memory_speedup = false;
    ds->minimize_requests_number = minimize_requests_number;

    ds->metadata.endianness = SEGY_MSB;
    ds->metadata.encoding = SEGY_EBCDIC;
    ds->metadata.format = SEGY_IBM_FLOAT_4_BYTE;
    ds->metadata.elemsize = 4;
    ds->metadata.ext_textheader_count = 0;
    ds->metadata.trace0 = -1;
    ds->metadata.samplecount = -1;
    ds->metadata.trace_bsize = -1;
    ds->metadata.traceheader_count = 1;
    ds->metadata.tracecount = -1;

    init_traceheader_mapping(
        &ds->traceheader_mapping_standard,
        segy_traceheader_default_name_map(),
        segy_traceheader_default_map(),
        "SEG00000"
    );

    init_traceheader_mapping(
        &ds->traceheader_mapping_extension1,
        segy_ext1_traceheader_default_name_map(),
        segy_ext1_traceheader_default_map(),
        "SEG00001"
    );

    /* keep additional reference to assure object does not get deleted before
     * segy_datasource is closed
     */
    Py_INCREF( py_stream );
    return ds;
}

} // namespace ds

struct stanza_header {
    std::string name;
    int headerindex;
    int headercount;

    stanza_header() = default;
    stanza_header( std::string n, int i, int c )
        : name( std::move( n ) ),
          headerindex( i ),
          headercount( c ) {}

    std::string normalized_name() const {
        std::string normalized;
        for( auto c : this->name ) {
            if( c != ' ' ) {
                normalized += std::tolower( static_cast<unsigned char>( c ) );
            }
        }
        return normalized;
    }

    int end_index() const { return headerindex + headercount; }

    size_t header_length() const {
        const size_t parentheses_count = 4;
        return this->name.size() + parentheses_count;
    }

    size_t data_size() const {
        return this->headercount * SEGY_TEXT_HEADER_SIZE - this->header_length();
    }
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

struct autods {
    autods() : ds( nullptr ), memory_ds_buffer() {}

    ~autods() {
        this->close();
    }

    operator segy_datasource*() const;
    operator bool() const;
    void swap( autods& other );
    int close();

    segy_datasource* ds;
    // only for memory datasource. Field is not directly used (it's .buf member
    // is), it's only purpose is to stay alive so that .buf is available for the
    // whole lifetime of segyfd.
    Py_buffer memory_ds_buffer;
};

autods::operator segy_datasource*() const {
    if( this->ds ) return this->ds;

    ValueError( "I/O operation on closed datasource" );
    return NULL;
}

autods::operator bool() const { return this->ds; }

void autods::swap( autods& other ) {
    std::swap( this->ds, other.ds );
    std::swap( this->memory_ds_buffer, other.memory_ds_buffer );
}

int autods::close() {
    int err = SEGY_OK;
    if( this->ds ) {
        err = segy_close( this->ds );
    }
    this->ds = NULL;
    if ( this->memory_ds_buffer.buf ) {
        PyBuffer_Release( &this->memory_ds_buffer );
        this->memory_ds_buffer = Py_buffer();
    }
    return err;
}

struct segyfd {
    PyObject_HEAD
    autods ds;
    unsigned long long trace0;
    int trace_bsize;
    int tracecount;
    int traceheader_count;
    int samplecount;
    int format;
    int elemsize;

    std::vector<stanza_header> stanzas;
    std::vector<segy_header_mapping> traceheader_mappings;
};

namespace {
/** Parse extended text headers, find out their number and determine the list of
 * stanza headers. Updates stanzas field. */
int parse_extended_text_headers( segyfd* self );

/** Frees segy_header_mapping names allocated on heap. */
int free_header_mappings_names( segy_header_mapping* mappings, size_t mappings_length );

/** Updates traceheader_mappings field from provided xml_stanza_data and user
 * defined iline/xline. If required mappings are not found, default are used.
 * Returns SEGY_OK if requested mapping override was successful, sets error and
 * returns error code otherwise.*/
int set_traceheader_mappings(
    segyfd* self,
    std::vector<char>& xml_stanza_data,
    PyObject* py_iline,
    PyObject* py_xline
);

/** Extracts xml into layout_stanza_data vector from the stanza with name
 * "seg:layout", regardless of how many extended headers it takes. Returns
 * SEGY_OK both when data was successfully extracted or not found. */
int extract_layout_stanza( segyfd* self, std::vector<char>& layout_stanza_data );

/**
 * Returns the dictionary {header_name: TraceHeaderLayout.}
 */
PyObject* traceheader_layouts( segyfd* self );

} // namespace

namespace fd {

int init( segyfd* self, PyObject* args, PyObject* kwargs ) {
    char* filename = NULL;
    char* mode = NULL;
    PyObject* stream = NULL;
    PyObject* memory_buffer = NULL;
    int minimize_requests_number = -1;

    static const char* keywords[] = {
        "filename",
        "mode",
        "stream",
        "memory_buffer",
        "minimize_requests_number",
        NULL
    };

    if( !PyArg_ParseTupleAndKeywords(
            args, kwargs, "|ssOOp",
            const_cast<char**>( keywords ),
            &filename,
            &mode,
            &stream,
            &memory_buffer,
            &minimize_requests_number
        ) ) {
        ValueError( "could not parse arguments" );
        return -1;
    }

    autods ds;
    if( stream ) {
        if (stream == Py_None) {
            ValueError( "stream must not be None" );
            return -1;
        }
        if( minimize_requests_number < 0 ) {
            ValueError( "minimize requests number is not set" );
            return -1;
        }
        ds.ds = ds::create_py_stream_datasource(
            stream, minimize_requests_number
        );
    } else if( memory_buffer ) {
        if (memory_buffer == Py_None) {
            ValueError( "buffer must not be None" );
            return -1;
        }
        auto flags = PyBUF_CONTIG | PyBUF_WRITABLE;
        const int err = PyObject_GetBuffer( memory_buffer, &ds.memory_ds_buffer, flags );
        if( err ) {
            ValueError( "Could not export buffer of requested type" );
            return -1;
        }
        ds.ds = segy_memopen(
            static_cast<unsigned char*>( ds.memory_ds_buffer.buf ),
            ds.memory_ds_buffer.len
        );
    } else if( filename && mode ) {
        if( std::strlen( mode ) == 0 ) {
            ValueError( "mode string must be non-empty" );
            return -1;
        }

        if( std::strlen( mode ) > 3 ) {
            ValueError( "invalid mode string '%s', good strings are %s",
                            mode, "'r' (read-only) and 'r+' (read-write)" );
            return -1;
        }

        segy_datasource* file = segy_open( filename, mode );
        if( !file && !strstr( "rb" "wb" "ab" "r+b" "w+b" "a+b", mode ) ) {
            ValueError( "invalid mode string '%s', good strings are %s",
                    mode, "'r' (read-only) and 'r+' (read-write)" );
            return -1;
        }

        if( !file ) {
            IOErrno();
        }
        ds.ds = file;
    } else {
        ValueError( "unknown input configuration" );
        return -1;
    }

    if( !ds ) {
        return -1;
    }

    /*
     * init can be called multiple times, which is treated as opening a new
     * file on the same object. That means the previous file handle must be
     * properly closed before the new file is set
     */
    self->ds.swap( ds );

    return 0;
}

PyObject* segyopen( segyfd* self, PyObject* args, PyObject* kwargs ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    int endianness = -1; // change to Py_None later
    int encoding = -1;
    PyObject *py_iline = Py_None;
    PyObject *py_xline = Py_None;
    static const char* keywords[] = {
        "endianness",
        "encoding",
        "iline",
        "xline",
        NULL
    };

    if( !PyArg_ParseTupleAndKeywords(
            args, kwargs, "|iiOO",
            const_cast<char**>( keywords ),
            &endianness,
            &encoding,
            &py_iline,
            &py_xline
        ) ) {
        return NULL;
    }

    // endianness must be set before any other header values are read
    if( endianness != SEGY_LSB && endianness != SEGY_MSB ) {
        int err = segy_endianness( ds, &endianness );
        if( err ) return Error( err );
    }
    ds->metadata.endianness = endianness;

    int err = parse_extended_text_headers( self );
    if( err ) return Error( err );

    std::vector<char> layout_stanza_data;
    err = extract_layout_stanza( self, layout_stanza_data );
    if( err ) return Error( err );

    err = set_traceheader_mappings( self, layout_stanza_data, py_iline, py_xline );
    if( err ) return NULL;

    int ext_textheader_count =
        self->stanzas.empty() ? 0 : self->stanzas.back().end_index();

    err = segy_collect_metadata( ds, endianness, encoding, ext_textheader_count );
    if( err != SEGY_OK ) {
        std::ostringstream msg;
        msg << "unable to gather basic metadata from the file, error " << segy_errstr( err )
            << ". Intermediate state:\n"
            << "  endianness=" << ds->metadata.endianness << "\n"
            << "  encoding=" << ds->metadata.encoding << "\n"
            << "  format=" << ds->metadata.format << "\n"
            << "  elemsize=" << ds->metadata.elemsize << "\n"
            << "  ext_textheader_count=" << ds->metadata.ext_textheader_count << "\n"
            << "  trace0=" << ds->metadata.trace0 << "\n"
            << "  samplecount=" << ds->metadata.samplecount << "\n"
            << "  trace_bsize=" << ds->metadata.trace_bsize << "\n"
            << "  traceheader_count=" << ds->metadata.traceheader_count << "\n"
            << "  tracecount=" << ds->metadata.tracecount;
        return RuntimeError( msg.str().c_str() );
    }

    self->format = ds->metadata.format;
    self->elemsize = ds->metadata.elemsize;
    self->trace0 = ds->metadata.trace0;
    self->samplecount = ds->metadata.samplecount;
    self->trace_bsize = ds->metadata.trace_bsize;
    self->traceheader_count = ds->metadata.traceheader_count;
    self->tracecount = ds->metadata.tracecount;


    std::vector<std::array<char, 8>> traceheader_names(self->traceheader_count);
    err = segy_traceheader_names(ds, reinterpret_cast<char(*)[8]>(traceheader_names.data()));
    if( err ) return Error( err );

    std::vector<segy_header_mapping> ordered_mappings;
    for (const auto& header_name : traceheader_names) {
        bool found = false;
        for (const auto& mapping : self->traceheader_mappings) {
            if (std::strncmp(mapping.name, header_name.data(), 8) == 0) {
                ordered_mappings.push_back(mapping);
                found = true;
                break;
            }
        }
        if (!found) {
            return KeyError("traceheader mapping for '%8.8s' not found", header_name.data());
        }
    }
    self->traceheader_mappings = std::move(ordered_mappings);

    Py_INCREF( self );
    return (PyObject*)self;
}

PyObject* segycreate( segyfd* self, PyObject* args, PyObject* kwargs ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    int endianness = -1;
    int encoding = -1;
    int samples;
    int tracecount;
    int ext_headers = 0;
    int traceheader_count = 1;
    int format = SEGY_IBM_FLOAT_4_BYTE;
    PyObject* py_layout_xml = Py_None;

    // https://mail.python.org/pipermail/python-dev/2006-February/060689.html
    // python3 fixes the non-constness of the kwlist arg in
    // ParseTupleAndKeywords, since C++ really prefers writing string literals
    // as const
    //
    // Remove the const_cast when python2 support is dropped
    static const char* kwlist[] = {
        "samples",
        "tracecount",
        "endianness",
        "encoding",
        "traceheader_count",
        "format",
        "ext_headers",
        "layout_xml",
        NULL,
    };

    if( !PyArg_ParseTupleAndKeywords( args, kwargs,
                "ii|iiiiiO",
                const_cast< char** >(kwlist),
                &samples,
                &tracecount,
                &endianness,
                &encoding,
                &traceheader_count,
                &format,
                &ext_headers,
                &py_layout_xml
             ) )
        return NULL;

    if( endianness != SEGY_MSB && endianness != SEGY_LSB ) {
        endianness = SEGY_MSB;
    }

    if( encoding != SEGY_EBCDIC && encoding != SEGY_ASCII ) {
        encoding = SEGY_EBCDIC;
    }

    ds->metadata.endianness = endianness;
    ds->metadata.encoding = encoding;

    std::vector<char> layout_stanza_data;

    if( py_layout_xml != Py_None ) {
        buffer_guard layout_xml( py_layout_xml, PyBUF_CONTIG );
        layout_stanza_data.resize( layout_xml.len() );
        std::memcpy(
            layout_stanza_data.data(),
            layout_xml.buf<char>(),
            layout_xml.len()
        );
    }

    int err = set_traceheader_mappings(
        self, layout_stanza_data, Py_None, Py_None
    );
    if( err ) return NULL;

    if( samples <= 0 )
        return ValueError( "expected samples > 0" );

    if( tracecount <= 0 )
        return ValueError( "expected tracecount > 0" );

    if( ext_headers < 0 )
        return ValueError( "ext_headers must be non-negative" );

    switch( format ) {
        case SEGY_IBM_FLOAT_4_BYTE:
        case SEGY_SIGNED_INTEGER_4_BYTE:
        case SEGY_SIGNED_SHORT_2_BYTE:
        case SEGY_FIXED_POINT_WITH_GAIN_4_BYTE:
        case SEGY_IEEE_FLOAT_4_BYTE:
        case SEGY_IEEE_FLOAT_8_BYTE:
        case SEGY_SIGNED_CHAR_3_BYTE:
        case SEGY_SIGNED_CHAR_1_BYTE:
        case SEGY_SIGNED_INTEGER_8_BYTE:
        case SEGY_UNSIGNED_INTEGER_4_BYTE:
        case SEGY_UNSIGNED_SHORT_2_BYTE:
        case SEGY_UNSIGNED_INTEGER_8_BYTE:
        case SEGY_UNSIGNED_INTEGER_3_BYTE:
        case SEGY_UNSIGNED_CHAR_1_BYTE:
            break;

        default:
            return ValueError( "unknown format identifier" );
    }

    int elemsize = segy_formatsize( format );
    ds->metadata.elemsize = elemsize;

    ds->metadata.format = format;
    ds->metadata.trace0 = SEGY_TEXT_HEADER_SIZE + SEGY_BINARY_HEADER_SIZE +
                          SEGY_TEXT_HEADER_SIZE * ext_headers;
    ds->metadata.samplecount = samples;
    ds->metadata.trace_bsize = segy_trsize( format, samples );
    ds->metadata.traceheader_count = traceheader_count;
    ds->metadata.tracecount = tracecount;

    self->trace0 = ds->metadata.trace0;
    self->trace_bsize = ds->metadata.trace_bsize;
    self->format = format;
    self->elemsize = elemsize;
    self->samplecount = samples;
    self->tracecount = tracecount;
    self->traceheader_count = traceheader_count;

    Py_INCREF( self );
    return (PyObject*) self;
}

PyObject* suopen( segyfd* self, PyObject* args, PyObject* kwargs ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    int endianness = -1;
    int encoding = -1;
    PyObject *py_iline = Py_None;
    PyObject *py_xline = Py_None;
    static const char* keywords[] = {
        "endianness",
        "encoding",
        "iline",
        "xline",
        NULL
    };

    if( !PyArg_ParseTupleAndKeywords(
            args, kwargs, "|iiOO",
            const_cast<char**>( keywords ),
            &endianness,
            &encoding,
            &py_iline,
            &py_xline
        ) ) {
        return NULL;
    }

    if( endianness != SEGY_MSB && endianness != SEGY_LSB ) {
        ValueError( "endianness must be set to a valid value" );
        return NULL;
    }

    if( encoding != SEGY_EBCDIC && encoding != SEGY_ASCII ) {
        int err = segy_encoding( ds, &encoding );
        if( err != SEGY_OK ) return NULL;
    }
    ds->metadata.endianness = endianness;
    ds->metadata.encoding = encoding;

    std::vector<char> layout_stanza_data;
    int err = set_traceheader_mappings(
        self, layout_stanza_data, py_iline, py_xline
    );
    if( err ) return NULL;

    ds->metadata.format = SEGY_IEEE_FLOAT_4_BYTE;
    ds->metadata.elemsize = segy_formatsize( ds->metadata.format );
    ds->metadata.trace0 = 0;

    char header[SEGY_TRACE_HEADER_SIZE] = {};
    err = segy_read_standard_traceheader( ds, 0, header );
    if( err )
        return IOError( "unable to read first trace header in SU file" );

    segy_field_data fd;
    segy_get_tracefield(
        header,
        ds->traceheader_mapping_standard.offset_to_entry_definition,
        SEGY_TR_SAMPLE_COUNT,
        &fd
    );
    ds->metadata.samplecount = fd.value.u16;
    ds->metadata.trace_bsize = ds->metadata.samplecount * ds->metadata.elemsize;

    err = segy_traces( ds, &ds->metadata.tracecount );
    if( err == SEGY_OK ) {
        self->format = ds->metadata.format;
        self->elemsize = ds->metadata.elemsize;
        self->trace0 = ds->metadata.trace0;
        self->samplecount = ds->metadata.samplecount;
        self->trace_bsize = ds->metadata.trace_bsize;
        self->traceheader_count = ds->metadata.traceheader_count;
        self->tracecount = ds->metadata.tracecount;
        Py_INCREF( self );
        return (PyObject*)self;
    }

    std::ostringstream msg;
    msg << "unable to gather basic metadata from the file, error " << segy_errstr( err )
        << ". Intermediate state:\n"
        << "  endianness=" << ds->metadata.endianness << "\n"
        << "  encoding=" << ds->metadata.encoding << "\n"
        << "  format=" << ds->metadata.format << "\n"
        << "  elemsize=" << ds->metadata.elemsize << "\n"
        << "  ext_textheader_count=" << ds->metadata.ext_textheader_count << "\n"
        << "  trace0=" << ds->metadata.trace0 << "\n"
        << "  samplecount=" << ds->metadata.samplecount << "\n"
        << "  trace_bsize=" << ds->metadata.trace_bsize << "\n"
        << "  traceheader_count=" << ds->metadata.traceheader_count << "\n"
        << "  tracecount=" << ds->metadata.tracecount;
    return RuntimeError( msg.str().c_str() );
}

void dealloc( segyfd* self ) {
    free_header_mappings_names(
        self->traceheader_mappings.data(),
        self->traceheader_mappings.size()
    );
    self->ds.close();
    Py_TYPE( self )->tp_free( (PyObject*) self );
}

PyObject* close( segyfd* self ) {
    /* multiple close() is a no-op */
    if( !self->ds ) return Py_BuildValue( "" );

    errno = 0;
    const int err = self->ds.close();
    if ( err ) {
        return Error( err );
    }

    if( errno ) return IOErrno();

    return Py_BuildValue( "" );
}

PyObject* flush( segyfd* self ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    errno = 0;
    segy_flush( self->ds );
    if( errno ) return IOErrno();

    return Py_BuildValue( "" );
}

PyObject* mmap( segyfd* self ) {
    segy_datasource* ds = self->ds;

    if( !ds ) return NULL;

    const int err = segy_mmap( ds );

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

PyObject* gettext( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    int index = 0;
    if( !PyArg_ParseTuple( args, "i", &index ) ) return NULL;

    heapbuffer buffer( segy_textheader_size() );
    if( !buffer ) return NULL;

    const int err = index == 0
              ? segy_read_textheader( ds, buffer )
              : segy_read_ext_textheader( ds, index - 1, buffer );

    if( err ) return Error( err );
    /*
     * segy-textheader-size returns sizeof(header)+1 to have space for
     * string-terminating null-byte, but here just the raw buffer is returned
     */
    return PyByteArray_FromStringAndSize(buffer, segy_textheader_size() - 1);
}

PyObject* puttext( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    int index;
    buffer_guard buffer;

    if( !PyArg_ParseTuple( args, "is*", &index, &buffer ) )
        return NULL;

    int size = std::min( int(buffer.len()), SEGY_TEXT_HEADER_SIZE );
    heapbuffer buf( SEGY_TEXT_HEADER_SIZE );
    if( !buf ) return NULL;

    const char* src = buffer.buf< const char >();
    std::copy( src, src + size, buf.ptr );

    const int err = segy_write_textheader( ds, index, buf );

    if( err ) return Error( err );

    return Py_BuildValue( "" );
}

PyObject* getstanza( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    int index = 0;
    if( !PyArg_ParseTuple( args, "i", &index ) ) return NULL;

    stanza_header stanza = self->stanzas[index];

    heapbuffer buffer( stanza.data_size() );
    if( !buffer ) return NULL;

    const int err = segy_read_stanza_data(
        ds,
        stanza.header_length(),
        stanza.headerindex,
        stanza.data_size(),
        buffer
    );
    if( err ) return Error( err );

    return PyByteArray_FromStringAndSize( buffer, stanza.data_size() );
}

PyObject* getbin( segyfd* self ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    char buffer[ SEGY_BINARY_HEADER_SIZE ] = {};

    const int err = segy_binheader( ds, buffer );
    if( err ) return Error( err );

    return PyByteArray_FromStringAndSize( buffer, sizeof( buffer ) );
}

PyObject* putbin( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    buffer_guard buffer;
    if( !PyArg_ParseTuple(args, "s*", &buffer ) ) return NULL;

    if( buffer.len() < SEGY_BINARY_HEADER_SIZE )
        return ValueError( "internal: binary buffer too small, "
                           "expected %i, was %zd",
                           SEGY_BINARY_HEADER_SIZE, buffer.len() );

    const int err = segy_write_binheader( ds, buffer.buf< const char >() );

    if( err == SEGY_INVALID_ARGS )
        return IOError( "file not open for writing. open with 'r+'" );
    if( err ) return Error( err );

    return Py_BuildValue( "" );
}

PyObject* getth( segyfd* self, PyObject *args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    int traceno;
    uint16_t traceheader_index;
    PyObject* bufferobj;

    if( !PyArg_ParseTuple( args, "iHO", &traceno, &traceheader_index, &bufferobj ) ) return NULL;

    buffer_guard buffer( bufferobj, PyBUF_CONTIG );
    if( !buffer ) return NULL;

    if( buffer.len() < SEGY_TRACE_HEADER_SIZE )
        return ValueError( "internal: trace header buffer too small, "
                           "expected %i, was %zd",
                           SEGY_TRACE_HEADER_SIZE, buffer.len() );

    if( traceheader_index >= self->traceheader_mappings.size() ) {
        return KeyError(
            "no trace header mapping available for index %d", traceheader_index
        );
    }

    int err = segy_read_traceheader(
        ds,
        traceno,
        traceheader_index,
        self->traceheader_mappings[traceheader_index].offset_to_entry_definition,
        buffer.buf()
    );

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

PyObject* putth( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    int traceno;
    uint16_t traceheader_index;
    buffer_guard buf;
    if( !PyArg_ParseTuple( args, "iHs*", &traceno, &traceheader_index, &buf ) ) return NULL;

    if( buf.len() < SEGY_TRACE_HEADER_SIZE )
        return ValueError( "internal: trace header buffer too small, "
                           "expected %i, was %zd",
                           SEGY_TRACE_HEADER_SIZE, buf.len() );

    if( traceheader_index >= self->traceheader_mappings.size() ) {
        return KeyError(
            "no trace header mapping available for index %d", traceheader_index
        );
    }

    const char* buffer = buf.buf< const char >();

    int err = segy_write_traceheader(
        ds,
        traceno,
        traceheader_index,
        self->traceheader_mappings[traceheader_index].offset_to_entry_definition,
        buffer
    );

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

PyObject* getfield( segyfd* self, PyObject* args ) {
    buffer_guard buffer;
    int field;
    PyObject* py_traceheader_index;

    if( !PyArg_ParseTuple(
            args,
            "s*Oi",
            &buffer,
            &py_traceheader_index,
            &field
        ) )
        return NULL;

    segy_field_data fd;
    int err;
    switch( buffer.len() ) {
        case SEGY_BINARY_HEADER_SIZE:
            err = segy_get_binfield(
                buffer.buf<const char>(), field, &fd
            );
            break;
        case SEGY_TRACE_HEADER_SIZE: {
            unsigned long traceheader_index = PyLong_AsUnsignedLong( py_traceheader_index );
            if( PyErr_Occurred() ) {
                return ValueError(
                    "traceheader_index must be a non-negative integer"
                );
            }
            if( traceheader_index >= self->traceheader_mappings.size() ) {
                return KeyError(
                    "no trace header mapping available for index %d", traceheader_index
                );
            }
            const segy_entry_definition* map =
                self->traceheader_mappings[traceheader_index].offset_to_entry_definition;
            err = segy_get_tracefield( buffer.buf<const char>(), map, field, &fd );
            break;
        }
        default:
            return BufferError( "buffer too small" );
    }
    if( err != SEGY_OK )
        return KeyError( "Got error code %d when requesting field %d", err, field );

    uint8_t datatype = segy_entry_type_to_datatype( fd.entry_type );
    switch( datatype ) {

        case SEGY_SIGNED_INTEGER_8_BYTE:
            return PyLong_FromLongLong( fd.value.i64 );
        case SEGY_SIGNED_INTEGER_4_BYTE:
            return PyLong_FromLong( fd.value.i32 );
        case SEGY_SIGNED_SHORT_2_BYTE:
            return PyLong_FromLong( fd.value.i16 );
        case SEGY_SIGNED_CHAR_1_BYTE:
            return PyLong_FromLong( fd.value.i8 );

        case SEGY_UNSIGNED_INTEGER_8_BYTE:
            return PyLong_FromUnsignedLongLong( fd.value.u64 );
        case SEGY_UNSIGNED_INTEGER_4_BYTE:
            return PyLong_FromUnsignedLong( fd.value.u32 );
        case SEGY_UNSIGNED_SHORT_2_BYTE:
            return PyLong_FromUnsignedLong( fd.value.u16 );
        case SEGY_UNSIGNED_CHAR_1_BYTE:
            return PyLong_FromUnsignedLong( fd.value.u8 );

        case SEGY_IBM_FLOAT_4_BYTE:
        case SEGY_IEEE_FLOAT_4_BYTE:
            return PyFloat_FromDouble( fd.value.f32 );
        case SEGY_IEEE_FLOAT_8_BYTE:
            return PyFloat_FromDouble( fd.value.f64 );

        case SEGY_STRING_8_BYTE: {
            return PyBytes_FromStringAndSize( fd.value.str8, 8 );
        }

        default:
            return KeyError( "Unhandled entry type %d for field %d", fd.entry_type, field );
    }
}

PyObject* putfield( segyfd* self, PyObject *args ) {
    PyObject* buffer_arg;
    PyObject* py_traceheader_index;
    int field;
    PyObject* value_arg;


    if( !PyArg_ParseTuple(
            args,
            "OOiO",
            &buffer_arg,
            &py_traceheader_index,
            &field,
            &value_arg
        ) ) return NULL;

    buffer_guard buffer;
    if( !PyArg_Parse( buffer_arg, "w*", &buffer ) )
        return NULL;

    segy_field_data fd;
    const segy_entry_definition* map;

    /*
     * We repeat some of internal logic here because Python does not keep type
     * information the same way as C does. As we do not know the type of the
     * data we have, we assume it to be the same as expected type in the
     * mapping.
     */
    switch( buffer.len() ) {
        case SEGY_BINARY_HEADER_SIZE: {
            const int offset = field - SEGY_TEXT_HEADER_SIZE - 1;
            if( offset < 0 || offset >= SEGY_BINARY_HEADER_SIZE ) {
                return KeyError( "Invalid field %d", field );
            }
            map = segy_binheader_map();
            fd.entry_type = map[offset].entry_type;
            break;
        }
        case SEGY_TRACE_HEADER_SIZE: {
            const int offset = field - 1;
            if( offset < 0 || offset >= SEGY_TRACE_HEADER_SIZE ) {
                return KeyError( "Invalid field %d", field );
            }

            unsigned long traceheader_index = PyLong_AsUnsignedLong( py_traceheader_index );
            if( PyErr_Occurred() ) {
                return ValueError(
                    "traceheader_index must be a non-negative integer"
                );
            }

            if( traceheader_index >= self->traceheader_mappings.size() ) {
                return KeyError(
                    "no trace header mapping available for index %d", traceheader_index
                );
            }
            map = self->traceheader_mappings[traceheader_index].offset_to_entry_definition;
            fd.entry_type = map[offset].entry_type;
            break;
        }
        default:
            return BufferError( "buffer too small" );
    }

    uint8_t datatype = segy_entry_type_to_datatype( fd.entry_type );
    switch( datatype ) {
        case SEGY_UNSIGNED_INTEGER_8_BYTE:
            {
                unsigned long long val = PyLong_AsUnsignedLongLong( value_arg );
                if (PyErr_Occurred() || val > UINT64_MAX) {
                    return ValueError( "Value out of range for unsigned long at field %d", field );
                }
                fd.value.u64 = val;
            }
            break;
        case SEGY_UNSIGNED_INTEGER_4_BYTE:
            {
                unsigned long val = PyLong_AsUnsignedLong( value_arg );
                if( PyErr_Occurred() || val > UINT32_MAX ) {
                    return ValueError( "Value out of range for unsigned int at field %d", field );
                }
                fd.value.u32 = val;
            }
            break;
        case SEGY_UNSIGNED_SHORT_2_BYTE:
            {
                unsigned long val = PyLong_AsUnsignedLong( value_arg );
                if( PyErr_Occurred() || val > UINT16_MAX ) {
                    return ValueError( "Value out of range for unsigned short at field %d", field );
                }
                fd.value.u16 = static_cast<uint16_t>( val );
            }
            break;
        case SEGY_UNSIGNED_CHAR_1_BYTE:
            {
                unsigned long val = PyLong_AsUnsignedLong( value_arg );
                if( PyErr_Occurred() || val > UINT8_MAX ) {
                    return ValueError( "Value out of range for unsigned char at field %d", field );
                }
                fd.value.u8 = static_cast<uint8_t>( val );
            }
            break;

        case SEGY_SIGNED_INTEGER_8_BYTE:
            {
                long long val = PyLong_AsLongLong( value_arg );
                if (PyErr_Occurred() || val > INT64_MAX || val < INT64_MIN ) {
                    return ValueError( "Value out of range for signed long at field %d", field );
                }
                fd.value.i64 = val;
            }
            break;
        case SEGY_SIGNED_INTEGER_4_BYTE:
            {
                long val = PyLong_AsLong( value_arg );
                if( PyErr_Occurred() || val > INT32_MAX || val < INT32_MIN ) {
                    return ValueError( "Value out of range for signed int at field %d", field );
                }
                fd.value.i32 = val;
            }
            break;
        case SEGY_SIGNED_SHORT_2_BYTE:
            {
                long val = PyLong_AsLong( value_arg );
                if( PyErr_Occurred() || val > INT16_MAX || val < INT16_MIN ) {
                    return ValueError( "Value out of range for signed short at field %d", field );
                }
                fd.value.i16 = static_cast<int16_t>( val );
            }
            break;
        case SEGY_SIGNED_CHAR_1_BYTE:
            {
                long val = PyLong_AsLong( value_arg );
                if( PyErr_Occurred() || val > INT8_MAX || val < INT8_MIN ) {
                    return ValueError( "Value out of range for signed char at field %d", field );
                }
                fd.value.u8 = static_cast<uint8_t>( val );
            }
            break;

        case SEGY_IEEE_FLOAT_8_BYTE:
            {
                double val = PyFloat_AsDouble( value_arg );
                if( PyErr_Occurred() ) {
                    return ValueError( "Value out of range for double at field %d", field );
                }
                fd.value.f64 = val;
                break;
            }
        case SEGY_IBM_FLOAT_4_BYTE:
        case SEGY_IEEE_FLOAT_4_BYTE:
            {
                float val = static_cast<float>( PyFloat_AsDouble( value_arg ) );
                if( PyErr_Occurred() ) {
                    return ValueError( "Value out of range for float at field %d", field );
                }
                fd.value.f32 = val;
                break;
            }
            break;
            case SEGY_STRING_8_BYTE: {
                char* ptr = nullptr;
                Py_ssize_t len = 0;
                if( PyBytes_AsStringAndSize( value_arg, &ptr, &len ) == -1 ) {
                    return ValueError( "Value must be a bytes object of length 8 at field %d", field );
                }
                if( len != 8 ) {
                    return ValueError( "Value must be exactly 8 bytes, got %zd", len );
                }
                memcpy( fd.value.str8, ptr, len );
                break;
            }
        default:
            return KeyError( "Field %d has unknown entry type %d", field, fd.entry_type );
    }

    int err;
    switch (buffer.len()) {
        case SEGY_BINARY_HEADER_SIZE:
            err = segy_set_binfield(
                buffer.buf<char>(), field, fd
            );
            break;
        case SEGY_TRACE_HEADER_SIZE:
            err = segy_set_tracefield(
                buffer.buf<char>(), map, field, fd
            );
            break;
        default:
            return ValueError( "unhandled buffer type" );
    }

    switch( err ) {
        case SEGY_OK:
            return Py_BuildValue("");
        case SEGY_INVALID_FIELD: return KeyError( "No such field %d", field );
        default:                 return Error( err );
    }
}

PyObject* field_forall( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    PyObject* bufferobj;
    int start, stop, step;
    uint16_t traceheader_index;
    int field;

    if( !PyArg_ParseTuple(
            args,
            "OHiiii",
            &bufferobj,
            &traceheader_index,
            &start,
            &stop,
            &step,
            &field
        ) )
        return NULL;

    if( step == 0 ) return ValueError( "slice step cannot be zero" );

    buffer_guard buffer( bufferobj, PyBUF_CONTIG );
    if( !buffer ) return NULL;

    if( traceheader_index >= self->traceheader_mappings.size() ) {
        return KeyError(
            "no trace header mapping available for index %d", traceheader_index
        );
    }
    const segy_entry_definition* map =
        self->traceheader_mappings[traceheader_index].offset_to_entry_definition;

    const int err = segy_field_forall( ds,
                                       traceheader_index,
                                       map,
                                       field,
                                       start,
                                       stop,
                                       step,
                                       buffer.buf< char >() );

    if( err ) return Error( err );

    Py_INCREF( bufferobj );
    return bufferobj;
}

PyObject* field_foreach( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    PyObject* bufferobj;
    uint16_t traceheader_index;
    buffer_guard indices; //int32 is expected, but not really assured
    int field;
    if( !PyArg_ParseTuple(
            args,
            "OHs*i",
            &bufferobj,
            &traceheader_index,
            &indices,
            &field
        ) )
        return NULL;

    buffer_guard bufout( bufferobj, PyBUF_CONTIG );
    if( !bufout ) return NULL;

    if( traceheader_index >= self->traceheader_mappings.size() ) {
        return KeyError(
            "no trace header mapping available for index %d", traceheader_index
        );
    }
    const segy_entry_definition* map =
        self->traceheader_mappings[traceheader_index].offset_to_entry_definition;

    int field_size = segy_formatsize( segy_entry_type_to_datatype(
                                        map[field - 1].entry_type ));

    int buffer_length = bufout.len() / field_size;
    int indices_length = indices.len() / sizeof( int32_t );

    if( buffer_length != indices_length )
        return ValueError( "internal: array size mismatch "
                           "(output %zd, indices %zd)",
                           buffer_length, indices_length );

    const int* ind = indices.buf< const int >();
    char* out = bufout.buf< char >();
    int err = 0;
    for( int i = 0; err == 0 && i < buffer_length; ++i ) {
        err = segy_field_forall( ds, traceheader_index, map, field,
                                     ind[ i ],
                                     ind[ i ] + 1,
                                     1,
                                     out + i * field_size );
    }

    if( err ) return Error( err );

    Py_INCREF( bufferobj );
    return bufferobj;
}

PyObject* metrics( segyfd* self ) {
    static const int text = SEGY_TEXT_HEADER_SIZE;
    static const int bin  = SEGY_BINARY_HEADER_SIZE;
    const int ext = (self->trace0 - (text + bin)) / text;
    segy_datasource* ds = self->ds;
    int encoding = ds->metadata.encoding;
    return Py_BuildValue( "{s:i, s:K, s:i, s:i, s:i, s:i, s:i, s:i}",
                          "tracecount",  self->tracecount,
                          "trace0",      self->trace0,
                          "trace_bsize", self->trace_bsize,
                          "samplecount", self->samplecount,
                          "format",      self->format,
                          "encoding",    encoding,
                          "traceheader_count", self->traceheader_count,
                          "ext_headers", ext );
}

struct metrics_errmsg {
    int il, xl, of;
    PyObject* operator()( int err ) const {
        switch( err ) {
            case SEGY_INVALID_FIELD:
                return IndexError( "invalid iline, (%i), xline (%i), "
                                   "or offset (%i) field", il, xl, of );

            case SEGY_INVALID_FIELD_DATATYPE:
                return ValueError( "invalid field datatype for "
                                   "iline, (%i), xline (%i) or offset (%i) field",
                                   il, xl, of );

            case SEGY_INVALID_SORTING:
                return RuntimeError( "unable to find sorting."
                                    "Check iline, (%i) and xline (%i) "
                                    "in case you are sure the file is "
                                    "a 3D sorted volume", il, xl);

            default:
                return Error( err );
        }
    }
};

PyObject* cube_metrics( segyfd* self ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    segy_header_mapping& mapping = ds->traceheader_mapping_standard;

    int il = mapping.name_to_offset[SEGY_TR_INLINE];
    int xl = mapping.name_to_offset[SEGY_TR_CROSSLINE];
    int offset = mapping.name_to_offset[SEGY_TR_OFFSET];

    metrics_errmsg errmsg = { il, xl, offset };

    int sorting = -1;
    int err = segy_sorting( ds, il,
                                xl,
                                offset,
                                &sorting );

    if( err ) return errmsg( err );

    int offset_count = -1;
    err = segy_offsets( ds, il,
                            xl,
                            self->tracecount,
                            &offset_count );

    if( err ) return errmsg( err );

    int xl_count = 0;
    int il_count = 0;
    err = segy_lines_count( ds, il,
                                xl,
                                sorting,
                                offset_count,
                                &il_count,
                                &xl_count );

    if( err == SEGY_NOTFOUND )
        return ValueError( "could not parse geometry, "
                           "open with strict=False" );

    if( err ) return errmsg( err );

    return Py_BuildValue( "{s:i, s:i, s:i, s:i, s:i, s:i, s:i}",
                          "sorting",      sorting,
                          "iline_field",  il,
                          "xline_field",  xl,
                          "offset_field", offset,
                          "offset_count", offset_count,
                          "iline_count",  il_count,
                          "xline_count",  xl_count );
}

long getitem( PyObject* dict, const char* key ) {
    return PyLong_AsLong( PyDict_GetItemString( dict, key ) );
}

PyObject* indices( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

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

    metrics_errmsg errmsg = { il_field, xl_field, offset_field };

    int err = segy_inline_indices( ds, il_field,
                                       sorting,
                                       iline_count,
                                       xline_count,
                                       offset_count,
                                       iline_out.buf< int >() );
    if( err ) return errmsg( err );

    err = segy_crossline_indices( ds, xl_field,
                                      sorting,
                                      iline_count,
                                      xline_count,
                                      offset_count,
                                      xline_out.buf< int >() );
    if( err ) return errmsg( err );

    err = segy_offset_indices( ds, offset_field,
                                   offset_count,
                                   offset_out.buf< int >() );
    if( err ) return errmsg( err );

    return Py_BuildValue( "" );
}

PyObject* gettr( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    PyObject* bufferobj;
    int start, length, step, sample_start, sample_stop, sample_step, samples;

    if( !PyArg_ParseTuple( args, "Oiiiiiii", &bufferobj, &start, &step, &length,
        &sample_start, &sample_stop, &sample_step, &samples ) )
        return NULL;

    buffer_guard buffer( bufferobj, PyBUF_CONTIG );
    if( !buffer) return NULL;

    const int skip = samples * self->elemsize;
    const long long bufsize = (long long) length * samples;

    if( buffer.len() < bufsize )
        return ValueError( "internal: data trace buffer too small, "
                           "expected %zi, was %zd",
                            bufsize, buffer.len() );

    int err = 0;
    int i = 0;
    char* buf = buffer.buf();
    for( ; err == 0 && i < length; ++i, buf += skip ) {
        err = segy_readsubtr( ds, start + (i * step),
                                  sample_start,
                                  sample_stop,
                                  sample_step,
                                  buf,
                                  NULL );
    }

    if( err == SEGY_FREAD_ERROR )
        return IOError( "I/O operation failed on data trace %d", i );

    if( err ) return Error( err );

    segy_to_native( self->format, bufsize, buffer.buf() );

    Py_INCREF( bufferobj );
    return bufferobj;
}

PyObject* puttr( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    int traceno;
    char* buffer;
    Py_ssize_t buflen;

    if( !PyArg_ParseTuple( args, "is#", &traceno, &buffer, &buflen ) )
        return NULL;

    if( self->trace_bsize > buflen )
        return ValueError("trace too short: expected %d bytes, got %d",
                          self->trace_bsize,
                          buflen );

    segy_from_native( self->format, self->samplecount, buffer );

    int err = segy_writetrace( ds, traceno,
                                   buffer );

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

PyObject* getline( segyfd* self, PyObject* args) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

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

    int err = segy_read_line( ds, line_trace0,
                                  line_length,
                                  stride,
                                  offsets,
                                  buffer.buf() );
    if( err ) return Error( err );

    segy_to_native( self->format,
                    self->samplecount * line_length,
                    buffer.buf() );

    Py_INCREF( bufferobj );
    return bufferobj;
}

PyObject* putline( segyfd* self, PyObject* args) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    int line_trace0;
    int line_length;
    int stride;
    int offsets;
    int index;
    int offset;
    PyObject* val;

    if( !PyArg_ParseTuple( args, "iiiiiiO", &line_trace0,
                                            &line_length,
                                            &stride,
                                            &offsets,
                                            &index,
                                            &offset,
                                            &val ) )
        return NULL;

    buffer_guard buffer( val, PyBUF_CONTIG );

    if( self->trace_bsize * line_length > buffer.len() )
        return ValueError("line too short: expected %d elements, got %zd",
                          self->samplecount * line_length,
                          buffer.len() / self->elemsize );

    const int elems = line_length * self->samplecount;
    segy_from_native( self->format, elems, buffer.buf() );

    int err = segy_write_line( ds, line_trace0,
                                   line_length,
                                   stride,
                                   offsets,
                                   buffer.buf() );

    segy_to_native( self->format, elems, buffer.buf() );

    switch( err ) {
        case SEGY_OK:
            return Py_BuildValue("");

        case SEGY_FWRITE_ERROR:
            return IOError( "I/O operation failed on line %d, offset %d",
                            index, offset );

        default:
            return Error( err );
    }
}

PyObject* getdepth( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

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
    char* buf = buffer.buf();
    const int skip = self->elemsize;

    for( ; err == 0 && traceno < count; ++traceno, buf += skip ) {
        err = segy_readsubtr( ds,
                              traceno * offsets,
                              depth,
                              depth + 1,
                              1,
                              buf,
                              NULL );
    }

    if( err == SEGY_FREAD_ERROR )
        return IOError( "I/O operation failed on data trace %d at depth %d",
                        traceno, depth );

    if( err ) return Error( err );

    segy_to_native( self->format, count, buffer.buf() );

    Py_INCREF( bufferobj );
    return bufferobj;
}

PyObject* putdepth( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    int depth;
    int count;
    int offsets;
    PyObject* val;

    if( !PyArg_ParseTuple( args, "iiiO", &depth,
                                         &count,
                                         &offsets,
                                         &val ) )
        return NULL;

    buffer_guard buffer( val, PyBUF_CONTIG );
    if( !buffer ) return NULL;

    if( count * self->elemsize > buffer.len() )
        return ValueError("slice too short: expected %d elements, got %zd",
                          count, buffer.len() / self->elemsize );

    int traceno = 0;
    int err = 0;
    const char* buf = buffer.buf();
    const int skip = self->elemsize;

    segy_from_native( self->format, count, buffer.buf() );

    for( ; err == 0 && traceno < count; ++traceno, buf += skip ) {
        err = segy_writesubtr( ds,
                               traceno * offsets,
                               depth,
                               depth + 1,
                               1,
                               buf,
                               NULL );
    }

    segy_to_native( self->format, count, buffer.buf() );

    if( err == SEGY_FREAD_ERROR )
        return IOError( "I/O operation failed on data trace %d at depth %d",
                        traceno, depth );

    if( err ) return Error( err );

    return Py_BuildValue( "" );
}

PyObject* getdt( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    float fallback;
    if( !PyArg_ParseTuple(args, "f", &fallback ) ) return NULL;

    float dt;
    int err = segy_sample_interval( ds, fallback, &dt );

    if( err == SEGY_OK )
        return PyFloat_FromDouble( dt );

    if( err != SEGY_FREAD_ERROR && err != SEGY_FSEEK_ERROR )
        return Error( err );

    /*
     * Figure out if the problem is reading the trace header
     * or the binary header
     */
    char buffer[ SEGY_BINARY_HEADER_SIZE ];
    err = segy_binheader( ds, buffer );

    if( err )
        return IOError( "I/O operation failed on binary header, "
                        "likely corrupted file" );

    err = segy_read_standard_traceheader( ds, 0, buffer );

    if( err == SEGY_FREAD_ERROR )
        return IOError( "I/O operation failed on trace header 0, "
                        "likely corrupted file" );

    return Error( err );
}

PyObject* getdelay( segyfd* self ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

    float delay;
    int err = segy_delay_recoding_time( ds, &delay );

    if( err == SEGY_OK )
        return PyFloat_FromDouble( delay );

    return Error( err );
}

PyObject* rotation( segyfd* self, PyObject* args ) {
    segy_datasource* ds = self->ds;
    if( !ds ) return NULL;

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
    int linenos_sz = static_cast<int>( linenos.len() / sizeof( int ) );
    int err = segy_rotation_cw( ds, line_length,
                                    stride,
                                    offsets,
                                    linenos.buf< const int >(),
                                    linenos_sz,
                                    &rotation );

    if( err ) return Error( err );

    return PyFloat_FromDouble( rotation );
}

PyObject* stanza_names( segyfd* self ) {
    PyObject* names = PyList_New( self->stanzas.size() );
    if( !names ) {
        PyErr_Print();
        return NULL;
    }
    for( size_t i = 0; i < self->stanzas.size(); ++i ) {
        const std::string& name = self->stanzas[i].name;
        PyObject* py_name = PyUnicode_FromStringAndSize( name.c_str(), name.size() );
        if( !py_name ) {
            PyErr_Print();
            Py_DECREF( names );
            return NULL;
        }
        PyList_SET_ITEM( names, i, py_name );
    }
    return names;
}

#ifdef IS_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#ifdef IS_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
PyMethodDef methods [] = {
    { "segyopen", (PyCFunction) fd::segyopen,
      METH_VARARGS | METH_KEYWORDS, "Open file." },
    { "segymake", (PyCFunction) fd::segycreate,
      METH_VARARGS | METH_KEYWORDS, "Create file." },

    { "suopen", (PyCFunction) fd::suopen,
      METH_VARARGS | METH_KEYWORDS, "Open SU file." },

    { "close", (PyCFunction) fd::close, METH_VARARGS, "Close file." },
    { "flush", (PyCFunction) fd::flush, METH_VARARGS, "Flush file." },
    { "mmap",  (PyCFunction) fd::mmap,  METH_NOARGS,  "mmap file."  },

    { "gettext", (PyCFunction) fd::gettext, METH_VARARGS, "Get text header." },
    { "puttext", (PyCFunction) fd::puttext, METH_VARARGS, "Put text header." },
    { "getstanza", (PyCFunction) fd::getstanza, METH_VARARGS, "Get stanza data." },

    { "getbin", (PyCFunction) fd::getbin, METH_VARARGS, "Get binary header." },
    { "putbin", (PyCFunction) fd::putbin, METH_VARARGS, "Put binary header." },

    { "getth", (PyCFunction) fd::getth, METH_VARARGS, "Get trace header." },
    { "putth", (PyCFunction) fd::putth, METH_VARARGS, "Put trace header." },

    { "getfield", (PyCFunction) fd::getfield, METH_VARARGS, "Get a header field." },
    { "putfield", (PyCFunction) fd::putfield, METH_VARARGS, "Put a header field." },

    { "field_forall",  (PyCFunction) fd::field_forall,  METH_VARARGS, "Field for-all."  },
    { "field_foreach", (PyCFunction) fd::field_foreach, METH_VARARGS, "Field for-each." },

    { "gettr", (PyCFunction) fd::gettr, METH_VARARGS, "Get trace." },
    { "puttr", (PyCFunction) fd::puttr, METH_VARARGS, "Put trace." },

    { "getline",  (PyCFunction) fd::getline,  METH_VARARGS, "Get line." },
    { "putline",  (PyCFunction) fd::putline,  METH_VARARGS, "Put line." },
    { "getdepth", (PyCFunction) fd::getdepth, METH_VARARGS, "Get depth." },
    { "putdepth", (PyCFunction) fd::putdepth, METH_VARARGS, "Put depth." },

    { "getdt",    (PyCFunction) fd::getdt,    METH_VARARGS, "Get sample interval (dt)." },
    { "getdelay", (PyCFunction) fd::getdelay, METH_NOARGS,  "Get recording delay."      },
    { "rotation", (PyCFunction) fd::rotation, METH_VARARGS, "Get clockwise rotation."   },

    { "metrics",      (PyCFunction) fd::metrics,      METH_NOARGS,  "Metrics."         },
    { "cube_metrics", (PyCFunction) fd::cube_metrics, METH_NOARGS,  "Cube metrics."    },
    { "indices",      (PyCFunction) fd::indices,      METH_VARARGS, "Indices."         },

    { "stanza_names", (PyCFunction) fd::stanza_names, METH_NOARGS, "Stanza names in order." },

    { "traceheader_layouts", (PyCFunction) traceheader_layouts, METH_VARARGS, "Layout of all traceheaders." },

    { NULL }
};
#ifdef IS_GCC
#pragma GCC diagnostic pop
#endif
#ifdef IS_CLANG
#pragma clang diagnostic pop
#endif

}

#ifdef IS_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#ifdef IS_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
PyTypeObject Segyfd = {
    PyVarObject_HEAD_INIT( NULL, 0 )
    "_segyio.segyfd",               /* name */
    sizeof( segyfd ),             /* basic size */
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
#ifdef IS_GCC
#pragma GCC diagnostic pop
#endif
#ifdef IS_CLANG
#pragma clang diagnostic pop
#endif

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
    Py_ssize_t indiceslen;
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
    int linenos_sz = static_cast<int>( indiceslen / sizeof( int ) );
    int err = segy_line_trace0( lineno,
                                other_line_length,
                                stride,
                                offsets,
                                indices,
                                linenos_sz,
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

    buffer_guard buffer( out, PyBUF_CONTIG );

    const int len = static_cast<int>( buffer.len() / sizeof( float ) );
    segy_to_native( format, len, buffer.buf() );

    Py_INCREF( out );
    return out;
}

#ifdef IS_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#ifdef IS_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
PyMethodDef SegyMethods[] = {
    { "binsize",  (PyCFunction) binsize,  METH_NOARGS, "Size of the binary header." },
    { "thsize",   (PyCFunction) thsize,   METH_NOARGS, "Size of the trace header."  },
    { "textsize", (PyCFunction) textsize, METH_NOARGS, "Size of the text header."   },

    { "trace_bsize", (PyCFunction) trbsize, METH_VARARGS, "Size of a trace (in bytes)." },

    { "line_metrics", (PyCFunction) line_metrics,  METH_VARARGS, "Find the length and stride of lines." },
    { "fread_trace0", (PyCFunction) fread_trace0,  METH_VARARGS, "Find trace0 of a line."               },
    { "native",       (PyCFunction) format,        METH_VARARGS, "Convert to native float."             },

    { NULL }
};
#ifdef IS_GCC
#pragma GCC diagnostic pop
#endif
#ifdef IS_CLANG
#pragma clang diagnostic pop
#endif

namespace {

struct NameMapEntry {
    std::string spec_entry_name;
    int segyio_entry_name;
};

struct TypeMapEntry {
    std::string spec_entry_type;
    SEGY_ENTRY_TYPE segyio_entry_type;
};

/** D8 names to SEGY names. Contains all the known names, not only the ones used
 * internally in the library. scale6 type entries are not fully supported as we
 * have separate names for mantissa and exponent parts of the type. */
static const NameMapEntry standard_name_map[] = {
    { "linetrc",     SEGY_TR_SEQ_LINE                },
    { "reeltrc",     SEGY_TR_SEQ_FILE                },
    { "ffid",        SEGY_TR_FIELD_RECORD            },
    { "chan",        SEGY_TR_NUMBER_ORIG_FIELD       },
    { "espnum",      SEGY_TR_ENERGY_SOURCE_POINT     },
    { "cdp",         SEGY_TR_ENSEMBLE                },
    { "cdptrc",      SEGY_TR_NUM_IN_ENSEMBLE         },
    { "trctype",     SEGY_TR_TRACE_ID                },
    { "vstack",      SEGY_TR_SUMMED_TRACES           },
    { "fold",        SEGY_TR_STACKED_TRACES          },
    { "rectype",     SEGY_TR_DATA_USE                },
    { "offset",      SEGY_TR_OFFSET                  },
    { "relev",       SEGY_TR_RECV_GROUP_ELEV         },
    { "selev",       SEGY_TR_SOURCE_SURF_ELEV        },
    { "sdepth",      SEGY_TR_SOURCE_DEPTH            },
    { "rdatum",      SEGY_TR_RECV_DATUM_ELEV         },
    { "sdatum",      SEGY_TR_SOURCE_DATUM_ELEV       },
    { "wdepthso",    SEGY_TR_SOURCE_WATER_DEPTH      },
    { "wdepthrc",    SEGY_TR_GROUP_WATER_DEPTH       },
    { "ed_scal",     SEGY_TR_ELEV_SCALAR             },
    { "co_scal",     SEGY_TR_SOURCE_GROUP_SCALAR     },
    { "sht_x",       SEGY_TR_SOURCE_X                },
    { "sht_y",       SEGY_TR_SOURCE_Y                },
    { "rec_x",       SEGY_TR_GROUP_X                 },
    { "rec_y",       SEGY_TR_GROUP_Y                 },
    { "coorunit",    SEGY_TR_COORD_UNITS             },
    { "wvel",        SEGY_TR_WEATHERING_VELO         },
    { "subwvel",     SEGY_TR_SUBWEATHERING_VELO      },
    { "shuphole",    SEGY_TR_SOURCE_UPHOLE_TIME      },
    { "rcuphole",    SEGY_TR_GROUP_UPHOLE_TIME       },
    { "shstat",      SEGY_TR_SOURCE_STATIC_CORR      },
    { "rcstat",      SEGY_TR_GROUP_STATIC_CORR       },
    { "stapply",     SEGY_TR_TOT_STATIC_APPLIED      },
    { "lagtimea",    SEGY_TR_LAG_A                   },
    { "lagtimeb",    SEGY_TR_LAG_B                   },
    { "delay",       SEGY_TR_DELAY_REC_TIME          },
    { "mutestrt",    SEGY_TR_MUTE_TIME_START         },
    { "muteend",     SEGY_TR_MUTE_TIME_END           },
    { "nsamps",      SEGY_TR_SAMPLE_COUNT            },
    { "dt",          SEGY_TR_SAMPLE_INTER            },
    { "gaintype",    SEGY_TR_GAIN_TYPE               },
    { "ingconst",    SEGY_TR_INSTR_GAIN_CONST        },
    { "initgain",    SEGY_TR_INSTR_INIT_GAIN         },
    { "corrflag",    SEGY_TR_CORRELATED              },
    { "sweepsrt",    SEGY_TR_SWEEP_FREQ_START        },
    { "sweepend",    SEGY_TR_SWEEP_FREQ_END          },
    { "sweeplng",    SEGY_TR_SWEEP_LENGTH            },
    { "sweeptyp",    SEGY_TR_SWEEP_TYPE              },
    { "sweepstp",    SEGY_TR_SWEEP_TAPERLEN_START    },
    { "sweepetp",    SEGY_TR_SWEEP_TAPERLEN_END      },
    { "tapertyp",    SEGY_TR_TAPER_TYPE              },
    { "aliasfil",    SEGY_TR_ALIAS_FILT_FREQ         },
    { "aliaslop",    SEGY_TR_ALIAS_FILT_SLOPE        },
    { "notchfil",    SEGY_TR_NOTCH_FILT_FREQ         },
    { "notchslp",    SEGY_TR_NOTCH_FILT_SLOPE        },
    { "lowcut",      SEGY_TR_LOW_CUT_FREQ            },
    { "highcut",     SEGY_TR_HIGH_CUT_FREQ           },
    { "lowcslop",    SEGY_TR_LOW_CUT_SLOPE           },
    { "hicslop",     SEGY_TR_HIGH_CUT_SLOPE          },
    { "year",        SEGY_TR_YEAR_DATA_REC           },
    { "day",         SEGY_TR_DAY_OF_YEAR             },
    { "hour",        SEGY_TR_HOUR_OF_DAY             },
    { "minute",      SEGY_TR_MIN_OF_HOUR             },
    { "second",      SEGY_TR_SEC_OF_MIN              },
    { "timebase",    SEGY_TR_TIME_BASE_CODE          },
    { "trweight",    SEGY_TR_WEIGHTING_FAC           },
    { "rstaswp1",    SEGY_TR_GEOPHONE_GROUP_ROLL1    },
    { "rstatrc1",    SEGY_TR_GEOPHONE_GROUP_FIRST    },
    { "rstatrcn",    SEGY_TR_GEOPHONE_GROUP_LAST     },
    { "gapsize",     SEGY_TR_GAP_SIZE                },
    { "overtrvl",    SEGY_TR_OVER_TRAVEL             },
    { "cdp_x",       SEGY_TR_CDP_X                   },
    { "cdp_y",       SEGY_TR_CDP_Y                   },
    { "iline",       SEGY_TR_INLINE                  },
    { "xline",       SEGY_TR_CROSSLINE               },
    { "sp",          SEGY_TR_SHOT_POINT              },
    { "sp_scal",     SEGY_TR_SHOT_POINT_SCALAR       },
    { "samp_unit",   SEGY_TR_MEASURE_UNIT            },
    { "trans_const", SEGY_TR_TRANSDUCTION_MANT       }, // and SEGY_TR_TRANSDUCTION_EXP
    { "trans_unit",  SEGY_TR_TRANSDUCTION_UNIT       },
    { "dev_id",      SEGY_TR_DEVICE_ID               },
    { "tm_scal",     SEGY_TR_SCALAR_TRACE_HEADER     },
    { "src_type",    SEGY_TR_SOURCE_TYPE             },
    { "src_dir1",    SEGY_TR_SOURCE_ENERGY_DIR_VERT  },
    { "src_dir2",    SEGY_TR_SOURCE_ENERGY_DIR_XLINE },
    { "src_dir3",    SEGY_TR_SOURCE_ENERGY_DIR_ILINE },
    { "smeasure",    SEGY_TR_SOURCE_MEASURE_MANT     }, // and SEGY_TR_SOURCE_MEASURE_EXP
    { "sm_unit",     SEGY_TR_SOURCE_MEASURE_UNIT     },
};

/** D8 names to SEGY names. Contains all the known names, not only the ones used
 * internally in the library. */
static const NameMapEntry ext1_name_map[] = {
    { "linetrc",   SEGY_EXT1_SEQ_LINE            },
    { "reeltrc",   SEGY_EXT1_SEQ_FILE            },
    { "ffid",      SEGY_EXT1_FIELD_RECORD        },
    { "cdp",       SEGY_EXT1_ENSEMBLE            },
    { "relev",     SEGY_EXT1_RECV_GROUP_ELEV     },
    { "rdepth",    SEGY_EXT1_RECV_GROUP_DEPTH    },
    { "selev",     SEGY_EXT1_SOURCE_SURF_ELEV    },
    { "sdepth",    SEGY_EXT1_SOURCE_DEPTH        },
    { "rdatum",    SEGY_EXT1_RECV_DATUM_ELEV     },
    { "sdatum",    SEGY_EXT1_SOURCE_DATUM_ELEV   },
    { "wdepthso",  SEGY_EXT1_SOURCE_WATER_DEPTH  },
    { "wdepthrc",  SEGY_EXT1_GROUP_WATER_DEPTH   },
    { "sht_x",     SEGY_EXT1_SOURCE_X            },
    { "sht_y",     SEGY_EXT1_SOURCE_Y            },
    { "rec_x",     SEGY_EXT1_GROUP_X             },
    { "rec_y",     SEGY_EXT1_GROUP_Y             },
    { "offset",    SEGY_EXT1_OFFSET              },
    { "nsamps",    SEGY_EXT1_SAMPLE_COUNT        },
    { "nanosecs",  SEGY_EXT1_NANOSEC_OF_SEC      },
    { "dt",        SEGY_EXT1_SAMPLE_INTER        },
    { "cable_num", SEGY_EXT1_RECORDING_DEVICE_NR },
    { "last_trc",  SEGY_EXT1_LAST_TRACE_FLAG     },
    { "cdp_x",     SEGY_EXT1_CDP_X               },
    { "cdp_y",     SEGY_EXT1_CDP_Y               },

    // segyio private
    { "header_name", SEGY_EXT1_TRACE_HEADER_NAME },
};

/** D8 entry type names to SEGY entry type names. scale6 type is not fully
 * supported as we have separate types for mantissa and exponent. */
static const TypeMapEntry entry_type_map[] = {
    { "int2",     SEGY_ENTRY_TYPE_INT2        },
    { "int4",     SEGY_ENTRY_TYPE_INT4        },
    { "int8",     SEGY_ENTRY_TYPE_INT8        },
    { "uint2",    SEGY_ENTRY_TYPE_UINT2       },
    { "uint4",    SEGY_ENTRY_TYPE_UINT4       },
    { "uint8",    SEGY_ENTRY_TYPE_UINT8       },
    { "ibmfp",    SEGY_ENTRY_TYPE_IBMFP       },
    { "ieee32",   SEGY_ENTRY_TYPE_IEEE32      },
    { "ieee64",   SEGY_ENTRY_TYPE_IEEE64      },
    { "linetrc",  SEGY_ENTRY_TYPE_LINETRC     },
    { "reeltrc",  SEGY_ENTRY_TYPE_REELTRC     },
    { "linetrc8", SEGY_ENTRY_TYPE_LINETRC8    },
    { "reeltrc8", SEGY_ENTRY_TYPE_REELTRC8    },
    { "coor4",    SEGY_ENTRY_TYPE_COOR4       },
    { "elev4",    SEGY_ENTRY_TYPE_ELEV4       },
    { "time2",    SEGY_ENTRY_TYPE_TIME2       },
    { "spnum4",   SEGY_ENTRY_TYPE_SPNUM4      },
    { "scale6",   SEGY_ENTRY_TYPE_SCALE6_MANT },

    // segyio private
    { "string8",  SEGY_ENTRY_TYPE_STRING8     },
};

static const std::unordered_map<SEGY_ENTRY_TYPE, std::string> segytype_to_spectype_map = [] {
    std::unordered_map<SEGY_ENTRY_TYPE, std::string> map;
    for( const auto& e : entry_type_map ) {
        map[e.segyio_entry_type] = e.spec_entry_type;
    }
    return map;
}();

static const std::unordered_map<std::string, SEGY_ENTRY_TYPE> spectype_to_segytype_map = [] {
    std::unordered_map<std::string, SEGY_ENTRY_TYPE> map;
    for( const auto& e : entry_type_map ) {
        map[e.spec_entry_type] = e.segyio_entry_type;
    }
    return map;
}();

int overwrite_field_offset(
    segy_header_mapping& mapping,
    uint8_t segyio_field_name,
    const char* spec_entry_name,
    SEGY_ENTRY_TYPE entry_type,
    PyObject* py_entry_byte
) {
    if( py_entry_byte == Py_None ) return SEGY_OK;

    unsigned long long raw_entry_byte = PyLong_AsUnsignedLongLong( py_entry_byte );
    if( PyErr_Occurred() || raw_entry_byte < 1 || raw_entry_byte > SEGY_TRACE_HEADER_SIZE ) {
        ValueError( "Custom field offset %llu out of range: ", raw_entry_byte );
        return SEGY_INVALID_ARGS;
    }
    uint8_t entry_byte = static_cast<uint8_t>( raw_entry_byte );
    mapping.name_to_offset[segyio_field_name] = entry_byte;

    segy_entry_definition& def = mapping.offset_to_entry_definition[entry_byte - 1];
    def.entry_type = entry_type;
    def.requires_nonzero_value = false;

    if( def.name ) {
        delete[] def.name;
    }
    def.name = new char[6];
    std::strcpy( def.name, spec_entry_name );

    return SEGY_OK;
}

int initialize_traceheader_mappings(
    segyfd* self,
    std::vector<char>& layout_stanza_data
) {
    segy_datasource* ds = self->ds;

    if( layout_stanza_data.empty() ) {
        /* Entry names provided by xml are of unknown length, so we must
         * allocate them on heap and delete them afterwards. Default C maps have
         * their names set to NULL. If we provided default names for entries
         * inside of the C code, then:
         * - we can't allocate them on heap statically, so there has to be a
         *   separate function doing the job and user would have to deallocate
         *   appropriately. This is complex and doesn't solve much.
         * - if we allocated them as static strings, then we would have to
         *   somehow keep track where strings were allocated to know if they
         *   should be freed or not.
         *
         * So we allocate names only here, in C++, where we know they always
         * will be freed.
         **/
        auto add_names = [](
            const NameMapEntry* name_map,
            size_t name_map_size,
            segy_header_mapping& mapping
        ) {
            for (size_t i = 0; i < name_map_size; ++i) {
                const NameMapEntry& entry = name_map[i];
                int offset = mapping.name_to_offset[entry.segyio_entry_name];
                if (offset != 0) {
                    char* name = new char[entry.spec_entry_name.size() + 1];
                    std::strcpy(name, entry.spec_entry_name.c_str());
                    mapping.offset_to_entry_definition[offset - 1].name = name;
                }
            }
        };

        const size_t standard_map_size = sizeof(standard_name_map) / sizeof(standard_name_map[0]);
        add_names( standard_name_map, standard_map_size, ds->traceheader_mapping_standard );
        self->traceheader_mappings.insert(
            self->traceheader_mappings.begin(), ds->traceheader_mapping_standard
        );

        const size_t ext1_map_size = sizeof(ext1_name_map) / sizeof(ext1_name_map[0]);
        add_names( ext1_name_map, ext1_map_size, ds->traceheader_mapping_extension1 );
        self->traceheader_mappings.insert(
            self->traceheader_mappings.begin() + 1, ds->traceheader_mapping_extension1
        );
        return SEGY_OK;
    }

    segy_header_mapping* mappings = nullptr;
    size_t mappings_length = 0;
    int err = segy_parse_layout_xml(
        layout_stanza_data.data(),
        layout_stanza_data.size(),
        &mappings,
        &mappings_length
    );
    if( err != SEGY_OK ) return err;

    self->traceheader_mappings =
        std::vector<segy_header_mapping>( mappings, mappings + mappings_length );
    delete[] mappings;

    int standard_index = -1;
    int extension1_index = -1;

    for( size_t i = 0; i < self->traceheader_mappings.size(); ++i ) {
        const auto& mapping = self->traceheader_mappings[i];
        if( strncmp( mapping.name, "SEG00000", 8 ) == 0 ) {
            standard_index = i;
        }
        if( strncmp( mapping.name, "SEG00001", 8 ) == 0 ) {
            extension1_index = i;
        }
    }

    // standard header definition is obligatory, so it anyway should have failed earlier
    if( standard_index != 0 ) return SEGY_NOTFOUND;

    if( extension1_index != -1 ) {
        if( extension1_index != 1 ) {
            auto extension1_mapping = self->traceheader_mappings[extension1_index];
            self->traceheader_mappings.erase(
                self->traceheader_mappings.begin() + extension1_index
            );
            self->traceheader_mappings.insert(
                self->traceheader_mappings.begin() + 1, extension1_mapping
            );
        }
    }

    return SEGY_OK;
}

int set_traceheader_mappings(
    segyfd* self,
    std::vector<char>& layout_stanza_data,
    PyObject* py_iline,
    PyObject* py_xline
) {
    int err = initialize_traceheader_mappings( self, layout_stanza_data );
    if( err != SEGY_OK ) {
        if( PyErr_Occurred() ) {
            PyErr_Clear();
        }
        ValueError( "error parsing trace header mappings" );
        return err;
    }

    segy_header_mapping& mapping = self->traceheader_mappings[0];
    err = overwrite_field_offset(
        mapping, SEGY_TR_INLINE, "iline", SEGY_ENTRY_TYPE_INT4, py_iline
    );
    if( err != SEGY_OK ) return err;

    err = overwrite_field_offset(
        mapping, SEGY_TR_CROSSLINE, "xline", SEGY_ENTRY_TYPE_INT4, py_xline
    );
    if( err != SEGY_OK ) return err;

    segy_datasource* ds = self->ds;
    memcpy(
        &ds->traceheader_mapping_standard,
        &self->traceheader_mappings[0],
        sizeof( ds->traceheader_mapping_standard )
    );
    if( self->traceheader_mappings.size() >= 2 &&
        strncmp( self->traceheader_mappings[1].name, "SEG00001", 8 ) == 0 ) {
        memcpy(
            &ds->traceheader_mapping_extension1,
            &self->traceheader_mappings[1],
            sizeof( ds->traceheader_mapping_extension1 )
        );
    }

    return SEGY_OK;
}

int parse_extended_text_headers( segyfd* self ) {
    segy_datasource* ds = self->ds;

    char binheader[SEGY_BINARY_HEADER_SIZE] = {};
    int err = segy_binheader( ds, binheader );
    if( err != SEGY_OK ) return err;

    segy_field_data fd;
    segy_get_binfield( binheader, SEGY_BIN_EXT_HEADERS, &fd );
    const int extra_headers_count = fd.value.i16;

    int i = 0;
    while( extra_headers_count != i ) {
        char* c_stanza_name = NULL;
        size_t stanza_name_length;

        err = segy_read_stanza_header(
            ds, i, &c_stanza_name, &stanza_name_length
        );
        if( err != SEGY_OK ) return err;

        if( stanza_name_length != 0 || self->stanzas.empty() ) {
            self->stanzas.emplace_back(
                std::string( c_stanza_name, stanza_name_length ),
                i,
                1
            );
        } else {
            self->stanzas.back().headercount += 1;
        }
        if( c_stanza_name ) free( c_stanza_name );
        if( self->stanzas.back().normalized_name() == "seg:endtext" ) break;

        ++i;
    }
    return SEGY_OK;
}

int free_header_mappings_names(
    segy_header_mapping* mappings,
    size_t mappings_length
) {
    if( !mappings ) return SEGY_OK;

    for( size_t i = 0; i < mappings_length; ++i ) {
        segy_header_mapping* mapping = &mappings[i];
        for( int j = 0; j < SEGY_TRACE_HEADER_SIZE; ++j ) {
            char*& name = mapping->offset_to_entry_definition[j].name;
            if( name ) {
                delete[] name;
            }
            name = nullptr;
        }
    }
    return SEGY_OK;
}

int set_mapping_name_to_offset(
    segy_header_mapping* mapping,
    std::string spec_entry_name,
    int byte
) {
    NameMapEntry* entry_name_map;
    size_t entry_name_map_size;
    const char* header_name = mapping->name;

    if( strncmp( header_name, "SEG00000", 8 ) == 0 ) {
        entry_name_map = const_cast<NameMapEntry*>( standard_name_map );
        entry_name_map_size = sizeof( standard_name_map ) / sizeof( standard_name_map[0] );
    } else if( strncmp( header_name, "SEG00001", 8 ) == 0 ) {
        entry_name_map = const_cast<NameMapEntry*>( ext1_name_map );
        entry_name_map_size = sizeof( ext1_name_map ) / sizeof( ext1_name_map[0] );
    } else {
        // no name maps for proprietary headers
        entry_name_map = nullptr;
        entry_name_map_size = 0;
    }

    int entry_name = -1;
    for( size_t i = 0; i < entry_name_map_size; ++i ) {
        if( spec_entry_name == entry_name_map[i].spec_entry_name ) {
            entry_name = entry_name_map[i].segyio_entry_name;
            break;
        }
    }
    if( entry_name >= 0 ) {
        mapping->name_to_offset[entry_name] = byte;
    }

    return SEGY_OK;
}

int set_mapping_offset_to_entry_defintion(
    segy_header_mapping* mapping,
    std::string spec_entry_name,
    int byte,
    std::string spec_entry_type,
    bool requires_nonzero_value
) {
    std::unique_ptr<char[]> spec_entry_name_ptr(
        new char[spec_entry_name.size() + 1]
    );
    if( !spec_entry_name_ptr ) return SEGY_MEMORY_ERROR;
    std::strcpy( spec_entry_name_ptr.get(), spec_entry_name.c_str() );

    SEGY_ENTRY_TYPE entry_type = SEGY_ENTRY_TYPE_UNDEFINED;
    if( spectype_to_segytype_map.count( spec_entry_type ) ) {
        entry_type = spectype_to_segytype_map.at( spec_entry_type );
    }
    if( entry_type == SEGY_ENTRY_TYPE_UNDEFINED ) return SEGY_INVALID_ARGS;

    if( entry_type == SEGY_ENTRY_TYPE_SCALE6_MANT ) {
        const int byte_mant = byte;
        const int byte_exp = byte + 4;
        if( byte_exp < 1 || byte_exp > SEGY_TRACE_HEADER_SIZE ) {
            return SEGY_INVALID_ARGS;
        }

        std::string mant_name = spec_entry_name + "_mant";
        std::unique_ptr<char[]> mant_name_ptr( new char[mant_name.size() + 1] );
        if( !mant_name_ptr ) return SEGY_MEMORY_ERROR;
        std::strcpy( mant_name_ptr.get(), mant_name.c_str() );

        std::string exp_name = spec_entry_name + "_exp";
        std::unique_ptr<char[]> exp_name_ptr( new char[exp_name.size() + 1] );
        if( !exp_name_ptr ) return SEGY_MEMORY_ERROR;
        std::strcpy( exp_name_ptr.get(), exp_name.c_str() );

        segy_entry_definition def_mant = {
            SEGY_ENTRY_TYPE_SCALE6_MANT,
            requires_nonzero_value,
            mant_name_ptr.release()
        };
        segy_entry_definition def_exp = {
            SEGY_ENTRY_TYPE_SCALE6_EXP,
            requires_nonzero_value,
            exp_name_ptr.release()
        };

        mapping->offset_to_entry_definition[byte_mant - 1] = def_mant;
        mapping->offset_to_entry_definition[byte_exp - 1] = def_exp;
        return SEGY_OK;
    }

    segy_entry_definition def = {
        entry_type,
        requires_nonzero_value,
        spec_entry_name_ptr.release()
    };
    mapping->offset_to_entry_definition[byte - 1] = def;
    return SEGY_OK;
}

int parse_py_TraceHeaderLayoutEntry_list( PyObject* entries, segy_header_mapping* mapping ) {
    Py_ssize_t entries_length = PyList_Size( entries );
    for( Py_ssize_t i = 0; i < entries_length; ++i ) {
        PyObject* entry = PyList_GetItem( entries, i );
        if( !entry ) return SEGY_INVALID_ARGS;

        PyObject* name_obj = PyObject_GetAttrString( entry, "name" );
        PyObject* byte_obj = PyObject_GetAttrString( entry, "byte" );
        PyObject* type_obj = PyObject_GetAttrString( entry, "type" );
        PyObject* zero_obj = PyObject_GetAttrString( entry, "requires_nonzero_value" );
        if( !name_obj || !byte_obj || !type_obj || !zero_obj ) {
            Py_XDECREF( name_obj );
            Py_XDECREF( byte_obj );
            Py_XDECREF( type_obj );
            Py_XDECREF( zero_obj );
            return SEGY_INVALID_ARGS;
        }
        std::string spec_entry_name( PyUnicode_AsUTF8( name_obj ) );
        int byte = (int)PyLong_AsLong( byte_obj );
        std::string spec_entry_type( PyUnicode_AsUTF8( type_obj ) );
        bool requires_nonzero_value = PyObject_IsTrue( zero_obj );

        Py_DECREF( name_obj );
        Py_DECREF( byte_obj );
        Py_DECREF( type_obj );
        Py_DECREF( zero_obj );

        if( byte < 1 || byte > SEGY_TRACE_HEADER_SIZE ) {
            return SEGY_INVALID_ARGS;
        }

        int err = set_mapping_name_to_offset(
            mapping, spec_entry_name, byte
        );
        if( err != SEGY_OK ) return err;

        err = set_mapping_offset_to_entry_defintion(
            mapping, spec_entry_name, byte, spec_entry_type, requires_nonzero_value
        );
        if( err != SEGY_OK ) return err;
    }

    /* Last 8 bytes of each trace header should contain a header name, which is
     * not in the map. To be able to deal with header names the same way we deal
     * with other fields, we need to add them to the map.
     *
     * The main header is an exception as there header name in bytes 233-240 is
     * optional. Previous versions of segyio assumed those fields could contain
     * custom user information. To be compliant with those versions, we allow
     * any field types in SEG00000 bytes 233-240.
     */
    const int header_name_offset = 233;
    const segy_entry_definition header_name_entry =
        mapping->offset_to_entry_definition[header_name_offset - 1];
    if( header_name_entry.entry_type == SEGY_ENTRY_TYPE_UNDEFINED ) {
        int err = set_mapping_offset_to_entry_defintion(
            mapping, "header_name", header_name_offset, "string8", false
        );
        if( err != SEGY_OK ) return err;
    } else {
        // we accept that users could have written their own values here only for SEG00000
        if( strncmp( mapping->name, "SEG00000", 8 ) != 0 ) {
            return SEGY_INVALID_ARGS;
        }
    }

    return SEGY_OK;
}

int parse_py_traceheader_layout_dict(
    PyObject* headers,
    segy_header_mapping* mappings
) {
    PyObject *py_header_name, *py_header_entities;
    Py_ssize_t pos = 0; // must be intialized to 0 per PyDict_Next docs

    while( PyDict_Next( headers, &pos, &py_header_name, &py_header_entities ) ) {
        if( !py_header_entities || !PyList_Check( py_header_entities ) ) {
            return SEGY_INVALID_ARGS;
        }
        segy_header_mapping mapping = {};

        Py_ssize_t header_size;
        const char* header_name = PyUnicode_AsUTF8AndSize( py_header_name, &header_size );
        if( !header_name || header_size > 8 ) {
            return SEGY_INVALID_ARGS;
        }
        strncpy( mapping.name, header_name, 8 );

        int err = parse_py_TraceHeaderLayoutEntry_list( py_header_entities, &mapping );
        if( err != SEGY_OK ) return err;

        mappings[pos - 1] = mapping;
    }
    return SEGY_OK;
}

extern "C" int segy_parse_layout_xml(
    const char* xml,
    size_t xml_size,
    segy_header_mapping** mappings,
    size_t* mappings_length
) {

    PyObject* module = PyImport_ImportModule( "segyio.utils" );
    if( !module ) {
        PyErr_Print();
        return SEGY_NOTFOUND;
    }

    PyObject* parse_func = PyObject_GetAttrString( module, "parse_trace_headers_layout" );
    if( !parse_func || !PyCallable_Check( parse_func ) ) {
        PyErr_Print();
        Py_DECREF( module );
        return SEGY_NOTFOUND;
    }

    PyObject* py_xml = PyUnicode_FromStringAndSize( xml, xml_size );
    if( !py_xml ) {
        PyErr_Print();
        Py_DECREF( parse_func );
        Py_DECREF( module );
        return SEGY_INVALID_ARGS;
    }
    PyObject* args = PyTuple_Pack( 1, py_xml );
    if( !args ) {
        PyErr_Print();
        Py_DECREF( py_xml );
        Py_DECREF( parse_func );
        Py_DECREF( module );
        return SEGY_INVALID_ARGS;
    }

    PyObject* headers = PyObject_CallObject( parse_func, args );

    Py_DECREF( args );
    Py_DECREF( py_xml );
    Py_DECREF( parse_func );
    Py_DECREF( module );

    if( !headers || !PyDict_Check( headers ) ) {
        if( PyErr_Occurred() ) {
            PyErr_Print();
        }
        Py_XDECREF( headers );
        return SEGY_INVALID_ARGS;
    }

    Py_ssize_t headers_number = PyDict_Size( headers );
    *mappings_length = static_cast<size_t>( headers_number );
    *mappings = new segy_header_mapping[*mappings_length]();
    if( !*mappings ) {
        return SEGY_MEMORY_ERROR;
    }

    const int err = parse_py_traceheader_layout_dict( headers, *mappings );
    Py_DECREF( headers );
    if( err != SEGY_OK ) {
        if( *mappings ) {
            free_header_mappings_names( *mappings, *mappings_length );
            delete[] *mappings;
        }
        if( PyErr_Occurred() ) {
            PyErr_Print();
        }
        return err;
    }

    return SEGY_OK;
}

int entry_defintion_to_py_TraceHeaderLayoutEntry(
    PyObject* entries,
    int offset,
    const segy_entry_definition& def,
    PyObject* entry_class
) {
    std::string entry_type;
    if( segytype_to_spectype_map.count( def.entry_type ) ) {
        entry_type = segytype_to_spectype_map.at( def.entry_type );
    }

    char* name = NULL;
    if( def.name != NULL ) {
        name = def.name;
    }

    PyObject* args = Py_BuildValue(
        "sisi",
        name,
        offset,
        entry_type.c_str(),
        def.requires_nonzero_value
    );
    if( !args ) {
        return SEGY_INVALID_ARGS;
    }

    PyObject* entry = PyObject_CallObject( entry_class, args );
    Py_DECREF( args );

    if( !entry ) {
        return SEGY_INVALID_ARGS;
    }
    PyList_Append( entries, entry );
    Py_DECREF( entry );
    return SEGY_OK;
}

int mapping_to_py_TraceHeaderLayout(
    PyObject* layout_dict,
    const segy_header_mapping& mapping,
    PyObject* layout_class,
    PyObject* entry_class
) {
    PyObject* header_name = PyUnicode_FromStringAndSize( mapping.name, strnlen( mapping.name, 8 ) );
    if( !header_name ) return SEGY_INVALID_ARGS;

    PyObject* entries = PyList_New( 0 );
    if( !entries ) {
        Py_DECREF( header_name );
        return SEGY_INVALID_ARGS;
    }

    for( int i = 0; i < SEGY_TRACE_HEADER_SIZE; ++i ) {
        const segy_entry_definition& def = mapping.offset_to_entry_definition[i];
        if( def.entry_type == SEGY_ENTRY_TYPE_UNDEFINED ) continue;

        int err = entry_defintion_to_py_TraceHeaderLayoutEntry( entries, i + 1, def, entry_class );
        if( err != SEGY_OK ) {
            Py_DECREF( header_name );
            Py_DECREF( entries );
            return err;
        }
    }

    PyObject* trace_header_layout = PyObject_CallFunctionObjArgs( layout_class, entries, NULL );
    if( !trace_header_layout ) {
        Py_DECREF( header_name );
        Py_DECREF( entries );
        return SEGY_INVALID_ARGS;
    }

    PyDict_SetItem( layout_dict, header_name, trace_header_layout );
    Py_DECREF( header_name );
    Py_DECREF( entries );
    Py_DECREF( trace_header_layout );

    return SEGY_OK;
}

PyObject* traceheader_layouts( segyfd* self ) {
    PyObject* mod = PyImport_ImportModule( "segyio.utils" );
    if( !mod ) {
        return NULL;
    }

    PyObject* entry_class = PyObject_GetAttrString( mod, "TraceHeaderLayoutEntry" );
    PyObject* layout_class = PyObject_GetAttrString( mod, "TraceHeaderLayout" );
    Py_DECREF( mod );

    if( !entry_class || !layout_class ) {
        return NULL;
    }

    if( !PyCallable_Check( entry_class ) || !PyCallable_Check( layout_class ) ) {
        return NULL;
    }

    PyObject* layout_dict = PyDict_New();
    if( !layout_dict ) return NULL;

    for( const auto& mapping : self->traceheader_mappings ) {
        int err = mapping_to_py_TraceHeaderLayout( layout_dict, mapping, layout_class, entry_class );
        if( err != SEGY_OK ) {
            Py_DECREF( layout_class );
            Py_DECREF( entry_class );
            Py_DECREF( layout_dict );
            return NULL;
        }
    }

    Py_DECREF( layout_class );
    Py_DECREF( entry_class );
    return layout_dict;
}

int extract_layout_stanza(
    segyfd* self,
    std::vector<char>& layout_stanza_data
) {
    segy_datasource* ds = self->ds;

    auto is_layout_stanza = []( const stanza_header& s ) {
        const std::string layout_stanza_name = "seg:layout";
        return s.normalized_name().compare( 0, layout_stanza_name.size(), layout_stanza_name ) == 0;
    };
    auto it = find_if( self->stanzas.begin(), self->stanzas.end(), is_layout_stanza );
    if( it == self->stanzas.end() ) return SEGY_OK;

    stanza_header stanza = *it;
    layout_stanza_data.resize( stanza.data_size() );

    return segy_read_stanza_data(
        ds,
        stanza.header_length(),
        stanza.headerindex,
        stanza.data_size(),
        layout_stanza_data.data()
    );
}

} // namespace
}

/* module initialization */
#ifdef IS_PY3K
#ifdef IS_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#ifdef IS_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
static struct PyModuleDef segyio_module = {
        PyModuleDef_HEAD_INIT,
        "_segyio",   /* name of module */
        NULL, /* module documentation, may be NULL */
        -1,  /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
        SegyMethods
};
#ifdef IS_GCC
#pragma GCC diagnostic pop
#endif
#ifdef IS_CLANG
#pragma clang diagnostic pop
#endif

PyMODINIT_FUNC
PyInit__segyio(void) {

    Segyfd.tp_new = PyType_GenericNew;
    if( PyType_Ready( &Segyfd ) < 0 ) return NULL;

    PyObject* m = PyModule_Create(&segyio_module);

    if( !m ) return NULL;

    Py_INCREF( &Segyfd );
    PyModule_AddObject( m, "segyfd", (PyObject*)&Segyfd );

    return m;
}
#else
PyMODINIT_FUNC
init_segyio(void) {
    Segyfd.tp_new = PyType_GenericNew;
    if( PyType_Ready( &Segyfd ) < 0 ) return;

    PyObject* m = Py_InitModule("_segyio", SegyMethods);

    Py_INCREF( &Segyfd );
    PyModule_AddObject( m, "segyfd", (PyObject*)&Segyfd );
}
#endif
