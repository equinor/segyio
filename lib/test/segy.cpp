#include <cmath>
#include <memory>
#include <limits>
#include <vector>

#include <catch/catch.hpp>

#include <segyio/segy.h>
#include <segyio/util.h>

#ifndef MMAP_TAG
#define MMAP_TAG ""
#endif

namespace {

struct slice { int start, stop, step; };
std::string str( const slice& s ) {
    return "(" + std::to_string( s.start ) +
           "," + std::to_string( s.stop ) +
           "," + std::to_string( s.step ) +
           ")";
}

class ApproxRange : public Catch::MatcherBase< std::vector< float > > {
    public:
        explicit ApproxRange( const std::vector< float >& xs ) :
            lhs( xs )
        {}

        virtual bool match( const std::vector< float >& xs ) const override {
            if( xs.size() != lhs.size() ) return false;

            for( size_t i = 0; i < xs.size(); ++i )
                if( xs[ i ] != Approx(this->lhs[ i ]) ) return false;

            return true;
        }

        virtual std::string describe() const override {
            using str = Catch::StringMaker< std::vector< float > >;
            return "~= " + str::convert( this->lhs );
        }

    private:
        std::vector< float > lhs;
};

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

SCENARIO( MMAP_TAG "reading a file", "[c.segy]" MMAP_TAG ) {
    const char* file = "test-data/small.sgy";

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
        int trace_bsize = segy_trace_bsize( samples );

        CHECK( trace0      == 3600 );
        CHECK( samples     == 50 );
        CHECK( trace_bsize == 50 * 4 );
    }

    const long trace0 = 3600;
    const int trace_bsize = 50 * 4;
    const int samples = 50;

    WHEN( "determining number of traces" ) {
        int traces = 0;
        Err err = segy_traces( fp, &traces, trace0, trace_bsize );
        REQUIRE( err == Err::ok() );
        CHECK( traces == 25 );

        GIVEN( "trace0 outside its domain" ) {
            WHEN( "trace0 is after end-of-file" ) {
                err = segy_traces( fp, &traces, 50000, trace_bsize );
                THEN( "segy_traces fail" )
                    CHECK( err == Err::args() );
                THEN( "the input does not change" )
                    CHECK( traces == 25 );
            }

            WHEN( "trace0 is negative" ) {
                err = segy_traces( fp, &traces, -1, trace_bsize );
                THEN( "segy_traces fail" )
                    CHECK( err == Err::args() );
                THEN( "the input does not change" )
                    CHECK( traces == 25 );
            }
        }
    }

    const int traces = 25;

