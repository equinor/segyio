#include <fstream>
#include <numeric>
#include <cmath>
#include <iomanip>
#include <memory>
#include <limits>
#include <vector>
#include <array>
#include <string.h>

// disable conversion from 'const _Elem' to '_Objty' MSC warnings.
// warnings reason is unknown, should be caused by Catch2 though, thus ignored
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#endif

#include <catch/catch.hpp>
#include "matchers.hpp"

#ifdef _MSC_VER
#pragma warning(pop)
#endif


#include <segyio/segy.h>

#include "test-config.hpp"

namespace {

struct segy_fclose {
    void operator()( segy_file* fp ) {
        if( fp ) segy_close( fp );
    }
};

using unique_segy = std::unique_ptr< segy_file, segy_fclose >;

segy_file* openfile( const std::string& path, const std::string& mode ) {
    const auto p = testcfg::config().apply( path.c_str() );
    unique_segy ptr( segy_open( p.c_str(), mode.c_str() ) );
    REQUIRE( ptr );

    int endianness = testcfg::config().lsbit ? SEGY_LSB : SEGY_MSB;
    int err = segy_collect_metadata( ptr.get(), endianness, -1, -1 );
    REQUIRE( err == SEGY_OK );

    testcfg::config().apply( ptr.get() );
    return ptr.release();
}

segy_file* openfile( const std::string& path, const std::string& mode, int endianness ) {
    testcfg::config().lsbit = endianness;
    return openfile( path, mode );
}

const std::string& copyfile( const std::string& src, const std::string& dst ) {
    std::ifstream  in( src, std::ios::binary );
    std::ofstream out( dst, std::ios::binary );

    out << in.rdbuf();
    return dst;
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

    static Err ok()       { return SEGY_OK; }
    static Err args()     { return SEGY_INVALID_ARGS; }
    static Err field()    { return SEGY_INVALID_FIELD; }
    static Err value()    { return SEGY_INVALID_FIELD_VALUE; }
    static Err datatype() { return SEGY_INVALID_FIELD_DATATYPE; }

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
                                      &offsets );
        THEN( "there is only one offset" ) {
            CHECK( err == Err::ok() );
            CHECK( offsets == 1 );
        }

