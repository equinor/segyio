#ifndef SEGYIO_HPP
#define SEGYIO_HPP

#include <ios>
#include <stdexcept>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

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
    int iline  = SEGY_TR_INLINE;
    int xline  = SEGY_TR_CROSSLINE;
    int offset = SEGY_TR_OFFSET;

    config& readonly()  { this->mode = io::in;    return *this; }
    config& readwrite() { this->mode = io::out;   return *this; }
    config& truncate()  { this->mode = io::trunc; return *this; }

    config& ilbyte( int x )     { this->iline  = x; return *this; }
    config& xlbyte( int x )     { this->xline  = x; return *this; }
    config& offsetbyte( int x ) { this->offset = x; return *this; }
};

class simple_file : protected filehandle {
    public:
        using size_type  = std::ptrdiff_t;

        simple_file() = default;
        simple_file( const std::string&, const config& = {} );

        template< typename... Args >
        simple_file& open( Args&&... );
        void close();

        /* GET */
        template< typename T = double >
        std::vector< T > read( int );

        template< typename T >
        std::vector< T >& read( int, std::vector< T >& );

        template< typename OutputIt >
        OutputIt read( int, OutputIt );

        template< typename T = double >
        std::vector< T > get_iline( int );

        template< typename T >
        std::vector< T >& get_iline( int, std::vector< T >& );

        template< typename OutputIt >
        OutputIt get_iline( int, OutputIt );

        /* PUT */
        template< typename T >
        const std::vector< T >& put( int, const std::vector< T >& );

        template< typename InputIt >
        InputIt put( int, InputIt );

        /* ATTRS */
        const std::vector< int >& inlines() const
        { return this->inline_labels; }

        const std::vector< int >& crosslines() const
        { return this->crossline_labels; }

        size_type size() const { return this->tracecount; };
        bool is_open() const { return bool(this->fp); }
        bool is_structured() const { return !this->inline_labels.empty(); };

    private:
        long trace0;
        int trsize;
        std::vector< char > buffer;

        int samples;
        int tracecount = 0;
        int format;

        int sorting = SEGY_UNKNOWN_SORTING;
        int offsets = 1;

        std::vector< int > inline_labels;
        std::vector< int > crossline_labels;

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

    //if( c.nostructure ) return;

    err = segy_sorting( this->get(),
                        c.iline,
                        c.xline,
                        c.offset,
                        &this->sorting,
                        this->trace0,
                        this->trsize );

    if( err ) throw std::runtime_error( "unable to determine sorting" );

    err = segy_offsets( this->get(),
                        c.iline,
                        c.xline,
                        this->tracecount,
                        &this->offsets,
                        this->trace0,
                        this->trsize );

    if( err ) throw std::runtime_error( "unable to determine offsets" );

    int ilcount, xlcount;
    err = segy_lines_count( this->get(),
                            c.iline,
                            c.xline,
                            this->sorting,
                            this->offsets,
                            &ilcount,
                            &xlcount,
                            this->trace0,
                            this->trsize );

    this->inline_labels.resize( ilcount );
    this->crossline_labels.resize( xlcount );

    this->buffer.resize( this->trsize * std::max( ilcount, xlcount ) );

    err = segy_inline_indices( this->get(),
                               c.iline,
                               this->sorting,
                               ilcount,
                               xlcount,
                               this->offsets,
                               inline_labels.data(),
                               this->trace0,
                               this->trsize );

    if( err )
        throw std::runtime_error( "unable to determine inline labels" );

    err = segy_crossline_indices( this->get(),
                                  c.xline,
                                  this->sorting,
                                  ilcount,
                                  xlcount,
                                  this->offsets,
                                  crossline_labels.data(),
                                  this->trace0,
                                  this->trsize );

