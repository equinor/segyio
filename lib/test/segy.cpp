#include <memory>
#include <vector>

#include <catch/catch.hpp>

#include <segyio/segy.h>

#ifndef MMAP_TAG
#define MMAP_TAG ""
#endif

SCENARIO( MMAP_TAG "reading a file", "[c.segy]" MMAP_TAG ) {
    const char* file = "test-data/small.sgy";

    std::unique_ptr< segy_file, decltype( &segy_close ) >
        ufp{ segy_open( file, "rb" ), &segy_close };

    REQUIRE( ufp );

    auto fp = ufp.get();
    if( MMAP_TAG != std::string("") )
        REQUIRE( segy_mmap( fp ) == 0 );

    WHEN( "finding traces initial byte offset and sizes" ) {
        char header[ SEGY_BINARY_HEADER_SIZE ];
        REQUIRE( segy_binheader( fp, header ) == 0 );
        int samples = segy_samples( header );
        long trace0 = segy_trace0( header );
        int trace_bsize = segy_trace_bsize( samples );

        CHECK( trace0      == 3600 );
        CHECK( samples     == 50 );
        CHECK( trace_bsize == 50 * 4 );
    }

    const long trace0 = 3600;
    const int trace_bsize = 50 * 4;

    WHEN( "determining number of traces" ) {
        int traces = 0;
        REQUIRE( segy_traces( fp, &traces, trace0, trace_bsize ) == 0 );
        CHECK( traces == 25 );

        GIVEN( "trace0 outside its domain" ) {
            WHEN( "trace0 is after end-of-file" ) {
                const int err = segy_traces( fp, &traces, 50000, trace_bsize );
                THEN( "segy_traces fail" ) CHECK( err == SEGY_INVALID_ARGS );
                THEN( "the input does not change" ) CHECK( traces == 25 );
            }

            WHEN( "trace0 is negative" ) {
                const int err = segy_traces( fp, &traces, -1, trace_bsize );
                THEN( "segy_traces fail" ) CHECK( err == SEGY_INVALID_ARGS );
                THEN( "the input does not change" ) CHECK( traces == 25 );
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
            const int err = segy_sorting( fp,
                                          il, xl, of,
                                          &sorting,
                                          trace0, trace_bsize );
            CHECK( err == 0 );
            CHECK( sorting == SEGY_INLINE_SORTING );
        }

        WHEN( "swapping inline and crossline position" ) {
            THEN( "crossline sorting is inferred" ) {
                int sorting = -1;
                const int err = segy_sorting( fp,
                                              xl, il, of,
                                              &sorting,
                                              trace0, trace_bsize );
                CHECK( err == 0 );
                CHECK( sorting == SEGY_CROSSLINE_SORTING );
            }
        }
    }

    const int sorting = SEGY_INLINE_SORTING;

    GIVEN( "a post stack file" ) {
        int offsets = -1;
        const int err = segy_offsets( fp,
                                      il, xl, traces,
                                      &offsets,
                                      trace0, trace_bsize );
        THEN( "there is only one offset" ) {
            CHECK( err == 0 );
            CHECK( offsets == 1 );
        }

        WHEN( "swapping inline and crossline position" ) {
            int offsets = -1;
            const int err = segy_offsets( fp,
                                          xl, il, traces,
                                          &offsets,
                                          trace0, trace_bsize );
            THEN( "there is only one offset" ) {
                CHECK( err == 0 );
                CHECK( offsets == 1 );
            }
        }
    }

    const int offsets = 1;

    WHEN( "determining offset labels" ) {
        int offset_index = -1;
        const int err = segy_offset_indices( fp,
                                             of, offsets,
                                             &offset_index,
                                             trace0, trace_bsize );
        CHECK( err == 0 );
        CHECK( offset_index == 1 );
    }

    WHEN( "counting lines" ) {
        int ilsz = -1;
        int xlsz = -1;

        WHEN( "using segy_count_lines" ) {
            const int err = segy_count_lines( fp,
                                              xl, offsets,
                                              &ilsz, &xlsz,
                                              trace0, trace_bsize );
            CHECK( err == 0 );
            CHECK( ilsz == 5 );
            CHECK( xlsz == 5 );
        }

        WHEN( "using segy_lines_count" ) {
            const int err = segy_lines_count( fp,
                                              il, xl, sorting, offsets,
                                              &ilsz, &xlsz,
                                              trace0, trace_bsize );
            CHECK( err == 0 );
            CHECK( ilsz == 5 );
            CHECK( xlsz == 5 );
        }
    }

    const int inlines_sizes = 5;
    const int crosslines_sizes = 5;

    WHEN( "inferring inline structure" ) {
        const std::vector< int > indices = { 1, 2, 3, 4, 5 };
        CHECK( segy_inline_length( crosslines_sizes ) == 5 );

        WHEN( "finding inline numbers" ) {
            std::vector< int > result( inlines_sizes );
            const int err = segy_inline_indices( fp,
                                                 il,
                                                 sorting,
                                                 inlines_sizes,
                                                 crosslines_sizes,
                                                 offsets,
                                                 result.data(),
                                                 trace0, trace_bsize );
            CHECK( err == 0 );
            CHECK_THAT( result, Catch::Equals( indices ) );
        }

        WHEN( "determining inline 4's first trace number" ) {
            GIVEN( "an inline sorted file" ) THEN( "the stride is 1" ) {
                int stride = -1;
                int err = segy_inline_stride( sorting, inlines_sizes, &stride );
                CHECK( err == 0 );
                CHECK( stride == 1 );
            }

            const int stride = 1;
            int line_trace0 = -1;
            const int err = segy_line_trace0( 4,
                                              crosslines_sizes, stride,
                                              offsets,
                                              indices.data(), inlines_sizes,
                                              &line_trace0 );
            CHECK( err == 0 );
            CHECK( line_trace0 == 15 );
        }
    }

    WHEN( "inferring crossline structure" ) {
        const std::vector< int > indices = { 20, 21, 22, 23, 24 };
        CHECK( segy_crossline_length( inlines_sizes ) == 5 );

        WHEN( "finding crossline numbers" ) {
            std::vector< int > result( crosslines_sizes );
            const int err = segy_crossline_indices( fp,
                                                    xl,
                                                    sorting,
                                                    inlines_sizes,
                                                    crosslines_sizes,
                                                    offsets,
                                                    result.data(),
                                                    trace0, trace_bsize );
            CHECK( err == 0 );
            CHECK_THAT( result, Catch::Equals( indices ) );
        }

        WHEN( "determining crossline 22's first trace number" ) {
            GIVEN( "an inline sorted file" ) THEN( "the stride is 5" ) {
                int stride = -1;
                int err = segy_crossline_stride( sorting,
                                                 crosslines_sizes,
                                                 &stride );
                CHECK( err == 0 );
                CHECK( stride == 5 );
            }

            const int stride = 5;
            int line_trace0 = -1;
            const int err = segy_line_trace0( 22,
                                              inlines_sizes, stride,
                                              offsets,
                                              indices.data(), crosslines_sizes,
                                              &line_trace0 );
            CHECK( err == 0 );
            CHECK( line_trace0 == 2 );
        }
    }
}