        WHEN( "swapping inline and crossline position" ) {
            offsets = -1;
            const Err erc = segy_offsets( fp,
                                          xl, il, traces,
                                          &offsets );
            THEN( "there is only one offset" ) {
                CHECK( erc == Err::ok() );
                CHECK( offsets == 1 );
            }
        }
    }

    const int offsets = 1;

    WHEN( "determining offset labels" ) {
        int offset_index = -1;
        const Err err = segy_offset_indices( fp,
                                             of, offsets,
                                             &offset_index );
        CHECK( err == Err::ok() );
        CHECK( offset_index == expected_offset );
    }

    WHEN( "counting lines" ) {
        int ilsz = -1;
        int xlsz = -1;

        WHEN( "using segy_count_lines" ) {
            const Err err = segy_count_lines( fp,
                                              xl, offsets,
                                              &ilsz, &xlsz );
            CHECK( err == Err::ok() );
            CHECK( ilsz == expected_ilines );
            CHECK( xlsz == expected_xlines );
        }

        WHEN( "using segy_lines_count" ) {
            const Err err = segy_lines_count( fp,
                                              il, xl, sorting, offsets,
                                              &ilsz, &xlsz );
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
        fp = openfile( "test-data/small.sgy", "rb" );
    }

    smallfix( const smallfix& ) = delete;
    smallfix& operator=( const smallfix& ) = delete;

    ~smallfix() {
        segy_fclose()( fp );
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
        Err err = segy_traceheader( fp, 0, header );
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
float arbitrary_float() {
    return -1.0f;
}

}

TEST_CASE_METHOD( smallbin,
                  "use bin-interval when trace-interval is zero",
                  "[c.segy]" ) {

    int hdt = arbitrary_int();
    segy_get_binfield_int( bin, SEGY_BIN_INTERVAL, &hdt );
    REQUIRE( hdt == 4000 );

    float dt = arbitrary_float();

    const Err err = segy_sample_interval( fp, 100.0, &dt );

    CHECK( err == Err::ok() );
    CHECK( dt == 4000 );
}

TEST_CASE( "use fallback interval when both trace and bin is negative",
           "[c.segy]" ) {
    auto ufp = unique_segy{
        segy_open("test-data/interval-neg-bin-neg-trace.sgy", "rb")
    };
    auto fp = ufp.get();
    Err err = segy_collect_metadata( fp, -1, -1, 0 );
    CHECK( err == Err::ok() );

    const float fallback = 100.0;

    float dt = arbitrary_float();
    err = segy_sample_interval( fp, fallback, &dt );

    CHECK( err == Err::ok() );
    CHECK( dt == fallback );
}

TEST_CASE( "use trace interval when bin is negative",
           "[c.segy]" ) {
    auto ufp = unique_segy{
        segy_open("test-data/interval-neg-bin-pos-trace.sgy", "rb")
    };
    auto fp = ufp.get();
    Err err = segy_collect_metadata( fp, -1, -1, 0 );
    CHECK( err == Err::ok() );

    const float fallback = 100.0;
    const float expected = 4000.0;

    float dt = arbitrary_float();
    err = segy_sample_interval( fp, fallback, &dt );

    CHECK( err == Err::ok() );
    CHECK( dt == expected );
}

TEST_CASE( "use bin interval when trace is negative",
           "[c.segy]" ) {
    auto ufp = unique_segy{
        segy_open("test-data/interval-pos-bin-neg-trace.sgy", "rb")
    };
    auto fp = ufp.get();
    Err err = segy_collect_metadata( fp, -1, -1, 0 );
    CHECK( err == Err::ok() );

    const float fallback = 100.0;
    const float expected = 2000.0;

    float dt = arbitrary_float();
    err = segy_sample_interval( fp, fallback, &dt );

    CHECK( err == Err::ok() );
    CHECK( dt == expected );
}

TEST_CASE_METHOD( smallbin,
                  "samples+positions from binary header are correct",
                  "[c.segy]" ) {
    int samples = segy_samples( bin );
    long trace0 = segy_trace0( bin );
    int trace_bsize = segy_trsize( SEGY_IBM_FLOAT_4_BYTE, samples );

    CHECK( trace0      == 3600 );
    CHECK( samples     == 50 );
    CHECK( trace_bsize == 50 * 4 );
}

TEST_CASE_METHOD( smallbasic,
                  "trace count is 25",
                  "[c.segy]" ) {
    int traces;
    Err err = segy_traces( fp, &traces );
    CHECK( success( err ) );
    CHECK( traces == 25 );
}

TEST_CASE_METHOD( smallbasic,
                  "trace0 beyond EOF is an argument error",
                  "[c.segy]" ) {
    const int input_traces = arbitrary_int();
    int traces = input_traces;
    fp->metadata.trace0 = 50000;
    Err err = segy_traces( fp, &traces );
    CHECK( err == Err::args() );
    CHECK( traces == input_traces );
}

TEST_CASE_METHOD( smallbasic,
                  "negative trace0 is an argument error",
                  "[c.segy]" ) {
    const int input_traces = arbitrary_int();
    int traces = input_traces;
    fp->metadata.trace0 = -1;
    Err err = segy_traces( fp, &traces );
    CHECK( err == Err::args() );
    CHECK( traces == input_traces );
}

TEST_CASE_METHOD( smallbasic,
                  "erroneous trace_bsize is detected",
                  "[c.segy]" ) {
    const int input_traces = arbitrary_int();
    int traces = input_traces;
    const int too_long_bsize = trace_bsize + sizeof( float );
    fp->metadata.trace_bsize = too_long_bsize;
    Err err = segy_traces( fp, &traces );
    CHECK( err == SEGY_TRACE_SIZE_MISMATCH );
    CHECK( traces == input_traces );
}

TEST_CASE_METHOD( smallheader,
                  "valid trace-header fields can be read",
                  "[c.segy]" ) {

    int ilno;
    Err err = segy_get_tracefield_int( header, SEGY_TR_INLINE, &ilno );
    CHECK( success( err ) );
    CHECK( ilno == 1 );
}

TEST_CASE_METHOD( smallheader,
                  "zero header field is an argument error",
                  "[c.segy]" ) {
    const int input_value = arbitrary_int();
    auto value = input_value;
    Err err = segy_get_tracefield_int( header, 0, &value );

    CHECK( err == Err::field() );
    CHECK( value == input_value );
}

TEST_CASE_METHOD( smallheader,
                  "negative header field is an argument error",
                  "[c.segy]" ) {
    const int input_value = arbitrary_int();
    auto value = input_value;
    Err err = segy_get_tracefield_int( header, -1, &value );

    CHECK( err == Err::field() );
    CHECK( value == input_value );
}

TEST_CASE_METHOD( smallheader,
                  "unaligned header field is an argument error",
                  "[c.segy]" ) {
    const int input_value = arbitrary_int();
    auto value = input_value;
    Err err = segy_get_tracefield_int( header, SEGY_TR_INLINE + 1, &value );

    CHECK( err == Err::field() );
    CHECK( value == input_value );
}

TEST_CASE_METHOD( smallheader,
                  "too large header field is an argument error",
                  "[c.segy]" ) {
    const int32_t input_value = arbitrary_int();
    auto value = input_value;
    Err err = segy_get_tracefield_int( header, SEGY_TRACE_HEADER_SIZE + 10, &value );

    CHECK( err == Err::field() );
    CHECK( value == input_value );
}

TEST_CASE_METHOD( smallfields,
                  "inline sorting is detected",
                  "[c.segy]" ) {
    int sorting;
    Err err = segy_sorting( fp, il, xl, of, &sorting );
    CHECK( success( err ) );
    CHECK( sorting == SEGY_INLINE_SORTING );
}

TEST_CASE_METHOD( smallfields,
                  "crossline sorting is detected with swapped il/xl",
                  "[c.segy]" ) {
    int sorting;
    Err err = segy_sorting( fp, xl, il, of, &sorting );
    CHECK( success( err ) );
    CHECK( sorting == SEGY_CROSSLINE_SORTING );
}

TEST_CASE_METHOD( smallfields,
                  "invalid in-between byte offsets are detected",
                  "[c.segy]" ) {
    int sorting;
    Err err = segy_sorting( fp, il + 1, xl, of, &sorting );
    CHECK( err == SEGY_INVALID_FIELD );

    err = segy_sorting( fp, il, xl + 1, of, &sorting );
    CHECK( err == SEGY_INVALID_FIELD );

    err = segy_sorting( fp, il, xl, of + 1, &sorting );
    CHECK( err == SEGY_INVALID_FIELD );
}

TEST_CASE_METHOD( smallfields,
                  "out of bounds byte offsets are detected",
                  "[c.segy]" ) {
    int sorting;
    Err err = segy_sorting( fp, 0, xl, of, &sorting );
    CHECK( err == SEGY_INVALID_FIELD );

    err = segy_sorting( fp, il, SEGY_TRACE_HEADER_SIZE + 1, of, &sorting );
    CHECK( err == SEGY_INVALID_FIELD );
}

TEST_CASE_METHOD( smallsize,
                  "post-stack file offset-count is 1",
                  "[c.segy]" ) {
    int offsets;
    Err err = segy_offsets( fp,
                            il,
                            xl,
                            traces,
                            &offsets );

    CHECK( success( err ) );
    CHECK( offsets == 1 );
}

TEST_CASE_METHOD( smallsize,
                  "swapped il/xl post-stack file offset-count is 1",
                  "[c.segy]" ) {
    int offsets;
    Err err = segy_offsets( fp,
                            xl,
                            il,
                            traces,
                            &offsets );

    CHECK( success( err ) );
    CHECK( offsets == 1 );
}

namespace {

int field_read_size(const int field) {
    constexpr auto size = SEGY_TRACE_HEADER_SIZE
                        + SEGY_TEXT_HEADER_SIZE
                        + SEGY_BINARY_HEADER_SIZE
                        ;
    std::array< char, size > header;
    header.fill(0x01);

    int output;
    segy_get_tracefield_int(header.data(), field, &output);

    if (output == 0x0101)     return 2;
    if (output == 0x01010101) return 4;

    FAIL("unexpected pattern: 0x"
         << std::setfill('0')
         << std::setw(8)
         << std::hex
         << output
     );

    /*
     * fallthrough to stop warning - negative should never give a succesful
     * test, and FAIL() should already have aborted
     */
    return -1;
}

}

TEST_CASE("testing size", "[c.segy]") {
    /* Test to verify all header words in source are of correct size.
        Linked to issue #406
    */
    auto size = [](int x) { return field_read_size(x); };

    CHECK( 4 == size( SEGY_TR_CDP_X ));
    CHECK( 4 == size( SEGY_TR_CDP_Y ));
    CHECK( 4 == size( SEGY_TR_CROSSLINE ));
    CHECK( 4 == size( SEGY_TR_ENERGY_SOURCE_POINT ));
    CHECK( 4 == size( SEGY_TR_ENSEMBLE ));
    CHECK( 4 == size( SEGY_TR_FIELD_RECORD ));
    CHECK( 4 == size( SEGY_TR_GROUP_WATER_DEPTH ));
    CHECK( 4 == size( SEGY_TR_GROUP_X ));
    CHECK( 4 == size( SEGY_TR_GROUP_Y ));
    CHECK( 4 == size( SEGY_TR_INLINE ));
    CHECK( 4 == size( SEGY_TR_NUMBER_ORIG_FIELD ));
    CHECK( 4 == size( SEGY_TR_NUM_IN_ENSEMBLE ));
    CHECK( 4 == size( SEGY_TR_OFFSET ));
    CHECK( 4 == size( SEGY_TR_RECV_DATUM_ELEV ));
    CHECK( 4 == size( SEGY_TR_RECV_GROUP_ELEV ));
    CHECK( 4 == size( SEGY_TR_SEQ_FILE ));
    CHECK( 4 == size( SEGY_TR_SEQ_LINE ));
    CHECK( 4 == size( SEGY_TR_SHOT_POINT ));
    CHECK( 4 == size( SEGY_TR_SOURCE_DATUM_ELEV ));
    CHECK( 4 == size( SEGY_TR_SOURCE_DEPTH ));
    CHECK( 4 == size( SEGY_TR_SOURCE_SURF_ELEV ));
    CHECK( 4 == size( SEGY_TR_SOURCE_X ));
    CHECK( 4 == size( SEGY_TR_SOURCE_Y ));
    CHECK( 4 == size( SEGY_TR_TRANSDUCTION_MANT ));
    CHECK( 4 == size( SEGY_TR_UNASSIGNED1 )) ;
    CHECK( 4 == size( SEGY_TR_UNASSIGNED2 ));

    CHECK( 2 == size( SEGY_TR_ALIAS_FILT_FREQ ));
    CHECK( 2 == size( SEGY_TR_ALIAS_FILT_SLOPE ));
    CHECK( 2 == size( SEGY_TR_COORD_UNITS ));
    CHECK( 2 == size( SEGY_TR_CORRELATED ));
    CHECK( 2 == size( SEGY_TR_DATA_USE ));
    CHECK( 2 == size( SEGY_TR_DAY_OF_YEAR ));
    CHECK( 2 == size( SEGY_TR_DELAY_REC_TIME ));
    CHECK( 2 == size( SEGY_TR_DEVICE_ID ));
    CHECK( 2 == size( SEGY_TR_ELEV_SCALAR ));
    CHECK( 2 == size( SEGY_TR_GAIN_TYPE ));
    CHECK( 2 == size( SEGY_TR_GAP_SIZE ));
    CHECK( 2 == size( SEGY_TR_GEOPHONE_GROUP_FIRST ));
    CHECK( 2 == size( SEGY_TR_GEOPHONE_GROUP_LAST ));
    CHECK( 2 == size( SEGY_TR_GEOPHONE_GROUP_ROLL1 ));
    CHECK( 2 == size( SEGY_TR_GROUP_STATIC_CORR ));
    CHECK( 2 == size( SEGY_TR_GROUP_UPHOLE_TIME ));
    CHECK( 2 == size( SEGY_TR_HIGH_CUT_FREQ ));
    CHECK( 2 == size( SEGY_TR_HIGH_CUT_SLOPE ));
    CHECK( 2 == size( SEGY_TR_HOUR_OF_DAY ));
    CHECK( 2 == size( SEGY_TR_INSTR_GAIN_CONST ));
    CHECK( 2 == size( SEGY_TR_INSTR_INIT_GAIN ));
    CHECK( 2 == size( SEGY_TR_LAG_A ));
    CHECK( 2 == size( SEGY_TR_LAG_B ));
    CHECK( 2 == size( SEGY_TR_LOW_CUT_FREQ ));
    CHECK( 2 == size( SEGY_TR_LOW_CUT_SLOPE ));
    CHECK( 2 == size( SEGY_TR_MEASURE_UNIT ));
    CHECK( 2 == size( SEGY_TR_MIN_OF_HOUR ));
    CHECK( 2 == size( SEGY_TR_MUTE_TIME_END ));
    CHECK( 2 == size( SEGY_TR_MUTE_TIME_START ));
    CHECK( 2 == size( SEGY_TR_NOTCH_FILT_FREQ ));
    CHECK( 2 == size( SEGY_TR_NOTCH_FILT_SLOPE ));
    CHECK( 2 == size( SEGY_TR_OVER_TRAVEL ));
    CHECK( 2 == size( SEGY_TR_SAMPLE_COUNT ));
    CHECK( 2 == size( SEGY_TR_SAMPLE_INTER ));
    CHECK( 2 == size( SEGY_TR_SCALAR_TRACE_HEADER ));
    CHECK( 2 == size( SEGY_TR_SEC_OF_MIN ));
    CHECK( 2 == size( SEGY_TR_SHOT_POINT_SCALAR ));
    CHECK( 2 == size( SEGY_TR_SOURCE_ENERGY_DIR_VERT ));
    CHECK( 2 == size( SEGY_TR_SOURCE_ENERGY_DIR_XLINE ));
    CHECK( 2 == size( SEGY_TR_SOURCE_ENERGY_DIR_ILINE ));
    CHECK( 2 == size( SEGY_TR_SOURCE_MEASURE_EXP ));
    CHECK( 2 == size( SEGY_TR_SOURCE_MEASURE_UNIT ));
    CHECK( 2 == size( SEGY_TR_SOURCE_STATIC_CORR ));
    CHECK( 2 == size( SEGY_TR_SOURCE_TYPE ));
    CHECK( 2 == size( SEGY_TR_SOURCE_UPHOLE_TIME ));
    CHECK( 2 == size( SEGY_TR_SUBWEATHERING_VELO ));
    CHECK( 2 == size( SEGY_TR_SUMMED_TRACES ));
    CHECK( 2 == size( SEGY_TR_SWEEP_FREQ_END ));
    CHECK( 2 == size( SEGY_TR_SWEEP_FREQ_START ));
    CHECK( 2 == size( SEGY_TR_SWEEP_LENGTH ));
    CHECK( 2 == size( SEGY_TR_SWEEP_TAPERLEN_END ));
    CHECK( 2 == size( SEGY_TR_SWEEP_TAPERLEN_START ));
    CHECK( 2 == size( SEGY_TR_SWEEP_TYPE ));
    CHECK( 2 == size( SEGY_TR_TAPER_TYPE ));
    CHECK( 2 == size( SEGY_TR_TIME_BASE_CODE ));
    CHECK( 2 == size( SEGY_TR_TOT_STATIC_APPLIED ));
    CHECK( 2 == size( SEGY_TR_TRACE_ID ));
    CHECK( 2 == size( SEGY_TR_TRANSDUCTION_EXP ));
    CHECK( 2 == size( SEGY_TR_TRANSDUCTION_UNIT ));
    CHECK( 2 == size( SEGY_TR_WEATHERING_VELO ));
    CHECK( 2 == size( SEGY_TR_WEIGHTING_FAC ));
    CHECK( 2 == size( SEGY_TR_YEAR_DATA_REC ));

}

TEST_CASE_METHOD( smallshape,
                  "correct # of lines are detected",
                  "[c.segy]" ) {

    int expected_ils = 5;
    int expected_xls = 5;

    int count_inlines;
    int count_crosslines;
    Err err_count = segy_count_lines( fp,
                                      xl,
                                      offsets,
                                      &count_inlines,
                                      &count_crosslines );

    int lines_inlines;
    int lines_crosslines;
    Err err_lines = segy_lines_count( fp,
                                      il,
                                      xl,
                                      sorting,
                                      offsets,
                                      &lines_inlines,
                                      &lines_crosslines );

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
                  "line lengths are correct",
                  "[c.segy]" ) {
    CHECK( segy_inline_length( xlines ) == ilines );
    CHECK( segy_inline_length( ilines ) == xlines );
}

TEST_CASE_METHOD( smallshape,
                  "correct offset labels are detected",
                  "[c.segy]" ) {

    const std::vector< int > expected = { 1 };
    std::vector< int > labels( 1 );
    Err err = segy_offset_indices( fp,
                                   of,
                                   offsets,
                                   labels.data() );
    CHECK( success( err ) );
    CHECK_THAT( labels, Catch::Equals( expected ) );
}

TEST_CASE_METHOD( smallshape,
                  "correct inline labels are detected",
                  "[c.segy]" ) {

    const std::vector< int > expected = { 1, 2, 3, 4, 5 };
    std::vector< int > labels( 5 );

    Err err = segy_inline_indices( fp,
                                   il,
                                   sorting,
                                   ilines,
                                   xlines,
                                   offsets,
                                   labels.data() );

    CHECK( success( err ) );
    CHECK_THAT( labels, Catch::Equals( expected ) );
}

TEST_CASE_METHOD( smallshape,
                  "correct crossline labels are detected",
                  "[c.segy]" ) {

    const std::vector< int > expected = { 20, 21, 22, 23, 24 };
    std::vector< int > labels( 5 );

    Err err = segy_crossline_indices( fp,
                                      xl,
                                      sorting,
                                      ilines,
                                      xlines,
                                      offsets,
                                      labels.data() );

    CHECK( success( err ) );
    CHECK_THAT( labels, Catch::Equals( expected ) );
}

TEST_CASE_METHOD( smallshape,
                  "correct inline stride is detected",
                  "[c.segy]" ) {
    int stride;
    Err err = segy_inline_stride( sorting, ilines, &stride );
    CHECK( success( err ) );
    CHECK( stride == 1 );
}

TEST_CASE_METHOD( smallshape,
                  "correct inline stride with swapped sorting",
                  "[c.segy]" ) {
    int stride;
    Err err = segy_inline_stride( SEGY_CROSSLINE_SORTING, ilines, &stride );
    CHECK( success( err ) );
    CHECK( stride == ilines );
}

TEST_CASE_METHOD( smallcube,
                  "correct first trace is detected for an inline",
                  "[c.segy]" ) {
    const int label = inlines.at( 3 );
    int line_trace0;
    Err err = segy_line_trace0( label,
                                xlines,
                                stride,
                                offsets,
                                inlines.data(),
                                (int) inlines.size(),
                                &line_trace0 );
    CHECK( success( err ) );
    CHECK( line_trace0 == 15 );
}

TEST_CASE_METHOD( smallcube,
                  "missing inline-label is detected and reported",
                  "[c.segy]" ) {
    const int label = inlines.back() + 1;
    int line_trace0;
    Err err = segy_line_trace0( label,
                                xlines,
                                stride,
                                offsets,
                                inlines.data(),
                                (int) inlines.size(),
                                &line_trace0 );
    CHECK( err == SEGY_MISSING_LINE_INDEX );
}

TEST_CASE_METHOD( smallshape,
                  "correct crossline stride is detected",
                  "[c.segy]" ) {
    int stride;
    Err err = segy_crossline_stride( sorting, xlines, &stride );
    CHECK( success( err ) );
    CHECK( stride == ilines );
}

TEST_CASE_METHOD( smallshape,
                  "correct crossline stride with swapped sorting",
                  "[c.segy]" ) {
    int stride;
    Err err = segy_crossline_stride( SEGY_CROSSLINE_SORTING, xlines, &stride );
    CHECK( success( err ) );
    CHECK( stride == 1 );
}

TEST_CASE_METHOD( smallcube,
                  "correct first trace is detected for a crossline",
                  "[c.segy]" ) {
    const int label = crosslines.at( 2 );
    const int stride = ilines;
    int line_trace0;
    Err err = segy_line_trace0( label,
                                ilines,
                                stride,
                                offsets,
                                crosslines.data(),
                                (int) crosslines.size(),
                                &line_trace0 );
    CHECK( success( err ) );
    CHECK( line_trace0 == 2 );
}

TEST_CASE_METHOD( smallcube,
                  "missing crossline-label is detected and reported",
                  "[c.segy]" ) {
    const int label = crosslines.back() + 1;
    const int stride = ilines;
    int line_trace0;
    Err err = segy_line_trace0( label,
                                ilines,
                                stride,
                                offsets,
                                inlines.data(),
                                (int) inlines.size(),
                                &line_trace0 );
    CHECK( err == SEGY_MISSING_LINE_INDEX );
}

TEST_CASE_METHOD( smallstep,
                  "read ascending strided subtrace",
                  "[c.segy]" ) {
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
                              rangebuf );
    segy_to_native( format, xs.size(), xs.data() );
    CHECK( success( err ) );
    CHECK_THAT( xs, ApproxRange( expected ) );
}

