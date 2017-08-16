#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <unistd.h>

#include <segyio/segy.h>
#include "apputils.c"

#define TRHSIZE SEGY_TRACE_HEADER_SIZE
#define BINSIZE SEGY_BINARY_HEADER_SIZE

static const int fields[] = {
    SEGY_TR_SEQ_LINE                ,
    SEGY_TR_SEQ_FILE                ,
    SEGY_TR_FIELD_RECORD            ,
    SEGY_TR_NUMBER_ORIG_FIELD       ,
    SEGY_TR_ENERGY_SOURCE_POINT     ,
    SEGY_TR_ENSEMBLE                ,
    SEGY_TR_NUM_IN_ENSEMBLE         ,
    SEGY_TR_TRACE_ID                ,
    SEGY_TR_SUMMED_TRACES           ,
    SEGY_TR_STACKED_TRACES          ,
    SEGY_TR_DATA_USE                ,
    SEGY_TR_OFFSET                  ,
    SEGY_TR_RECV_GROUP_ELEV         ,
    SEGY_TR_SOURCE_SURF_ELEV        ,
    SEGY_TR_SOURCE_DEPTH            ,
    SEGY_TR_RECV_DATUM_ELEV         ,
    SEGY_TR_SOURCE_DATUM_ELEV       ,
    SEGY_TR_SOURCE_WATER_DEPTH      ,
    SEGY_TR_GROUP_WATER_DEPTH       ,
    SEGY_TR_ELEV_SCALAR             ,
    SEGY_TR_SOURCE_GROUP_SCALAR     ,
    SEGY_TR_SOURCE_X                ,
    SEGY_TR_SOURCE_Y                ,
    SEGY_TR_GROUP_X                 ,
    SEGY_TR_GROUP_Y                 ,
    SEGY_TR_COORD_UNITS             ,
    SEGY_TR_WEATHERING_VELO         ,
    SEGY_TR_SUBWEATHERING_VELO      ,
    SEGY_TR_SOURCE_UPHOLE_TIME      ,
    SEGY_TR_GROUP_UPHOLE_TIME       ,
    SEGY_TR_SOURCE_STATIC_CORR      ,
    SEGY_TR_GROUP_STATIC_CORR       ,
    SEGY_TR_TOT_STATIC_APPLIED      ,
    SEGY_TR_LAG_A                   ,
    SEGY_TR_LAG_B                   ,
    SEGY_TR_DELAY_REC_TIME          ,
    SEGY_TR_MUTE_TIME_START         ,
    SEGY_TR_MUTE_TIME_END           ,
    SEGY_TR_SAMPLE_COUNT            ,
    SEGY_TR_SAMPLE_INTER            ,
    SEGY_TR_GAIN_TYPE               ,
    SEGY_TR_INSTR_GAIN_CONST        ,
    SEGY_TR_INSTR_INIT_GAIN         ,
    SEGY_TR_CORRELATED              ,
    SEGY_TR_SWEEP_FREQ_START        ,
    SEGY_TR_SWEEP_FREQ_END          ,
    SEGY_TR_SWEEP_LENGTH            ,
    SEGY_TR_SWEEP_TYPE              ,
    SEGY_TR_SWEEP_TAPERLEN_START    ,
    SEGY_TR_SWEEP_TAPERLEN_END      ,
    SEGY_TR_TAPER_TYPE              ,
    SEGY_TR_ALIAS_FILT_FREQ         ,
    SEGY_TR_ALIAS_FILT_SLOPE        ,
    SEGY_TR_NOTCH_FILT_FREQ         ,
    SEGY_TR_NOTCH_FILT_SLOPE        ,
    SEGY_TR_LOW_CUT_FREQ            ,
    SEGY_TR_HIGH_CUT_FREQ           ,
    SEGY_TR_LOW_CUT_SLOPE           ,
    SEGY_TR_HIGH_CUT_SLOPE          ,
    SEGY_TR_YEAR_DATA_REC           ,
    SEGY_TR_DAY_OF_YEAR             ,
    SEGY_TR_HOUR_OF_DAY             ,
    SEGY_TR_MIN_OF_HOUR             ,
    SEGY_TR_SEC_OF_MIN              ,
    SEGY_TR_TIME_BASE_CODE          ,
    SEGY_TR_WEIGHTING_FAC           ,
    SEGY_TR_GEOPHONE_GROUP_ROLL1    ,
    SEGY_TR_GEOPHONE_GROUP_FIRST    ,
    SEGY_TR_GEOPHONE_GROUP_LAST     ,
    SEGY_TR_GAP_SIZE                ,
    SEGY_TR_OVER_TRAVEL             ,
    SEGY_TR_CDP_X                   ,
    SEGY_TR_CDP_Y                   ,
    SEGY_TR_INLINE                  ,
    SEGY_TR_CROSSLINE               ,
    SEGY_TR_SHOT_POINT              ,
    SEGY_TR_SHOT_POINT_SCALAR       ,
    SEGY_TR_MEASURE_UNIT            ,
    SEGY_TR_TRANSDUCTION_MANT       ,
    SEGY_TR_TRANSDUCTION_EXP        ,
    SEGY_TR_TRANSDUCTION_UNIT       ,
    SEGY_TR_DEVICE_ID               ,
    SEGY_TR_SCALAR_TRACE_HEADER     ,
    SEGY_TR_SOURCE_TYPE             ,
    SEGY_TR_SOURCE_ENERGY_DIR_MANT  ,
    SEGY_TR_SOURCE_ENERGY_DIR_EXP   ,
    SEGY_TR_SOURCE_MEASURE_MANT     ,
    SEGY_TR_SOURCE_MEASURE_EXP      ,
    SEGY_TR_SOURCE_MEASURE_UNIT     ,
    SEGY_TR_UNASSIGNED1             ,
    SEGY_TR_UNASSIGNED2
};

