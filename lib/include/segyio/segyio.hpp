#ifndef SEGYIO_HPP
#define SEGYIO_HPP

#include <ios>
#include <stdexcept>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <map>

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

struct header {
    int SEQ_LINE;
    int SEQ_FILE;
    int FIELD_RECORD;
    int NUMBER_ORIG_FIELD;
    int ENERGY_SOURCE_POINT;
    int ENSEMBLE;
    int NUM_IN_ENSEMBLE;
    int TRACE_ID;
    int SUMMED_TRACES;
    int STACKED_TRACES;
    int DATA_USE;
    int OFFSET;
    int RECV_GROUP_ELEV;
    int SOURCE_SURF_ELEV;
    int SOURCE_DEPTH;
    int RECV_DATUM_ELEV;
    int SOURCE_DATUM_ELEV;
    int SOURCE_WATER_DEPTH;
    int GROUP_WATER_DEPTH;
    int ELEV_SCALAR;
    int SOURCE_GROUP_SCALAR;
    int SOURCE_X;
    int SOURCE_Y;
    int GROUP_X;
    int GROUP_Y;
    int COORD_UNITS;
    int WEATHERING_VELO;
    int SUBWEATHERING_VELO;
    int SOURCE_UPHOLE_TIME;
    int GROUP_UPHOLE_TIME;
    int SOURCE_STATIC_CORR;
    int GROUP_STATIC_CORR;
    int TOT_STATIC_APPLIED;
    int LAG_A;
    int LAG_B;
    int DELAY_REC_TIME;
    int MUTE_TIME_START;
    int MUTE_TIME_END;
    int SAMPLE_COUNT;
    int SAMPLE_INTER;
    int GAIN_TYPE;
    int INSTR_GAIN_CONST;
    int INSTR_INIT_GAIN;
    int CORRELATED;
    int SWEEP_FREQ_START;
    int SWEEP_FREQ_END;
    int SWEEP_LENGTH;
    int SWEEP_TYPE;
    int SWEEP_TAPERLEN_START;
    int SWEEP_TAPERLEN_END;
    int TAPER_TYPE;
    int ALIAS_FILT_FREQ;
    int ALIAS_FILT_SLOPE;
    int NOTCH_FILT_FREQ;
    int NOTCH_FILT_SLOPE;
    int LOW_CUT_FREQ;
    int HIGH_CUT_FREQ;
    int LOW_CUT_SLOPE;
    int HIGH_CUT_SLOPE;
    int YEAR_DATA_REC;
    int DAY_OF_YEAR;
    int HOUR_OF_DAY;
    int MIN_OF_HOUR;
    int SEC_OF_MIN;
    int TIME_BASE_CODE;
    int WEIGHTING_FAC;
    int GEOPHONE_GROUP_ROLL1;
    int GEOPHONE_GROUP_FIRST;
    int GEOPHONE_GROUP_LAST;
    int GAP_SIZE;
    int OVER_TRAVEL;
    int CDP_X;
    int CDP_Y;
    int INLINE;
    int CROSSLINE;
    int SHOT_POINT;
    int SHOT_POINT_SCALAR;
    int MEASURE_UNIT;
    int TRANSDUCTION_MANT;
    int TRANSDUCTION_EXP;
    int TRANSDUCTION_UNIT;
    int DEVICE_ID;
    int SCALAR_TRACE_HEADER;
    int SOURCE_TYPE;
    int SOURCE_ENERGY_DIR_MANT;
    int SOURCE_ENERGY_DIR_EXP;
    int SOURCE_MEASURE_MANT;
    int SOURCE_MEASURE_EXP;
    int SOURCE_MEASURE_UNIT;
    int UNASSIGNED1;
    int UNASSIGNED2;
};

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

        float get_dt();
        float get_dt( float );

        header get_traceheader( int );

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

        template< typename T = double >
        std::vector< T > get_xline( int );

        template< typename T >
        std::vector< T >& get_xline( int, std::vector< T >& );

        template< typename OutputIt >
        OutputIt get_xline( int, OutputIt );

        template< typename T = int >
        std::vector< T > get_attributes( int, int, int, int );

        template< typename T >
        std::vector< T >& get_attributes(
            int, int, int, int, std::vector< T >& );

        template< typename OutputIt >
        OutputIt get_attributes( int, int, int, int, OutputIt );

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

        const long& get_trace0() const
        { return this->trace0; }

        const int& get_trsize() const
        { return this->trsize; }

        const int& get_samples() const
        { return this->samples; }

        const char* get_format() const {
            std::map< int, const char* > d;
            d[1] = "4-byte IBM float";
            d[2] = "4-byte signed integer";
            d[3] = "2-byte signed integer";
            d[4] = "4-byte fixed point with gain";
            d[5] = "4-byte IEEE float";
            d[8] = "1-byte signed char";

            if ( d.count( this->format ) < 1 ) { return "Unknown format"; }
            else { return d[ this->format ]; }
        }

        const int& get_sorting() const
        { return this->sorting; }

        const int& get_offsets() const
        { return this->offsets; }

        const int& get_ext_headers() const
        { return this->ext_headers; }

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
        int ext_headers; // possibly int32_t

        int sorting = SEGY_UNKNOWN_SORTING;
        int offsets = 1;

        float dt;

        std::vector< int > inline_labels;
        std::vector< int > crossline_labels;

        inline void range_check( int ) const;
        inline void open_check() const;

        int slice_length( int, int, int );
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

