#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <segyio/segy.h>


static const char* getSampleFormatName( int format ) {
    switch( format ) {
        case SEGY_IBM_FLOAT_4_BYTE:
            return "IBM Float";         
        case SEGY_SIGNED_INTEGER_4_BYTE:
            return "Int 32";         
        case SEGY_SIGNED_SHORT_2_BYTE:
            return "Int 16";         
        case SEGY_FIXED_POINT_WITH_GAIN_4_BYTE:
            return "Fixed Point with gain (Obsolete)";         
        case SEGY_IEEE_FLOAT_4_BYTE:
            return "IEEE Float";         
        case SEGY_NOT_IN_USE_1:
            return "Not in Use 1";         
        case SEGY_NOT_IN_USE_2:
            return "Not in Use 2";         
        case SEGY_SIGNED_CHAR_1_BYTE:
            return "Int 8";         
        default:
            return "Unknown";
    }
}

static const char* getFastestDirectionName( int sorting ) {
    if ( sorting == SEGY_CROSSLINE_SORTING) {
        return "CROSSLINE";
    } else {
        return "INLINE_SORTING";
    }
}

int main(int argc, char* argv[]) {

    int err;

    if( argc < 2 ) {
        puts("Missing argument, expected run signature:");
        printf("  %s <segy_file> [mmap] [INLINE_BYTE CROSSLINE_BYTE]\n", argv[0]);
        printf("  Inline and crossline bytes default to: 189 and 193\n");
        exit(1);
    }

    int xl_field = SEGY_TR_CROSSLINE;
    int il_field = SEGY_TR_INLINE;

    bool memory_map = argc > 2 && strcmp( argv[ 2 ], "mmap" ) == 0;

    if( ( memory_map && argc > 4 ) || ( !memory_map && argc > 2 ) ) {
        int argindex = memory_map ? 2 : 3;

        il_field = atoi(argv[ argindex + 0 ]);
        xl_field = atoi(argv[ argindex + 1 ]);
    }

    clock_t start = clock();

    segy_file* fp = segy_open( argv[ 1 ], "rb" );
    if( !fp ) {
        perror( "fopen()" );
        exit( SEGY_FOPEN_ERROR );
    }

    if( memory_map ) {
        err = segy_mmap( fp );
        if( err != SEGY_OK )
            fputs( "Could not mmap file. Using fstream fallback.", stderr );
    }


    char header[ SEGY_BINARY_HEADER_SIZE ];
    err = segy_binheader( fp, header );

    if( err != 0 ) {
        perror( "Unable to read segy binary header" );
        exit( err );
    }

    const int format = segy_format( header );
    const int samples = segy_samples( header );
    const long trace0 = segy_trace0( header );
    const int trace_bsize = segy_trace_bsize( samples );

    int traces;
    err = segy_traces( fp, &traces, trace0, trace_bsize );

    if( err != 0 ) {
        perror( "Could not determine traces" );
        exit( err );
    }

    int sorting;
    err = segy_sorting( fp, il_field,
                            xl_field,
                            SEGY_TR_OFFSET,
                            &sorting,
                            trace0, trace_bsize );
    if( err != 0 ) {
        perror( "Could not determine sorting" );
        exit( err );
    }

    int offsets;
    err = segy_offsets( fp, il_field, xl_field, traces, &offsets, trace0, trace_bsize );
    if( err != 0 ) {
        perror( "Could not determine offsets" );
        exit( err );
    }

    int inline_count, crossline_count;
    if( sorting == SEGY_INLINE_SORTING ) {
        err = segy_count_lines( fp, xl_field, offsets, &inline_count, &crossline_count, trace0, trace_bsize );
    } else {
        err = segy_count_lines( fp, il_field, offsets, &crossline_count, &inline_count, trace0, trace_bsize );
    }

    if( err != 0 ) {
        fprintf( stderr, "Errcode %d\n", err );
        perror( "Could not count lines" );
        exit( err );
    }

    int* inline_indices = malloc( sizeof( int ) * inline_count );
    int* crossline_indices = malloc( sizeof( int ) * crossline_count );

    err = segy_inline_indices( fp, il_field, sorting, inline_count, crossline_count, offsets, inline_indices, trace0, trace_bsize );
    if( err != 0 ) {
        perror( "Could not determine inline numbers" );
        exit( err );
    }

    err = segy_crossline_indices( fp, xl_field, sorting, inline_count, crossline_count, offsets, crossline_indices, trace0, trace_bsize );
    if( err != 0 ) {
        fprintf( stderr, "Errcode %d\n", err );
        perror( "Could not determine crossline numbers" );
        exit( err );
    }

    clock_t diff = clock() - start;

    printf( "Crosslines..........: %d\n", crossline_count);
    printf( "Inlines.............: %d\n", inline_count);
    printf( "Offsets.............: %d\n", offsets);
    printf( "Samples.............: %d\n", samples);
    printf( "Sample format.......: %s\n", getSampleFormatName( format ) );
    printf( "Fastest direction...: %s\n", getFastestDirectionName( sorting ) );


    puts("");
    puts("Crossline indexes:");

    for( int i = 0; i < crossline_count; i++ ) {
        printf( "%d ", crossline_indices[i] );
    }

    puts("\n");
    puts("Inline indexes:");

    for( int i = 0; i < inline_count; i++ ) {
        printf( "%d ", inline_indices[i] );
    }

    puts("\n");
    puts("Sample indexes:");

    //for (int i = 0; i < spec->sample_count; i++) {
    //    printf("%.2f ", spec->sample_indexes[i]);
    //}
    puts("\n");

    printf("Inspection took : %.2f s\n", (double) diff / CLOCKS_PER_SEC);

    free( inline_indices );
    free( crossline_indices );
    segy_close( fp );

    exit(0);
}
