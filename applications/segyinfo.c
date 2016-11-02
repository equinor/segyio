#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <segyio/segy.h>

static void printSegyTraceInfo( const char* buf ) {
    int cdp, tsf, xl, il;
    segy_get_field( buf, CDP, &cdp );
    segy_get_field( buf, TRACE_SEQUENCE_FILE, &tsf );
    segy_get_field( buf, CROSSLINE_3D, &xl );
    segy_get_field( buf, INLINE_3D, &il );

    printf("cdp:               %d\n", cdp );
    printf("TraceSequenceFile: %d\n", tsf );
    printf("Crossline3D:       %d\n", xl );
    printf("Inline3D:          %d\n", il );
}

static inline int minimum( int x, int y ) {
    return x < y ? x : y;
}

static inline int maximum( int x, int y ) {
    return x > y ? x : y;
}

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

    if( argc > 2 && strcmp( argv[ 2 ], "mmap" ) == 0 )
        segy_mmap( fp );

    int err;
    char header[ SEGY_BINARY_HEADER_SIZE ];
    err = segy_binheader( fp, header );

    if( err != 0 ) {
        perror( "Unable to read segy binary header:" );
        exit( err );
    }

    const int format = segy_format( header );
    const unsigned int samples = segy_samples( header );
    const long trace0 = segy_trace0( header );
    const unsigned int trace_bsize = segy_trace_bsize( samples );
    int extended_headers;
    err = segy_get_bfield( header, BIN_ExtendedHeaders, &extended_headers );

    if( err != 0 ) {
        perror( "Can't read 'extended headers' field from binary header" );
        exit( err );
    }

    size_t traces;
    err = segy_traces( fp, &traces, trace0, trace_bsize );

    if( err != 0 ) {
        perror( "Could not determine traces" );
        exit( err );
    }

    printf( "Sample format: %d\n", format );
    printf( "Samples per trace: %d\n", samples );
    printf( "Traces: %zu\n", traces );
    printf("Extended text header count: %d\n", extended_headers );
    puts("");


    char traceh[ SEGY_TRACE_HEADER_SIZE ];
    err = segy_traceheader( fp, 0, traceh, trace0, trace_bsize );
    if( err != 0 ) {
        perror( "Unable to read trace 0:" );
        exit( err );
    }

    puts("Info from first trace:");
    printSegyTraceInfo( traceh );

    err = segy_traceheader( fp, 1, traceh, trace0, trace_bsize );
    if( err != 0 ) {
        perror( "Unable to read trace 1:" );
        exit( err );
    }

    puts("");
    puts("Info from second trace:");
    printSegyTraceInfo( traceh );

    clock_t start = clock();
    int min_sample_count = 999999999;
    int max_sample_count = 0;
    for( int i = 0; i < (int)traces; i++ ) {
        err = segy_traceheader( fp, i, traceh, trace0, trace_bsize );
        if( err != 0 ) {
            perror( "Unable to read trace" );
            exit( err );
        }

        int sample_count;
        err = segy_get_field( traceh, TRACE_SAMPLE_COUNT, &sample_count );

        if( err != 0 ) {
            fprintf( stderr, "Invalid trace header field: %d\n", TRACE_SAMPLE_COUNT );
            exit( err );
        }

        min_sample_count = minimum( sample_count, min_sample_count );
        max_sample_count = maximum( sample_count, max_sample_count );
    }

    puts("");
    puts("Info from last trace:");
    err = segy_traceheader( fp, traces - 1, traceh, trace0, trace_bsize );

    if( err != 0 ) {
        perror( "Unable to read trace." );
        exit( err );
    }

    printSegyTraceInfo( traceh );

    puts("");
    printf("Min sample count: %d\n", min_sample_count);
    printf("Max sample count: %d\n", max_sample_count);
    puts("");

    clock_t diff = clock() - start;
    printf("Read all trace headers in: %.2f s\n", (double) diff / CLOCKS_PER_SEC);

    segy_close( fp );
    return 0;
}
