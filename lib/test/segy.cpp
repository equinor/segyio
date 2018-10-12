#include <fstream>
#include <numeric>
#include <cmath>
#include <memory>
#include <limits>
#include <vector>

#include <catch/catch.hpp>
#include "matchers.hpp"

#include <segyio/segy.h>
#include <segyio/util.h>

#ifndef MMAP_TAG
#define MMAP_TAG ""
#endif

namespace {

const std::string& copyfile( const std::string& src, const std::string& dst ) {
    std::ifstream  in( src, std::ios::binary );
    std::ofstream out( dst, std::ios::binary );

    out << in.rdbuf();
    return dst;
}

constexpr bool strequal( const char* x, const char* y ) {
    return *x == *y && (*x == '\0' || strequal(x + 1, y + 1) );
}

constexpr bool mmapd() {
    return !strequal( MMAP_TAG, "" );
}

struct slice { int start, stop, step; };
std::string str( const slice& s ) {
    return "(" + std::to_string( s.start ) +
           "," + std::to_string( s.stop ) +
           "," + std::to_string( s.step ) +
           ")";
}

struct Err {
    // cppcheck-suppress noExplicitConstructor
    Err( int err ) : err( err ) {}

    bool operator == ( Err other ) const { return this->err == other.err; }
    bool operator != ( Err other ) const { return !(*this == other); }

    static Err ok()    { return SEGY_OK; }
    static Err args()  { return SEGY_INVALID_ARGS; }
    static Err field() { return SEGY_INVALID_FIELD; }

    int err;
};

}

namespace Catch {
template<>
struct StringMaker< Err > {
    static std::string convert( const Err& err ) {
        switch( err.err ) {
            case SEGY_OK : return "OK";
            case SEGY_FOPEN_ERROR: return "SEGY_FOPEN_ERROR";
            case SEGY_FSEEK_ERROR: return "SEGY_FSEEK_ERROR";
            case SEGY_FREAD_ERROR: return "SEGY_FREAD_ERROR";
            case SEGY_FWRITE_ERROR: return "SEGY_FWRITE_ERROR";
            case SEGY_INVALID_FIELD: return "SEGY_INVALID_FIELD";
            case SEGY_INVALID_SORTING: return "SEGY_INVALID_SORTING";
            case SEGY_MISSING_LINE_INDEX: return "SEGY_MISSING_LINE_INDEX";
            case SEGY_INVALID_OFFSETS: return "SEGY_INVALID_OFFSETS";
            case SEGY_TRACE_SIZE_MISMATCH: return "SEGY_TRACE_SIZE_MISMATCH";
            case SEGY_INVALID_ARGS: return "SEGY_INVALID_ARGS";
            case SEGY_MMAP_ERROR: return "SEGY_MMAP_ERROR";
            case SEGY_MMAP_INVALID: return "SEGY_MMAP_INVALID";
        }
        return "Unknown error";
    }
};
}

namespace {

void regular_geometry( segy_file* fp,
                       int traces,
                       long trace0,
                       int trace_bsize,
                       int expected_ilines,
                       int expected_xlines,
                       int expected_offset) {
    /* A simple "no-surprises" type of file, that's inline sorted, post-stack
     * (1 offset only), no weird header field positions, meaning the test would
     * be pure repetition for common files
     */

    const int il = SEGY_TR_INLINE;
    const int xl = SEGY_TR_CROSSLINE;
    const int of = SEGY_TR_OFFSET;
    const int sorting = SEGY_INLINE_SORTING;

    GIVEN( "a post stack file" ) {
        int offsets = -1;
        const Err err = segy_offsets( fp,
                                      il, xl, traces,
                                      &offsets,
                                      trace0, trace_bsize );
        THEN( "there is only one offset" ) {
            CHECK( err == Err::ok() );
            CHECK( offsets == 1 );
        }

        WHEN( "swapping inline and crossline position" ) {
            int offsets = -1;
            const Err err = segy_offsets( fp,
                                          xl, il, traces,
                                          &offsets,
                                          trace0, trace_bsize );
            THEN( "there is only one offset" ) {
                CHECK( err == Err::ok() );
                CHECK( offsets == 1 );
            }
        }
    }

    const int offsets = 1;

    WHEN( "determining offset labels" ) {
        int offset_index = -1;
        const Err err = segy_offset_indices( fp,
                                             of, offsets,
                                             &offset_index,
                                             trace0, trace_bsize );
        CHECK( err == Err::ok() );
        CHECK( offset_index == expected_offset );
    }

    WHEN( "counting lines" ) {
        int ilsz = -1;
        int xlsz = -1;

        WHEN( "using segy_count_lines" ) {
            const Err err = segy_count_lines( fp,
                                              xl, offsets,
                                              &ilsz, &xlsz,
                                              trace0, trace_bsize );
            CHECK( err == Err::ok() );
            CHECK( ilsz == expected_ilines );
            CHECK( xlsz == expected_xlines );
        }

        WHEN( "using segy_lines_count" ) {
            const Err err = segy_lines_count( fp,
                                              il, xl, sorting, offsets,
                                              &ilsz, &xlsz,
                                              trace0, trace_bsize );
            CHECK( err == Err::ok() );
            CHECK( ilsz == expected_ilines );
            CHECK( xlsz == expected_xlines );
        }
    }
}

bool success( Err err ) {
    return err == Err::ok();
}

struct smallfix {
    segy_file* fp = nullptr;

    smallfix() {
        fp = segy_open( "test-data/small.sgy", "rb" );
        REQUIRE( fp );

        if( MMAP_TAG != std::string("") )
            REQUIRE( Err( segy_mmap( fp ) ) == Err::ok() );
    }

    smallfix( const smallfix& ) = delete;
    smallfix& operator=( const smallfix& ) = delete;

    ~smallfix() {
        if( fp ) segy_close( fp );
    }
};

struct smallbin : smallfix {
    char bin[ SEGY_BINARY_HEADER_SIZE ];

    smallbin() : smallfix() {
        REQUIRE( Err( segy_binheader( fp, bin ) ) == Err::ok() );
    }
};

struct smallbasic : smallfix {
    long trace0 = 3600;
    int trace_bsize = 50 * 4;
    int samples = 50;
};

struct smallheader : smallbasic {
    char header[ SEGY_TRACE_HEADER_SIZE ];