static const char* su[91] = {
    "tracl"   ,
    "tracr"   ,
    "fldr"    ,
    "tracf"   ,
    "ep"      ,
    "cdp"     ,
    "cdpt"    ,
    "trid"    ,
    "nvs"     ,
    "nhs"     ,
    "duse"    ,
    "offset"  ,
    "gelev"   ,
    "selev"   ,
    "sdepth"  ,
    "gdel"    ,
    "sdel"    ,
    "swdep"   ,
    "gwdep"   ,
    "scalel"  ,
    "scalco"  ,
    "sx"      ,
    "sy"      ,
    "gx"      ,
    "gy"      ,
    "counit"  ,
    "wevel"   ,
    "swevel"  ,
    "sut"     ,
    "gut"     ,
    "sstat"   ,
    "gstat"   ,
    "tstat"   ,
    "laga"    ,
    "lagb"    ,
    "delrt"   ,
    "muts"    ,
    "mute"    ,
    "ns"      ,
    "dt"      ,
    "gain"    ,
    "igc"     ,
    "igi"     ,
    "corr"    ,
    "sfs"     ,
    "sfe"     ,
    "slen"    ,
    "styp"    ,
    "stat"    ,
    "stae"    ,
    "tatyp"   ,
    "afilf"   ,
    "afils"   ,
    "nofilf"  ,
    "nofils"  ,
    "lcf"     ,
    "hcf"     ,
    "lcs"     ,
    "hcs"     ,
    "year"    ,
    "day"     ,
    "hour"    ,
    "minute"  ,
    "sec"     ,
    "timbas"  ,
    "trwf"    ,
    "grnors"  ,
    "grnofr"  ,
    "grnlof"  ,
    "gaps"    ,
    "otrav"   ,
    "cdpx"    ,
    "cdpy"    ,
    "iline"   ,
    "xline"   ,
    "sp"      ,
    "scalsp"  ,
    "trunit"  ,
    "tdcm"    ,
    "tdcp"    ,
    "tdunit"  ,
    "triden"  ,
    "sctrh"   ,
    "stype"   ,
    "sedm"    ,
    "sede"    ,
    "smm"     ,
    "sme"     ,
    "smunit"  ,
    "uint1"   ,
    "uint2"
};

