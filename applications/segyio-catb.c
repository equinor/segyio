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

static void u64_to_buf(char *buf, size_t buflen, unsigned long long int value) {
    snprintf(buf, buflen, "%llu", value);
}

static void i64_to_buf(char *buf, size_t buflen, long long int value) {
    snprintf(buf, buflen, "%lld", value);
}

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

        {SEGY_BIN_EXT_ENSEMBLE_TRACES, "extntrpr", "Extended number of data traces per ensemble"},
        {SEGY_BIN_EXT_AUX_ENSEMBLE_TRACES, "extnart", "Extended number of auxiliary traces per ensemble"},
        {SEGY_BIN_EXT_SAMPLES, "exthns", "Extended number of samples per data trace"},
        {SEGY_BIN_EXT_INTERVAL, "exthdt", "Extended sample interval, IEEE double precision (64-bit)"},
        {SEGY_BIN_EXT_INTERVAL_ORIG, "extdto", "Extended sample interval of original field recording, IEEE double precision (64-bit)"},
        {SEGY_BIN_EXT_SAMPLES_ORIG, "extnso", "Extended number of samples per data trace in original recording"},
        {SEGY_BIN_EXT_ENSEMBLE_FOLD, "extfold", "Extended ensemble fold"},
        {SEGY_BIN_INTEGER_CONSTANT, "intconst", "The integer constant 1690906010"},
        {SEGY_BIN_SEGY_REVISION, "rev", "SEG Y Format Revision Number"},
        {SEGY_BIN_SEGY_REVISION_MINOR, "revmin", "SEG Y Format Min Revision Number"},
        {SEGY_BIN_TRACE_FLAG, "trflag", "Fixed length trace flag"},
        {SEGY_BIN_EXT_HEADERS, "exth", "Number of 3200-byte, Extended Textual File Headers"},
        {SEGY_BIN_MAX_ADDITIONAL_TR_HEADERS, "maxtrh", "Maximum number of additional 240-byte trace headers"},
        {SEGY_BIN_SURVEY_TYPE, "survty", "Survey type"},
        {SEGY_BIN_TIME_BASIS_CODE, "timebc", "Time basis code"},
        {SEGY_BIN_NR_TRACES_IN_STREAM, "ntrst", "Number of traces in this file or stream"},
        {SEGY_BIN_FIRST_TRACE_OFFSET, "ftroff", "Byte offset of first trace relative to start of stream"},
        {SEGY_BIN_NR_TRAILER_RECORDS, "ntr", "Number of 3200-byte data trailer stanza records"}
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
            segy_field_data fd;
            err = segy_get_binfield( binheader, field_data[c].offset, &fd );
            if ( err ) return errmsg( err, "Unable to read field" );

            bool value_is_zero = false;
            char value_str[40]; // should be enough for all strings

            switch( fd.datatype ) {
                case SEGY_UNSIGNED_INTEGER_8_BYTE: {
                    u64_to_buf( value_str, sizeof( value_str ), fd.value.u64 );
                } break;
                case SEGY_UNSIGNED_SHORT_2_BYTE: {
                    u64_to_buf( value_str, sizeof( value_str ), fd.value.u16 );
                } break;
                case SEGY_UNSIGNED_CHAR_1_BYTE: {
                    u64_to_buf( value_str, sizeof( value_str ), fd.value.u8 );
                } break;
                case SEGY_SIGNED_INTEGER_4_BYTE: {
                    i64_to_buf( value_str, sizeof( value_str ), fd.value.i32 );
                } break;
                case SEGY_SIGNED_SHORT_2_BYTE: {
                    i64_to_buf( value_str, sizeof( value_str ), fd.value.i16 );
                } break;
                case SEGY_IEEE_FLOAT_8_BYTE: {
                    double value = fd.value.f64;
                    value_is_zero = (value == 0.0);
                    snprintf( value_str, sizeof( value_str ), "%f", value );
                } break;
                default: {
                    return errmsg( -1, "Unhandled field format" );
                }
            }

            if (strcmp( value_str, "0" ) == 0) {
                value_is_zero = true;
            }

            if( opts.nonzero && value_is_zero ) continue;
            if( opts.description ) {
                int byte_offset = (field_data[c].offset - SEGY_TEXT_HEADER_SIZE);
                const char* short_name = field_data[c].short_name;
                const char* description = field_data[c].description;
                printf( "%s\t%s\t%d\t%s\n", short_name, value_str, byte_offset, description );
            } else {
                printf( "%-10s\t%s\n", field_data[c].short_name, value_str );
            }
        }
        segy_close( fp );
    }
    return 0;
}