    smallheader() : smallbasic() {
        Err err = segy_traceheader( fp, 0, header, trace0, trace_bsize );
        REQUIRE( success( err ) );
    }
};

struct smallfields : smallbasic {
    int il = SEGY_TR_INLINE;
    int xl = SEGY_TR_CROSSLINE;
    int of = SEGY_TR_OFFSET;
};

struct smallstep : smallbasic {
    int traceno = 10;
    int format = SEGY_IBM_FLOAT_4_BYTE;
};

struct smallsize : smallfields {
    int traces = 25;
};

struct smallshape : smallsize {
    int offsets = 1;
    int ilines = 5;
    int xlines = 5;
    int sorting = SEGY_INLINE_SORTING;
};

struct smallcube : smallshape {
    int stride = 1;
    std::vector< int > inlines = { 1, 2, 3, 4, 5 };
    std::vector< int > crosslines = { 20, 21, 22, 23, 24 };
    int format = SEGY_IBM_FLOAT_4_BYTE;
};

int arbitrary_int() {
    /*
     * in order to verify that functions don't modify their output arguments if
     * the function fail, it has to be compared to an arbitrary value that
     * should be equal before and after. It has to be initialised, but the
     * value itself is of no significance.
     */
    return -1;
}

}

TEST_CASE_METHOD( smallbin,
                  MMAP_TAG "samples+positions from binary header are correct",
                  MMAP_TAG "[c.segy]" ) {
    int samples = segy_samples( bin );
    long trace0 = segy_trace0( bin );
    int trace_bsize = segy_trsize( SEGY_IBM_FLOAT_4_BYTE, samples );

    CHECK( trace0      == 3600 );
    CHECK( samples     == 50 );
    CHECK( trace_bsize == 50 * 4 );
}

TEST_CASE_METHOD( smallfix,
                  MMAP_TAG "sample format can be overriden",
                  MMAP_TAG "[c.segy]" ) {
    Err err = segy_set_format( fp, SEGY_IEEE_FLOAT_4_BYTE );
    CHECK( err == Err::ok() );
}

TEST_CASE_METHOD( smallfix,
                  MMAP_TAG "sample format fails on invalid format",
                  MMAP_TAG "[c.segy]" ) {
    Err err = segy_set_format( fp, 10 );
    CHECK( err == Err::args() );
}

TEST_CASE_METHOD( smallbasic,
                  MMAP_TAG "trace count is 25",
                  MMAP_TAG "[c.segy]" ) {
    int traces;
    Err err = segy_traces( fp, &traces, trace0, trace_bsize );
    CHECK( success( err ) );
    CHECK( traces == 25 );
}

TEST_CASE_METHOD( smallbasic,
                  MMAP_TAG "trace0 beyond EOF is an argument error",
                  MMAP_TAG "[c.segy]" ) {
    const int input_traces = arbitrary_int();
    int traces = input_traces;
    Err err = segy_traces( fp, &traces, 50000, trace_bsize );
    CHECK( err == Err::args() );
    CHECK( traces == input_traces );
}

TEST_CASE_METHOD( smallbasic,
                  MMAP_TAG "negative trace0 is an argument error",
                  MMAP_TAG "[c.segy]" ) {
    const int input_traces = arbitrary_int();
    int traces = input_traces;
    Err err = segy_traces( fp, &traces, -1, trace_bsize );
    CHECK( err == Err::args() );
    CHECK( traces == input_traces );
}

TEST_CASE_METHOD( smallbasic,
                  MMAP_TAG "erroneous trace_bsize is detected",
                  MMAP_TAG "[c.segy]" ) {
    const int input_traces = arbitrary_int();
    int traces = input_traces;
    const int too_long_bsize = trace_bsize + sizeof( float );
    Err err = segy_traces( fp, &traces, trace0, too_long_bsize );
    CHECK( err == SEGY_TRACE_SIZE_MISMATCH );
    CHECK( traces == input_traces );
}

TEST_CASE_METHOD( smallheader,
                  MMAP_TAG "valid trace-header fields can be read",
                  MMAP_TAG "[c.segy]" ) {

    int32_t ilno;
    Err err = segy_get_field( header, SEGY_TR_INLINE, &ilno );
    CHECK( success( err ) );
    CHECK( ilno == 1 );
}

TEST_CASE_METHOD( smallheader,
                  MMAP_TAG "zero header field is an argument error",
                  MMAP_TAG "[c.segy]" ) {
    const int32_t input_value = arbitrary_int();
    auto value = input_value;
    Err err = segy_get_field( header, 0, &value );

    CHECK( err == Err::field() );
    CHECK( value == input_value );
}

TEST_CASE_METHOD( smallheader,
                  MMAP_TAG "negative header field is an argument error",
                  MMAP_TAG "[c.segy]" ) {
    const int32_t input_value = arbitrary_int();
    auto value = input_value;
    Err err = segy_get_field( header, -1, &value );

    CHECK( err == Err::field() );
    CHECK( value == input_value );
}

TEST_CASE_METHOD( smallheader,
                  MMAP_TAG "unaligned header field is an argument error",
                  MMAP_TAG "[c.segy]" ) {
    const int32_t input_value = arbitrary_int();
    auto value = input_value;
    Err err = segy_get_field( header, SEGY_TR_INLINE + 1, &value );

    CHECK( err == Err::field() );
    CHECK( value == input_value );
}

TEST_CASE_METHOD( smallheader,
                  MMAP_TAG "too large header field is an argument error",
                  MMAP_TAG "[c.segy]" ) {
    const int32_t input_value = arbitrary_int();
    auto value = input_value;
    Err err = segy_get_field( header, SEGY_TRACE_HEADER_SIZE + 10, &value );

    CHECK( err == Err::field() );
    CHECK( value == input_value );
}

TEST_CASE_METHOD( smallfields,
                  MMAP_TAG "inline sorting is detected",
                  MMAP_TAG "[c.segy]" ) {
    int sorting;
    Err err = segy_sorting( fp, il, xl, of, &sorting, trace0, trace_bsize );
    CHECK( success( err ) );
    CHECK( sorting == SEGY_INLINE_SORTING );
}

TEST_CASE_METHOD( smallfields,
                  MMAP_TAG "crossline sorting is detected with swapped il/xl",
                  MMAP_TAG "[c.segy]" ) {
    int sorting;
    Err err = segy_sorting( fp, xl, il, of, &sorting, trace0, trace_bsize );
    CHECK( success( err ) );
    CHECK( sorting == SEGY_CROSSLINE_SORTING );
}