static const char* segynames[91] = {
   "SEQ_LINE"            ,
   "SEQ_FILE"            ,
   "FIELD_RECORD"        ,
   "NUMBER_ORIG_FIELD"   ,
   "ENERGY_SOURCE_POINT" ,
   "ENSEMBLE"            ,
   "NUM_IN_ENSEMBLE"     ,
   "TRACE_ID"            ,
   "SUMMED_TRACES"       ,
   "STACKED_TRACES"      ,
   "DATA_USE"            ,
   "OFFSET"              ,
   "RECV_GROUP_ELEV"     ,
   "SOURCE_SURF_ELEV"    ,
   "SOURCE_DEPTH"        ,
   "RECV_DATUM_ELEV"     ,
   "SOURCE_DATUM_ELEV"   ,
   "SOURCE_WATER_DEPTH"  ,
   "GROUP_WATER_DEPTH"   ,
   "ELEV_SCALAR"         ,
   "SOURCE_GROUP_SCALAR" ,
   "SOURCE_X"            ,
   "SOURCE_Y"            ,
   "GROUP_X"             ,
   "GROUP_Y"             ,
   "COORD_UNITS"         ,
   "WEATHERING_VELO"     ,
   "SUBWEATHERING_VELO"  ,
   "SOURCE_UPHOLE_TIME"  ,
   "GROUP_UPHOLE_TIME"   ,
   "SOURCE_STATIC_CORR"  ,
   "GROUP_STATIC_CORR"   ,
   "TOT_STATIC_APPLIED"  ,
   "LAG_A"               ,
   "LAG_B"               ,
   "DELAY_REC_TIME"      ,
   "MUTE_TIME_START"     ,
   "MUTE_TIME_END"       ,
   "SAMPLE_COUNT"        ,
   "SAMPLE_INTER"        ,
   "GAIN_TYPE"           ,
   "INSTR_GAIN_CONST"    ,
   "INSTR_INIT_GAIN"     ,
   "CORRELATED"          ,
   "SWEEP_FREQ_START"    ,
   "SWEEP_FREQ_END"      ,
   "SWEEP_LENGTH"        ,
   "SWEEP_TYPE"          ,
   "SWEEP_TAPERLEN_START",
   "SWEEP_TAPERLEN_END"  ,
   "TAPER_TYPE"          ,
   "ALIAS_FILT_FREQ"     ,
   "ALIAS_FILT_SLOPE"    ,
   "NOTCH_FILT_FREQ"     ,
   "NOTCH_FILT_SLOPE"    ,
   "LOW_CUT_FREQ"        ,
   "HIGH_CUT_FREQ"       ,
   "LOW_CUT_SLOPE"       ,
   "HIGH_CUT_SLOPE"      ,
   "YEAR_DATA_REC"       ,
   "DAY_OF_YEAR"         ,
   "HOUR_OF_DAY"         ,
   "MIN_OF_HOUR"         ,
   "SEC_OF_MIN"          ,
   "TIME_BASE_CODE"      ,
   "WEIGHTING_FAC"       ,
   "GEOPHONE_GROUP_ROLL1",
   "GEOPHONE_GROUP_FIRST",
   "GEOPHONE_GROUP_LAST" ,
   "GAP_SIZE"            ,
   "OVER_TRAVEL"         ,
   "CDP_X"               ,
   "CDP_Y"               ,
   "INLINE"              ,
   "CROSSLINE"           ,
   "SHOT_POINT"          ,
   "SHOT_POINT_SCALAR"   ,
   "MEASURE_UNIT"        ,
   "TRANSDUCTION_MANT"   ,
   "TRANSDUCTION_EXP"    ,
   "TRANSDUCTION_UNIT"   ,
   "DEVICE_ID"           ,
   "SCALAR_TRACE_HEADER" ,
   "SOURCE_TYPE"         ,
   "SOURCE_ENERGY_DIR_MA",
   "SOURCE_ENERGY_DIR_EX",
   "SOURCE_MEASURE_MANT" ,
   "SOURCE_MEASURE_EXP"  ,
   "SOURCE_MEASURE_UNIT" ,
   "UNASSIGNED1"         ,
   "UNASSIGNED2"
};

