#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <unistd.h>

#include "apputils.h"
#include <segyio/segy.h>

static int printhelp(){
    puts( "Usage: segyio-catb [OPTION]... [FILE]...\n"
          "Concatenate the binary header from FILE(s) to seismic unix "
          "output.\n"
          "\n"
          "--version    output version information and exit\n" 
          "--help       display this help and exit\n"
          "\n"
        );
    return 0;
}

static int get_binary_value( char* binheader, int bfield ){
    int32_t f;
    segy_get_bfield( binheader, bfield, &f );
    return f;
}

struct options {
    int version, help;
    const char* errmsg;
};

static struct options parse_options( int argc, char** argv ){
    struct options opts;
    opts.version = 0, opts.help = 0;
    opts.errmsg = NULL;

    struct options opthelp, optversion;
    opthelp.help = 1, opthelp.errmsg = NULL;
    optversion.version = 1, optversion.errmsg = NULL;
    
    static struct option long_options[] = {
        {"version",     no_argument,    0,    'V'},
        {"help",        no_argument,    0,    'h'},
        {0, 0, 0, 0}
    };

    opterr = 1;
     
    while( true ){

        int option_index = 0;
        int c = getopt_long( argc, argv, "",
                            long_options, &option_index);

        if ( c == -1 ) break;
        
        switch( c ){
            case  0: break;
            case 'h': return opthelp;
            case 'V': return optversion;
            default: 
                 opthelp.errmsg = "";
                 return opthelp;
        }
    }
    return opts;
}

int main( int argc, char** argv ){

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
    
    if( argc == 1 ){
         int err = errmsg(2, "Missing argument\n");
         printhelp();
         return err;
    }

    struct options opts = parse_options( argc, argv );
   
    if( opts.help )    return printhelp();
    if( opts.version ) return printversion( "segyio-catb" );

    for( int i = optind; i < argc; ++i ){
        segy_file* fp = segy_open( argv[ i ], "r" );

        if( !fp ) return errmsg(opterr, "No such file or directory");

        char binheader[ SEGY_BINARY_HEADER_SIZE ];
        int err = segy_binheader( fp, binheader );        

        if( err ) return errmsg(opterr, "Unable to read binary header"); 

        for( int c = 0; c < 30; ++c ){
            printf( "%s\t%d\n", su[ c ],
                    get_binary_value( binheader, bfield_value[ c ] ));
        }
        
        segy_close( fp );
    }
    return 0;
}