TEST_CASE_METHOD( smallfields,
                  MMAP_TAG "invalid in-between byte offsets are detected",
                  MMAP_TAG "[c.segy]" ) {
    int sorting;
    Err err = segy_sorting( fp, il + 1, xl, of, &sorting, trace0, trace_bsize );
    CHECK( err == SEGY_INVALID_FIELD );

    err = segy_sorting( fp, il, xl + 1, of, &sorting, trace0, trace_bsize );
    CHECK( err == SEGY_INVALID_FIELD );

    err = segy_sorting( fp, il, xl, of + 1, &sorting, trace0, trace_bsize );
    CHECK( err == SEGY_INVALID_FIELD );
}

TEST_CASE_METHOD( smallsize,
                  MMAP_TAG "post-stack file offset-count is 1",
                  MMAP_TAG "[c.segy]" ) {
    int offsets;
    Err err = segy_offsets( fp,
                            il,
                            xl,
                            traces,
                            &offsets,
                            trace0,
                            trace_bsize );

    CHECK( success( err ) );
    CHECK( offsets == 1 );
}

TEST_CASE_METHOD( smallsize,
                  MMAP_TAG "swapped il/xl post-stack file offset-count is 1",
                  MMAP_TAG "[c.segy]" ) {
    int offsets;
    Err err = segy_offsets( fp,
                            xl,
                            il,
                            traces,
                            &offsets,
                            trace0,
                            trace_bsize );

    CHECK( success( err ) );
    CHECK( offsets == 1 );
}

TEST_CASE_METHOD( smallshape,
                  MMAP_TAG "correct # of lines are detected",
                  MMAP_TAG "[c.segy]" ) {

    int expected_ils = 5;
    int expected_xls = 5;

    int count_inlines;
    int count_crosslines;
    Err err_count = segy_count_lines( fp,
                                      xl,
                                      offsets,
                                      &count_inlines,
                                      &count_crosslines,
                                      trace0,
                                      trace_bsize );

    int lines_inlines;
    int lines_crosslines;
    Err err_lines = segy_lines_count( fp,
                                      il,
                                      xl,
                                      sorting,
                                      offsets,
                                      &lines_inlines,
                                      &lines_crosslines,
                                      trace0,
                                      trace_bsize );

    CHECK( success( err_count ) );
    CHECK( success( err_lines ) );

    CHECK( count_inlines    == expected_ils );
    CHECK( count_crosslines == expected_xls );

    CHECK( lines_inlines    == expected_ils );
    CHECK( lines_crosslines == expected_xls );

    CHECK( lines_inlines    == count_inlines );
    CHECK( lines_crosslines == count_crosslines );
}

TEST_CASE_METHOD( smallshape,
                  MMAP_TAG "line lengths are correct",
                  MMAP_TAG "[c.segy]" ) {
    CHECK( segy_inline_length( xlines ) == ilines );
    CHECK( segy_inline_length( ilines ) == xlines );
}

TEST_CASE_METHOD( smallshape,
                  MMAP_TAG "correct offset labels are detected",
                  MMAP_TAG "[c.segy]" ) {

    const std::vector< int > expected = { 1 };
    std::vector< int > labels( 1 );
    Err err = segy_offset_indices( fp,
                                   of,
                                   offsets,
                                   labels.data(),
                                   trace0,
                                   trace_bsize );
    CHECK( success( err ) );
    CHECK_THAT( labels, Catch::Equals( expected ) );
}

TEST_CASE_METHOD( smallshape,
                  MMAP_TAG "correct inline labels are detected",
                  MMAP_TAG "[c.segy]" ) {

    const std::vector< int > expected = { 1, 2, 3, 4, 5 };
    std::vector< int > labels( 5 );

    Err err = segy_inline_indices( fp,
                                   il,
                                   sorting,
                                   ilines,
                                   xlines,
                                   offsets,
                                   labels.data(),
                                   trace0,
                                   trace_bsize );

    CHECK( success( err ) );
    CHECK_THAT( labels, Catch::Equals( expected ) );
}

TEST_CASE_METHOD( smallshape,
                  MMAP_TAG "correct crossline labels are detected",
                  MMAP_TAG "[c.segy]" ) {

    const std::vector< int > expected = { 20, 21, 22, 23, 24 };
    std::vector< int > labels( 5 );

    Err err = segy_crossline_indices( fp,
                                      xl,
                                      sorting,
                                      ilines,
                                      xlines,
                                      offsets,
                                      labels.data(),
                                      trace0,
                                      trace_bsize );

    CHECK( success( err ) );
    CHECK_THAT( labels, Catch::Equals( expected ) );
}

TEST_CASE_METHOD( smallshape,
                  MMAP_TAG "correct inline stride is detected",
                  MMAP_TAG "[c.segy]" ) {
    int stride;
    Err err = segy_inline_stride( sorting, ilines, &stride );
    CHECK( success( err ) );
    CHECK( stride == 1 );
}

TEST_CASE_METHOD( smallshape,
                  MMAP_TAG "correct inline stride with swapped sorting",
                  MMAP_TAG "[c.segy]" ) {
    int stride;
    Err err = segy_inline_stride( SEGY_CROSSLINE_SORTING, ilines, &stride );
    CHECK( success( err ) );
    CHECK( stride == ilines );
}

TEST_CASE_METHOD( smallcube,
                  MMAP_TAG "correct first trace is detected for an inline",
                  MMAP_TAG "[c.segy]" ) {
    const int label = inlines.at( 3 );
    int line_trace0;
    Err err = segy_line_trace0( label,
                                xlines,
                                stride,
                                offsets,
                                inlines.data(),
                                inlines.size(),
                                &line_trace0 );
    CHECK( success( err ) );
    CHECK( line_trace0 == 15 );
}

TEST_CASE_METHOD( smallcube,
                  MMAP_TAG "missing inline-label is detected and reported",
                  MMAP_TAG "[c.segy]" ) {
    const int label = inlines.back() + 1;
    int line_trace0;
    Err err = segy_line_trace0( label,
                                xlines,
                                stride,
                                offsets,
                                inlines.data(),
                                inlines.size(),
                                &line_trace0 );
    CHECK( err == SEGY_MISSING_LINE_INDEX );
}

TEST_CASE_METHOD( smallshape,
                  MMAP_TAG "correct crossline stride is detected",
                  MMAP_TAG "[c.segy]" ) {
    int stride;
    Err err = segy_crossline_stride( sorting, xlines, &stride );
    CHECK( success( err ) );
    CHECK( stride == ilines );
}