static int help() {
    puts( "Usage: segyio-catr [OPTION]... FILE\n"
          "Print specific trace headers from FILE\n"
          "\n"
          "-t,  --trace=NUMBER          trace to print\n"
          "-r,  --range START STOP STEP range of traces to print\n"
          "-s,  --strict                fail on unreadable tracefields\n"
          "-S,  --non-strict            don't fail on unreadable tracefields\n"
          "                             this is the default behaviour\n"
          "-n,  --segyio-names          print with segyio tracefield names\n"
          "-v,  --verbose               increase verbosity\n"
          "     --version               output version information and exit\n"
          "     --help                  display this help and exit\n"
          "\n"

          "the -r flag can takes up to three values: start, stop, step\n"
          "where all values are defaulted to zero\n"
          "flags -r and -t can be called multiple times\n"
        );
    return 0;
}

typedef struct { int start, stop, step; } range;

struct options {
    char* src;
    range* r;
    int rsize;
    int verbosity;
    int version, help;
    int strict, labels;
    const char* errmsg;
};

enum { su_labels, segyio_labels } label_ids;

static range fill_range( range r, int field, int val ) {
    switch( field ) {
        case 0: r.start = val; return r;
        case 1: r.stop = val; return r;
        case 2: r.step = val; return r;
        default:
            exit( 12 );
    }
}

static struct options parse_options( int argc, char** argv ){
    int rallocsize = 32;

    struct options opts;
    opts.rsize = 0; opts.r = calloc( sizeof( range ), rallocsize );
    opts.verbosity = 0;
    opts.version = 0; opts.help = 0;
    opts.strict = 0; opts.labels = su_labels;
    opts.errmsg = NULL;

    static struct option long_options[] = {
        { "trace",          required_argument,  0, 't' },
        { "range",          required_argument,  0, 'r' },
        { "segyio",         no_argument,        0, 'k' },
        { "strict",         no_argument,        0, 's' },
        { "non-strict",     no_argument,        0, 'S' },
        { "verbose",        no_argument,        0, 'v' },
        { "version",        no_argument,        0, 'V' },
        { "help",           no_argument,        0, 'h' },
        { 0, 0, 0, 0,}
    };

