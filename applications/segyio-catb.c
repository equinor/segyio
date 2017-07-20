#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <unistd.h>

#include <segyio/segy.h>

static int printhelp(){
    puts( "Usage: segyio-catb [OPTION]... [FILE]...\n"
          "Concatenate the binary header from FILE(s) to seismic unix"
          "output.\n"
          "\n"
          "--version    output version information and exit\n" 
          "--help       display this help and exit\n"
          "\n"
        );
    return 0;
}

static int printversion(){
    printf( "segyio-catb (segyio version %d.%d)\n",
             segyio_MAJOR, segyio_MINOR );
    return 0;
}

static int get_binary_value( char* binheader, int bfield ){
    int32_t f;
    segy_get_bfield( binheader, bfield, &f );
    return f;
}

int main( int argc, char* const argv[] ){

   static const char* su[ 30 ] = {
        "jobid",
        "lino",
        "reno",
        "ntrpr",
        "nart",
        "hdt",
        "dto",
        "hns",
        "nso",
        "format",
        "fold",
        "tsort",
        "vscode",
        "hsfs",
        "hsfe",
        "hslen",
        "hstyp",
        "schn",
        "hstas",
        "hstae",
        "htatyp",
        "hcorr",
        "bgrcv",
        "rcvm",
        "mfeet",
        "polyt",
        "vpol",
        "rev",
        "trflag",
        "exth"
    };

    static int bfield_value[ 30 ] = {
        SEGY_BIN_JOB_ID,
        SEGY_BIN_LINE_NUMBER,
        SEGY_BIN_REEL_NUMBER,
        SEGY_BIN_TRACES,
        SEGY_BIN_AUX_TRACES,
        SEGY_BIN_INTERVAL,
        SEGY_BIN_INTERVAL_ORIG,
        SEGY_BIN_SAMPLES,
        SEGY_BIN_SAMPLES_ORIG,
        SEGY_BIN_FORMAT,
        SEGY_BIN_ENSEMBLE_FOLD,
        SEGY_BIN_SORTING_CODE,
        SEGY_BIN_VERTICAL_SUM,
        SEGY_BIN_SWEEP_FREQ_START,
        SEGY_BIN_SWEEP_FREQ_END,
        SEGY_BIN_SWEEP_LENGTH,
        SEGY_BIN_SWEEP,
        SEGY_BIN_SWEEP_CHANNEL,
        SEGY_BIN_SWEEP_TAPER_START,
        SEGY_BIN_SWEEP_TAPER_END,
        SEGY_BIN_TAPER,
        SEGY_BIN_CORRELATED_TRACES,
        SEGY_BIN_BIN_GAIN_RECOVERY,
        SEGY_BIN_AMPLITUDE_RECOVERY,
        SEGY_BIN_MEASUREMENT_SYSTEM,
        SEGY_BIN_IMPULSE_POLARITY,
        SEGY_BIN_VIBRATORY_POLARITY,
        SEGY_BIN_SEGY_REVISION,
        SEGY_BIN_TRACE_FLAG,
        SEGY_BIN_EXT_HEADERS
    };
    
    if( argc == 1 ) return printhelp();

    static int version = 0;
    
    static struct option long_options[] = {
        {"version",     no_argument,    &version,    1 },
        {"help",        no_argument,    0,          'h'},
        {0, 0, 0, 0}
    };


    while( true ){

        int option_index = 0;
        int c = getopt_long( argc, argv, "",
                            long_options, &option_index);

        if ( c == -1 ) break;

        switch( c ){
            case  0: break;
            case 'h': return printhelp();
            default:  return printhelp();
        }
    }

    if( version ) return printversion();

    for( int i = optind; i < argc; ++i ){
        segy_file* fp = segy_open( argv[ i ], "r" );
        if( !fp ) printf("segyio-catb: %s: No such file or directory\n",
                           argv[ i ] );
        if( !fp ) continue;

        char binheader[ SEGY_BINARY_HEADER_SIZE ];
        int err = segy_binheader( fp, binheader );        
        if( err ) printf( "segyio-catb: %s: Unable to read binary header\n",
                           argv[ i ] );
        if( err ) continue;

        for( int c = 0; c < 30; ++c ){
            printf( "%s\t%d\n", su[ c ],
                    get_binary_value( binheader, bfield_value[ c ] ));
        }
        
        segy_close( fp );
    }
    return 0;
}