TEST_CASE_METHOD( smallstep,
                  "read descending strided subtrace",
                  "[c.segy]" ) {
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
                              rangebuf );
    segy_to_native( format, xs.size(), xs.data() );
    CHECK( success( err ) );
    CHECK_THAT( xs, ApproxRange( expected ) );
}

TEST_CASE_METHOD( smallstep,
                  "read descending contiguous subtrace",
                  "[c.segy]" ) {
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
                              rangebuf );
    segy_to_native( format, xs.size(), xs.data() );
    CHECK( success( err ) );
    CHECK_THAT( xs, ApproxRange( expected ) );
}

TEST_CASE_METHOD( smallstep,
                  "read descending strided subtrace with pre-start",
                  "[c.segy]" ) {
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
                              nullptr );
    segy_to_native( format, xs.size(), xs.data() );
    CHECK( success( err ) );
    CHECK_THAT( xs, ApproxRange( expected ) );
}

TEST_CASE_METHOD( smallcube,
                  "reading the first inline gives correct values",
                  "[c.segy]" ) {

    // first line starts at first trace
    const int line_trace0 = 0;
    const std::vector< float > expected = [=] {
        std::vector< float > xs( samples * xlines );
        for( int i = 0; i < xlines; ++i ) {
            Err err = segy_readtrace( fp,
                                      i,
                                      xs.data() + (i * samples) );
            REQUIRE( success( err ) );
        }

        segy_to_native( format, xs.size(), xs.data() );
        return xs;
    }();

    std::vector< float > line( expected.size() );
    Err err = segy_read_line( fp,
                              line_trace0,
                              (int) crosslines.size(),
                              stride,
                              offsets,
                              line.data() );

    segy_to_native( format, line.size(), line.data() );
    CHECK( success( err ) );
    CHECK_THAT( line, ApproxRange( expected ) );
}

