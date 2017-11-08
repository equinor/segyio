#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <unistd.h>

#include "apputils.c"
#include <segyio/segy.h>

static int help() {
    puts( "Usage: segyio-crop [OPTION]... SRC DST\n"
          "Copy a sub cube from SRC to DST\n"
          "\n"
          "-i, --iline-begin=LINE     inline to copy from\n"
          "-I, --iline-end=LINE       inline to copy to (inclusive)\n"
          "-x, --xline-begin=LINE     crossline to copy from\n"
          "-X, --xline-end=LINE       crossline to copy to (inclusive)\n"
          "    --inline-begin         alias to --iline-begin\n"
          "    --crossline-begin      alias to --xline-begin\n"
          "-s, --sample-begin=TIME    measurement to copy from\n"
          "-S, --sample-end=TIME      measurement to copy to (inclusive)\n"
          "-b, --il                   inline header word byte offset\n"
          "-B, --xl                   crossline header word byte offset\n"
          "-v, --verbose              increase verbosity\n"
          "    --version              output version information and exit\n"
          "    --help                 display this help and exit\n"
          "\n"
          "If no begin/end options are specified, this program is\n"
          "essentially a copy. If a begin option is omitted, the program\n"
          "copies from the start. If an end option is omitted, the program\n"
          "copies until the end.\n"
        );
    return 0;
}

struct delay {
    int delay;
    int skip;
    int len;
};

static struct delay delay_recording_time( const char* trheader,
                                          int sbeg,
                                          int send,
                                          int dt,
                                          int samples ) {

    long long t0 = trfield( trheader, SEGY_TR_DELAY_REC_TIME );
    int trdt     = trfield( trheader, SEGY_TR_SAMPLE_INTER );
    if( trdt ) dt = trdt;

    /*
     * begin/end not specified - copy the full trace, so dont try to identify
     * the sub trace
     */
    struct delay d = { t0, 0, samples };
    if( sbeg < 0 && send == INT_MAX ) return d;

    /* determine what to cut off at the start of the trace */
    if( sbeg - t0 > 0 ) {
        long long skip = ((sbeg - t0) * 1000) / dt;
        d.delay   = t0 + ((skip * dt) / 1000 );
        d.skip    = skip;
        d.len -= d.skip;
    }

    /* determine what to cut off at the end of the trace */
    if( (long long)send * 1000 < (t0 * 1000) + (samples * dt) ) {
        long long t0us = t0 * 1000;
        long long sendus = (long long)send * 1000;
        d.len -= (t0us + ((samples - 1) * dt) - sendus) / dt;
    }

    return d;
}