TEST_CASE_METHOD( smallshape,
                  MMAP_TAG "correct crossline stride with swapped sorting",
                  MMAP_TAG "[c.segy]" ) {
    int stride;
    Err err = segy_crossline_stride( SEGY_CROSSLINE_SORTING, xlines, &stride );
    CHECK( success( err ) );
    CHECK( stride == 1 );
}

TEST_CASE_METHOD( smallcube,
                  MMAP_TAG "correct first trace is detected for a crossline",
                  MMAP_TAG "[c.segy]" ) {
    const int label = crosslines.at( 2 );
    const int stride = ilines;
    int line_trace0;
    Err err = segy_line_trace0( label,
                                ilines,
                                stride,
                                offsets,
                                crosslines.data(),
                                crosslines.size(),
                                &line_trace0 );
    CHECK( success( err ) );
    CHECK( line_trace0 == 2 );
}

TEST_CASE_METHOD( smallcube,
                  MMAP_TAG "missing crossline-label is detected and reported",
                  MMAP_TAG "[c.segy]" ) {
    const int label = crosslines.back() + 1;
    const int stride = ilines;
    int line_trace0;
    Err err = segy_line_trace0( label,
                                ilines,
                                stride,
                                offsets,
                                inlines.data(),
                                inlines.size(),
                                &line_trace0 );
    CHECK( err == SEGY_MISSING_LINE_INDEX );
}

TEST_CASE_METHOD( smallstep,
                  MMAP_TAG "read ascending strided subtrace",
                  MMAP_TAG "[c.segy]" ) {
    const int start = 3;
    const int stop  = 19;
    const int step  = 5;
    void* rangebuf = nullptr;
    const std::vector< float > expected = { 3.20003f,
                                            3.20008f,
                                            3.20013f,
                                            3.20018f };
    std::vector< float > xs( expected.size() );

    Err err = segy_readsubtr( fp,
                              traceno,
                              start,
                              stop,
                              step,
                              xs.data(),
                              rangebuf,
                              trace0,
                              trace_bsize );
    segy_to_native( format, xs.size(), xs.data() );
    CHECK( success( err ) );
    CHECK_THAT( xs, ApproxRange( expected ) );
}

TEST_CASE_METHOD( smallstep,
                  MMAP_TAG "read descending strided subtrace",
                  MMAP_TAG "[c.segy]" ) {
    const int start = 18;
    const int stop  = 2;
    const int step  = -5;

    void* rangebuf = nullptr;
    const std::vector< float > expected = { 3.20018f,
                                            3.20013f,
                                            3.20008f,
                                            3.20003f };
    std::vector< float > xs( expected.size() );

    Err err = segy_readsubtr( fp,
                              traceno,
                              start,
                              stop,
                              step,
                              xs.data(),
                              rangebuf,
                              trace0,
                              trace_bsize );
    segy_to_native( format, xs.size(), xs.data() );
    CHECK( success( err ) );
    CHECK_THAT( xs, ApproxRange( expected ) );
}

TEST_CASE_METHOD( smallstep,
                  MMAP_TAG "read descending contiguous subtrace",
                  MMAP_TAG "[c.segy]" ) {
    const int start = 3;
    const int stop  = -1;
    const int step  = -1;

    void* rangebuf = nullptr;
    const std::vector< float > expected = { 3.20003f,
                                            3.20002f,
                                            3.20001f,
                                            3.20000f };
    std::vector< float > xs( expected.size() );

    Err err = segy_readsubtr( fp,
                              traceno,
                              start,
                              stop,
                              step,
                              xs.data(),
                              rangebuf,
                              trace0,
                              trace_bsize );
    segy_to_native( format, xs.size(), xs.data() );
    CHECK( success( err ) );
    CHECK_THAT( xs, ApproxRange( expected ) );
}

TEST_CASE_METHOD( smallstep,
                  MMAP_TAG "read descending strided subtrace with pre-start",
                  MMAP_TAG "[c.segy]" ) {
    const int start = 24;
    const int stop  = -1;
    const int step  = -5;

    const std::vector< float > expected = { 3.20024f,
                                            3.20019f,
                                            3.20014f,
                                            3.20009f,
                                            3.20004f };
    std::vector< float > xs( expected.size() );

    Err err = segy_readsubtr( fp,
                              traceno,
                              start,
                              stop,
                              step,
                              xs.data(),
                              nullptr,
                              trace0,
                              trace_bsize );
    segy_to_native( format, xs.size(), xs.data() );
    CHECK( success( err ) );
    CHECK_THAT( xs, ApproxRange( expected ) );
}

TEST_CASE_METHOD( smallcube,
                  MMAP_TAG "reading the first inline gives correct values",
                  MMAP_TAG "[c.segy]" ) {

    // first line starts at first trace
    const int line_trace0 = 0;
    const std::vector< float > expected = [=] {
        std::vector< float > xs( samples * xlines );
        for( int i = 0; i < xlines; ++i ) {
            Err err = segy_readtrace( fp,
                                      i,
                                      xs.data() + (i * samples),
                                      trace0,
                                      trace_bsize );
            REQUIRE( success( err ) );
        }

        segy_to_native( format, xs.size(), xs.data() );
        return xs;
    }();

    std::vector< float > line( expected.size() );
    Err err = segy_read_line( fp,
                              line_trace0,
                              crosslines.size(),
                              stride,
                              offsets,
                              line.data(),
                              trace0,
                              trace_bsize );

    segy_to_native( format, line.size(), line.data() );
    CHECK( success( err ) );
    CHECK_THAT( line, ApproxRange( expected ) );
}

TEST_CASE_METHOD( smallcube,
                  MMAP_TAG "reading the first crossline gives correct values",
                  MMAP_TAG "[c.segy]" ) {

    // first line starts at first trace
    const int line_trace0 = 0;
    const int stride = ilines;
    const std::vector< float > expected = [=] {
        std::vector< float > xs( samples * ilines );
        for( int i = 0; i < ilines; ++i ) {
            Err err = segy_readtrace( fp,
                                      i * stride,
                                      xs.data() + (i * samples),
                                      trace0,
                                      trace_bsize );
            REQUIRE( success( err ) );
        }

        segy_to_native( format, xs.size(), xs.data() );
        return xs;
    }();

    std::vector< float > line( expected.size() );
    Err err = segy_read_line( fp,
                              line_trace0,
                              inlines.size(),
                              stride,
                              offsets,
                              line.data(),
                              trace0,
                              trace_bsize );

    segy_to_native( format, line.size(), line.data() );
    CHECK( success( err ) );
    CHECK_THAT( line, ApproxRange( expected ) );
}