TEST_CASE_METHOD( smallcube,
                  "reading the first crossline gives correct values",
                  "[c.segy]" ) {

    // first line starts at first trace
    const int line_trace0 = 0;
    const int stride = ilines;
    const std::vector< float > expected = [=] {
        std::vector< float > xs( samples * ilines );
        for( int i = 0; i < ilines; ++i ) {
            Err err = segy_readtrace( fp,
                                      i * stride,
                                      xs.data() + (i * samples) );
            REQUIRE( success( err ) );
        }

        segy_to_native( format, xs.size(), xs.data() );
        return xs;
    }();

    std::vector< float > line( expected.size() );
    Err err = segy_read_line( fp,
                              line_trace0,
                              (int) inlines.size(),
                              stride,
                              offsets,
                              line.data() );

    segy_to_native( format, line.size(), line.data() );
    CHECK( success( err ) );
    CHECK_THAT( line, ApproxRange( expected ) );
}

template< int Start, int Stop, int Step >
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
        std::string name = std::string("write-sub-trace ")
                         + "[" + std::to_string( start )
                         + "," + std::to_string( stop )
                         + "," + std::to_string( step )
                         + std::string(testcfg::config().memmap ? "-mmap" : "")
                         + std::string(testcfg::config().lsbit  ? "-lsb"  : "")
                         + "].sgy";

        std::string orig = testcfg::config().lsbit
                               ? "test-data/small-lsb.sgy"
                               : "test-data/small.sgy";
        copyfile( orig, name );

        fp = openfile( name, "r+b" );

        trace.assign( 50, 0 );
        expected.resize( trace.size() );

        Err err = segy_writetrace( fp, traceno, trace.data() );
        REQUIRE( success( err ) );
    }

    ~writesubtr() {
        REQUIRE( fp );

        /* test that writes are observable */

        trace.assign( trace.size(), 0 );
        Err err = segy_readtrace( fp, traceno, trace.data() );

        CHECK( success( err ) );
        segy_to_native( format, trace.size(), trace.data() );

        CHECK_THAT( trace, ApproxRange( expected ) );
        segy_close( fp );
    }

    writesubtr( const writesubtr& ) = delete;
    writesubtr& operator=( const writesubtr& ) = delete;
};

