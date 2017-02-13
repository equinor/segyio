#include <limits.h>
#include <math.h>
#include <stdlib.h>

#include <segyio/segy.h>
#include <segyio/util.h>

#include "unittest.h"

static void test_interpret_file() {
    /*
     * test all structural properties of a file, i.e. traces, samples, sorting
     * etc, but don't actually read any data
     */
    const char *file = "test-data/small.sgy";

    int err;
    char header[ SEGY_BINARY_HEADER_SIZE ];
    int sorting;
    int traces;
    unsigned int inlines_sz, crosslines_sz;
    unsigned int offsets, stride;
    unsigned int line_trace0, line_length;
    const int il = SEGY_TR_INLINE;
    const int xl = SEGY_TR_CROSSLINE;

    segy_file* fp = segy_open( file, "rb" );

    assertTrue( fp != NULL, "Could not open file." );
    err = segy_binheader( fp, header );
    assertTrue( err == 0, "Could not read binary header." );

    const long trace0 = segy_trace0( header );
    assertTrue( trace0 == 3600,
                "Wrong byte offset of the first trace header. Expected 3600." );

    const int samples = segy_samples( header );
    assertTrue( samples == 50, "Expected 350 samples per trace." );

    const int trace_bsize = segy_trace_bsize( samples );
    assertTrue( trace_bsize == 50 * 4,
                "Wrong trace byte size. Expected samples * 4-byte float." );

    err = segy_traces( fp, &traces, trace0, trace_bsize );
    assertTrue( err == 0, "Could not determine number of traces in this file." );
    assertTrue( traces == 25, "Expected 25 traces in the file." );

    err = segy_sorting( fp, xl, il, &sorting, trace0, trace_bsize );
    assertTrue( err == 0, "Could not figure out file sorting." );
    assertTrue( sorting == SEGY_CROSSLINE_SORTING, "Expected crossline sorting." );

    err = segy_sorting( fp, il, xl, &sorting, trace0, trace_bsize );
    assertTrue( err == 0, "Could not figure out file sorting." );
    assertTrue( sorting == SEGY_INLINE_SORTING, "Expected inline sorting." );

    err = segy_offsets( fp, il, xl, traces, &offsets, trace0, trace_bsize );
    assertTrue( err == 0, "Could not figure out offsets." );
    assertTrue( offsets == 1, "Expected offsets to be 1 (no extra offsets)." );

    int offset_index = -1;
    err = segy_offset_indices( fp, 37, 1, &offset_index, trace0, trace_bsize );
    assertTrue( err == 0, "Could not figure out offset indices." );
    assertTrue( offset_index == 1, "Expected offset index to be 1." );

    err = segy_count_lines( fp, xl, offsets, &inlines_sz, &crosslines_sz, trace0, trace_bsize );
    assertTrue( err == 0, "Could not determine line count in this file." );
    assertTrue( inlines_sz == 5, "Expected 5 inlines." );
    assertTrue( crosslines_sz == 5, "Expected 5 crosslines." );

    /* Inline-specific information */
    unsigned int inline_indices[ 5 ];
    err = segy_inline_indices( fp, il, sorting, inlines_sz, crosslines_sz, offsets, inline_indices, trace0, trace_bsize );
    assertTrue( err == 0, "Could not determine inline linenos." );
    for( unsigned int i = 0, ref = 1; i < 5; ++i, ++ref )
        assertTrue( inline_indices[ i ] == ref,
                    "Inline lineno mismatch, should be [1..5]." );

    err = segy_inline_stride( sorting, inlines_sz, &stride );
    assertTrue( err == 0, "Failure while reading stride." );
    assertTrue( stride == 1, "Expected inline stride = 1." );

    err = segy_line_trace0( 4, crosslines_sz, stride, offsets, inline_indices, inlines_sz, &line_trace0 );
    assertTrue( err == 0, "Could not determine 2484's trace0." );
    assertTrue( line_trace0 == 15, "Line 4 should start at traceno 15." );

    line_length = segy_inline_length( crosslines_sz);
    assertTrue( line_length == 5, "Inline length should be 5." );

    /* Crossline-specific information */
    unsigned int crossline_indices[ 5 ];
    err = segy_crossline_indices( fp, xl, sorting, inlines_sz, crosslines_sz, offsets, crossline_indices, trace0, trace_bsize );
    assertTrue( err == 0, "Could not determine crossline linenos." );
    for( unsigned int i = 0, ref = 20; i < 5; ++i, ++ref )
        assertTrue( crossline_indices[ i ] == ref,
                    "Crossline lineno mismatch, should be [20..24]." );

    err = segy_crossline_stride( sorting, crosslines_sz, &stride );
    assertTrue( err == 0, "Failure while reading stride." );
    assertTrue( stride == 5, "Expected crossline stride = 5." );

    err = segy_line_trace0( 22, crosslines_sz, stride, offsets, crossline_indices, inlines_sz, &line_trace0 );
    assertTrue( err == 0, "Could not determine 22's trace0." );
    assertTrue( line_trace0 == 2, "Line 22 should start at traceno 2." );

    line_length = segy_crossline_length( inlines_sz );
    assertTrue( line_length == 5, "Crossline length should be 5." );

    segy_close( fp );
}

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

