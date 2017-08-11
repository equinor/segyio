#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <unistd.h>

#include <segyio/segy.h>

static const int fields[91] = {
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

char su[][91] = {
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

static int help() {
    puts( "Usage: segyio-r [OPTION]... SRC\n"
          "Print specific traceheaders from SRC\n"
	      "\n"
          "-t, --traceheader=NUMBER                         Single trace header\n"
          "-i, --traceheader-int-beg=NUMBER                 First trace header in interval\n"
          "-I, --traceheader-int-end=NUMBER                 Last trace header in interval (inclusive)\n"
          "-v, --verbose             		         Increase verbosity\n"
          "    --version             		         Output version information and exit\n"
          "    --help                		         Display this help and exit\n"
          "\n"
          "The < -t >tag can be set multiple times and \n"
          "can also be used in combination with the interval < -i > < -I > tags.\n"
          /* ADD posibility of not spesifying both -i and -I */
        );
    return 0;
}

static int errmsg( int errcode, const char* msg ) {
    if( !msg ) return errcode;

    fputs( msg,  stderr );
    fputc( '\n', stderr );
    return errcode;
}

static int errmsg2( int errcode, const char* prelude, const char* msg) {
   if( !prelude ) return errmsg( errcode, msg);

   fputs( prelude, stderr );
   fputc( ':', stderr );
   fputc( ' ', stderr );
   return errmsg( errcode, msg );
}

static int printversion() {
    printf( "Segyio-r (segyio version %d.%d)\n", segyio_MAJOR, segyio_MINOR);
    return 0;
}

static void print_header( const char* header ) {
    for( int bch = 0; bch < 241; ++bch ) {
        putchar( header[ bch ] );
    }
}

static int parseint( const char* str, int* x ) {
    char* endptr;
     *x = strtol( str, &endptr, 10);

    if( *endptr != '\0' ) return 1;
    if( *x < 0 ) return 2;
    return 0;
}

static int samp_size( char const* bheader, int* samsize ) {
    int code;
    int err = segy_get_bfield( bheader, SEGY_BIN_FORMAT, &code );
    if( err != 0 )
        return -1;

    switch( code ){
        case 3:
            *samsize = 2; break;
        case 1:
        case 2:
        case 4:
            *samsize = 4; break;

        default:
            return -1;
    }
    return 0;
}

static int check_args( int single, int start, int end, int index, int traces ){
    if( start != INT_MAX && end == -1 )
        end = traces;

    if( start == INT_MAX && end != -1 )
        start = 0;

    if( start != INT_MAX && end != -1 ) {
        if( index >= start && index <= end )
            return 0;
    }


    if( index == single ) {
        if( start == INT_MAX && end == -1 )
            return 0;
        if( start != INT_MAX || end != -1) {
            if( start > single < end)
                return 0;
        }
    }
    return 1;
}

static int argument_range( int single, int start, int end,  int num ) {
    if( single >= num || end >= num )
        return 1;

    if( start >= num && start != INT_MAX )
        return 1;

    return 0;
}

#define TRHSIZE SEGY_TRACE_HEADER_SIZE
#define BINSIZE SEGY_BINARY_HEADER_SIZE

int main( int argc, char** argv) {

    if( argc < 2 ) {
        return help();
    }

    static int version = 0;
    static int verbosity = 0;

    static struct option long_options[] = {
	{ "traceheader", required_argument, 0, 't' },
	{ "traceheader-int-beg", required_argument, 0, 'i' },
	{ "traceheader-int-end", required_argument, 0, 'I' },
	{ "verbose", no_argument, 0, 'v' },
	{ "version", no_argument, &version, 1 },
	{ "help", no_argument, 0, 'h' },
	{ 0, 0, 0, 0,}
    };

    int trhno = -1;
    int trhbeg = INT_MAX;
    int trhend = -1;

    static const char* parsenum_errmsg[] = { "", "num must be an integer",
						 "num must be non-negative" };
    while( true ) {
	    int option_index = 0;
	    int c = getopt_long( argc, argv, "t:i:I:",
		  	     long_options, &option_index );

	    if( c == -1 ) break;

	    int ret;
	    switch( c ) {
	        case 0: break;
	        case 'h': return help();
	        case 'v': verbosity++; break;
	        case 'i':
	        	ret = parseint( optarg, &trhbeg );
		        if( ret == 0 ) break;
		        return errmsg( EINVAL, 	parsenum_errmsg[ ret ] );
	        case 'I':
	        	ret = parseint( optarg, &trhend );
		        if( ret == 0 ) break;
		        return errmsg( EINVAL, 	parsenum_errmsg[ ret ] );
	        case 't':
		        ret = parseint( optarg, &trhno );
		        if( ret == 0 ) break;
		        return errmsg( EINVAL, 	parsenum_errmsg[ ret ] );
	        default: return help();
	    }
    }

    if( version ) return printversion();

    char trheader[ TRHSIZE ];
    char binheader[ BINSIZE ];

    for( int i = optind; i < argc; ++i ) {
        segy_file* src = segy_open( argv[ i ], "r" );

        if( !src )
            return errmsg2( errno, "Unable to open src", strerror( errno ) );

        if( argc == 2 ) {
            return help();
        }

        int err = segy_binheader(src, binheader);
        if(err != 0)
            return errmsg(errno, "Unable to read binheader");

        int samnr;
        err = segy_get_bfield(binheader, SEGY_BIN_SAMPLES, &samnr);
        if(err != 0)
            return errmsg(errno, "Unable to read binheader field: Samples");

        int samsize;
        err = samp_size(binheader, &samsize);
        if( err != 0 )
            return errmsg(errno, "Unable to read binheader field: Format ");

        long trace0 = segy_trace0( binheader );

        int numtrh;
        err = segy_get_bfield( binheader, SEGY_BIN_TRACES, &numtrh );
        if( err != 0 )
            return errmsg(errno, "Unable to determine number of traces in file");

        err = argument_range( trhno, trhbeg, trhend, numtrh );
            if( err != 0 )
                return errmsg(errno, "Argument out of range");
        for(int i = 0; i < numtrh; i++){
            err = segy_traceheader( src, i, trheader, trace0, samnr*samsize);
            if( err != 0)
                return errmsg(errno, "Unable to read trace header");

            int print_trh = check_args(trhno, trhbeg, trhend, i, numtrh);
            if( print_trh == 0 ){
                for( int i = 0; i < 91; i++ ) {
                    int f;
                    err = segy_get_field(trheader, fields[i], &f);
                    if(err != 0)
                        return errmsg(errno, "Unable to read trace field");
                    printf("%s\t%i\n", su[i], f);

                }
                printf("\n");
                //printf("Trace: %d\n", i);
            }
        }
        segy_close( src );
    }
    return 0;
}