template< int Start, int Stop, int Step, bool memmap = mmapd() >
struct writesubtr {
    segy_file* fp = nullptr;

    int start = Start;
    int stop  = Stop;
    int step  = Step;

    int traceno = 5;
    int trace0 = 3600;
    int trace_bsize = 50 * 4;

    int format = SEGY_IBM_FLOAT_4_BYTE;
    void* rangebuf = nullptr;

    std::vector< float > trace;
    std::vector< float > expected;

    writesubtr() {
        std::string name = MMAP_TAG + std::string("write-sub-trace ")
                         + "[" + std::to_string( start )
                         + "," + std::to_string( stop )
                         + "," + std::to_string( step )
                         + "].sgy";

        copyfile( "test-data/small.sgy", name );

        fp = segy_open( name.c_str(), "r+b" );
        REQUIRE( fp );

        if( memmap )
            REQUIRE( success( segy_mmap( fp ) ) );

        trace.assign( 50, 0 );
        expected.resize( trace.size() );

        Err err = segy_writetrace( fp, traceno, trace.data(), trace0, trace_bsize );
        REQUIRE( success( err ) );
    }

    ~writesubtr() {
        REQUIRE( fp );

        /* test that writes are observable */

        trace.assign( trace.size(), 0 );
        Err err = segy_readtrace( fp, traceno, trace.data(), trace0, trace_bsize );

        CHECK( success( err ) );
        segy_to_native( format, trace.size(), trace.data() );

        CHECK_THAT( trace, ApproxRange( expected ) );
        segy_close( fp );
    }

    writesubtr( const writesubtr& ) = delete;
    writesubtr& operator=( const writesubtr& ) = delete;
};

TEST_CASE_METHOD( (writesubtr< 3, 19, 5 >),
                  MMAP_TAG "write ascending strided subtrace",
                  MMAP_TAG "[c.segy]" ) {
    std::vector< float > out = { 3, 8, 13, 18 };
    expected.at( 3 )  = out.at( 0 );
    expected.at( 8 )  = out.at( 1 );
    expected.at( 13 ) = out.at( 2 );
    expected.at( 18 ) = out.at( 3 );
    segy_from_native( format, out.size(), out.data() );

    Err err = segy_writesubtr( fp, traceno,
                                   start,
                                   stop,
                                   step,
                                   out.data(),
                                   rangebuf,
                                   trace0,
                                   trace_bsize );

    CHECK( success( err ) );
}

TEST_CASE_METHOD( (writesubtr< 18, 2, -5 >),
                  MMAP_TAG "write descending strided subtrace",
                  MMAP_TAG "[c.segy]" ) {
    std::vector< float > out = { 18, 13, 8, 3 };
    expected.at( 18 ) = out.at( 0 );
    expected.at( 13 ) = out.at( 1 );
    expected.at( 8 )  = out.at( 2 );
    expected.at( 3 )  = out.at( 3 );
    segy_from_native( format, out.size(), out.data() );

    Err err = segy_writesubtr( fp, traceno,
                                   start,
                                   stop,
                                   step,
                                   out.data(),
                                   rangebuf,
                                   trace0,
                                   trace_bsize );

    CHECK( success( err ) );
}

TEST_CASE_METHOD( (writesubtr< 24, -1, -5 >),
                  MMAP_TAG "write descending strided subtrace with pre-start",
                  MMAP_TAG "[c.segy]" ) {
    std::vector< float > out = { 24, 19, 14, 9, 4 };
    expected.at( 24 ) = out.at( 0 );
    expected.at( 19 ) = out.at( 1 );
    expected.at( 14 ) = out.at( 2 );
    expected.at( 9 )  = out.at( 3 );
    expected.at( 4 )  = out.at( 4 );
    segy_from_native( format, out.size(), out.data() );

    Err err = segy_writesubtr( fp, traceno,
                                   start,
                                   stop,
                                   step,
                                   out.data(),
                                   rangebuf,
                                   trace0,
                                   trace_bsize );

    CHECK( success( err ) );
}

TEST_CASE_METHOD( smallfields,
                  MMAP_TAG "reading inline label from every trace header",
                  MMAP_TAG "[c.segy]" ) {
    const std::vector< int > inlines = {
        1, 1, 1, 1, 1,
        2, 2, 2, 2, 2,
        3, 3, 3, 3, 3,
        4, 4, 4, 4, 4,
        5, 5, 5, 5, 5,
    };

    const int start = 0, stop = 25, step = 1;

    std::vector< int > out( inlines.size() );
    const Err err = segy_field_forall( fp,
                                       il,
                                       start, stop, step,
                                       out.data(),
                                       trace0,
                                       trace_bsize );

    CHECK( success( err ) );
    CHECK_THAT( out, Catch::Equals( inlines ) );
}

TEST_CASE_METHOD( smallfields,
                  MMAP_TAG "reading crossline label from every trace header",
                  MMAP_TAG "[c.segy]" ) {
    const std::vector< int > crosslines = {
        20, 21, 22, 23, 24,
        20, 21, 22, 23, 24,
        20, 21, 22, 23, 24,
        20, 21, 22, 23, 24,
        20, 21, 22, 23, 24,
    };

    const int start = 0, stop = 25, step = 1;

    std::vector< int > out( crosslines.size() );
    const Err err = segy_field_forall( fp,
                                       xl,
                                       start, stop, step,
                                       out.data(),
                                       trace0,
                                       trace_bsize );

    CHECK( success( err ) );
    CHECK_THAT( out, Catch::Equals( crosslines ) );
}

TEST_CASE_METHOD( smallfields,
                  MMAP_TAG "reading every 3rd crossline label",
                  MMAP_TAG "[c.segy]" ) {
    const std::vector< int > crosslines = {
            21,         24,
                22,
        20,         23,
            21,         24,
                22,
    };
    const int start = 1, stop = 25, step = 3;

    std::vector< int > out( crosslines.size() );
    const Err err = segy_field_forall( fp,
                                       xl,
                                       start, stop, step,
                                       out.data(),
                                       trace0,
                                       trace_bsize );

    CHECK( success( err ) );
    CHECK_THAT( out, Catch::Equals( crosslines ) );
}