TEST_CASE_METHOD( (writesubtr< 3, 19, 5 >),
                  "write ascending strided subtrace",
                  "[c.segy]" ) {
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
                                   rangebuf );

    CHECK( success( err ) );
}

TEST_CASE_METHOD( (writesubtr< 18, 2, -5 >),
                  "write descending strided subtrace",
                  "[c.segy]" ) {
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
                                   rangebuf );

    CHECK( success( err ) );
}

TEST_CASE_METHOD( (writesubtr< 24, -1, -5 >),
                  "write descending strided subtrace with pre-start",
                  "[c.segy]" ) {
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
                                   rangebuf );

    CHECK( success( err ) );
}

TEST_CASE_METHOD( smallfields,
                  "reading inline label from every trace header",
                  "[c.segy]" ) {
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
                                       out.data() );

    CHECK( success( err ) );
    CHECK_THAT( out, Catch::Equals( inlines ) );
}

TEST_CASE_METHOD( smallfields,
                  "reading crossline label from every trace header",
                  "[c.segy]" ) {
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
                                       out.data() );

    CHECK( success( err ) );
    CHECK_THAT( out, Catch::Equals( crosslines ) );
}

TEST_CASE_METHOD( smallfields,
                  "reading every 3rd crossline label",
                  "[c.segy]" ) {
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
                                       out.data() );

    CHECK( success( err ) );
    CHECK_THAT( out, Catch::Equals( crosslines ) );
}

TEST_CASE_METHOD( smallfields,
                  "reverse-reading every 3rd crossline label",
                  "[c.segy]" ) {
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
                                       out.data() );

    CHECK( success( err ) );
    CHECK_THAT( out, Catch::Equals( crosslines ) );
}

TEST_CASE_METHOD( smallfields,
                  "reverse-reading every 5th crossline label",
                  "[c.segy]" ) {
    const std::vector< int > crosslines = { 24, 24, 24, 24, 24 };
    const int start = 24, stop = -1, step = -5;

    std::vector< int > out( crosslines.size() );
    const Err err = segy_field_forall( fp,
                                       xl,
                                       start, stop, step,
                                       out.data() );

    CHECK( success( err ) );
    CHECK_THAT( out, Catch::Equals( crosslines ) );
}

TEST_CASE( "setting unaligned header-field fails",
           "[c.segy]" ) {
    char header[ SEGY_TRACE_HEADER_SIZE ];
    const int32_t v = arbitrary_int();

    Err err = segy_set_tracefield_int( header, SEGY_TR_INLINE + 1, v );
    CHECK( err == Err::field() );
}

TEST_CASE( "setting negative header-field fails",
           "[c.segy]" ) {
    char header[ SEGY_TRACE_HEADER_SIZE ];
    const int32_t v = arbitrary_int();

    Err err = segy_set_tracefield_int( header, -1, v );
    CHECK( err == Err::field() );
}

TEST_CASE( "setting too large header-field fails",
           "[c.segy]" ) {
    char header[ SEGY_TRACE_HEADER_SIZE ];
    const int32_t v = arbitrary_int();

    Err err = segy_set_tracefield_int( header, SEGY_TRACE_HEADER_SIZE + 10, v );
    CHECK( err == Err::field() );
}

TEST_CASE( "setting correct header fields succeeds",
           "[c.segy]" ) {
    char header[ SEGY_TRACE_HEADER_SIZE ];
    const int32_t input = 1;
    const int field = SEGY_TR_INLINE;

    Err err = segy_set_tracefield_int( header, field, input );
    CHECK( success( err ) );


    int32_t output;
    err = segy_get_tracefield_int( header, field, &output );
    CHECK( success( err ) );

    CHECK( output == input );
}

SCENARIO( "modifying trace header", "[c.segy]" ) {

    const int samples = 10;
    const float emptytr[ samples ] = {};
    const char emptyhdr[ SEGY_TRACE_HEADER_SIZE ] = {};


    WHEN( "writing iline no" ) {
        char header[ SEGY_TRACE_HEADER_SIZE ] = {};

        Err err = segy_set_tracefield_int( header, SEGY_TR_INLINE, 2 );
        CHECK( err == Err::ok() );
        err = segy_set_tracefield_int( header, SEGY_TR_SOURCE_GROUP_SCALAR, -100 );
        CHECK( err == Err::ok() );

        THEN( "the header buffer is updated") {
            int ilno = 0;
            int scale = 0;
            err = segy_get_tracefield_int( header, SEGY_TR_INLINE, &ilno );
            CHECK( err == Err::ok() );
            err = segy_get_tracefield_int( header, SEGY_TR_SOURCE_GROUP_SCALAR, &scale );
            CHECK( err == Err::ok() );

            CHECK( ilno == 2 );
            CHECK( scale == -100 );
        }

        const char* file = "write-traceheader.sgy";

        unique_segy ufp( segy_open( file, "w+b" ) );
        auto fp = ufp.get();
        fp->metadata.samplecount = samples;
        fp->metadata.trace_bsize = segy_trsize( SEGY_IBM_FLOAT_4_BYTE, samples );
        fp->metadata.trace0 = 0;

        /* make a file and write to last trace (to accurately get size) */
        err = segy_write_traceheader( fp, 10, emptyhdr );
        REQUIRE( err == Err::ok() );

        err = segy_writetrace( fp, 10, emptytr );
        REQUIRE( err == Err::ok() );
        /* memory map only after writing last trace, so size is correct */
        testcfg::config().mmap( fp );

        err = segy_write_traceheader( fp, 5, header );
        CHECK( err == Err::ok() );

        THEN( "changes are observable on disk" ) {
            char fresh[ SEGY_TRACE_HEADER_SIZE ] = {};
            int ilno = 0;
            int scale = 0;
            err = segy_traceheader( fp, 5, fresh );
            CHECK( err == Err::ok() );
            err = segy_get_tracefield_int( fresh, SEGY_TR_INLINE, &ilno );
            CHECK( err == Err::ok() );
            err = segy_get_tracefield_int( fresh, SEGY_TR_SOURCE_GROUP_SCALAR, &scale );
            CHECK( err == Err::ok() );

            CHECK( ilno == 2 );
            CHECK( scale == -100 );
        }
    }
}

namespace {

int check_sorting( const std::string& name ) {
    int sorting;

    unique_segy ufp( segy_open( name.c_str(), "rb" ) );
    auto fp = ufp.get();
    Err err = segy_collect_metadata( fp, -1, -1, 0 );
    REQUIRE( success( err ) );

    err = segy_sorting( fp,
                            SEGY_TR_INLINE,
                            SEGY_TR_CROSSLINE,
                            SEGY_TR_OFFSET,
                            &sorting );
    CHECK( success( err ) );

    return sorting;
}

}

TEST_CASE("is sorted when (il, xl, offset) decreases", "[c.segy]") {
    auto sorting = check_sorting( "test-data/small-ps-dec-il-xl-off.sgy" );
    CHECK( sorting == SEGY_INLINE_SORTING );
}

TEST_CASE("is sorted when offset increases, (il, xl) decreases", "[c.segy]") {
    auto sorting = check_sorting( "test-data/small-ps-dec-il-inc-xl-off.sgy" );
    CHECK( sorting == SEGY_INLINE_SORTING );
}

TEST_CASE("is sorted when (il, offset) increases, xl decreases", "[c.segy]") {
    auto sorting = check_sorting( "test-data/small-ps-dec-xl-inc-il-off.sgy" );
    CHECK( sorting == SEGY_INLINE_SORTING );
}