static int valid_trfield( int x ) {
    switch( x ) {
        case SEGY_TR_SEQ_LINE:
        case SEGY_TR_SEQ_FILE:
        case SEGY_TR_FIELD_RECORD:
        case SEGY_TR_NUMBER_ORIG_FIELD:
        case SEGY_TR_ENERGY_SOURCE_POINT:
        case SEGY_TR_ENSEMBLE:
        case SEGY_TR_NUM_IN_ENSEMBLE:
        case SEGY_TR_TRACE_ID:
        case SEGY_TR_SUMMED_TRACES:
        case SEGY_TR_STACKED_TRACES:
        case SEGY_TR_DATA_USE:
        case SEGY_TR_OFFSET:
        case SEGY_TR_RECV_GROUP_ELEV:
        case SEGY_TR_SOURCE_SURF_ELEV:
        case SEGY_TR_SOURCE_DEPTH:
        case SEGY_TR_RECV_DATUM_ELEV:
        case SEGY_TR_SOURCE_DATUM_ELEV:
        case SEGY_TR_SOURCE_WATER_DEPTH:
        case SEGY_TR_GROUP_WATER_DEPTH:
        case SEGY_TR_ELEV_SCALAR:
        case SEGY_TR_SOURCE_GROUP_SCALAR:
        case SEGY_TR_SOURCE_X:
        case SEGY_TR_SOURCE_Y:
        case SEGY_TR_GROUP_X:
        case SEGY_TR_GROUP_Y:
        case SEGY_TR_COORD_UNITS:
        case SEGY_TR_WEATHERING_VELO:
        case SEGY_TR_SUBWEATHERING_VELO:
        case SEGY_TR_SOURCE_UPHOLE_TIME:
        case SEGY_TR_GROUP_UPHOLE_TIME:
        case SEGY_TR_SOURCE_STATIC_CORR:
        case SEGY_TR_GROUP_STATIC_CORR:
        case SEGY_TR_TOT_STATIC_APPLIED:
        case SEGY_TR_LAG_A:
        case SEGY_TR_LAG_B:
        case SEGY_TR_DELAY_REC_TIME:
        case SEGY_TR_MUTE_TIME_START:
        case SEGY_TR_MUTE_TIME_END:
        case SEGY_TR_SAMPLE_COUNT:
        case SEGY_TR_SAMPLE_INTER:
        case SEGY_TR_GAIN_TYPE:
        case SEGY_TR_INSTR_GAIN_CONST:
        case SEGY_TR_INSTR_INIT_GAIN:
        case SEGY_TR_CORRELATED:
        case SEGY_TR_SWEEP_FREQ_START:
        case SEGY_TR_SWEEP_FREQ_END:
        case SEGY_TR_SWEEP_LENGTH:
        case SEGY_TR_SWEEP_TYPE:
        case SEGY_TR_SWEEP_TAPERLEN_START:
        case SEGY_TR_SWEEP_TAPERLEN_END:
        case SEGY_TR_TAPER_TYPE:
        case SEGY_TR_ALIAS_FILT_FREQ:
        case SEGY_TR_ALIAS_FILT_SLOPE:
        case SEGY_TR_NOTCH_FILT_FREQ:
        case SEGY_TR_NOTCH_FILT_SLOPE:
        case SEGY_TR_LOW_CUT_FREQ:
        case SEGY_TR_HIGH_CUT_FREQ:
        case SEGY_TR_LOW_CUT_SLOPE:
        case SEGY_TR_HIGH_CUT_SLOPE:
        case SEGY_TR_YEAR_DATA_REC:
        case SEGY_TR_DAY_OF_YEAR:
        case SEGY_TR_HOUR_OF_DAY:
        case SEGY_TR_MIN_OF_HOUR:
        case SEGY_TR_SEC_OF_MIN:
        case SEGY_TR_TIME_BASE_CODE:
        case SEGY_TR_WEIGHTING_FAC:
        case SEGY_TR_GEOPHONE_GROUP_ROLL1:
        case SEGY_TR_GEOPHONE_GROUP_FIRST:
        case SEGY_TR_GEOPHONE_GROUP_LAST:
        case SEGY_TR_GAP_SIZE:
        case SEGY_TR_OVER_TRAVEL:
        case SEGY_TR_CDP_X:
        case SEGY_TR_CDP_Y:
        case SEGY_TR_INLINE:
        case SEGY_TR_CROSSLINE:
        case SEGY_TR_SHOT_POINT:
        case SEGY_TR_SHOT_POINT_SCALAR:
        case SEGY_TR_MEASURE_UNIT:
        case SEGY_TR_TRANSDUCTION_MANT:
        case SEGY_TR_TRANSDUCTION_EXP:
        case SEGY_TR_TRANSDUCTION_UNIT:
        case SEGY_TR_DEVICE_ID:
        case SEGY_TR_SCALAR_TRACE_HEADER:
        case SEGY_TR_SOURCE_TYPE:
        case SEGY_TR_SOURCE_ENERGY_DIR_MANT:
        case SEGY_TR_SOURCE_ENERGY_DIR_EXP:
        case SEGY_TR_SOURCE_MEASURE_MANT:
        case SEGY_TR_SOURCE_MEASURE_EXP:
        case SEGY_TR_SOURCE_MEASURE_UNIT:
        case SEGY_TR_UNASSIGNED1:
        case SEGY_TR_UNASSIGNED2:
            return 1;

        default: return 0;
    }
}

#define TRHSIZE  SEGY_TRACE_HEADER_SIZE
#define BINSIZE  SEGY_BINARY_HEADER_SIZE
#define TEXTSIZE SEGY_TEXT_HEADER_SIZE

struct options {
    int ibeg, iend;
    int xbeg, xend;
    int sbeg, send;
    int il, xl;
    char* src;
    char* dst;
    int verbosity;
    int version, help;
    const char* errmsg;
};

static struct options parse_options( int argc, char** argv ) {
    struct options opts;
    opts.ibeg = -1, opts.iend = INT_MAX;
    opts.xbeg = -1, opts.xend = INT_MAX;
    opts.sbeg = -1, opts.send = INT_MAX;
    opts.il = SEGY_TR_INLINE, opts.xl = SEGY_TR_CROSSLINE;
    opts.verbosity = 0;
    opts.version = 0, opts.help = 0;
    opts.errmsg = NULL;