TEST_CASE_METHOD( smallfields,
                  MMAP_TAG "reverse-reading every 3rd crossline label",
                  MMAP_TAG "[c.segy]" ) {
    const std::vector< int > crosslines = {
                22,
        24,         21,
            23,         20,
                22,
        24,         21
    };
    const int start = 22, stop = 0, step = -3;

    std::vector< int > out( crosslines.size() );
    const Err err = segy_field_forall( fp,
                                       xl,
                                       start, stop, step,
                                       out.data(),
                                       trace0,
                                       trace_bsize );

    CHECK( success( err ) );
    CHECK_THAT( out, Catch::Equals( crosslines ) );
}

TEST_CASE_METHOD( smallfields,
                  MMAP_TAG "reverse-reading every 5th crossline label",
                  MMAP_TAG "[c.segy]" ) {
    const std::vector< int > crosslines = { 24, 24, 24, 24, 24 };
    const int start = 24, stop = -1, step = -5;

    std::vector< int > out( crosslines.size() );
    const Err err = segy_field_forall( fp,
                                       xl,
                                       start, stop, step,
                                       out.data(),
                                       trace0,
                                       trace_bsize );

    CHECK( success( err ) );
    CHECK_THAT( out, Catch::Equals( crosslines ) );
}

TEST_CASE( MMAP_TAG "setting unaligned header-field fails",
           MMAP_TAG "[c.segy]" ) {
    char header[ SEGY_TRACE_HEADER_SIZE ];
    const int32_t v = arbitrary_int();

    Err err = segy_set_field( header, SEGY_TR_INLINE + 1, v );
    CHECK( err == Err::field() );
}

TEST_CASE( MMAP_TAG "setting negative header-field fails",
           MMAP_TAG "[c.segy]" ) {
    char header[ SEGY_TRACE_HEADER_SIZE ];
    const int32_t v = arbitrary_int();

    Err err = segy_set_field( header, -1, v );
    CHECK( err == Err::field() );
}

TEST_CASE( MMAP_TAG "setting too large header-field fails",
           MMAP_TAG "[c.segy]" ) {
    char header[ SEGY_TRACE_HEADER_SIZE ];
    const int32_t v = arbitrary_int();

    Err err = segy_set_field( header, SEGY_TRACE_HEADER_SIZE + 10, v );
    CHECK( err == Err::field() );
}

TEST_CASE( MMAP_TAG "setting correct header fields succeeds",
           MMAP_TAG "[c.segy]" ) {
    char header[ SEGY_TRACE_HEADER_SIZE ];
    const int32_t input = 1;
    const int field = SEGY_TR_INLINE;

    Err err = segy_set_field( header, field, input );
    CHECK( success( err ) );


    int32_t output;
    err = segy_get_field( header, field, &output );
    CHECK( success( err ) );

    CHECK( output == input );
}

SCENARIO( MMAP_TAG "modifying trace header", "[c.segy]" MMAP_TAG ) {

    const int samples = 10;
    int trace_bsize = segy_trsize( SEGY_IBM_FLOAT_4_BYTE, samples );
    const int trace0 = 0;
    const float emptytr[ samples ] = {};
    const char emptyhdr[ SEGY_TRACE_HEADER_SIZE ] = {};


    WHEN( "writing iline no" ) {
        char header[ SEGY_TRACE_HEADER_SIZE ] = {};

        Err err = segy_set_field( header, SEGY_TR_INLINE, 2 );
        CHECK( err == Err::ok() );
        err = segy_set_field( header, SEGY_TR_SOURCE_GROUP_SCALAR, -100 );
        CHECK( err == Err::ok() );

        THEN( "the header buffer is updated") {
            int ilno = 0;
            int scale = 0;
            err = segy_get_field( header, SEGY_TR_INLINE, &ilno );
            CHECK( err == Err::ok() );
            err = segy_get_field( header, SEGY_TR_SOURCE_GROUP_SCALAR, &scale );
            CHECK( err == Err::ok() );

            CHECK( ilno == 2 );
            CHECK( scale == -100 );
        }

        const char* file = MMAP_TAG "write-traceheader.sgy";

        std::unique_ptr< segy_file, decltype( &segy_close ) >
            ufp{ segy_open( file, "w+b" ), &segy_close };

        REQUIRE( ufp );
        auto fp = ufp.get();

        /* make a file and write to last trace (to accurately get size) */
        err = segy_write_traceheader( fp, 10, emptyhdr, trace0, trace_bsize );
        REQUIRE( err == Err::ok() );
        err = segy_writetrace( fp, 10, emptytr, trace0, trace_bsize );
        REQUIRE( err == Err::ok() );
        if( MMAP_TAG != std::string("") )
            REQUIRE( Err( segy_mmap( fp ) ) == Err::ok() );

        err = segy_write_traceheader( fp, 5, header, trace0, trace_bsize );
        CHECK( err == Err::ok() );

        THEN( "changes are observable on disk" ) {
            char header[ SEGY_TRACE_HEADER_SIZE ] = {};
            int ilno = 0;
            int scale = 0;
            err = segy_traceheader( fp, 5, header, trace0, trace_bsize );
            CHECK( err == Err::ok() );
            err = segy_get_field( header, SEGY_TR_INLINE, &ilno );
            CHECK( err == Err::ok() );
            err = segy_get_field( header, SEGY_TR_SOURCE_GROUP_SCALAR, &scale );
            CHECK( err == Err::ok() );

            CHECK( ilno == 2 );
            CHECK( scale == -100 );
        }
    }
}