static void testReadInLine_4(){
    const char *file = "test-data/small.sgy";

    int sorting;
    int traces;
    unsigned int inlines_sz, crosslines_sz;
    unsigned int offsets, stride;
    unsigned int line_trace0, line_length;

    /* test specific consts */
    const int il = SEGY_TR_INLINE, xl = SEGY_TR_CROSSLINE;

    char header[ SEGY_BINARY_HEADER_SIZE ];

    segy_file* fp = segy_open( file, "rb" );
    assertTrue( 0 == segy_binheader( fp, header ), "Could not read header" );
    const long trace0 = segy_trace0( header );
    const int samples = segy_samples( header );
    const int trace_bsize = segy_trace_bsize( samples );
    const int format = segy_format( header );

    unsigned int inline_indices[ 5 ];
    const unsigned int inline_length = 5;
    float* data = malloc( inline_length * samples * sizeof( float ) );

    int ok = 0;
    ok = !segy_traces( fp, &traces, trace0, trace_bsize )
      && !segy_sorting( fp, il, xl, &sorting, trace0, trace_bsize )
      && !segy_offsets( fp, il, xl, traces, &offsets, trace0, trace_bsize )
      && !segy_count_lines( fp, xl, offsets, &inlines_sz, &crosslines_sz, trace0, trace_bsize )
      && !segy_inline_indices( fp, il, sorting, inlines_sz, crosslines_sz, offsets, inline_indices, trace0, trace_bsize )
      && !segy_inline_stride( sorting, inlines_sz, &stride )
      && !segy_line_trace0( 4, crosslines_sz, stride, offsets, inline_indices, inlines_sz, &line_trace0 );

    line_length = segy_inline_length(crosslines_sz);

      ok = ok && !segy_read_line( fp, line_trace0, line_length, stride, offsets, data, trace0, trace_bsize )
      && !segy_to_native( format, inline_length * samples, data );

    assertTrue( ok, "Error in setup. "
                    "This integrity should be covered by other tests." );

    //first xline
    //first sample
    assertClose(4.2f, data[0], 0.0001f);
    //middle sample
    assertClose(4.20024f, data[samples/2-1], 0.0001f);
    //last sample
    assertClose(4.20049f, data[samples-1], 0.0001f);

    //middle xline
    int middle_line = 2;
    //first sample
    assertClose(4.22f, data[samples*middle_line+0], 0.0001);
    //middle sample
    assertClose(4.22024f, data[samples*middle_line+samples/2-1], 0.0001);
    //last sample
    assertClose(4.22049f, data[samples*middle_line+samples-1], 0.0001);

    //last xline
    int last_line = (crosslines_sz-1);
    //first sample
    assertClose(4.24f, data[samples*last_line+0], 0);
    //middle sample
    assertClose(4.24024f, data[samples*last_line+samples/2-1], 0.0001);
    //last sample
    assertClose(4.24049f, data[samples*last_line+samples-1], 0.0001);

    for( float* ptr = data; ptr < data + (samples * line_length); ++ptr )
        assertTrue( *ptr >= 4.0f && *ptr <= 5.0f, "Sample value not in range." );

    free(data);
    segy_close(fp);
}