TEST_CASE("is sorted when (il, xl) increases, offset decreases", "[c.segy]") {
    auto sorting = check_sorting( "test-data/small-ps-dec-off-inc-il-xl.sgy" );
    CHECK( sorting == SEGY_INLINE_SORTING );
}

TEST_CASE("is sorted when (il, xl, offset) increases", "[c.segy]") {
    auto sorting = check_sorting( "test-data/small-ps-dec-il-xl-inc-off.sgy" );
    CHECK( sorting == SEGY_INLINE_SORTING );
}

TEST_CASE("is sorted when offset increases, (il, xl) decreases (post-stack)",
          "[c.segy]") {
    auto sorting = check_sorting( "test-data/small-ps-dec-il-off-inc-xl.sgy" );
    CHECK( sorting == SEGY_INLINE_SORTING );
}

TEST_CASE("is sorted when il increases, (xl, offset) decreases", "[c.segy]") {
    auto sorting = check_sorting( "test-data/small-ps-dec-xl-off-inc-il.sgy" );
    CHECK( sorting == SEGY_INLINE_SORTING );
}


SCENARIO( "reading text header", "[c.segy]" ) {
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

        unique_segy ufp{ openfile( file, "rb", SEGY_MSB ) };
        auto fp = ufp.get();

        char ascii[ SEGY_TEXT_HEADER_SIZE + 1 ] = {};
        const Err err = segy_read_textheader( fp, ascii );

        CHECK( err == Err::ok() );
        CHECK( ascii == expected );
}

TEST_CASE("open file with >32k traces", "[c.segy]") {
    const char* file = "test-data/long.sgy";
    unique_segy ufp(segy_open(file, "rb"));
    auto fp = ufp.get();

    char bin[SEGY_BINARY_HEADER_SIZE];
    Err err = segy_binheader(fp, bin);
    REQUIRE(err == Err::ok());

    /*
     * This file was created with 60000 samples and three traces. Before SEG-Y
     * rev2 this had a max-size of 2^15 - 1 because it was 2s complement
     * 16-bit. However, rev2 allows this to be unsigned 16-bit as a negative
     * sample-size is non-sensical.
     *
     * segy_samples is aware of this, but the get_bfield function returns
     * signed/unsigned faithfully
     */
    const auto samples = segy_samples(bin);
    CHECK(samples == 60000);
}

#ifdef HOST_BIG_ENDIAN
    #define HOST_LSB 0
    #define HOST_MSB 1
#else
    #define HOST_LSB 1
    #define HOST_MSB 0
#endif

/*
 * There is no native 3-byte integral type in C++, so hack a minimal one
 * together. We don't need arithmetic, only conversion to int32 and from int16
 * (which is what the source file is in).
 *
 * Type is incomplete, but important parts of implementation are type's size,
 * its individual bytes, and how it is created from int16. The test files were
 * created by just adding a single, 0x00 byte or 0xFF byte at the
 * most-significant byte position, and otherwise memcpy'd.
 */
struct int24_base {
    unsigned char bytes[3];

    int24_base() : bytes{0, 0, 0} {}
    // cppcheck-suppress noExplicitConstructor
    int24_base(const int16_t& x) {
        /* Sign is allowed to be negative even if type represents unsigned int,
         * which is done to keep behavior consistent with that of other formats
         */
        char sign = x < 0 ? -1 : 0;
#if HOST_LSB
        memcpy(&this->bytes[0], &x, 2);
        this->bytes[2] = sign;
#else
        this->bytes[0] = sign;
        memcpy(&this->bytes[1], &x, 2);
#endif
    }

    operator std::int32_t () const noexcept (true) {
#if HOST_LSB
        return (static_cast<std::int32_t>(this->bytes[0]) << 0)
             | (static_cast<std::int32_t>(this->bytes[1]) << 8)
             | (static_cast<std::int32_t>(this->bytes[2]) << 16)
             | (static_cast<std::int32_t>(this->bytes[2]) << 24);
#else
        return (static_cast<std::int32_t>(this->bytes[0]) << 24)
             | (static_cast<std::int32_t>(this->bytes[0]) << 16)
             | (static_cast<std::int32_t>(this->bytes[1]) << 8)
             | (static_cast<std::int32_t>(this->bytes[2]) << 0);

#endif
    }
};

static_assert(
    sizeof(int24_base) == 3,
    "int24 type is padded, but is expected to be 3 bytes"
);

static_assert(
    std::is_standard_layout< int24_base >::value,
    "int24 must be standard layout"
);

using int24 = int24_base;
using uint24 = int24_base;


/*
 * open a copy of f3, but pre-converted to a different format, to check that
 * other formats are read correctly
 */
template < typename T >
void f3_in_format(int fmt) {
    const auto name = "test-data/Format" + std::to_string(fmt);
    unique_segy ufp{ openfile(name, "rb") };
    auto* fp = ufp.get();
    REQUIRE(fp);

    char header[SEGY_BINARY_HEADER_SIZE];
    REQUIRE(Err(segy_binheader(fp, header)) == Err::ok());

    const int samples = segy_samples(header);
    const long trace0 = segy_trace0(header);
    const int format  = segy_format(header);
    const int trsize  = segy_trsize(fmt, samples);

    CHECK(samples == 75);
    CHECK(trace0  == 3600);
    CHECK(format  == fmt);
    CHECK(trsize  == int(samples * sizeof(T)));

    /* read f3 from 2-byte, expand, and compare */
    std::vector< T > fptrace(samples);
    Err err = segy_readtrace(fp,
            0,
            fptrace.data() );
    REQUIRE(err == Err::ok());

    const auto f3fmt = SEGY_SIGNED_SHORT_2_BYTE;
    unique_segy uf3{ openfile("test-data/f3.sgy", "rb") };
    auto* f3 = uf3.get();
    REQUIRE(f3);

    std::vector< std::int16_t > f3trace(samples);
    err = segy_readtrace(f3,
            0,
            f3trace.data() );
    REQUIRE(err == Err::ok());

    segy_to_native(fmt, fptrace.size(), fptrace.data());
    segy_to_native(f3fmt,  f3trace.size(), f3trace.data());

    CHECK_THAT(fptrace, SimilarRange< T >::from(f3trace));
}

/*
 * The Format family of files are all f3.sgy, but converted to the
 * corresponding numeric format
 */
TEST_CASE("can open IBM float", "[c.segy][format]") {
    f3_in_format< float >( SEGY_IBM_FLOAT_4_BYTE );
}

TEST_CASE("can open 4-byte signed integer", "[c.segy][format]") {
    f3_in_format< std::int32_t >(SEGY_SIGNED_INTEGER_4_BYTE);
}

TEST_CASE("can open 2-byte signed integer", "[c.segy][format]") {
    f3_in_format< std::int16_t >(SEGY_SIGNED_SHORT_2_BYTE);
}

TEST_CASE("can open IEEE float", "[c.segy][format]" ) {
    f3_in_format< float >(SEGY_IEEE_FLOAT_4_BYTE);
}

TEST_CASE("can open IEEE double", "[c.segy][format]") {
    f3_in_format< double >(SEGY_IEEE_FLOAT_8_BYTE);
}

TEST_CASE("can open 1-byte signed char", "[c.segy][format]") {
    f3_in_format< std::int8_t >(SEGY_SIGNED_CHAR_1_BYTE);
}