    if( err )
        throw std::runtime_error( "unable to determine crossline labels" );
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

template< typename T >
std::vector< T > simple_file::get_iline( int i ) {
    std::vector< T > out;
    this->get_iline( i, out );
    return out;
}

template< typename T >
std::vector< T >& simple_file::get_iline( int i, std::vector< T >& out ) {
    out.resize( this->crossline_labels.size() * this->samples );
    this->get_iline( i, out.begin() );
    return out;
}

template< typename OutputIt >
OutputIt simple_file::get_iline( int i, OutputIt out ) {

    this->open_check();
    const int iline_len = this->crossline_labels.size();

    int stride = 0;
    auto err = segy_inline_stride( this->sorting,
                                   this->crossline_labels.size(),
                                   &stride );

    if( err ) throw std::runtime_error( "unable to determine stride" );

    int line_trace0 = 0;
    err = segy_line_trace0( i,
                            iline_len,
                            stride,
                            this->offsets,
                            this->inline_labels.data(),
                            this->inline_labels.size(),
                            &line_trace0 );

    if( err == SEGY_MISSING_LINE_INDEX )
        throw std::out_of_range( "No such key " + std::to_string( i ) );

    err = segy_read_line( this->get(),
                          line_trace0,
                          iline_len,
                          stride,
                          this->offsets,
                          this->buffer.data(),
                          this->trace0,
                          this->trsize );

    if( err ) throw std::runtime_error( "unable to read line" );

    const int linesize = iline_len * this->samples;
    segy_to_native( this->format, linesize, this->buffer.data() );

    const auto* raw = this->buffer.data();
    switch( this->format ) {
        case SEGY_IBM_FLOAT_4_BYTE:
        case SEGY_IEEE_FLOAT_4_BYTE:
            return std::copy(
                    reinterpret_cast< const float* >( raw ),
                    reinterpret_cast< const float* >( raw ) + linesize,
                    out );

        case SEGY_SIGNED_INTEGER_4_BYTE:
            return std::copy(
                    reinterpret_cast< const std::int32_t* >( raw ),
                    reinterpret_cast< const std::int32_t* >( raw ) + linesize,
                    out );

        case SEGY_SIGNED_SHORT_2_BYTE:
            return std::copy(
                    reinterpret_cast< const std::int16_t* >( raw ),
                    reinterpret_cast< const std::int16_t* >( raw ) + linesize,
                    out );

        case SEGY_SIGNED_CHAR_1_BYTE:
            return std::copy(
                    reinterpret_cast< const std::int8_t* >( raw ),
                    reinterpret_cast< const std::int8_t* >( raw ) + linesize,
                    out );

        default:
            throw std::logic_error(
                "this->format is broken (was "
                + std::to_string( this->format )
                + ")"
            );
    }
}

template< typename T >
const std::vector< T >& simple_file::put( int i, const std::vector< T >& in ) {
    if( in.size() != std::size_t( this->samples ) ) {
        throw std::length_error(
            "simple_file::length_check: "
            "in.size() (which is " + std::to_string( in.size() ) + ")"
            " != samples "
            "(which is " + std::to_string( this->samples ) + ")"
        );
    }

    this->put( i, in.begin() );
    return in;
}

template< typename InputIt >
InputIt simple_file::put( int i, InputIt in ) {

    this->open_check();
    this->range_check( i );

    auto* raw = this->buffer.data();
    switch( this->format ) {
        case SEGY_IBM_FLOAT_4_BYTE:
        case SEGY_IEEE_FLOAT_4_BYTE:
            std::copy( in,
                       in + this->samples,
                       reinterpret_cast< float* >( raw ) );
            break;

        case SEGY_SIGNED_INTEGER_4_BYTE:
            std::copy( in,
                       in + this->samples,
                       reinterpret_cast< std::int32_t* >( raw ) );
            break;

        case SEGY_SIGNED_SHORT_2_BYTE:
            std::copy( in,
                       in + this->samples,
                       reinterpret_cast< std::int16_t* >( raw ) );
            break;

        case SEGY_SIGNED_CHAR_1_BYTE:
            std::copy( in,
                       in + this->samples,
                       reinterpret_cast< std::int8_t* >( raw ) );
            break;

        default:
            throw std::logic_error(
                "this->format is broken (was "
                + std::to_string( this->format )
                + ")"
            );
    }

    segy_from_native( this->format, this->samples, this->buffer.data() );

    auto err = segy_writetrace( this->get(),
                                i,
                                this->buffer.data(),
                                this->trace0,
                                this->trsize );

    if( err ) throw std::runtime_error( "error writing trace" );

    return in + this->samples;
}

}

#endif //SEGYIO_HPP