static void testReadCrossLine_22(){
    const char *file = "test-data/small.sgy";

    int sorting;
    int traces;
    unsigned int inlines_sz, crosslines_sz;
    unsigned int offsets, stride;
    unsigned int line_trace0, line_length;

    /* test specific consts */
    const int il = SEGY_TR_INLINE, xl = SEGY_TR_CROSSLINE;

    char header[ SEGY_BINARY_HEADER_SIZE ];

    segy_file* fp = segy_open( file, "rb" );
    assertTrue( 0 == segy_binheader( fp, header ), "Could not read header" );
    const long trace0 = segy_trace0( header );
    const int samples = segy_samples( header );
    const int trace_bsize = segy_trace_bsize( samples );
    const int format = segy_format( header );

    unsigned int crossline_indices[ 5 ];
    const unsigned int crossline_length = 5;
    float* data = malloc( crossline_length * samples * sizeof(float) );

    int ok = 0;
    ok = !segy_traces( fp, &traces, trace0, trace_bsize )
      && !segy_sorting( fp, il, xl, &sorting, trace0, trace_bsize )
      && !segy_offsets( fp, il, xl, traces, &offsets, trace0, trace_bsize )
      && !segy_count_lines( fp, xl, offsets, &inlines_sz, &crosslines_sz, trace0, trace_bsize )
      && !segy_crossline_indices( fp, xl, sorting, inlines_sz, crosslines_sz, offsets, crossline_indices, trace0, trace_bsize )
      && !segy_crossline_stride( sorting, crosslines_sz, &stride )
      && !segy_line_trace0( 22, crosslines_sz, stride, offsets, crossline_indices, inlines_sz, &line_trace0 );

    line_length = segy_crossline_length(inlines_sz);

    ok = ok && !segy_read_line( fp, line_trace0, line_length, stride, offsets, data, trace0, trace_bsize )
      && !segy_to_native( format, crossline_length * samples, data );

    assertTrue( ok, "Error in setup. "
                    "This integrity should be covered by other tests." );

    //first inline
    //first sample
    assertClose(1.22f, data[0], 0.0001);
    //middle sample
    assertClose(1.22024f, data[samples/2-1], 0.0001);
    //last sample
    assertClose(1.22049f, data[samples-1], 0.0001);

    //middle inline
    int middle_line = 2;
    //first sample
    assertClose(3.22f, data[samples*middle_line+0], 0.0001);
    //middle sample
    assertClose(3.22024f, data[samples*middle_line+samples/2-1], 0.0001);
    //last sample
    assertClose(3.22049f, data[samples*middle_line+samples-1], 0.0001);

    //last inline
    int last_line = (line_length-1);
    //first sample
    assertClose(5.22f, data[samples*last_line+0], 0.0001);
    //middle sample
    assertClose(5.22024f, data[samples*last_line+samples/2-1], 0.0001);
    //last sample
    assertClose(5.22049f, data[samples*last_line+samples-1], 0.0001);

    for( float* ptr = data; ptr < data + (samples * line_length); ++ptr ) {
        float x = *ptr - floorf(*ptr);
        assertTrue( 0.219f <= x && x <= 0.231f, "Sample value not in range" );
    }

    free(data);
    segy_close(fp);
}

static void test_modify_trace_header() {
    const char *file = "test-data/small-traceheader.sgy";

    int err;
    char bheader[ SEGY_BINARY_HEADER_SIZE ];

    segy_file* fp = segy_open( file, "r+b" );
    err = segy_binheader( fp, bheader );
    assertTrue( err == 0, "Could not read header" );
    const long trace0 = segy_trace0( bheader );
    const int samples = segy_samples( bheader );
    const int trace_bsize = segy_trace_bsize( samples );

    char traceh[ SEGY_TRACE_HEADER_SIZE ];
    err = segy_traceheader( fp, 0, traceh, trace0, trace_bsize );
    assertTrue( err == 0, "Error while reading trace header." );

    int ilno;
    err = segy_get_field( traceh, SEGY_TR_INLINE, &ilno );
    assertTrue( err == 0, "Error while reading iline no from field." );
    assertTrue( ilno == 1, "Wrong iline no." );

    err = segy_set_field( traceh, SEGY_TR_INLINE, ilno + 1 );
    assertTrue( err == 0, "Error writing to trace header buffer." );

    err = segy_get_field( traceh, SEGY_TR_INLINE, &ilno );
    assertTrue( err == 0, "Error while reading iline no from dirty field." );
    assertTrue( ilno == 2, "Wrong iline no." );

    err = segy_write_traceheader( fp, 0, traceh, trace0, trace_bsize );
    segy_flush( fp, false );

    err = segy_traceheader( fp, 0, traceh, trace0, trace_bsize );
    assertTrue( err == 0, "Error while reading trace header." );
    err = segy_get_field( traceh, SEGY_TR_INLINE, &ilno );
    assertTrue( err == 0, "Error while reading iline no from dirty field." );
    assertTrue( ilno == 2, "Wrong iline no." );

    err = segy_set_field( traceh, SEGY_TR_INLINE, 1 );
    assertTrue( err == 0, "Error writing to trace header buffer." );
    err = segy_write_traceheader( fp, 0, traceh, trace0, trace_bsize );
    assertTrue( err == 0, "Error writing traceheader." );

    err = segy_set_field( traceh, SEGY_TR_SOURCE_GROUP_SCALAR, -100 );
    assertTrue( err == 0, "Error writing to trace header buffer." );
    int32_t scale;
    err = segy_get_field( traceh, SEGY_TR_SOURCE_GROUP_SCALAR, &scale );
    assertTrue( err == 0, "Error while reading SourceGroupScalar from dirty field." );

    segy_close( fp );
}

