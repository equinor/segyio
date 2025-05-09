#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <unistd.h>

#include "apputils.h"
#include <segyio/segy.h>

static int printhelp(void){
    puts( "Usage: segyio-catb [OPTION]... [FILE]...\n"
          "Concatenate the binary header from FILE(s) to seismic unix "
          "output.\n"
          "\n"
          "-n,  --nonzero       only print fields with non-zero values\n"
          "-d,  --description   print with byte offset and field description\n"
          "     --version       output version information and exit\n"
          "     --help          display this help and exit\n"
          "\n"
        );
    return 0;
}

struct options {
    int version;
    int help;
    int nonzero;
    int description;
    const char* errmsg;
};

static struct options parse_options( int argc, char** argv ){
    struct options opts;
    opts.nonzero = 0;
    opts.description = 0;
    opts.version = 0, opts.help = 0;
    opts.errmsg = NULL;

    static struct option long_options[] = {
        {"version",         no_argument,    0,    'V'},
        {"help",            no_argument,    0,    'h'},
        {"description",     no_argument,    0,    'd'},
        {"nonzero",         no_argument,    0,    'n'},
        {0, 0, 0, 0}
    };

    opterr = 1;

    while( true ){

        int option_index = 0;
        int c = getopt_long( argc, argv, "nd",
                            long_options, &option_index);

        if ( c == -1 ) break;

        switch( c ){
            case  0: break;
            case 'h': opts.help = 1;    return opts;
            case 'V': opts.version = 1; return opts;
            case 'd': opts.description = 1; break;
            case 'n': opts.nonzero = 1; break;
            default:
                 opts.help = 1;
                 opts.errmsg = "";
                 return opts;
        }
    }
    return opts;
}

typedef struct {
    int offset;
    const char* short_name;
    const char* description;
} binary_field_type;

int main( int argc, char** argv ){

    if( argc == 1 ){
         int err = errmsg(2, "Missing argument\n");
         printhelp();
         return err;
    }

    binary_field_type field_data[] = {
        {SEGY_BIN_JOB_ID, "jobid", "Job identification number"},
        {SEGY_BIN_LINE_NUMBER, "lino", "Line number"},
        {SEGY_BIN_REEL_NUMBER, "reno", "Reel number"},
        {SEGY_BIN_ENSEMBLE_TRACES, "ntrpr", "Number of data traces per ensemble"},
        {SEGY_BIN_AUX_ENSEMBLE_TRACES, "nart", "Number of auxiliary traces per ensemble"},
        {SEGY_BIN_INTERVAL, "hdt", "Sample interval in microseconds (μs)"},
        {SEGY_BIN_INTERVAL_ORIG, "dto", "Sample interval in microseconds (μs) of original field recording"},
        {SEGY_BIN_SAMPLES, "hns", "Number of samples per data trace"},
        {SEGY_BIN_SAMPLES_ORIG, "nso", "Number of samples per data trace for original field recording"},
        {SEGY_BIN_FORMAT, "format", "Data sample format code"},
        {SEGY_BIN_ENSEMBLE_FOLD, "fold", "Ensemble fold"},
        {SEGY_BIN_SORTING_CODE, "tsort", "Trace sorting code"},
        {SEGY_BIN_VERTICAL_SUM, "vscode", "Vertical sum code"},
        {SEGY_BIN_SWEEP_FREQ_START, "hsfs", "Sweep frequency at start (Hz)"},
        {SEGY_BIN_SWEEP_FREQ_END, "hsfe", "Sweep frequency at end (Hz)"},
        {SEGY_BIN_SWEEP_LENGTH, "hslen", "Sweep length (ms)"},
        {SEGY_BIN_SWEEP, "hstyp", "Sweep type code"},
        {SEGY_BIN_SWEEP_CHANNEL, "schn", "Trace number of sweep channel"},
        {SEGY_BIN_SWEEP_TAPER_START, "hstas", "Sweep trace taper length in milliseconds at start if tapered"},
        {SEGY_BIN_SWEEP_TAPER_END, "hstae", "Sweep trace taper length in milliseconds at end"},
        {SEGY_BIN_TAPER, "htatyp", "Taper type"},
        {SEGY_BIN_CORRELATED_TRACES, "hcorr", "Correlated data traces"},
        {SEGY_BIN_BIN_GAIN_RECOVERY, "bgrcv", "Binary gain recovered"},
        {SEGY_BIN_AMPLITUDE_RECOVERY, "rcvm", "Amplitude recovery method"},
        {SEGY_BIN_MEASUREMENT_SYSTEM, "mfeet", "Measurement system"},
        {SEGY_BIN_IMPULSE_POLARITY, "polyt", "Impulse signal polarity"},
        {SEGY_BIN_VIBRATORY_POLARITY, "vpol", "Vibratory polarity code"},
        {SEGY_BIN_SEGY_REVISION, "rev", "SEG Y Format Revision Number"},
        {SEGY_BIN_TRACE_FLAG, "trflag", "Fixed length trace flag"},
        {SEGY_BIN_EXT_HEADERS, "exth", "Number of 3200-byte, Extended Textual File Headers"},
    };

    struct options opts = parse_options( argc, argv );

    if( opts.help )    return printhelp();
    if( opts.version ) return printversion( "segyio-catb" );

    for( int i = optind; i < argc; ++i ){
        segy_file* fp = segy_open( argv[ i ], "r" );

        if( !fp ) return errmsg(opterr, "No such file or directory");

        char binheader[ SEGY_BINARY_HEADER_SIZE ];
        int err = segy_binheader( fp, binheader );

        if( err ) return errmsg(opterr, "Unable to read binary header");

        int nr_fields = sizeof(field_data)/sizeof(binary_field_type);
        for( int c = 0; c < nr_fields; ++c ){
            segy_field_data fd = segy_get_field( binheader, field_data[c].offset );
            if ( fd.error ) return errmsg( fd.error, "Unable to read field" );

            int byte_offset = (field_data[c].offset - SEGY_TEXT_HEADER_SIZE);
            const char* short_name = field_data[c].short_name;
            const char* description = field_data[c].description;

            if( fd.datatype == SEGY_UNSIGNED_INTEGER_8_BYTE ) {
                long long int field = fd.value.u64;
                if( opts.nonzero && field == 0) continue;
                if( opts.description )
                    printf( "%s\t%lld\t%d\t%s\n", short_name, field, byte_offset, description );
                else
                    printf( "%s\t%lld\n", short_name, field );
            }
            else if( fd.datatype == SEGY_IEEE_FLOAT_8_BYTE ) {
                double field = fd.value.f64;
                if( opts.nonzero && field == 0.0) continue;
                if( opts.description )
                    printf( "%s\t%f\t%d\t%s\n", short_name, field, byte_offset, description );
                else
                    printf( "%s\t%f\n", short_name, field );
            }
            else {
                int field;
                err = segy_field_data_to_int( &fd, &field );
                if( err ) return errmsg( err, "Unable to convert segy_field_data to int32" );

                if( opts.nonzero && field == 0) continue;
                if( opts.description )
                    printf( "%s\t%d\t%d\t%s\n", short_name, field, byte_offset, description );
                else
                    printf( "%s\t%d\n", short_name, field );
            }
        }
        segy_close( fp );
    }
    return 0;
}