    static const char* parsenum_errmsg[] = { "",
                         "num must be an integer",
                         "num must be non-negative" };
    opterr = 1;
    while( true ) {
       int option_index = 0;
       int c = getopt_long( argc, argv, "sSkvt:r:",
                            long_options, &option_index );

       if( c == -1 ) break;

       int ret;
       switch( c ) {
           case 0: break;
           case 'h': opts.help = 1;     return opts;
           case 'V': opts.version = 1;  return opts;
           case 'v': ++opts.verbosity; break;
           case 's': opts.strict = 1; break;
           case 'S': opts.strict = 0; break;
           case 'k': opts.labels = segyio_labels; break;
           case 'r': // intentional fallthrough
           case 't':
                if( opts.rsize == rallocsize ) {
                    rallocsize *= 2;
                    range* re = realloc( opts.r, rallocsize*sizeof( range ) );
                    if( !re ) {
                        opts.errmsg = "Unable to reallocate memory";
                        return opts;
                    }
                    opts.r = re;
                }

                range* r = opts.r + opts.rsize;
                r->start = r->stop = r->step = 0;

                if( c == 't' ) {
                    ret = parseint( optarg, &r->start );

                    if( ret ) {
                        opts.errmsg = parsenum_errmsg[ ret ];
                        return opts;
                    }

                    goto done;
                }

               ret = sscanf(optarg," %d %d %d", &r->start,
                                                &r->stop,
                                                &r->step );
               if( ret && ( r->start < 0 || r->stop < 0 || r->step < 0 ) ) {
                    opts.errmsg = "range parameters must be positive";
                    return opts;
               }

               // if sscanf found something it consumes 1 argument, and we
               // won't have to rewind optind
               if( !ret ) optind--;

               for( int pos = ret; optind < argc && pos < 3; pos++, optind++ ) {
                    int val;
                    ret = parseint( argv[ optind ], &val );
                    /* parseint returns 1 when the string contains more than an
                     * int (or not an int at all) we assume that the remaining
                     * range parameters were defaulted and give control back to
                     * argument parsing
                     *
                     * such invocations include:
                     *   segyio-catr -r 1 foo.sgy
                     *   segyio-catr -r 1 2 -s foo.sgy
                     */
                    if( ret == 0 ) *r = fill_range( *r, pos, val );
                    if( ret == 1 )  goto done;
                    if( ret == 2 ) {
                        opts.errmsg = parsenum_errmsg[ ret ];
                        return opts;
                    }
                }

                done: ++opts.rsize; break;

           default: opts.help = 1; opts.errmsg = ""; return opts;
       }
    }

    if( argc - optind != 1 ) {
        errmsg( 0, "Wrong number of files" );
        opts.errmsg = "";
        return opts;
    }
    opts.src = argv[ optind ];

    if( opts.rsize == 0 ) opts.rsize = 1;

    return opts;
}

int main( int argc, char** argv ) {
    struct options opts = parse_options( argc, argv );

    if( opts.help ) return help();
    if( opts.version ) return printversion( "segyio-catr" );
    if( opts.errmsg ) return errmsg( EINVAL, opts.errmsg );

    int strict = opts.strict;

    const char** labels;
    switch( opts.labels ) {
        case su_labels: labels = su; break;
        case segyio_labels: labels = segynames; break;
        default: labels = su; break;
    }

    for( range* r = opts.r; r < opts.r + opts.rsize; ++r ) {
        if( r->start > r->stop && strict )
            return errmsg( -3, "Range is empty" );
    }

    char trheader[ TRHSIZE ];
    char binheader[ BINSIZE ];

    segy_file* src = segy_open( opts.src, "r" );
    if( !src )
        return errmsg2( errno, "Unable to open src", strerror( errno ) );

    int err = segy_binheader( src, binheader );
    if( err )
        return errmsg( errno, "Unable to read binheader" );

    int samnr = segy_samples( binheader );
    int trace_bsize = segy_trace_bsize( samnr );
    long trace0 = segy_trace0( binheader );

    int numtrh;
    err = segy_traces( src, &numtrh, trace0, trace_bsize );
    if( err )
        return errmsg( errno, "Unable to determine number of traces in file" );

    for( range* r = opts.r; r < opts.r + opts.rsize; ++r ) {
        if( r->stop == 0 ) r->stop = r->start;
        if( r->step == 0 ) r->step = 1;
    }

    for( range* r = opts.r; r < opts.r + opts.rsize; ++r ) {
        for( int i = r->start; i <= r->stop; i += r->step ) {
            if( i > numtrh && strict )
              return errmsg2( errno, "Unable to read traceheader",
                                     "out of range" );
            if( i > numtrh ) break;

            err = segy_traceheader( src, i - 1, trheader, trace0, trace_bsize );
            if( err )
                return errmsg( errno, "Unable to read trace header" );

            for( int j = 0; j < 91; j++ ) {
                int f;
                segy_get_field( trheader, fields[j], &f );
                printf( "%s\t%d\n", labels[j], f );
            }
        }
    }
    free( opts.r );
    segy_close( src );

    return 0;
}