static const char* expected_textheader =
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

static void test_text_header() {
    const char *file = "test-data/text.sgy";
    segy_file* fp = segy_open( file, "rb" );

    char ascii[ SEGY_TEXT_HEADER_SIZE + 1 ] = { 0 };
    int err = segy_read_textheader(fp, ascii);
    assertTrue( err == 0, "Could not read text header" );
    assertTrue( strcmp(expected_textheader, ascii) == 0, "Text headers did not match" );

    segy_close( fp );
}

static void test_trace_header_errors() {
    const char *file = "test-data/small.sgy";
    segy_file* fp = segy_open( file, "rb" );
    int err;

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

static void test_file_error_codes() {
    const char *file = "test-data/small.sgy";
    segy_file* fp = segy_open( file, "rb" );
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
    err = segy_sorting( fp, 0, 0, &sorting, 3600, 350 );
    assertTrue( err == SEGY_FSEEK_ERROR, "Could seek in invalid file." );

    err = segy_readtrace( fp, 0, NULL, 3600, 350 );
    assertTrue( err == SEGY_FSEEK_ERROR, "Could seek in invalid file." );

    err = segy_writetrace( fp, 0, NULL, 3600, 350 );
    assertTrue( err == SEGY_FSEEK_ERROR, "Could seek in invalid file." );

    unsigned int l1, l2;
    err = segy_count_lines( fp, 0, 1, &l1, &l2, 3600, 350 );
    assertTrue( err == SEGY_FSEEK_ERROR, "Could seek in invalid file." );
}

static void test_error_codes_sans_file() {
    int err;

    unsigned int linenos[] = { 0, 1, 2 };
    err = segy_line_trace0( 10, 3, 1, 1, linenos, 3, NULL );
    assertTrue( err == SEGY_MISSING_LINE_INDEX,
                "Found line number that shouldn't exist." );

    unsigned int stride;
    err = segy_inline_stride( SEGY_INLINE_SORTING + 3, 10, &stride );
    assertTrue( err == SEGY_INVALID_SORTING,
                "Expected sorting to be invalid." );

    err = segy_crossline_stride( SEGY_INLINE_SORTING + 3, 10, &stride );
    assertTrue( err == SEGY_INVALID_SORTING,
                "Expected sorting to be invalid." );
}

static void test_file_size_above_4GB(){
    segy_file* fp = segy_open( "4gbfile", "w+b" );

    int trace = 5000000;
    int trace_bsize = 1000;
    long long tracesize = trace_bsize + SEGY_TRACE_HEADER_SIZE;
    long trace0 = 0;

    int err = segy_seek( fp, trace, trace0, trace_bsize );
    assertTrue(err==0, "");

    long long pos = segy_ftell( fp );
    assertTrue(pos > (long long)INT_MAX, "pos smaller than INT_MAX. "
                              "This means there's an overflow somewhere" );
    assertTrue(pos == trace * tracesize, "seek overflow");
    segy_close(fp);
}

int main() {
    puts("starting");
    puts("interpret file");
    test_interpret_file();
    /* test_interpret_file_prestack(); */
    puts("read inline 4");
    testReadInLine_4();
    puts("read inline 22");
    testReadCrossLine_22();
    puts("mod traceh");
    test_modify_trace_header();
    puts("mod texth");
    test_text_header();
    puts("test traceh");
    test_trace_header_errors();
    /*
     * due to its barely-defined behavorial nature, this test is commented out
     * for most runs, as it would trip up the memcheck test
     *
     * test_file_error_codes();
     */
    test_error_codes_sans_file();

    test_file_size_above_4GB();
    exit(0);
}