SCENARIO( MMAP_TAG "reading text header", "[c.segy]" MMAP_TAG ) {
    const std::string expected =
"C 1 DATE: 22/02/2016                                                            "
"C 2 AN INCREASE IN AMPLITUDE EQUALS AN INCREASE IN ACOUSTIC IMPEDANCE           "
"C 3 FIRST SAMPLE: 4 MS, LAST SAMPLE: 1400 MS, SAMPLE INTERVAL: 4 MS             "
"C 4 DATA RANGE: INLINES=(2479-2500) (INC 1),CROSSLINES=(1428-1440) (INC 1)      "
"C 5 PROCESSING GRID CORNERS:                                                    "
"C 6 DISTANCE BETWEEN INLINES: 2499.75 M, CROSSLINES: 1250 M                     "
"C 7 1: INLINE 2479, CROSSLINE 1428, UTM-X 9976386.00, UTM-Y 9989096.00          "
"C 8 2: INLINE 2479, CROSSLINE 1440, UTM-X 9983886.00, UTM-Y 10002087.00         "
"C 9 3: INLINE 2500, CROSSLINE 1428, UTM-X 10021847.00, UTM-Y 9962849.00         "
"C10 4: INLINE 2500, CROSSLINE 1440, UTM-X 10029348.00, UTM-Y 9975839.00         "
"C11 TRACE HEADER POSITION:                                                      "
"C12   INLINE BYTES 005-008    | OFFSET BYTES 037-040                            "
"C13   CROSSLINE BYTES 021-024 | CMP UTM-X BYTES 181-184                         "
"C14   CMP UTM-Y BYTES 185-188                                                   "
"C15 END EBCDIC HEADER                                                           "
"C16                                                                             "
"C17                                                                             "
"C18                                                                             "
"C19                                                                             "
"C20                                                                             "
"C21                                                                             "
"C22                                                                             "
"C23                                                                             "
"C24                                                                             "
"C25                                                                             "
"C26                                                                             "
"C27                                                                             "
"C28                                                                             "
"C29                                                                             "
"C30                                                                             "
"C31                                                                             "
"C32                                                                             "
"C33                                                                             "
"C34                                                                             "
"C35                                                                             "
"C36                                                                             "
"C37                                                                             "
"C38                                                                             "
"C39                                                                             "
"C40                                                                            \x80";

        const char* file = "test-data/text.sgy";

        std::unique_ptr< segy_file, decltype( &segy_close ) >
            ufp{ segy_open( file, "rb" ), &segy_close };

        REQUIRE( ufp );

        auto fp = ufp.get();
        if( MMAP_TAG != std::string("") )
            REQUIRE( Err( segy_mmap( fp ) ) == Err::ok() );

        char ascii[ SEGY_TEXT_HEADER_SIZE + 1 ] = {};
        const Err err = segy_read_textheader( fp, ascii );

        CHECK( err == Err::ok() );
        CHECK( ascii == expected );
}

SCENARIO( MMAP_TAG "reading a large file", "[c.segy]" MMAP_TAG ) {
    GIVEN( "a large file" ) {
        const char* file = MMAP_TAG "4G-file.sgy";

        std::unique_ptr< segy_file, decltype( &segy_close ) >
            ufp{ segy_open( file, "w+b" ), &segy_close };
        REQUIRE( ufp );

        auto fp = ufp.get();

        const int trace = 5000000;
        const int trace_bsize = 1000;
        const long long tracesize = trace_bsize + SEGY_TRACE_HEADER_SIZE;
        const long trace0 = 0;

        const Err err = segy_seek( fp, trace, trace0, trace_bsize );
        CHECK( err == Err::ok() );
        WHEN( "reading past 4GB (pos >32bit)" ) {
            THEN( "there is no overflow" ) {
                const long long pos = segy_ftell( fp );
                CHECK( pos > std::numeric_limits< int >::max() );
                CHECK( pos != -1 );
                CHECK( pos == trace * tracesize );
            }
        }
    }
}

SCENARIO( MMAP_TAG "reading a 2-byte int file", "[c.segy][2-byte]" MMAP_TAG ) {
    const char* file = "test-data/f3.sgy";

    std::unique_ptr< segy_file, decltype( &segy_close ) >
        ufp{ segy_open( file, "rb" ), &segy_close };

    REQUIRE( ufp );

    auto fp = ufp.get();
    if( MMAP_TAG != std::string("") )
        REQUIRE( Err( segy_mmap( fp ) ) == Err::ok() );

    WHEN( "finding traces initial byte offset and sizes" ) {
        char header[ SEGY_BINARY_HEADER_SIZE ];
        REQUIRE( Err( segy_binheader( fp, header ) ) == Err::ok() );
        int samples = segy_samples( header );
        long trace0 = segy_trace0( header );
        int format = segy_format( header );
        int trace_bsize = segy_trsize( format, samples );

        THEN( "the correct values are inferred from the binary header" ) {
            CHECK( format      == SEGY_SIGNED_SHORT_2_BYTE );
            CHECK( trace0      == 3600 );
            CHECK( samples     == 75 );
            CHECK( trace_bsize == 75 * 2 );
        }

        WHEN( "the format is valid" ) THEN( "setting format succeeds" ) {
                Err err = segy_set_format( fp, format );
                CHECK( err == Err::ok() );
        }

        WHEN( "the format is invalid" ) THEN( "setting format fails" ) {
                Err err = segy_set_format( fp, 50 );
                CHECK( err == Err::args() );
        }
    }

    const int format      = SEGY_SIGNED_SHORT_2_BYTE;
    const long trace0     = 3600;
    const int samples     = 75;
    const int trace_bsize = samples * 2;

    WHEN( "reading data without setting format" ) {
        std::int16_t val;
        Err err = segy_readsubtr( fp, 10,
                                      25, 26, 1,
                                      &val,
                                      nullptr,
                                      trace0, trace_bsize );

        CHECK( err == Err::ok() );

        err = segy_to_native( format, sizeof( val ), &val );
        CHECK( err == Err::ok() );

        THEN( "the value is incorrect" ) {
            CHECK( val != -1170 );
        }
    }

    Err err = segy_set_format( fp, format );
    REQUIRE( err == Err::ok() );

    WHEN( "determining number of traces" ) {
        int traces = 0;
        Err err = segy_traces( fp, &traces, trace0, trace_bsize );
        REQUIRE( err == Err::ok() );
        CHECK( traces == 414 );

        GIVEN( "trace0 outside its domain" ) {
            WHEN( "trace0 is after end-of-file" ) {
                err = segy_traces( fp, &traces, 500000, trace_bsize );

                THEN( "segy_traces fail" )
                    CHECK( err == Err::args() );

                THEN( "the input does not change" )
                    CHECK( traces == 414 );
            }

            WHEN( "trace0 is negative" ) {
                err = segy_traces( fp, &traces, -1, trace_bsize );

                THEN( "segy_traces fail" )
                    CHECK( err == Err::args() );

                THEN( "the input does not change" )
                    CHECK( traces == 414 );
            }
        }
    }

    const int traces = 414;
    const int ilines = 23;
    const int xlines = 18;
    const int offset_label = 0;
    regular_geometry( fp, traces,
                          trace0,
                          trace_bsize,
                          ilines,
                          xlines,
                          offset_label );


    const int offsets = 1;
    const int il = SEGY_TR_INLINE;
    const int sorting = SEGY_INLINE_SORTING;
    WHEN( "inferring inline structure" ) {
        std::vector< int > indices( 23, 0 );
        std::iota( indices.begin(), indices.end(), 111 );

        WHEN( "finding inline numbers" ) {
            std::vector< int > result( ilines );
            const Err err = segy_inline_indices( fp,
                                                 il,
                                                 sorting,
                                                 ilines,
                                                 xlines,
                                                 offsets,
                                                 result.data(),
                                                 trace0, trace_bsize );
            CHECK( err == Err::ok() );
            CHECK_THAT( result, Catch::Equals( indices ) );
        }
    }

    WHEN( "reading a trace header" ) {
        char buf[ SEGY_TRACE_HEADER_SIZE ] = {};

        GIVEN( "a valid field" ) {
            Err err = segy_traceheader( fp, 0, buf, trace0, trace_bsize );
            CHECK( err == Err::ok() );

            int ilno = 0;
            err = segy_get_field( buf, SEGY_TR_INLINE, &ilno );
            CHECK( err == Err::ok() );
            CHECK( ilno == 111 );
        }

        GIVEN( "an invalid field" ) {
            int x = -1;
            Err err = segy_get_field( buf, SEGY_TRACE_HEADER_SIZE + 10, &x );
            CHECK( err == Err::field() );
            CHECK( x == -1 );

            err = segy_get_field( buf, SEGY_TR_INLINE + 1, &x );
            CHECK( err == Err::field() );
            CHECK( x == -1 );
        }
    }

    WHEN( "reading a subtrace" ) {

        const std::vector< slice > inputs = {
            {  20, 45,   5 },
            {  40, 20,  -5 },
            {  53, 50,  -1 },
        };

        /*
         * these values have been manually checked with numpy, with this:
         * https://github.com/Statoil/segyio/issues/238#issuecomment-373735526
         */
        const std::vector< std::vector< std::int16_t > > expect = {
            {    0, -1170,  5198, -2213,  -888 },
            { -888, -2213,  5198, -1170,  0    },
            {-2609, -2625,   681               },
        };

        for( size_t i = 0; i < inputs.size(); ++i ) {
            WHEN( "slice is " + str( inputs[ i ] ) ) {
                std::vector< std::int16_t > buf( expect[ i ].size() );

                auto start = inputs[ i ].start;
                auto stop  = inputs[ i ].stop;
                auto step  = inputs[ i ].step;

                Err err = segy_readsubtr( fp,
                                          10,
                                          start, stop, step,
                                          buf.data(),
                                          nullptr,
                                          trace0, trace_bsize );
                segy_to_native( format, buf.size(), buf.data() );
                CHECK( err == Err::ok() );
                CHECK_THAT( buf, Catch::Equals( expect[ i ] ) );
            }
        }
    }
}

