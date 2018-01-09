// Omit from static analysis.
#ifndef __clang_analyzer__

#include <limits.h>
#include <math.h>
#include <stdlib.h>

#include <segyio/segy.h>
#include <segyio/util.h>

#include "unittest.h"

/* Prestack test for when we have an approperiate prestack file

void test_interpret_file_prestack() {
    const char *file = "test-data/prestack.sgy";

    int err;
    char header[ SEGY_BINARY_HEADER_SIZE ];
    int sorting;
    size_t traces;
    unsigned int inlines_sz, crosslines_sz;
    unsigned int offsets, stride;
    unsigned int line_trace0, line_length;
    const int il = SEGY_TR_INLINE;
    const int xl = SEGY_TR_CROSSLINE;

    FILE* fp = fopen( file, "r" );

    assertTrue( fp != NULL, "Could not open file." );
    err = segy_binheader( fp, header );
    assertTrue( err == 0, "Could not read binary header." );

    const long trace0 = segy_trace0( header );
    assertTrue( trace0 == 3600,
                "Wrong byte offset of the first trace header. Expected 3600." );

    const unsigned int samples = segy_samples( header );
    assertTrue( samples == 326, "Expected 350 samples per trace." );

    const size_t trace_bsize = segy_trace_bsize( samples );
    assertTrue( trace_bsize == 326 * 4,
                "Wrong trace byte size. Expected samples * 4-byte float." );

    err = segy_traces( fp, &traces, trace0, trace_bsize );
    assertTrue( err == 0, "Could not determine number of traces in this file." );
    assertTrue( traces == 16926, "Expected 286 traces in the file." );

    err = segy_sorting( fp, xl, il, &sorting, trace0, trace_bsize );
    assertTrue( err == 0, "Could not figure out file sorting." );
    assertTrue( sorting == SEGY_CROSSLINE_SORTING, "Expected crossline sorting." );

    err = segy_sorting( fp, il, xl, &sorting, trace0, trace_bsize );
    assertTrue( err == 0, "Could not figure out file sorting." );
    assertTrue( sorting == SEGY_INLINE_SORTING, "Expected inline sorting." );

    err = segy_offsets( fp, il, xl, traces, &offsets, trace0, trace_bsize );
    assertTrue( err == 0, "Could not figure out offsets." );
    assertTrue( offsets == 26, "Expected offsets to be 26." );

    err = segy_count_lines( fp, xl, offsets, &inlines_sz, &crosslines_sz, trace0, trace_bsize );
    assertTrue( err == 0, "Could not determine line count in this file." );
    assertTrue( inlines_sz == 31, "Expected 22 inlines." );
    assertTrue( crosslines_sz == 21, "Expected 13 crosslines." );

    unsigned int inline_indices[ 31 ];
    err = segy_inline_indices( fp, il, sorting, inlines_sz, crosslines_sz, offsets, inline_indices, trace0, trace_bsize );
    assertTrue( err == 0, "Could not determine inline linenos." );
    for( unsigned int i = 0, ref = 2500; i < 31; ++i, ++ref )
        assertTrue( inline_indices[ i ] == ref,
                    "Inline lineno mismatch, should be [2500..2530]." );

    err = segy_inline_stride( sorting, inlines_sz, &stride );
    assertTrue( err == 0, "Failure while reading stride." );
    assertTrue( stride == 1, "Expected inline stride = 1." );

    err = segy_line_trace0( 2504, crosslines_sz, stride, inline_indices, inlines_sz, &line_trace0 );
    assertTrue( err == 0, "Could not determine 2504's trace0." );
    assertTrue( line_trace0 == 84, "Line 2504 should start at traceno 84." );

    err = segy_inline_length( sorting, traces, inlines_sz, offsets, &line_length );
    assertTrue( err == 0, "Could not determine line length." );
    assertTrue( line_length == 31, "Inline length should be 31." );

    unsigned int crossline_indices[ 21 ];
    err = segy_crossline_indices( fp, xl, sorting, inlines_sz, crosslines_sz, offsets, crossline_indices, trace0, trace_bsize );
    assertTrue( err == 0, "Could not determine crossline linenos." );
    for( unsigned int i = 0, ref = 2220; i < 21; ++i, ++ref )
        assertTrue( crossline_indices[ i ] == ref,
                    "Crossline lineno mismatch, should be [2220..2240]." );

    err = segy_crossline_stride( sorting, crosslines_sz, &stride );
    assertTrue( err == 0, "Failure while reading stride." );
    assertTrue( stride == 21, "Expected crossline stride = 13." );

    err = segy_line_trace0( 2222, crosslines_sz, stride, crossline_indices, inlines_sz, &line_trace0 );
    assertTrue( err == 0, "Could not determine 2222's trace0." );
    assertTrue( line_trace0 == 2, "Line 2222 should start at traceno 5." );

    err = segy_crossline_length( sorting, traces, inlines_sz, offsets, &line_length );
    assertTrue( err == 0, "Failure finding length" );
    assertTrue( line_length == 31, "Crossline length should be 31." );

    fclose( fp );
}

*/