float simple_file::get_dt() {
    float fallback = -1;
    return this->get_dt( fallback );
}

float simple_file::get_dt( float fallback ) {
    auto err = segy_sample_interval( this->get(), fallback, &this->dt );

    if( err == SEGY_OK )
        return this->dt;

    if( err != SEGY_FREAD_ERROR && err != SEGY_FSEEK_ERROR )
        throw( "unable to read sample interval" );

    /*
     * Figure out of the problem is reading the binary header or
     * the traceheader
     */

     char buffer[ SEGY_BINARY_HEADER_SIZE ];

     err = segy_binheader( this->get(), buffer );
     if( err ) throw std::runtime_error( "I/O operations failed on binary"
                                         "header, likely corrupted file" );

     err = segy_traceheader(this->get(), 0, buffer, this->trace0, 0 );
     if( err == SEGY_FREAD_ERROR )
        throw( "I/O operations failed on trace header, likely corrupted file" );

     return -1; //WIP
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
std::vector< T > simple_file::get_xline( int i ) {
    std::vector< T > out;
    this->get_xline( i, out );
    return out;
}

template< typename T >
std::vector< T >& simple_file::get_xline( int i, std::vector< T >& out ) {
    out.resize( this->inline_labels.size() * this->samples );
    this->get_xline( i, out.begin() );
    return out;
}

template< typename OutputIt >
OutputIt simple_file::get_xline( int i, OutputIt out ) {

    this->open_check();
    const int xline_len = this->inline_labels.size();

    int stride = 0;
    auto err = segy_crossline_stride( this->sorting,
                                      this->inline_labels.size(),
                                      &stride );

    if( err ) throw std::runtime_error( "unable to determine stride" );

    int line_trace0 = 0;
    err = segy_line_trace0( i,
                            xline_len,
                            stride,
                            this->offsets,
                            this->crossline_labels.data(),
                            this->crossline_labels.size(),
                            &line_trace0 );

    if( err == SEGY_MISSING_LINE_INDEX )
        throw std::out_of_range( "No such key " + std::to_string( i ) );

    err = segy_read_line( this->get(),
                          line_trace0,
                          xline_len,
                          stride,
                          this->offsets,
                          this->buffer.data(),
                          this->trace0,
                          this->trsize );

    if( err ) throw std::runtime_error( "unable to read line" );

    const int linesize = xline_len * this->samples;
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
std::vector< T > simple_file::get_attributes( int i,
                                              int start,
                                              int stop,
                                              int step ) {

    std::vector< T > out;
    this->get_attributes( i, start, stop, step, out );
    return out;
}

template< typename T >
std::vector< T >& simple_file::get_attributes( int i,
                                               int start,
                                               int stop,
                                               int step,
                                               std::vector< T >& out ) {
    const int length = slice_length( start, stop, step );
    if( !length ) throw std::runtime_error( "invalid slice range" );

    out.resize( length );
    this->get_attributes(  i, start, stop, step, out.begin() );
    return out;
}

template< typename OutputIt >
OutputIt simple_file::get_attributes( int i,
                                      int start,
                                      int stop,
                                      int step,
                                      OutputIt out ) {

    this->open_check();
    int trace_bsize = segy_trace_bsize( this->samples );

    const int length = slice_length( start, stop, step );
    if( !length ) throw std::runtime_error( "invalid slice range" );

    std::vector< int > buffer;
    buffer.resize( length );

    auto err = segy_field_forall( this->get(),
                                  i,
                                  start,
                                  stop,
                                  step,
                                  buffer.data(),
                                  this->trace0,
                                  trace_bsize ); // [start stop>

    if( err ) throw std::runtime_error( "unable to read header field" );

    const auto* raw = buffer.data();
    return std::copy( reinterpret_cast< const std::int32_t* >( raw ),
                      reinterpret_cast< const std::int32_t* >( raw ) + length,
                      out );
}

int simple_file::slice_length( int start, int stop, int step ) {
    if( step == 0 ) return 0;

    if ( (step < 0 && stop >= start ) ||
         (step > 0 && start >= stop ) ) return 0;

    if( step > 0 ) return ( stop - start - 1 ) / step + 1;

    return ( stop - start + 1) / step + 1;
}

header simple_file::get_traceheader( int traceno ) {

    this->open_check();
    int trace_bsize = segy_trace_bsize( this->samples );

    this->buffer.resize( SEGY_TRACE_HEADER_SIZE );

    auto err = segy_traceheader( this->get(),
                                 traceno,
                                 this->buffer.data(),
                                 this->trace0,
                                 trace_bsize );

    if( err ) throw std::runtime_error( "unable to read trace header" );

    header t = header();

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SEQ_LINE,
                          &t.SEQ_LINE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SEQ_FILE,
                          &t.SEQ_FILE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_FIELD_RECORD,
                          &t.FIELD_RECORD );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_NUMBER_ORIG_FIELD,
                          &t.NUMBER_ORIG_FIELD );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_ENERGY_SOURCE_POINT,
                          &t.ENERGY_SOURCE_POINT );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_ENSEMBLE,
                          &t.ENSEMBLE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_NUM_IN_ENSEMBLE,
                          &t.NUM_IN_ENSEMBLE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_TRACE_ID,
                          &t.TRACE_ID );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SUMMED_TRACES,
                          &t.SUMMED_TRACES );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_STACKED_TRACES,
                          &t.STACKED_TRACES );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_DATA_USE,
                          &t.DATA_USE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_OFFSET,
                          &t.OFFSET );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_RECV_GROUP_ELEV,
                          &t.RECV_GROUP_ELEV );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_SURF_ELEV,
                          &t.SOURCE_SURF_ELEV );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_DEPTH,
                          &t.SOURCE_DEPTH );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_RECV_DATUM_ELEV,
                          &t.RECV_DATUM_ELEV );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_DATUM_ELEV,
                          &t.SOURCE_DATUM_ELEV );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_WATER_DEPTH,
                          &t.SOURCE_WATER_DEPTH );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_GROUP_WATER_DEPTH,
                          &t.GROUP_WATER_DEPTH );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_ELEV_SCALAR,
                          &t.ELEV_SCALAR );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_GROUP_SCALAR,
                          &t.SOURCE_GROUP_SCALAR );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_X,
                          &t.SOURCE_X );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_Y,
                          &t.SOURCE_Y );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_GROUP_X,
                          &t.GROUP_X );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_GROUP_Y,
                          &t.GROUP_Y );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_COORD_UNITS,
                          &t.COORD_UNITS );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_WEATHERING_VELO,
                          &t.WEATHERING_VELO );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SUBWEATHERING_VELO,
                          &t.SUBWEATHERING_VELO );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_UPHOLE_TIME,
                          &t.SOURCE_UPHOLE_TIME );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_GROUP_UPHOLE_TIME,
                          &t.GROUP_UPHOLE_TIME );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_STATIC_CORR,
                          &t.SOURCE_STATIC_CORR );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_GROUP_STATIC_CORR,
                          &t.GROUP_STATIC_CORR );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_TOT_STATIC_APPLIED,
                          &t.TOT_STATIC_APPLIED );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_LAG_A,
                          &t.LAG_A );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_LAG_B,
                          &t.LAG_B );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_DELAY_REC_TIME,
                          &t.DELAY_REC_TIME );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_MUTE_TIME_START,
                          &t.MUTE_TIME_START );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_MUTE_TIME_END,
                          &t.MUTE_TIME_END );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SAMPLE_COUNT,
                          &t.SAMPLE_COUNT );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SAMPLE_INTER,
                          &t.SAMPLE_INTER );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_GAIN_TYPE,
                          &t.GAIN_TYPE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_INSTR_GAIN_CONST,
                          &t.INSTR_GAIN_CONST );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_INSTR_INIT_GAIN,
                          &t.INSTR_INIT_GAIN );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_CORRELATED,
                          &t.CORRELATED );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SWEEP_FREQ_START,
                          &t.SWEEP_FREQ_START );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SWEEP_FREQ_END,
                          &t.SWEEP_FREQ_END );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SWEEP_LENGTH,
                          &t.SWEEP_LENGTH );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SWEEP_TYPE,
                          &t.SWEEP_TYPE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SWEEP_TAPERLEN_START,
                          &t.SWEEP_TAPERLEN_START );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SWEEP_TAPERLEN_END,
                          &t.SWEEP_TAPERLEN_END );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_TAPER_TYPE,
                          &t.TAPER_TYPE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_ALIAS_FILT_FREQ,
                          &t.ALIAS_FILT_FREQ );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_ALIAS_FILT_SLOPE,
                          &t.ALIAS_FILT_SLOPE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_NOTCH_FILT_FREQ,
                          &t.NOTCH_FILT_FREQ );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_NOTCH_FILT_SLOPE,
                          &t.NOTCH_FILT_SLOPE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_LOW_CUT_FREQ,
                          &t.LOW_CUT_FREQ );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_HIGH_CUT_FREQ,
                          &t.HIGH_CUT_FREQ );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_LOW_CUT_SLOPE,
                          &t.LOW_CUT_SLOPE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_HIGH_CUT_SLOPE,
                          &t.HIGH_CUT_SLOPE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_YEAR_DATA_REC,
                          &t.YEAR_DATA_REC );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_DAY_OF_YEAR,
                          &t.DAY_OF_YEAR );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_HOUR_OF_DAY,
                          &t.HOUR_OF_DAY );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_MIN_OF_HOUR,
                          &t.MIN_OF_HOUR );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SEC_OF_MIN,
                          &t.SEC_OF_MIN );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_TIME_BASE_CODE,
                          &t.TIME_BASE_CODE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_WEIGHTING_FAC,
                          &t.WEIGHTING_FAC );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_GEOPHONE_GROUP_ROLL1,
                          &t.GEOPHONE_GROUP_ROLL1 );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_GEOPHONE_GROUP_FIRST,
                          &t.GEOPHONE_GROUP_FIRST );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_GEOPHONE_GROUP_LAST,
                          &t.GEOPHONE_GROUP_LAST );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_GAP_SIZE,
                          &t.GAP_SIZE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_OVER_TRAVEL,
                          &t.OVER_TRAVEL );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_CDP_X,
                          &t.CDP_X );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_CDP_Y,
                          &t.CDP_Y );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_INLINE,
                          &t.INLINE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_CROSSLINE,
                          &t.CROSSLINE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SHOT_POINT,
                          &t.SHOT_POINT );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SHOT_POINT_SCALAR,
                          &t.SHOT_POINT_SCALAR );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_MEASURE_UNIT,
                          &t.MEASURE_UNIT );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_TRANSDUCTION_MANT,
                          &t.TRANSDUCTION_MANT );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_TRANSDUCTION_EXP,
                          &t.TRANSDUCTION_EXP );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_TRANSDUCTION_UNIT,
                          &t.TRANSDUCTION_UNIT );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_DEVICE_ID,
                          &t.DEVICE_ID );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SCALAR_TRACE_HEADER,
                          &t.SCALAR_TRACE_HEADER );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_TYPE,
                          &t.SOURCE_TYPE );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_ENERGY_DIR_MANT,
                          &t.SOURCE_ENERGY_DIR_MANT );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_ENERGY_DIR_EXP,
                          &t.SOURCE_ENERGY_DIR_EXP );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_MEASURE_MANT,
                          &t.SOURCE_MEASURE_MANT );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_MEASURE_EXP,
                          &t.SOURCE_MEASURE_EXP );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_SOURCE_MEASURE_UNIT,
                          &t.SOURCE_MEASURE_UNIT );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_UNASSIGNED1,
                          &t.UNASSIGNED1 );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    err = segy_get_field( this->buffer.data(),
                          SEGY_TR_UNASSIGNED2,
                          &t.UNASSIGNED2 );
    if( err ) throw std::runtime_error( "unable to read trace header field" );

    return t;
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