SCENARIO( MMAP_TAG "checking sorting for wonky files",
                   "[c.segy]" MMAP_TAG ) {
    WHEN( "checking sorting when il, xl and offset is all garbage ") {
        /*
         * In the case where all tracefields ( il, xl, offset ) = ( 0, 0, 0 )
         * the sorting detection should return 'SEGY_INVALID_SORTING'. To test
         * this the traceheader field 'SEGY_TR_SEQ_LINE', which we know is zero
         * in all traceheaders in file small.sgy, is passed to segy_sorting as
         * il, xl and offset.
         */
        const char* file = "test-data/small.sgy";

        std::unique_ptr< segy_file, decltype( &segy_close ) >
            ufp{ segy_open( file, "rb" ), &segy_close };

        REQUIRE( ufp );
        auto fp = ufp.get();

        char header[ SEGY_BINARY_HEADER_SIZE ];
        REQUIRE( Err( segy_binheader( fp, header ) ) == Err::ok() );

        long trace0 = segy_trace0( header );
        int samples = segy_samples( header );
        int format = segy_format( header );
        int trace_bsize = segy_trsize( format, samples );

        int sorting;
            int err = segy_sorting( fp,
                                    SEGY_TR_SEQ_LINE,
                                    SEGY_TR_SEQ_LINE,
                                    SEGY_TR_SEQ_LINE,
                                    &sorting,
                                    trace0,
                                    trace_bsize );

        CHECK( err == SEGY_OK );
        CHECK( sorting == SEGY_UNKNOWN_SORTING );
    }

    WHEN( "checking sorting when file have dimentions 1x1 ") {
        const char* file = "test-data/1x1.sgy";

        std::unique_ptr< segy_file, decltype( &segy_close ) >
            ufp{ segy_open( file, "rb" ), &segy_close };

        REQUIRE( ufp );
        auto fp = ufp.get();

        char header[ SEGY_BINARY_HEADER_SIZE ];
        REQUIRE( Err( segy_binheader( fp, header ) ) == Err::ok() );

        long trace0 = segy_trace0( header );
        int samples = segy_samples( header );
        int format = segy_format( header );
        int trace_bsize = segy_trsize( format, samples );

        int sorting;
        int err = segy_sorting( fp,
                                SEGY_TR_INLINE,
                                SEGY_TR_CROSSLINE,
                                SEGY_TR_OFFSET,
                                &sorting,
                                trace0,
                                trace_bsize );

        CHECK( err == SEGY_OK );
        CHECK( sorting == SEGY_CROSSLINE_SORTING );
    }

    WHEN( "checking sorting when file have dimentions 1xN ") {
        const char* file = "test-data/1xN.sgy";

        std::unique_ptr< segy_file, decltype( &segy_close ) >
            ufp{ segy_open( file, "rb" ), &segy_close };

        REQUIRE( ufp );
        auto fp = ufp.get();

        char header[ SEGY_BINARY_HEADER_SIZE ];
        REQUIRE( Err( segy_binheader( fp, header ) ) == Err::ok() );

        long trace0 = segy_trace0( header );
        int samples = segy_samples( header );
        int format = segy_format( header );
        int trace_bsize = segy_trsize( format, samples );

        int sorting;
        int err = segy_sorting( fp,
                                SEGY_TR_INLINE,
                                SEGY_TR_CROSSLINE,
                                SEGY_TR_OFFSET,
                                &sorting,
                                trace0,
                                trace_bsize );

        CHECK( err == SEGY_OK );
        CHECK( sorting == SEGY_INLINE_SORTING );
    }
}