static void test_trace_header_errors( bool mmap ) {
    const char *file = "test-data/small.sgy";
    segy_file* fp = segy_open( file, "rb" );
    int err;
    if ( mmap ) segy_mmap( fp );

    char binheader[ SEGY_BINARY_HEADER_SIZE ];
    err = segy_binheader( fp, binheader );
    assertTrue( err == 0, "Could not read binary header." );
    const int samples = segy_samples( binheader );
    const int bsize = segy_trace_bsize( samples );
    const long trace0 = segy_trace0( binheader );

    char header[ SEGY_TRACE_HEADER_SIZE ];

    err = segy_traceheader( fp, 0, header, trace0, bsize );
    assertTrue( err == 0, "Could not read trace header." );

    /* reading outside header should yield invalid field */
    int field;
    err = segy_get_field( header, SEGY_TRACE_HEADER_SIZE + 10, &field );
    assertTrue( err == SEGY_INVALID_FIELD, "Reading outside trace header." );

    /* Reading between known byte offsets should yield error */
    err = segy_get_field( header, SEGY_TR_INLINE + 1, &field );
    assertTrue( err == SEGY_INVALID_FIELD, "Reading between ok byte offsets." );

    err = segy_get_field( header, SEGY_TR_INLINE, &field );
    assertTrue( err == SEGY_OK, "Reading failed at valid byte offset." );

    err = segy_get_field( header, SEGY_TR_DAY_OF_YEAR, &field );
    assertTrue( err == SEGY_OK, "Reading failed at valid byte offset." );

    segy_close( fp );
}