TEST_CASE("can open 3-byte signed char", "[c.segy][format][uniq]") {
    // int24 implementation sanity check
    REQUIRE(static_cast<std::int32_t>(int24(int16_t(500))) == 500);
    REQUIRE(static_cast<std::int32_t>(int24(int16_t(-500))) == -500);

    f3_in_format< int24 >(SEGY_SIGNED_CHAR_3_BYTE);
}

TEST_CASE("can open 8-byte signed integer", "[c.segy][format]") {
    f3_in_format< std::int64_t >(SEGY_SIGNED_INTEGER_8_BYTE);
}

TEST_CASE("can open 4-byte unsigned integer", "[c.segy][format]") {
    f3_in_format< std::uint32_t >(SEGY_UNSIGNED_INTEGER_4_BYTE);
}

TEST_CASE("can open 2-byte unsigned short", "[c.segy][format]") {
    f3_in_format< std::uint16_t >(SEGY_UNSIGNED_SHORT_2_BYTE);
}

TEST_CASE("can open 8-byte unsigned integer", "[c.segy][format]") {
    f3_in_format< std::uint64_t >(SEGY_UNSIGNED_INTEGER_8_BYTE);
}

TEST_CASE("can open 3-byte unsigned integer", "[c.segy][format]") {
    // uint24 implementation sanity check
    REQUIRE(static_cast<std::int32_t>(uint24(int16_t(500))) == 500);

    f3_in_format< uint24 >(SEGY_UNSIGNED_INTEGER_3_BYTE);
}

TEST_CASE("can open 1-byte unsigned char", "[c.segy][format]") {
    f3_in_format< std::uint8_t >(SEGY_UNSIGNED_CHAR_1_BYTE);
}

SCENARIO( "reading a 2-byte int file", "[c.segy][2-byte]" ) {
    unique_segy ufp{ openfile( "test-data/f3.sgy", "rb" ) };
    auto fp = ufp.get();

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
    }

    const int format      = SEGY_SIGNED_SHORT_2_BYTE;
    const long trace0     = 3600;
    const int samples     = 75;
    const int trace_bsize = samples * 2;

    WHEN( "reading data without collecting metadata" ) {
        unique_segy ufp( segy_open( "test-data/f3.sgy", "rb" ) );
        auto fp = ufp.get();
        fp->metadata.trace0 = trace0;
        fp->metadata.trace_bsize = trace_bsize;
        /*
         * explicitly zero this buffer - if segy_collect_metadata is called then
         * this function should read 2 bytes, but now 4 is read instead.
         */
        std::int16_t val[2] = { 0, 0 };
        Err err = segy_readsubtr( fp, 10,
                                      25, 26, 1,
                                      &val,
                                      nullptr );

        CHECK( err == Err::ok() );

        err = segy_to_native( format, 1, &val );
        CHECK( err == Err::ok() );

        THEN( "the value is incorrect" ) {
            CHECK( val[0] != -1170 );
            CHECK( val[1] != 0 );
        }
    }

    Err err = Err::ok();

    WHEN( "determining number of traces" ) {
        int traces = 0;
        err = segy_traces( fp, &traces );
        REQUIRE( err == Err::ok() );
        CHECK( traces == 414 );

        GIVEN( "trace0 outside its domain" ) {
            WHEN( "trace0 is after end-of-file" ) {
                fp->metadata.trace0 = 500000;
                err = segy_traces( fp, &traces);

                THEN( "segy_traces fail" ) {
                    CHECK( err == Err::args() );
                }

                THEN( "the input does not change" ) {
                    CHECK( traces == 414 );
                }
            }

            WHEN( "trace0 is negative" ) {
                fp->metadata.trace0 = -1;
                err = segy_traces( fp, &traces );

                THEN( "segy_traces fail" ) {
                    CHECK( err == Err::args() );
                }

                THEN( "the input does not change" ) {
                    CHECK( traces == 414 );
                }
            }
        }
    }

    const int traces = 414;
    const int ilines = 23;
    const int xlines = 18;
    const int offset_label = 0;
    regular_geometry( fp, traces,
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
            err = segy_inline_indices( fp,
                                       il,
                                       sorting,
                                       ilines,
                                       xlines,
                                       offsets,
                                       result.data() );
            CHECK( err == Err::ok() );
            CHECK_THAT( result, Catch::Equals( indices ) );
        }
    }

    WHEN( "reading a trace header" ) {
        char buf[ SEGY_TRACE_HEADER_SIZE ] = {};

        GIVEN( "a valid field" ) {
            err = segy_traceheader( fp, 0, buf );
            CHECK( err == Err::ok() );

            int ilno = 0;
            err = segy_get_tracefield_int( buf, SEGY_TR_INLINE, &ilno );
            CHECK( err == Err::ok() );
            CHECK( ilno == 111 );
        }

        GIVEN( "an invalid field" ) {
            int x = -1;
            err = segy_get_tracefield_int( buf, SEGY_TRACE_HEADER_SIZE + 10, &x );
            CHECK( err == Err::field() );
            CHECK( x == -1 );

            err = segy_get_tracefield_int( buf, SEGY_TR_INLINE + 1, &x );
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

                err = segy_readsubtr( fp,
                                      10,
                                      start, stop, step,
                                      buf.data(),
                                      nullptr );
                segy_to_native( format, buf.size(), buf.data() );
                CHECK( err == Err::ok() );
                CHECK_THAT( buf, Catch::Equals( expect[ i ] ) );
            }
        }
    }
}

SCENARIO( "checking sorting for wonky files", "[c.segy]" ) {
    WHEN( "checking sorting when il, xl and offset are of unsupported type ") {
        unique_segy ufp{ openfile( "test-data/small.sgy", "rb" ) };
        auto fp = ufp.get();

        char header[ SEGY_BINARY_HEADER_SIZE ];
        REQUIRE( Err( segy_binheader( fp, header ) ) == Err::ok() );

        int sorting;
        int err;

        err = segy_sorting(
            fp,
            SEGY_TR_SEQ_LINE,
            SEGY_TR_CROSSLINE,
            SEGY_TR_OFFSET,
            &sorting
        );
        CHECK( err == SEGY_INVALID_FIELD_DATATYPE );

        err = segy_sorting(
            fp,
            SEGY_TR_INLINE,
            SEGY_TR_DAY_OF_YEAR,
            SEGY_TR_OFFSET,
            &sorting
        );
        CHECK( err == SEGY_INVALID_FIELD_DATATYPE );
    }

    WHEN( "checking sorting when il, xl and offset is all garbage ") {
        /*
         * In the case where all tracefields ( il, xl, offset ) = ( 0, 0, 0 )
         * the sorting detection should return 'SEGY_INVALID_SORTING'. To test
         * this the traceheader field 'SEGY_TR_FIELD_RECORD', which we know is
         * zero in all traceheaders in file small.sgy, is passed to segy_sorting
         * as il, xl and offset.
         */
        unique_segy ufp{ openfile( "test-data/small.sgy", "rb" ) };
        auto fp = ufp.get();

        char header[ SEGY_BINARY_HEADER_SIZE ];
        REQUIRE( Err( segy_binheader( fp, header ) ) == Err::ok() );

        int sorting;
            int err = segy_sorting( fp,
                                    SEGY_TR_FIELD_RECORD,
                                    SEGY_TR_FIELD_RECORD,
                                    SEGY_TR_FIELD_RECORD,
                                    &sorting );

        CHECK( err == SEGY_OK );
        CHECK( sorting == SEGY_UNKNOWN_SORTING );
    }

    WHEN( "checking sorting when file have dimentions 1x1 ") {
        const char* file = "test-data/1x1.sgy";

        unique_segy ufp( segy_open( file, "rb" ) );
        auto fp = ufp.get();
        testcfg::config().mmap( fp );
        int err = segy_collect_metadata( fp, -1, -1, 0 );
        REQUIRE( success( err ) );

        char header[ SEGY_BINARY_HEADER_SIZE ];
        REQUIRE( Err( segy_binheader( fp, header ) ) == Err::ok() );

        int sorting;
        err = segy_sorting( fp,
                            SEGY_TR_INLINE,
                            SEGY_TR_CROSSLINE,
                            SEGY_TR_OFFSET,
                            &sorting );

        CHECK( err == SEGY_OK );
        CHECK( sorting == SEGY_CROSSLINE_SORTING );
    }

    WHEN( "checking sorting when file have dimentions 1xN ") {
        const char* file = "test-data/1xN.sgy";

        unique_segy ufp( segy_open( file, "rb" ) );
        auto fp = ufp.get();
        testcfg::config().mmap( fp );
        int err = segy_collect_metadata( fp, -1, -1, 0 );
        REQUIRE( success( err ) );

        char header[ SEGY_BINARY_HEADER_SIZE ];
        REQUIRE( Err( segy_binheader( fp, header ) ) == Err::ok() );

        int sorting;
        err = segy_sorting( fp,
                            SEGY_TR_INLINE,
                            SEGY_TR_CROSSLINE,
                            SEGY_TR_OFFSET,
                            &sorting );

        CHECK( err == SEGY_OK );
        CHECK( sorting == SEGY_INLINE_SORTING );
    }
}

