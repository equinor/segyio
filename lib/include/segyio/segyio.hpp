#ifndef SEGYIO_HPP
#define SEGYIO_HPP

#include <ios>
#include <stdexcept>
#include <vector>

#include <segyio/segy.h>

namespace sio {

using openmode = std::ios::openmode;
namespace io {
    constexpr static const auto in    = std::ios::in;
    constexpr static const auto out   = std::ios::out;
    constexpr static const auto trunc = std::ios::trunc;
}

namespace {

const char* modestring( openmode m ) {
    if( m & io::trunc ) return "w+b";
    if( m & io::out )   return "r+b";
    if( m & io::in )    return "rb";

    std::string was;
    if( m & std::ios::app )     was += "app|";
    if( m & std::ios::binary )  was += "binary|";
    if( m & std::ios::ate )     was += "ate|";
    was.pop_back();

    throw std::runtime_error(
        "openmode must be either in|out|trunc, was " + was
    );
}

}

struct filehandle {

    filehandle() = default;
    filehandle( filehandle&& ) = default;

    filehandle( const char* path, openmode perm ) :
        fp( segy_open( path, modestring( perm ) ) ),
        mode( perm ),
        filename( path ) {

        if( !fp )
            throw std::runtime_error( "no such file: " + std::string( path ) );
    }

    filehandle( const filehandle& o ) {
        if( !o.fp ) return;

        filehandle next( o.filename.c_str(), o.mode );
        std::swap( *this, next );
    }

    filehandle& operator=( filehandle&& o ) {
        this->fp.swap( o.fp );
        this->mode = o.mode;
        this->filename.assign( std::move( o.filename ) );
        return *this;
    }

    filehandle& open( const char* path, openmode perm ) {
        filehandle next( path, perm );
        return *this = std::move( next );
    }

    void close() {
        this->fp.reset( nullptr );
    }

    segy_file* get() {
        return this->fp.get();
    }

    struct closer {
        void operator()( segy_file* p ) { segy_close( p ); }
    };

    std::unique_ptr< segy_file, closer > fp;
    openmode mode = io::in;
    std::string filename;
};

struct config {
    openmode mode = io::in;

    config& readonly()  { this->mode = io::in;    return *this; }
    config& readwrite() { this->mode = io::out;   return *this; }
    config& truncate()  { this->mode = io::trunc; return *this; }
};

class simple_file : protected filehandle {
    public:
        using size_type  = std::ptrdiff_t;

        simple_file() = default;
        simple_file( const std::string&, const config& = {} );

        template< typename... Args >
        simple_file& open( Args&&... );
        void close();

        template< typename T = double >
        std::vector< T > read( int );

        template< typename T >
        std::vector< T >& read( int, std::vector< T >& );

        template< typename OutputIt >
        OutputIt read( int, OutputIt );

        size_type size() const { return this->tracecount; };
        bool is_open() const { return bool(this->fp); }

    private:
        long trace0;
        int trsize;
        std::vector< char > buffer;

        int samples;
        int tracecount = 0;
        int format;

        inline void range_check( int ) const;
        inline void open_check() const;
};

simple_file::simple_file( const std::string& path, const config& c ) :
    filehandle( path.c_str(), c.mode ) {

    char buffer[ SEGY_BINARY_HEADER_SIZE ] = {};
    auto err = segy_binheader( this->get(), buffer );

    if( err ) throw std::runtime_error( "unable to read header" );

    // TODO: robustly fall back if format is unset or other errors
    const auto samples = segy_samples( buffer );
    this->trace0 = segy_trace0( buffer );
    this->format = segy_format( buffer );
    this->trsize = segy_trsize( this->format, samples );
    this->buffer.resize( this->trsize );

    this->samples = segy_samples( buffer );
    err = segy_traces( this->get(), &this->tracecount, this->trace0, this->trsize );

    if( err ) throw std::runtime_error( "unable to count traces" );
}

template< typename... Args >
simple_file& simple_file::open( Args&&... args ) {
    simple_file next( std::forward< Args >( args )... );
    std::swap( *this, next );
    return *this;
}

void simple_file::close() {
    this->filehandle::close();
    this->tracecount = 0;
}

template< typename T >
std::vector< T > simple_file::read( int i ) {
    std::vector< T > out( this->samples );
    this->read( i, out.begin() );
    return out;
}

template< typename T >
std::vector< T >& simple_file::read( int i, std::vector< T >& out ) {
    out.resize( this->samples );
    this->read( i, out.begin() );
    return out;
}

void simple_file::range_check( int i ) const {
    if( i >= this->size() ) throw std::out_of_range(
        "simple_file::range_check: "
        "i (which is " + std::to_string( i ) + ")"
        " >= this->size() "
        "(which is " + std::to_string( this->size() ) + ")"
    );

    if( i < 0 ) throw std::out_of_range(
        "simple_file::range_check: "
        "i (which is " + std::to_string( i ) + ")"
        " < 0"
    );
}

void simple_file::open_check() const {
    if( !this->is_open() )
        throw std::runtime_error( "I/O operation on closed file" );
}

template< typename OutputIt >
OutputIt simple_file::read( int i, OutputIt out ) {

    this->open_check();
    this->range_check( i );
    auto err = segy_readtrace( this->get(),
                               i,
                               this->buffer.data(),
                               this->trace0,
                               this->trsize );

    if( err ) throw std::runtime_error( "error reading trace" );

    segy_to_native( this->format, this->samples, this->buffer.data() );

    const auto* raw = this->buffer.data();
    switch( this->format ) {
        case SEGY_IBM_FLOAT_4_BYTE:
        case SEGY_IEEE_FLOAT_4_BYTE:
            return std::copy(
                    reinterpret_cast< const float* >( raw ),
                    reinterpret_cast< const float* >( raw ) + this->samples,
                    out );

        case SEGY_SIGNED_INTEGER_4_BYTE:
            return std::copy(
                    reinterpret_cast< const std::int32_t* >( raw ),
                    reinterpret_cast< const std::int32_t* >( raw ) + this->samples,
                    out );

        case SEGY_SIGNED_SHORT_2_BYTE:
            return std::copy(
                    reinterpret_cast< const std::int16_t* >( raw ),
                    reinterpret_cast< const std::int16_t* >( raw ) + this->samples,
                    out );

        case SEGY_SIGNED_CHAR_1_BYTE:
            return std::copy(
                    reinterpret_cast< const std::int8_t* >( raw ),
                    reinterpret_cast< const std::int8_t* >( raw ) + this->samples,
                    out );

        default:
            throw std::logic_error(
                "this->format is broken (was "
                + std::to_string( this->format ) 
                + ")"
            );
    }
}

}

#endif //SEGYIO_HPP