    const int il = SEGY_TR_INLINE;
    const int xl = SEGY_TR_CROSSLINE;
    const int of = SEGY_TR_OFFSET;
    GIVEN( "an inline sorted file" ) {
        THEN( "inline sorting is inferred" ) {
            int sorting = -1;
            const Err err = segy_sorting( fp,
                                          il, xl, of,
                                          &sorting,
                                          trace0, trace_bsize );
            CHECK( err == Err::ok() );
            CHECK( sorting == SEGY_INLINE_SORTING );
        }

        WHEN( "swapping inline and crossline position" ) {
            THEN( "crossline sorting is inferred" ) {
                int sorting = -1;
                const Err err = segy_sorting( fp,
                                              xl, il, of,
                                              &sorting,
                                              trace0, trace_bsize );
                CHECK( err == Err::ok() );
                CHECK( sorting == SEGY_CROSSLINE_SORTING );
            }
        }
    }

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
        CHECK( offset_index == 1 );
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
            CHECK( ilsz == 5 );
            CHECK( xlsz == 5 );
        }

        WHEN( "using segy_lines_count" ) {
            const Err err = segy_lines_count( fp,
                                              il, xl, sorting, offsets,
                                              &ilsz, &xlsz,
                                              trace0, trace_bsize );
            CHECK( err == Err::ok() );
            CHECK( ilsz == 5 );
            CHECK( xlsz == 5 );
        }
    }

    const int inlines_sizes = 5;
    const int crosslines_sizes = 5;
    const int format = SEGY_IBM_FLOAT_4_BYTE;

    WHEN( "inferring inline structure" ) {
        const std::vector< int > indices = { 1, 2, 3, 4, 5 };
        CHECK( segy_inline_length( crosslines_sizes ) == 5 );

        WHEN( "finding inline numbers" ) {
            std::vector< int > result( inlines_sizes );
            const Err err = segy_inline_indices( fp,
                                                 il,
                                                 sorting,
                                                 inlines_sizes,
                                                 crosslines_sizes,
                                                 offsets,
                                                 result.data(),
                                                 trace0, trace_bsize );
            CHECK( err == Err::ok() );
            CHECK_THAT( result, Catch::Equals( indices ) );
        }

        WHEN( "determining inline 4's first trace number" ) {
            GIVEN( "an inline sorted file" ) THEN( "the stride is 1" ) {
                int stride = -1;
                Err err = segy_inline_stride( sorting, inlines_sizes, &stride );
                CHECK( err == Err::ok() );
                CHECK( stride == 1 );
            }

            const int stride = 1;
            int line_trace0 = -1;
            const Err err = segy_line_trace0( 4,
                                              crosslines_sizes, stride,
                                              offsets,
                                              indices.data(), inlines_sizes,
                                              &line_trace0 );
            CHECK( err == Err::ok() );
            CHECK( line_trace0 == 15 );
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
            CHECK( ilno == 1 );
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

    WHEN( "inferring crossline structure" ) {
        const std::vector< int > indices = { 20, 21, 22, 23, 24 };
        CHECK( segy_crossline_length( inlines_sizes ) == 5 );

        WHEN( "finding crossline numbers" ) {
            std::vector< int > result( crosslines_sizes );
            const Err err = segy_crossline_indices( fp,
                                                    xl,
                                                    sorting,
                                                    inlines_sizes,
                                                    crosslines_sizes,
                                                    offsets,
                                                    result.data(),
                                                    trace0, trace_bsize );
            CHECK( err == Err::ok() );
            CHECK_THAT( result, Catch::Equals( indices ) );
        }

        WHEN( "determining crossline 22's first trace number" ) {
            GIVEN( "an inline sorted file" ) THEN( "the stride is 5" ) {
                int stride = -1;
                Err err = segy_crossline_stride( sorting,
                                                 crosslines_sizes,
                                                 &stride );
                CHECK( err == Err::ok() );
                CHECK( stride == 5 );
            }

            const int stride = 5;
            int line_trace0 = -1;
            const Err err = segy_line_trace0( 22,
                                              inlines_sizes, stride,
                                              offsets,
                                              indices.data(), crosslines_sizes,
                                              &line_trace0 );
            CHECK( err == Err::ok() );
            CHECK( line_trace0 == 2 );
        }
    }

    WHEN( "reading a subtrace" ) {

        const std::vector< slice > inputs = {
            {  3,  19,   5 },
            { 18,   2,  -5 },
            {  3,  -1,  -1 },
            { 24,  -1,  -5 }
        };

        const std::vector< std::vector< float > > expect = {
            { 3.20003f, 3.20008f, 3.20013f, 3.20018f },
            { 3.20018f, 3.20013f, 3.20008f, 3.20003f },
            { 3.20003f, 3.20002f, 3.20001f, 3.20000f },
            { 3.20024f, 3.20019f, 3.20014f, 3.20009f, 3.20004f }
        };

        for( size_t i = 0; i < inputs.size(); ++i ) {
            WHEN( "slice is " + str( inputs[ i ] ) ) {
                std::vector< float > buf( expect[ i ].size() );

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
                CHECK_THAT( buf, ApproxRange( expect[ i ] ) );
            }
        }
    }

    const std::vector< int > inlines = { 1, 2, 3, 4, 5 };
    const std::vector< int > crosslines = { 20, 21, 22, 23, 24 };
    const int inline_length = 5;
    const int crossline_length = 5;

    WHEN( "reading an inline" ) {
        for( const auto il : inlines ) {
            std::vector< float > line( inline_length * samples );
            const int stride = 1;

            int line_trace0 = -1;
            Err err = segy_line_trace0( il,
                                        inline_length,
                                        stride,
                                        offsets,
                                        inlines.data(),
                                        inlines.size(),
                                        &line_trace0 );
            REQUIRE( err == Err::ok() );
            REQUIRE( line_trace0 != -1 );

            err = segy_read_line( fp,
                                  line_trace0,
                                  inlines_sizes,
                                  stride,
                                  offsets,
                                  line.data(),
                                  trace0, trace_bsize );
            REQUIRE( err == Err::ok() );
            segy_to_native( format, line.size(), line.data() );

            for( const auto xl : crosslines ) {
                auto point = std::to_string(il) + ", " + std::to_string(xl);
                THEN( "at intersection (" + point + ")" ) {
                    const auto i = (xl - 20) * samples;
                    for( size_t s = 0; s < samples; ++s ) {
                        const float expected = il + (0.01 * xl) + (1e-5 * s);
                        CHECK( line.at(i+s) == Approx( expected ) );
                    }
                }
            }
        }
    }

    WHEN( "reading a crossline" ) {
        for( const auto xl : crosslines ) {
            std::vector< float > line( crossline_length * samples );
            const int stride = 5;

            int line_trace0 = -1;
            Err err = segy_line_trace0( xl,
                                        crosslines_sizes,
                                        stride,
                                        offsets,
                                        crosslines.data(),
                                        crosslines.size(),
                                        &line_trace0 );
            REQUIRE( err == Err::ok() );
            REQUIRE( line_trace0 != -1 );

            err = segy_read_line( fp,
                                  line_trace0,
                                  crosslines_sizes,
                                  stride,
                                  offsets,
                                  line.data(),
                                  trace0, trace_bsize );
            REQUIRE( err == Err::ok() );
            segy_to_native( format, line.size(), line.data() );

            for( const auto il : inlines ) {
                auto point = std::to_string(il) + ", " + std::to_string(xl);
                THEN( "at intersection (" + point + ")" ) {
                    const auto i = (il - 1) * samples;
                    for( size_t s = 0; s < samples; ++s ) {
                        const float expected = il + (0.01 * xl) + (1e-5 * s);
                        CHECK( line.at(i+s) == Approx( expected ) );
                    }
                }
            }
        }
    }
}

SCENARIO( MMAP_TAG "writing to a file", "[c.segy]" MMAP_TAG ) {
    const long trace0 = 3600;
    const int trace_bsize = 50 * 4;
    const float dummy[ 50 ] = {};

    WHEN( "writing parts of a trace" ) {
        const int format = SEGY_IBM_FLOAT_4_BYTE;

        const std::vector< slice > inputs = {
            {  3,  19,   5 },
            { 18,   2,  -5 },
            {  3,  -1,  -1 },
            { 24,  -1,  -5 }
        };

        const std::vector< std::vector< float > > expect = {
            { 3.20003f, 3.20008f, 3.20013f, 3.20018f },
            { 3.20018f, 3.20013f, 3.20008f, 3.20003f },
            { 3.20003f, 3.20002f, 3.20001f, 3.20000f },
            { 3.20024f, 3.20019f, 3.20014f, 3.20009f, 3.20004f }
        };

        for( size_t i = 0; i < inputs.size(); ++i ) {
            WHEN( "slice is " + str( inputs[ i ] ) ) {
                const auto file = "wsubtr" MMAP_TAG + str(inputs[i]) + ".sgy";

                std::unique_ptr< segy_file, decltype( &segy_close ) >
                    ufp{ segy_open( file.c_str(), "w+b" ), &segy_close };

                REQUIRE( ufp );
                auto fp = ufp.get();

                Err err = segy_writetrace( fp, 10, dummy, trace0, trace_bsize );
                REQUIRE( err == Err::ok() );

                if( MMAP_TAG != std::string("") )
                    REQUIRE( Err( segy_mmap( fp ) ) == Err::ok() );

                std::vector< float > buf( expect[ i ].size() );

                auto start = inputs[ i ].start;
                auto stop  = inputs[ i ].stop;
                auto step  = inputs[ i ].step;

                auto out = expect[ i ];
                segy_from_native( format, out.size(), out.data() );

                err = segy_writesubtr( fp,
                                       i,
                                       start, stop, step,
                                       out.data(),
                                       nullptr,
                                       trace0, trace_bsize );
                CHECK( err == Err::ok() );
                THEN( "updates are observable" ) {
                    err = segy_readsubtr( fp,
                                          i,
                                          start, stop, step,
                                          buf.data(),
                                          nullptr,
                                          trace0, trace_bsize );
                    segy_to_native( format, buf.size(), buf.data() );
                    CHECK( err == Err::ok() );
                    CHECK_THAT( buf, ApproxRange( expect[ i ] ) );
                }
            }
        }
    }
}

SCENARIO( MMAP_TAG "extracting header fields", "[c.segy]" MMAP_TAG ) {

    const char* file = "test-data/small.sgy";

    std::unique_ptr< segy_file, decltype( &segy_close ) >
        ufp{ segy_open( file, "rb" ), &segy_close };

    REQUIRE( ufp );
    auto fp = ufp.get();

    const int trace0 = 3600;
    const int trace_bsize = 50 * 4;

    if( MMAP_TAG != std::string("") )
        REQUIRE( Err( segy_mmap( fp ) ) == Err::ok() );

    WHEN( "reading inline labels" ) {
        const std::vector< int > inlines = {
            1, 1, 1, 1, 1,
            2, 2, 2, 2, 2,
            3, 3, 3, 3, 3,
            4, 4, 4, 4, 4,
            5, 5, 5, 5, 5,
        };

        std::vector< int > buf( inlines.size() );

        const slice input = { 0, 25, 1 };
        const auto start = input.start;
        const auto stop  = input.stop;
        const auto step  = input.step;

        const Err err = segy_field_forall( fp,
                                           SEGY_TR_INLINE,
                                           start, stop, step,
                                           buf.data(),
                                           trace0, trace_bsize );
        CHECK( err == Err::ok() );

        WHEN( "in range " + str( input ) )
            CHECK_THAT( buf, Catch::Equals( inlines ) );
    }

    WHEN( "reading crossline labels" ) {
        const std::vector< std::pair< slice, std::vector< int > > > pairs = {
            { {  0, 25,  1 }, { 20, 21, 22, 23, 24,
                                20, 21, 22, 23, 24,
                                20, 21, 22, 23, 24,
                                20, 21, 22, 23, 24,
                                20, 21, 22, 23, 24, }, },

            { {  1, 25,  3 }, {     21,         24,
                                        22,
                                20,         23,
                                    21,         24,
                                        22,         }, },

            { { 22,  0, -3 }, {         22,
                                24,         21,
                                    23,         20,
                                        22,
                                24,         21      }, },

            { { 24, -1, -5 }, { 24, 24, 24, 24, 24  }, },
        };

        for( const auto& p : pairs ) {
            const auto& input = p.first;
            const auto& xl = p.second;

            std::vector< int > buf( xl.size() );

            const auto start = input.start;
            const auto stop  = input.stop;
            const auto step  = input.step;

            const Err err = segy_field_forall( fp,
                                               SEGY_TR_CROSSLINE,
                                               start, stop, step,
                                               buf.data(),
                                               trace0, trace_bsize );
            CHECK( err == Err::ok() );

            WHEN( "in range " + str( input ) )
                CHECK_THAT( buf, Catch::Equals( xl ) );
        }
    }
}

SCENARIO( MMAP_TAG "modifying trace header", "[c.segy]" MMAP_TAG ) {

    const int samples = 10;
    const int trace_bsize = segy_trace_bsize( samples );
    const int trace0 = 0;
    const float emptytr[ samples ] = {};
    const char emptyhdr[ SEGY_TRACE_HEADER_SIZE ] = {};


    WHEN( "writing iline no" ) {
        char header[ SEGY_TRACE_HEADER_SIZE ] = {};

        GIVEN( "an invalid field" ) {
            Err err = segy_set_field( header, SEGY_TR_INLINE + 1, 2 );
            CHECK( err == Err::field() );
        }

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