    struct options opthelp, optversion;
    opthelp.help = 1, opthelp.errmsg = NULL;
    optversion.version = 1, optversion.errmsg = NULL;

    static struct option long_options[] = {
        { "iline-begin",        required_argument, 0, 'i' },
        { "iline-end",          required_argument, 0, 'I' },
        { "inline-begin",       required_argument, 0, 'i' },
        { "inline-end",         required_argument, 0, 'I' },

        { "xline-begin",        required_argument, 0, 'x' },
        { "xline-end",          required_argument, 0, 'X' },
        { "crossline-begin",    required_argument, 0, 'x' },
        { "crossline-end",      required_argument, 0, 'X' },

        { "sample-begin",       required_argument, 0, 's' },
        { "sample-end",         required_argument, 0, 'S' },

        { "il",                 required_argument, 0, 'b' },
        { "xl",                 required_argument, 0, 'B' },

        { "verbose",            no_argument,       0, 'v' },
        { "version",            no_argument,       0, 'V' },
        { "help",               no_argument,       0, 'h' },
        { 0, 0, 0, 0 }
    };

    static const char* parsenum_errmsg[] = { "", "num must be an integer",
                                                 "num must be non-negative" };

    opterr = 1;

    while( true ) {
        int option_index = 0;
        int c = getopt_long( argc, argv, "vi:I:x:X:s:S:b:B:",
                             long_options, &option_index );

        if( c == -1 ) break;

        int ret;
        switch( c ) {
            case  0: break;
            case 'h': return opthelp;
            case 'V': return optversion;
            case 'v': ++opts.verbosity; break;

            case 'i':
                ret = parseint( optarg, &opts.ibeg );
                if( ret == 0 ) break;
                opts.errmsg = parsenum_errmsg[ ret ];
                return opts;

            case 'I':
                ret = parseint( optarg, &opts.iend );
                if( ret == 0 ) break;
                opts.errmsg = parsenum_errmsg[ ret ];
                return opts;

            case 'x':
                ret = parseint( optarg, &opts.xbeg );
                if( ret == 0 ) break;
                opts.errmsg = parsenum_errmsg[ ret ];
                return opts;

            case 'X':
                ret = parseint( optarg, &opts.xend );
                if( ret == 0 ) break;
                opts.errmsg = parsenum_errmsg[ ret ];
                return opts;

            case 's':
                ret = parseint( optarg, &opts.sbeg );
                if( ret == 0 ) break;
                opts.errmsg = parsenum_errmsg[ ret ];
                return opts;

            case 'S':
                ret = parseint( optarg, &opts.send );
                if( ret == 0 ) break;
                opts.errmsg = parsenum_errmsg[ ret ];
                return opts;

            case 'b':
                ret = parseint( optarg, &opts.il );
                if( ret == 0 ) break;
                opts.errmsg = parsenum_errmsg[ ret ];
                return opts;

            case 'B':
                ret = parseint( optarg, &opts.xl );
                if( ret == 0 ) break;
                opts.errmsg = parsenum_errmsg[ ret ];
                return opts;

            default:
                opthelp.errmsg = "";
                return opthelp;
        }
    }

    if( argc - optind != 2 ) {
        errmsg( 0, "Wrong number of files" );
        return opthelp;
    }

    opts.src = argv[ optind + 0 ];
    opts.dst = argv[ optind + 1 ];
    return opts;
}

