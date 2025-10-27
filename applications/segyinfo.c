#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <segyio/segy.h>

static void printSegyTraceInfo( const char* buf ) {
    int cdp, tsf, xl, il;
    segy_get_tracefield_int( buf, SEGY_TR_ENSEMBLE, &cdp );
    segy_get_tracefield_int( buf, SEGY_TR_SEQ_FILE, &tsf );
    segy_get_tracefield_int( buf, SEGY_TR_CROSSLINE, &xl );
    segy_get_tracefield_int( buf, SEGY_TR_INLINE, &il );

    printf("cdp:               %d\n", cdp );
    printf("TraceSequenceFile: %d\n", tsf );
    printf("Crossline3D:       %d\n", xl );
    printf("Inline3D:          %d\n", il );
}

#define minimum(x,y) ((x) < (y) ? (x) : (y))
#define maximum(x,y) ((x) > (y) ? (x) : (y))

int main(int argc, char* argv[]) {

    if( argc < 2 ) {
        puts("Missing argument, expected run signature:");
        printf("  %s <segy_file> [mmap]\n", argv[0]);
        exit(1);
    }

    segy_file* fp = segy_open( argv[ 1 ], "rb" );
    if( !fp ) {
        perror( "fopen():" );
        exit( 3 );
    }

    if( argc > 2 && strcmp( argv[ 2 ], "mmap" ) == 0 ) {
        int err = segy_mmap( fp );
        if( err != SEGY_OK )
            fputs( "Could not mmap file. Using fstream fallback.", stderr );
    }

    int err = segy_collect_metadata( fp, -1, -1, -1 );
    const int format = fp->metadata.format;
    const int samples = fp->metadata.samplecount;
    const long trace_bsize = fp->metadata.trace_bsize;
    const int extended_headers = fp->metadata.ext_textheader_count;
    const int traces = fp->metadata.tracecount;

    printf( "Sample format: %d\n", format );
    printf( "Samples per trace: %d\n", samples );
    printf( "Traces: %d\n", traces );
    printf("Extended text header count: %d\n", extended_headers );
    puts("");

    if (err != SEGY_OK) {
        perror( "Could not collect metadata" );
        exit( err );
    }

    char traceh[ SEGY_TRACE_HEADER_SIZE ];
    err = segy_read_standard_traceheader( fp, 0, traceh );
    if( err != 0 ) {
        perror( "Unable to read trace 0:" );
        exit( err );
    }

    puts("Info from first trace:");
    printSegyTraceInfo( traceh );

    err = segy_read_standard_traceheader( fp, 1, traceh );
    if( err != 0 ) {
        perror( "Unable to read trace 1:" );
        exit( err );
    }

    puts("");
    puts("Info from second trace:");
    printSegyTraceInfo( traceh );

    clock_t start = clock();
    float* trbuf = malloc( sizeof( float ) * trace_bsize );

    float minval = FLT_MAX;
    float maxval = FLT_MIN;

    int min_sample_count = 999999999;
    int max_sample_count = 0;
    for( int i = 0; i < traces; ++i ) {
        err = segy_read_standard_traceheader( fp, i, traceh );
        if( err != 0 ) {
            perror( "Unable to read trace" );
            exit( err );
        }

        int sample_count;
        err = segy_get_tracefield_int( traceh, SEGY_TR_SAMPLE_COUNT, &sample_count );

        if( err != 0 ) {
            fprintf( stderr, "Invalid trace header field: %d\n", SEGY_TR_SAMPLE_COUNT );
            exit( err );
        }

        min_sample_count = minimum( sample_count, min_sample_count );
        max_sample_count = maximum( sample_count, max_sample_count );

        err = segy_readtrace( fp, i, trbuf );

        if( err != 0 ) {
            fprintf( stderr, "Unable to read trace: %d\n", i );
            exit( err );
        }

        segy_to_native( format, samples, trbuf );

        for( int j = 0; j < samples; ++j ) {
            minval = minimum( trbuf[ j ], minval );
            maxval = maximum( trbuf[ j ], maxval );
        }
    }

    free( trbuf );

    puts("");
    puts("Info from last trace:");
    err = segy_read_standard_traceheader( fp, traces - 1, traceh );

    if( err != 0 ) {
        perror( "Unable to read trace." );
        exit( err );
    }

    printSegyTraceInfo( traceh );

    puts("");
    printf("Min sample count: %d\n", min_sample_count);
    printf("Max sample count: %d\n", max_sample_count);
    printf("Min sample value: %f\n", minval );
    printf("Max sample value: %f\n", maxval );
    puts("");

    clock_t diff = clock() - start;
    printf("Read all trace headers in: %.2f s\n", (double) diff / CLOCKS_PER_SEC);

    segy_close( fp );
    return 0;
}