static void test_file_error_codes( bool mmap ) {
    const char *file = "test-data/small.sgy";
    segy_file* fp = segy_open( file, "rb" );
    if ( mmap ) segy_mmap( fp );
    segy_close( fp );

    int err;
    char binheader[ SEGY_BINARY_HEADER_SIZE ];
    err = segy_binheader( fp, binheader );
    assertTrue( err == SEGY_FSEEK_ERROR,
                "Could read binary header from invalid file." );

    const int samples = segy_samples( binheader );
    const int trace_bsize = segy_trace_bsize( samples );
    const long trace0 = segy_trace0( binheader );

    char header[ SEGY_TRACE_HEADER_SIZE ];

    err = segy_traceheader( fp, 0, header, trace0, trace_bsize );
    assertTrue( err == SEGY_FSEEK_ERROR, "Could not read trace header." );

    /* reading outside header should yield invalid field */
    int field;
    err = segy_get_field( header, SEGY_TRACE_HEADER_SIZE + 10, &field );
    assertTrue( err == SEGY_INVALID_FIELD, "Reading outside trace header." );

    /* Reading between known byte offsets should yield error */
    err = segy_get_field( header, SEGY_TR_INLINE + 1, &field );
    assertTrue( err == SEGY_INVALID_FIELD, "Reading between ok byte offsets." );

    err = segy_get_field( header, SEGY_TR_INLINE, &field );
    assertTrue( err == SEGY_OK, "Reading failed at valid byte offset." );

    err = segy_get_field( header, SEGY_TR_DAY_OF_YEAR, &field );
    assertTrue( err == SEGY_OK, "Reading failed at valid byte offset." );

    err = segy_read_textheader(fp, NULL);
    assertTrue( err == SEGY_FSEEK_ERROR, "Could seek in invalid file." );

    int traces;
    err = segy_traces( fp, &traces, 3600, 350 );
    assertTrue( err == SEGY_FSEEK_ERROR, "Could seek in invalid file." );

    int sorting;
    err = segy_sorting( fp, 0, 0, 0, &sorting, 3600, 350 );
    assertTrue( err == SEGY_FSEEK_ERROR, "Could seek in invalid file." );

    err = segy_readtrace( fp, 0, NULL, 3600, 350 );
    assertTrue( err == SEGY_FSEEK_ERROR, "Could seek in invalid file." );

    err = segy_writetrace( fp, 0, NULL, 3600, 350 );
    assertTrue( err == SEGY_FSEEK_ERROR, "Could seek in invalid file." );

    int l1, l2;
    err = segy_readsubtr( fp, 0, 2, 0, 1, NULL, NULL, 3600, 350 );
    assertTrue( err == SEGY_INVALID_ARGS, "Could pass fst larger than lst" );

    err = segy_readsubtr( fp, 0, -1, 10, 1, NULL, NULL, 3600, 350 );
    assertTrue( err == SEGY_INVALID_ARGS, "Could pass negative fst" );

    err = segy_writesubtr( fp, 0, 2, 0, 1, NULL, NULL, 3600, 350 );
    assertTrue( err == SEGY_INVALID_ARGS, "Could pass fst larger than lst" );

    err = segy_writesubtr( fp, 0, -1, 10, 1, NULL, NULL, 3600, 350 );
    assertTrue( err == SEGY_INVALID_ARGS, "Could pass negative fst" );

    err = segy_count_lines( fp, 0, 1, &l1, &l2, 3600, 350 );
    assertTrue( err == SEGY_FSEEK_ERROR, "Could seek in invalid file." );
}

static void test_error_codes_sans_file() {
    int err;

    int linenos[] = { 0, 1, 2 };
    err = segy_line_trace0( 10, 3, 1, 1, linenos, 3, NULL );
    assertTrue( err == SEGY_MISSING_LINE_INDEX,
                "Found line number that shouldn't exist." );

    int stride;
    err = segy_inline_stride( SEGY_INLINE_SORTING + 3, 10, &stride );
    assertTrue( err == SEGY_INVALID_SORTING,
                "Expected sorting to be invalid." );

    err = segy_crossline_stride( SEGY_INLINE_SORTING + 3, 10, &stride );
    assertTrue( err == SEGY_INVALID_SORTING,
                "Expected sorting to be invalid." );
}

static void test_file_size_above_4GB( bool mmap ){
    segy_file* fp = segy_open( "4gbfile", "w+b" );
    if ( mmap ) segy_mmap( fp );
    int trace = 5000000;
    int trace_bsize = 1000;
    long long tracesize = trace_bsize + SEGY_TRACE_HEADER_SIZE;
    long trace0 = 0;

    int err = segy_seek( fp, trace, trace0, trace_bsize );
    assertTrue(err==0, "");

    long long pos = segy_ftell( fp );
    assertTrue(pos > (long long)INT_MAX, "pos smaller than INT_MAX. "
                              "This means there's an overflow somewhere" );
    assertTrue(pos != -1, "overflow in off_t");
    assertTrue(pos == trace * tracesize, "seek overflow");
    segy_close(fp);
}

int main() {
    puts("starting");
    /* test_interpret_file_prestack(); */
    puts("test traceh");
    test_trace_header_errors( false );
    test_trace_header_errors( true );

    /*
     * due to its barely-defined behavorial nature, this test is commented out
     * for most runs, as it would trip up the memcheck test
     *
     * test_file_error_codes();
     */
    test_error_codes_sans_file();

    test_file_size_above_4GB( false );
    test_file_size_above_4GB( true );

    exit(0);
}

#endif // __clang_analyzer__