int main( int argc, char** argv ) {

    struct options opts = parse_options( argc, argv );

    if( opts.help ) return help() + (opts.errmsg ? 2 : 0);
    if( opts.version ) return printversion( "segyio-crop" );
    if( opts.errmsg ) return errmsg( EINVAL, opts.errmsg );

    int ibeg = opts.ibeg;
    int iend = opts.iend;
    int xbeg = opts.xbeg;
    int xend = opts.xend;
    int sbeg = opts.sbeg;
    int send = opts.send;
    int il = opts.il;
    int xl = opts.xl;
    int verbosity = opts.verbosity;

    if( !valid_trfield( il ) )
        return errmsg( -3, "Invalid inline byte offset" );

    if( !valid_trfield( xl ) )
        return errmsg( -3, "Invalid crossline byte offset" );

    if( ibeg > iend )
        return errmsg( -4, "Invalid iline interval - file would be empty" );

    if( xbeg > xend )
        return errmsg( -4, "Invalid xline interval - file would be empty" );

    if( sbeg > send )
        return errmsg( -4, "Invalid sample interval - file would be empty" );

    char textheader[ TEXTSIZE ] = { 0 };
    char binheader[ BINSIZE ]   = { 0 };
    char trheader[ TEXTSIZE ]   = { 0 };

    FILE* src = fopen( opts.src, "rb" );
    if( !src )
        return errmsg2( errno, "Unable to open src", strerror( errno ) );

    FILE* dst = fopen( opts.dst, "wb" );
    if( !dst )
        return errmsg2( errno, "Unable to open dst", strerror( errno ) );

    /* copy the textual and binary headers */
    if( verbosity > 0 ) puts( "Copying text header" );
    int sz;
    sz = fread( textheader, TEXTSIZE, 1, src );
    if( sz != 1 ) return errmsg2( errno, "Unable to read text header",
                                         strerror( errno ) );

    sz = fwrite( textheader, TEXTSIZE, 1, dst );
    if( sz != 1 ) return errmsg2( errno, "Unable to write text header",
                                         strerror( errno ) );

    if( verbosity > 0 ) puts( "Copying binary header" );
    sz = fread( binheader, BINSIZE, 1, src );
    if( sz != 1 ) return errmsg2( errno, "Unable to read binary header",
                                         strerror( errno ) );

    sz = fwrite( binheader, BINSIZE, 1, dst );
    if( sz != 1 ) return errmsg2( errno, "Unable to write binary header",
                                         strerror( errno ) );

    int ext_headers = bfield( binheader, SEGY_BIN_EXT_HEADERS );
    if( ext_headers < 0 ) return errmsg( -1, "Malformed binary header" );

    for( int i = 0; i < ext_headers; ++i ) {
        if( verbosity > 0 ) puts( "Copying extended text header" );
        sz = fread( textheader, TEXTSIZE, 1, src );
        if( sz != 1 ) return errmsg2( errno, "Unable to read ext text header",
                                             strerror( errno ) );

        sz = fwrite( textheader, TEXTSIZE, 1, dst );
        if( sz != 1 ) return errmsg2( errno, "Unable to write ext text header",
                                             strerror( errno ) );
    }

    if( verbosity > 2 ) puts( "Computing samples-per-trace" );
    const int bindt = bfield( binheader, SEGY_BIN_INTERVAL );
    const int src_samples = bfield( binheader, SEGY_BIN_SAMPLES );
    if( src_samples < 0 )
        return errmsg( -2, "Could not determine samples per trace" );

    if( verbosity > 2 ) printf( "Found %d samples per trace\n", src_samples );

    float* trace = malloc( src_samples * sizeof( float ) );

    if( verbosity > 0 ) puts( "Copying traces" );
    long long traces = 0;
    while( true ) {
        sz = fread( trheader, TRHSIZE, 1, src );

        if( sz != 1 && feof( src ) ) break;
        if( sz != 1 && ferror( src ) )
            return errmsg( ferror( src ), "Unable to read trace header" );

        int ilno = trfield( trheader, SEGY_TR_INLINE );
        int xlno = trfield( trheader, SEGY_TR_CROSSLINE );

        /* outside copy interval - skip this trace */
        if( ilno < ibeg || ilno > iend || xlno < xbeg || xlno > xend ) {
            fseek( src, sizeof( float ) * src_samples, SEEK_CUR );
            continue;
        }

        sz = fread( trace, sizeof( float ), src_samples, src );
        if( sz != src_samples )
            return errmsg2( errno, "Unable to read trace",
                                   strerror( errno ) );

        /* figure out how to crop the trace */
        struct delay d = delay_recording_time( trheader,
                                               sbeg,
                                               send,
                                               bindt,
                                               src_samples );

        segy_set_field( trheader, SEGY_TR_DELAY_REC_TIME, d.delay );
        segy_set_bfield( binheader, SEGY_BIN_SAMPLES, d.len );

        if( verbosity > 2 ) printf( "Copying trace %lld\n", traces );
        sz = fwrite( trheader, TRHSIZE, 1, dst );
        if( sz != 1 )
            return errmsg2( errno, "Unable to write trace header",
                                   strerror( errno ) );

        sz = fwrite( trace + d.skip, sizeof( float ), d.len, dst );
        if( sz != d.len )
            return errmsg2( errno, "Unable to write trace",
                                   strerror( errno ) );
        ++traces;
    }

    fseek( dst, SEGY_TEXT_HEADER_SIZE, SEEK_SET );
    sz = fwrite( binheader, BINSIZE, 1, dst );
    if( sz != 1 ) return errmsg2( errno, "Unable to write binary header",
                                         strerror( errno ) );

    free( trace );
    fclose( dst );
    fclose( src );
}