TEST_CASE("1-byte header words are correctly read", "[c.segy]") {
    std::array< char, 400 > header {};
    header[300] = 0x01;
    header[301] = 0x02;

    int one;
    int two;

    Err err = segy_get_binfield_int(header.data(), SEGY_BIN_SEGY_REVISION, &one);
    CHECK(err == Err::ok());
    CHECK(one == 0x01);

    err = segy_get_binfield_int(header.data(), SEGY_BIN_SEGY_REVISION_MINOR, &two);
    CHECK(err == Err::ok());
    CHECK(two == 0x02);
}

TEST_CASE("segy_samples uses ext-samples word", "[c.segy]") {
    std::array< char, 400 > header {};

    static constexpr auto ext_samples = 68;

    SECTION("when samples word is zero") {
        /* ext-samples lowest byte */
        header[ext_samples + 3] = 0x01;
        const auto samples = segy_samples(header.data());
        CHECK(samples == 1);
    }

    SECTION("when the rev2 word is set") {
        header[300] = 0x02;
        header[ext_samples + 3] = 0x01;
        const auto samples = segy_samples(header.data());
        CHECK(samples == 1);
    }
}

TEST_CASE( "segy_get_field reads values correctly", "[c.segy]" ) {

    char header[SEGY_TRACE_HEADER_SIZE] = { 0 };

    int16_t value = GENERATE( 0, 1, -1, 0x0102, 0x0201, -32767, -32766 );
    uint8_t b0 = 0xFF & ( (uint16_t)value >> 0 );
    uint8_t b1 = 0xFF & ( (uint16_t)value >> 8 );

    SECTION( "test get trace field, edge cases int16" ) {
        header[SEGY_TR_TRACE_ID - 1] = b1;
        header[SEGY_TR_TRACE_ID - 0] = b0;

        segy_field_data read_value;
        Err err = segy_get_tracefield(
            header, segy_traceheader_default_map(), SEGY_TR_TRACE_ID, &read_value
        );
        CHECK( success( err ) );
        CHECK( read_value.entry_type == SEGY_ENTRY_TYPE_INT2 );
        CHECK( read_value.value.i16 == value );
    }

    SECTION( "test get bin field, edge cases int16" ) {
        header[SEGY_BIN_SORTING_CODE - 3201] = b1;
        header[SEGY_BIN_SORTING_CODE - 3200] = b0;

        segy_field_data read_value;
        Err err = segy_get_binfield(
            header, SEGY_BIN_SORTING_CODE, &read_value
        );
        CHECK( success( err ) );
        CHECK( read_value.entry_type == SEGY_ENTRY_TYPE_INT2 );
        CHECK( read_value.value.i16 == value );
    }
}

TEST_CASE( "segy_set_field write values correctly", "[c.segy]" ) {

    char header[SEGY_TRACE_HEADER_SIZE] = { 0 };
    int16_t value = GENERATE( 0, 1, -1, 0x0102, 0x0201, -32767, -32766 );

    segy_field_data fd;
    fd.entry_type = SEGY_ENTRY_TYPE_INT2;
    fd.value.i16 = value;

    SECTION( "test set trace field, edge cases int16" ) {
        Err err = segy_set_tracefield(
            header, segy_traceheader_default_map(), SEGY_TR_TRACE_ID, fd
        );
        CHECK( err == Err::ok() );

        uint8_t b0 = header[SEGY_TR_TRACE_ID - 0];
        uint8_t b1 = header[SEGY_TR_TRACE_ID - 1];
        uint16_t read_value = b1 << 8 | b0;
        CHECK( read_value == (uint16_t)value );
    }

    SECTION( "test set bin field, edge cases int16" ) {
        Err err = segy_set_tracefield(
            header, segy_traceheader_default_map(), SEGY_TR_TRACE_ID, fd
        );
        CHECK( err == Err::ok() );

        uint8_t b0 = header[SEGY_TR_TRACE_ID - 0];
        uint8_t b1 = header[SEGY_TR_TRACE_ID - 1];
        uint16_t read_value = b1 << 8 | b0;
        CHECK( read_value == (uint16_t)value );
    }
}

TEST_CASE( "segy_get/set_bin/tracefield errors", "[c.segy]" ) {

    char header[SEGY_TRACE_HEADER_SIZE] = { 0 };

    SECTION( "test value outside of int16 range for segy_set_tracefield_int" ) {
        int32_t value = 0xFFFF + 1;
        Err err = segy_set_tracefield_int( header, SEGY_TR_TRACE_ID, value );
        CHECK( err == Err::value() );
    }

    SECTION( "test mismatched type for segy_set_tracefield" ) {
        segy_field_data fd;
        fd.value.i16 = 42;
        fd.entry_type = SEGY_ENTRY_TYPE_INT4;
        Err err = segy_set_tracefield(
            header, segy_traceheader_default_map(), SEGY_TR_TRACE_ID, fd
        );
        CHECK( err == Err::datatype() );
    }

    SECTION( "test traceheader field offset out of bounds" ) {
        segy_field_data fd;

        Err err = segy_get_tracefield(
            header, segy_traceheader_default_map(), 0, &fd
        );
        CHECK( err == Err::field() );

        err = segy_get_tracefield(
            header, segy_traceheader_default_map(), SEGY_TRACE_HEADER_SIZE + 1, &fd
        );
        CHECK( err == Err::field() );
    }

    SECTION( "test binheader field offset out of bounds" ) {
        segy_field_data fd;

        Err err = segy_get_binfield(
            header, 0, &fd
        );
        CHECK( err == Err::field() );

        err = segy_get_binfield(
            header, SEGY_BINARY_HEADER_SIZE + SEGY_TEXT_HEADER_SIZE + 1, &fd
        );
        CHECK( err == Err::field() );
    }
}
