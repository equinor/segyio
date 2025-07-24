#define _POSIX_SOURCE /* fileno */

/* 64-bit off_t in fseeko/ftello */
#define _POSIX_C_SOURCE 200808L
#define _FILE_OFFSET_BITS 64

#if defined(_WIN32) || defined(_MSC_VER)
    /* MultiByteToWideChar */
    #include <windows.h>
#endif

#ifdef HAVE_MMAP
  #define _POSIX_SOURCE
  #include <sys/mman.h>
#endif //HAVE_MMAP

#ifdef HAVE_SYS_STAT_H
  #include <sys/types.h>
  #include <sys/stat.h>
#endif //HAVE_SYS_STAT_H

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <segyio/segy.h>

static const unsigned char a2e[256] = {
    0,  1,  2,  3,  55, 45, 46, 47, 22, 5,  37, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 60, 61, 50, 38, 24, 25, 63, 39, 28, 29, 30, 31,
    64, 79, 127,123,91, 108,80, 125,77, 93, 92, 78, 107,96, 75, 97,
    240,241,242,243,244,245,246,247,248,249,122,94, 76, 126,110,111,
    124,193,194,195,196,197,198,199,200,201,209,210,211,212,213,214,
    215,216,217,226,227,228,229,230,231,232,233,74, 224,90, 95, 109,
    121,129,130,131,132,133,134,135,136,137,145,146,147,148,149,150,
    151,152,153,162,163,164,165,166,167,168,169,192,106,208,161,7,
    32, 33, 34, 35, 36, 21, 6,  23, 40, 41, 42, 43, 44, 9,  10, 27,
    48, 49, 26, 51, 52, 53, 54, 8,  56, 57, 58, 59, 4,  20, 62, 225,
    65, 66, 67, 68, 69, 70, 71, 72, 73, 81, 82, 83, 84, 85, 86, 87,
    88, 89, 98, 99, 100,101,102,103,104,105,112,113,114,115,116,117,
    118,119,120,128,138,139,140,141,142,143,144,154,155,156,157,158,
    159,160,170,171,172,173,174,175,176,177,178,179,180,181,182,183,
    184,185,186,187,188,189,190,191,202,203,204,205,206,207,218,219,
    220,221,222,223,234,235,236,237,238,239,250,251,252,253,254,255
};

static const unsigned char e2a[256] = {
    0,  1,  2,  3,  156,9,  134,127,151,141,142, 11,12, 13, 14, 15,
    16, 17, 18, 19, 157,133,8,  135,24, 25, 146,143,28, 29, 30, 31,
    128,129,130,131,132,10, 23, 27, 136,137,138,139,140,5,  6,  7,
    144,145,22, 147,148,149,150,4,  152,153,154,155,20, 21, 158,26,
    32, 160,161,162,163,164,165,166,167,168,91, 46, 60, 40, 43, 33,
    38, 169,170,171,172,173,174,175,176,177,93, 36, 42, 41, 59, 94,
    45, 47, 178,179,180,181,182,183,184,185,124,44, 37, 95, 62, 63,
    186,187,188,189,190,191,192,193,194,96, 58, 35, 64, 39, 61, 34,
    195,97, 98, 99, 100,101,102,103,104,105,196,197,198,199,200,201,
    202,106,107,108,109,110,111,112,113,114,203,204,205,206,207,208,
    209,126,115,116,117,118,119,120,121,122,210,211,212,213,214,215,
    216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,
    123,65, 66, 67, 68, 69, 70, 71, 72, 73, 232,233,234,235,236,237,
    125,74, 75, 76, 77, 78, 79, 80, 81, 82, 238,239,240,241,242,243,
    92, 159,83, 84, 85, 86, 87, 88, 89, 90, 244,245,246,247,248,249,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 250,251,252,253,254,255
};

static int encode( char* dst,
                   const char* src,
                   const unsigned char* conv,
                   size_t n ) {
    for( size_t i = 0; i < n; ++i )
        dst[ i ] = (char)conv[ (unsigned char) src[ i ] ];

    return SEGY_OK;
}

#ifdef HOST_BIG_ENDIAN
    #define HOST_LSB 0
    #define HOST_MSB 1
#else
    #define HOST_LSB 1
    #define HOST_MSB 0
#endif

#if defined __GNUC__
    #define bswap64(x) __builtin_bswap64((x))
    #define bswap32(x) __builtin_bswap32((x))
    #define bswap16(x) __builtin_bswap16((x))
#else
    #define bswap64(v) ( ((v & 0x00000000000000FFull) << 56) \
                       | ((v & 0x000000000000FF00ull) << 40) \
                       | ((v & 0x0000000000FF0000ull) << 24) \
                       | ((v & 0x00000000FF000000ull) <<  8) \
                       | ((v & 0x000000FF00000000ull) >>  8) \
                       | ((v & 0x0000FF0000000000ull) >> 24) \
                       | ((v & 0x00FF000000000000ull) >> 40) \
                       | ((v & 0xFF00000000000000ull) >> 56) \
                       )

    #define bswap32(v) ( (((v) & 0x000000FF) << 24) \
                       | (((v) & 0x0000FF00) <<  8) \
                       | (((v) & 0x00FF0000) >>  8) \
                       | (((v) & 0xFF000000) >> 24) \
                       )

    #define bswap16(v) ( (((v) & 0x00FF) << 8) \
                       | (((v) & 0xFF00) >> 8) \
                       )
#endif // __GNUC__

static uint16_t htobe16( uint16_t v ) {
#if HOST_LSB
    return bswap16(v);
#else
    return v;
#endif
}

static uint32_t htobe32( uint32_t v ) {
#if HOST_LSB
    return bswap32(v);
#else
    return v;
#endif
}

static uint64_t htobe64( uint64_t v) {
#if HOST_LSB
    return bswap64(v);
#else
    return v;
#endif
}

static uint16_t be16toh( uint16_t v ) {
#if HOST_LSB
    return bswap16(v);
#else
    return v;
#endif
}

static uint32_t be32toh( uint32_t v ) {
#if HOST_LSB
    return bswap32(v);
#else
    return v;
#endif
}

static uint64_t be64toh( uint64_t v ) {
#if HOST_LSB
    return bswap64(v);
#else
    return v;
#endif
}

#define IEEEMAX 0x7FFFFFFF
#define IEMAXIB 0x611FFFFF
#define IEMINIB 0x21200000

static inline void ibm_native( void* buf ) {
    static const int it[8] = { 0x21800000, 0x21400000, 0x21000000, 0x21000000,
                               0x20c00000, 0x20c00000, 0x20c00000, 0x20c00000 };
    static const int mt[8] = { 8, 4, 2, 2, 1, 1, 1, 1 };
    unsigned int manthi, iexp, inabs;
    int ix;
    uint32_t u;

    memcpy( &u, buf, sizeof( u ) );

    manthi = u & 0x00ffffff;
    ix     = manthi >> 21;
    iexp   = ( ( u & 0x7f000000 ) - it[ix] ) << 1;
    manthi = manthi * mt[ix] + iexp;
    inabs  = u & 0x7fffffff;
    if ( inabs > IEMAXIB ) manthi = IEEEMAX;
    manthi = manthi | ( u & 0x80000000 );
    u = ( inabs < IEMINIB ) ? 0 : manthi;
    memcpy( buf, &u, sizeof( u ) );
}

static inline void native_ibm( void* buf ) {
    static const int it[4] = { 0x21200000, 0x21400000, 0x21800000, 0x22100000 };
    static const int mt[4] = { 2, 4, 8, 1 };
    unsigned int manthi, iexp, ix;
    uint32_t u;

    memcpy( &u, buf, sizeof( u ) );

    ix     = ( u & 0x01800000 ) >> 23;
    iexp   = ( ( u & 0x7e000000 ) >> 1 ) + it[ix];
    manthi = ( mt[ix] * ( u & 0x007fffff) ) >> 3;
    manthi = ( manthi + iexp ) | ( u & 0x80000000 );
    u      = ( u & 0x7fffffff ) ? manthi : 0;
    memcpy( buf, &u, sizeof( u ) );
}

static void bswap64_mem( char* dst, const char* src ) {
    uint64_t v;
    memcpy( &v, src, 8 );
    v = bswap64( v );
    memcpy( dst, &v, 8 );
}
static void bswap32_mem( char* dst, const char* src ) {
    uint32_t v;
    memcpy( &v, src, 4 );
    v = bswap32( v );
    memcpy( dst, &v, 4 );
}

static void bswap16_mem( char* dst, const char* src ) {
    uint16_t v;
    memcpy( &v, src, 2 );
    v = bswap16( v );
    memcpy( dst, &v, 2 );
}


/*
    Lookup table for segy field data types. Types are defined in the SEGY_FORMAT enum.
    All values not explicitly set are 0 which is undefined in the SEGY_FORMAT.
*/
static uint8_t tr_field_type[SEGY_TRACE_HEADER_SIZE] = {
    [SEGY_TR_SEQ_LINE               ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_SEQ_FILE               ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_FIELD_RECORD           ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_NUMBER_ORIG_FIELD      ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_ENERGY_SOURCE_POINT    ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_ENSEMBLE               ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_NUM_IN_ENSEMBLE        ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_TRACE_ID               ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SUMMED_TRACES          ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_STACKED_TRACES         ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_DATA_USE               ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_OFFSET                 ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_RECV_GROUP_ELEV        ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_SOURCE_SURF_ELEV       ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_SOURCE_DEPTH           ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_RECV_DATUM_ELEV        ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_SOURCE_DATUM_ELEV      ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_SOURCE_WATER_DEPTH     ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_GROUP_WATER_DEPTH      ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_ELEV_SCALAR            ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SOURCE_GROUP_SCALAR    ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SOURCE_X               ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_SOURCE_Y               ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_GROUP_X                ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_GROUP_Y                ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_COORD_UNITS            ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_WEATHERING_VELO        ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SUBWEATHERING_VELO     ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SOURCE_UPHOLE_TIME     ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_GROUP_UPHOLE_TIME      ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SOURCE_STATIC_CORR     ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_GROUP_STATIC_CORR      ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_TOT_STATIC_APPLIED     ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_LAG_A                  ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_LAG_B                  ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_DELAY_REC_TIME         ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_MUTE_TIME_START        ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_MUTE_TIME_END          ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SAMPLE_COUNT           ] = SEGY_UNSIGNED_SHORT_2_BYTE,
    [SEGY_TR_SAMPLE_INTER           ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_GAIN_TYPE              ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_INSTR_GAIN_CONST       ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_INSTR_INIT_GAIN        ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_CORRELATED             ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SWEEP_FREQ_START       ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SWEEP_FREQ_END         ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SWEEP_LENGTH           ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SWEEP_TYPE             ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SWEEP_TAPERLEN_START   ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SWEEP_TAPERLEN_END     ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_TAPER_TYPE             ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_ALIAS_FILT_FREQ        ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_ALIAS_FILT_SLOPE       ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_NOTCH_FILT_FREQ        ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_NOTCH_FILT_SLOPE       ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_LOW_CUT_FREQ           ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_HIGH_CUT_FREQ          ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_LOW_CUT_SLOPE          ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_HIGH_CUT_SLOPE         ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_YEAR_DATA_REC          ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_DAY_OF_YEAR            ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_HOUR_OF_DAY            ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_MIN_OF_HOUR            ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SEC_OF_MIN             ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_TIME_BASE_CODE         ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_WEIGHTING_FAC          ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_GEOPHONE_GROUP_ROLL1   ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_GEOPHONE_GROUP_FIRST   ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_GEOPHONE_GROUP_LAST    ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_GAP_SIZE               ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_OVER_TRAVEL            ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_CDP_X                  ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_CDP_Y                  ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_INLINE                 ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_CROSSLINE              ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_SHOT_POINT             ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_SHOT_POINT_SCALAR      ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_MEASURE_UNIT           ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_TRANSDUCTION_MANT      ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_TRANSDUCTION_EXP       ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_TRANSDUCTION_UNIT      ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_DEVICE_ID              ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SCALAR_TRACE_HEADER    ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SOURCE_TYPE            ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SOURCE_ENERGY_DIR_VERT ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SOURCE_ENERGY_DIR_XLINE] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SOURCE_ENERGY_DIR_ILINE] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SOURCE_MEASURE_MANT    ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_SOURCE_MEASURE_EXP     ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_SOURCE_MEASURE_UNIT    ] = SEGY_SIGNED_SHORT_2_BYTE,
    [SEGY_TR_UNASSIGNED1            ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [SEGY_TR_UNASSIGNED2            ] = SEGY_SIGNED_INTEGER_4_BYTE,
};


#define HEADER_SIZE SEGY_TEXT_HEADER_SIZE

/*
    Lookup table for segy binary field data types. Types are defined in the SEGY_FORMAT enum.
    All values not explicitly set are 0 which is undefined in the SEGY_FORMAT.
*/
static uint8_t bin_field_type[SEGY_BINARY_HEADER_SIZE] = {
    [- HEADER_SIZE + SEGY_BIN_JOB_ID                    ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [- HEADER_SIZE + SEGY_BIN_LINE_NUMBER               ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [- HEADER_SIZE + SEGY_BIN_REEL_NUMBER               ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [- HEADER_SIZE + SEGY_BIN_ENSEMBLE_TRACES           ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_AUX_ENSEMBLE_TRACES       ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_INTERVAL                  ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_INTERVAL_ORIG             ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_SAMPLES                   ] = SEGY_UNSIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_SAMPLES_ORIG              ] = SEGY_UNSIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_FORMAT                    ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_ENSEMBLE_FOLD             ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_SORTING_CODE              ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_VERTICAL_SUM              ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_SWEEP_FREQ_START          ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_SWEEP_FREQ_END            ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_SWEEP_LENGTH              ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_SWEEP                     ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_SWEEP_CHANNEL             ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_SWEEP_TAPER_START         ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_SWEEP_TAPER_END           ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_TAPER                     ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_CORRELATED_TRACES         ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_BIN_GAIN_RECOVERY         ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_AMPLITUDE_RECOVERY        ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_MEASUREMENT_SYSTEM        ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_IMPULSE_POLARITY          ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_VIBRATORY_POLARITY        ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_EXT_ENSEMBLE_TRACES       ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [- HEADER_SIZE + SEGY_BIN_EXT_AUX_ENSEMBLE_TRACES   ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [- HEADER_SIZE + SEGY_BIN_EXT_SAMPLES               ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [- HEADER_SIZE + SEGY_BIN_EXT_INTERVAL              ] = SEGY_IEEE_FLOAT_8_BYTE,
    [- HEADER_SIZE + SEGY_BIN_EXT_INTERVAL_ORIG         ] = SEGY_IEEE_FLOAT_8_BYTE,
    [- HEADER_SIZE + SEGY_BIN_EXT_SAMPLES_ORIG          ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [- HEADER_SIZE + SEGY_BIN_EXT_ENSEMBLE_FOLD         ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [- HEADER_SIZE + SEGY_BIN_INTEGER_CONSTANT          ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [- HEADER_SIZE + SEGY_BIN_UNASSIGNED1               ] = SEGY_NOT_IN_USE_1,
    [- HEADER_SIZE + SEGY_BIN_SEGY_REVISION             ] = SEGY_UNSIGNED_CHAR_1_BYTE,
    [- HEADER_SIZE + SEGY_BIN_SEGY_REVISION_MINOR       ] = SEGY_UNSIGNED_CHAR_1_BYTE,
    [- HEADER_SIZE + SEGY_BIN_TRACE_FLAG                ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_EXT_HEADERS               ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_MAX_ADDITIONAL_TR_HEADERS ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_SURVEY_TYPE               ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_TIME_BASIS_CODE           ] = SEGY_SIGNED_SHORT_2_BYTE,
    [- HEADER_SIZE + SEGY_BIN_NR_TRACES_IN_STREAM       ] = SEGY_UNSIGNED_INTEGER_8_BYTE,
    [- HEADER_SIZE + SEGY_BIN_FIRST_TRACE_OFFSET        ] = SEGY_UNSIGNED_INTEGER_8_BYTE,
    [- HEADER_SIZE + SEGY_BIN_NR_TRAILER_RECORDS        ] = SEGY_SIGNED_INTEGER_4_BYTE,
    [- HEADER_SIZE + SEGY_BIN_UNASSIGNED2               ] = SEGY_NOT_IN_USE_1,
};


/*
 * Determine the file size in bytes. If this function succeeds, the file
 * pointer will be reset to wherever it was before this call. If this call
 * fails for some reason, the return value is 0 and the file pointer location
 * will be determined by the behaviour of fseek.
 *
 * sys/stat.h is POSIX, but is well enough supported by Windows. The long long
 * data type is required to support files >4G (as long only guarantees 32 bits).
 */
#ifdef HAVE_SYS_STAT_H
static int file_size( FILE* fp, long long* size ) {

    /*
     * the file size will be unaccurate unless userland buffers are flushed if
     * the file is new or appended to
     */
    if( fflush( fp ) != 0 ) return SEGY_FWRITE_ERROR;
#ifdef HAVE_FSTATI64
    // this means we're on windows where fstat is unreliable for filesizes >2G
    // because long is only 4 bytes
    struct _stati64 st;
    const int err = _fstati64( _fileno( fp ), &st );
#else
    struct stat st;
    const int err = fstat( fileno( fp ), &st );
#endif

    if( err != 0 ) return SEGY_FSEEK_ERROR;
    *size = st.st_size;
    return SEGY_OK;
}
#endif //HAVE_SYS_STAT_H

static int fileread( segy_datasource* self, void* buffer, size_t size ) {
    FILE* file = (FILE*)self->stream;
    size_t readc = fread( buffer, size, 1, file );
    if ( readc != 1 ) return SEGY_FREAD_ERROR;
    return SEGY_OK;
}

static int filewrite( segy_datasource* self, const void* buffer, size_t size ) {
    if ( !self->writable ) return SEGY_READONLY;
    FILE* file = (FILE*)self->stream;
    size_t writec = fwrite( buffer, size, 1, file );
    if ( writec != 1 ) return SEGY_FWRITE_ERROR;
    return SEGY_OK;
}

static int fileseek( segy_datasource* self, long long pos, int whence ) {
    FILE* file = (FILE*)self->stream;
    int err;
#if LONG_MAX == LLONG_MAX
    err = fseek( file, (long)pos, whence );
#elif HAVE_FSEEKO
    err = fseeko( file, pos, whence );
#elif HAVE_FSEEKI64
    err = _fseeki64( file, pos, whence );
#else
    error "no 64-bit alternative to fseek() found, and long is too small"
#endif
    if( err != 0 ) return SEGY_FSEEK_ERROR;
    return SEGY_OK;
}

static int filetell( segy_datasource* self, long long* pos ) {
    FILE* file = (FILE*)self->stream;
#if LONG_MAX == LLONG_MAX
    *pos = ftell( file );
#elif HAVE_FTELLO
    *pos = ftello( file );
#elif HAVE_FTELLI64
    *pos = _ftelli64( file );
#else
    error "no 64-bit alternative to ftell() found, and long is too small"
#endif
    return SEGY_OK;
}

static int filesize( segy_datasource* self, long long* out ) {
    FILE* file = (FILE*)self->stream;
    int err = file_size( file, out );
    return err;
}

static int fileflush( segy_datasource* self ) {
    // flush is a no-op for read-only files
    if ( !self->writable ) return SEGY_OK;
    FILE* file = (FILE*)self->stream;

    int flusherr = fflush( file );
    if ( flusherr != 0 ) return SEGY_FWRITE_ERROR;

    return SEGY_OK;
}

static int fileclose( segy_datasource* self ) {
    int err = fileflush( self );
    if ( err != SEGY_OK ) return err;

    FILE* file = (FILE*)self->stream;
    err = fclose( file );
    if ( err != 0 ) return SEGY_FWRITE_ERROR;
    return err;
}

/* Describes a file loaded into memory. */
struct memfile {
    unsigned char* addr;
    unsigned char* cur;
    size_t size;
};
typedef struct memfile memfile;

static int memread( segy_datasource* self, void* buffer, size_t size ) {
    memfile* mp = (memfile*)self->stream;
    const unsigned char* begin = mp->addr;
    const unsigned char* end = mp->addr + mp->size;
    const unsigned char* src = mp->cur;
    const unsigned char* srcend = src + size;

    if ( src < begin || src > end || srcend > end )
        return SEGY_FREAD_ERROR;

    memcpy( buffer, src, size );
    mp->cur = mp->cur + size;
    return SEGY_OK;
}

/* Writes to the memfile. Overwrites memory, does not shrink/extend fsize. */
static int memwrite( segy_datasource* self, const void* buffer, size_t size ) {
    memfile* mp = (memfile*)self->stream;
    const unsigned char* begin = mp->addr;
    const unsigned char* end = mp->addr + mp->size;
    unsigned char* dest = mp->cur;
    const unsigned char* destend = dest + size;

    if ( dest < begin || dest > end || destend > end )
        return SEGY_FWRITE_ERROR;

    memcpy( dest, buffer, size );
    mp->cur = mp->cur + size;
    return SEGY_OK;
}

static int memseek( segy_datasource* self, long long pos, int whence ) {
    memfile* mp = (memfile*)self->stream;
    /*
     * mmap fseek doesn't fail (it's just a pointer readjustment) and won't
     * set errno, in order to keep its behaviour consistent with fseek,
     * which can easily reposition itself past the end-of-file
     */
    switch ( whence ) {
        case SEEK_SET:
            mp->cur = mp->addr + pos;
            return SEGY_OK;
        case SEEK_CUR:
            mp->cur = mp->cur + pos;
            return SEGY_OK;
        case SEEK_END:
            mp->cur = mp->addr + mp->size + pos;
            return SEGY_OK;
        default:
            return SEGY_FSEEK_ERROR;
    }
}

static int memtell( segy_datasource* self, long long* pos ) {
    const memfile* mp = (memfile*)self->stream;
    *pos = mp->cur - mp->addr;
    return SEGY_OK;
}

static int memsize( segy_datasource* self, long long* out ) {
    const memfile* mp = (memfile*)self->stream;
    *out = mp->size;
    return SEGY_OK;
}

static int mmapflush( segy_datasource* self ) {
#ifdef HAVE_MMAP
    const memfile* mp = (memfile*)self->stream;
    int syncerr = msync( mp->addr, mp->size, MS_SYNC );
    if( syncerr != 0 ) return SEGY_MMAP_ERROR;
#endif // HAVE_MMAP
    return SEGY_OK;
}

static int memflush( segy_datasource* self ) {
    (void)self; // mark parameter as unused
    return SEGY_OK;
}

static int mmapclose( segy_datasource* self ) {
#ifdef HAVE_MMAP
    int err = mmapflush( self );
    if ( err != SEGY_OK ) return err;

    memfile* mp = (memfile*)self->stream;
    err = munmap( mp->addr, mp->size );
    free( mp );
    if ( err != 0 ) return SEGY_MMAP_ERROR;
#endif // HAVE_MMAP
    return SEGY_OK;
}

static int memclose( segy_datasource* self ) {
    memfile* mp = (memfile*)self->stream;
    free( mp );
    return SEGY_OK;
}

static int formatsize( int format ) {
    switch( format ) {
        case SEGY_IBM_FLOAT_4_BYTE:             return 4;
        case SEGY_SIGNED_INTEGER_4_BYTE:        return 4;
        case SEGY_SIGNED_INTEGER_8_BYTE:        return 8;
        case SEGY_SIGNED_SHORT_2_BYTE:          return 2;
        case SEGY_FIXED_POINT_WITH_GAIN_4_BYTE: return 4;
        case SEGY_IEEE_FLOAT_4_BYTE:            return 4;
        case SEGY_IEEE_FLOAT_8_BYTE:            return 8;
        case SEGY_SIGNED_CHAR_1_BYTE:           return 1;
        case SEGY_UNSIGNED_CHAR_1_BYTE:         return 1;
        case SEGY_UNSIGNED_INTEGER_4_BYTE:      return 4;
        case SEGY_UNSIGNED_SHORT_2_BYTE:        return 2;
        case SEGY_UNSIGNED_INTEGER_8_BYTE:      return 8;
        case SEGY_SIGNED_CHAR_3_BYTE:           return 3;
        case SEGY_UNSIGNED_INTEGER_3_BYTE:      return 3;
        case SEGY_NOT_IN_USE_1:
        case SEGY_NOT_IN_USE_2:
        default:
            return -1;
    }
}

#define MODEBUF_SIZE 5

segy_file* segy_open( const char* path, const char* mode ) {

    if( !path || !mode ) return NULL;

    // append a 'b' if it is not passed by the user; not a problem on unix, but
    // windows and other platforms fail without it
    char binary_mode[ MODEBUF_SIZE ] = { 0 };
    strncpy( binary_mode, mode, 3 );

    size_t mode_len = strlen( binary_mode );
    if( binary_mode[ mode_len - 1 ] != 'b' ) binary_mode[ mode_len ] = 'b';

     // Account for invalid mode. On unix this is fine, but windows crashes the
     // process if mode is invalid
    if( !strstr( "rb" "wb" "ab" "r+b" "w+b" "a+b", binary_mode ) )
        return NULL;

#ifdef _WIN32
    /*
     * fun with windows.
     * Reported in https://github.com/Statoil/segyio/issues/266
     *
     * Windows uses UTF-16 internally, also for path names, and fopen does not
     * handle UTF-16 strings on Windows. It's assumed input is UTF-8 or ascii ,
     * and a simple conversion to the UTF-16 (wchar_t) version with _wfopen
     * makes non-ascii paths work on Windows.
     */
    wchar_t wmode[ MODEBUF_SIZE ] = { 0 };
    mbstowcs( wmode, binary_mode, strlen( binary_mode ) );

    /*
     * call MultiByteToWideChar twice, once to figure out the size of the
     * resulting wchar string, and once to do the actual conversion
     */
    int wpathlen = MultiByteToWideChar( CP_UTF8, 0, path, -1, NULL, 0 );
    wchar_t* wpath = calloc( wpathlen + 1, sizeof( wchar_t ) );
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wpathlen );
    FILE* fp = _wfopen( wpath, wmode );
    free( wpath );
#else
    FILE* fp = fopen( path, binary_mode );
#endif

    if( !fp ) return NULL;

    segy_file* ds = malloc( sizeof( segy_file ) );

    if( !ds ) {
        fclose( fp );
        return NULL;
    }

    ds->stream = fp;

    ds->read = fileread;
    ds->write = filewrite;
    ds->seek = fileseek;
    ds->tell = filetell;
    ds->size = filesize;
    ds->flush = fileflush;
    ds->close = fileclose;
    ds->writable = strstr( binary_mode, "+" ) || strstr( binary_mode, "w" );

    // on init assume a size of 4-bytes-per-element and big-endian
    ds->elemsize = 4;
    ds->lsb = false;

    ds->minimize_requests_number = true;
    ds->memory_speedup = false;

    return ds;
}

segy_datasource* segy_memopen( unsigned char* addr, size_t size ) {
    if( !addr || !size ) return NULL;

    /* as we don't support operations that change file size on update, no memory
     * resize would be needed
     */
    memfile* mp = malloc( sizeof( memfile ) );
    if( !mp ) {
        return NULL;
    }
    mp->addr = addr;
    mp->cur = addr;
    mp->size = size;

    segy_datasource* ds = malloc( sizeof( segy_datasource ) );
    if( !ds ) {
        free( mp );
        return NULL;
    }
    ds->stream = mp;

    ds->read = memread;
    ds->write = memwrite;
    ds->seek = memseek;
    ds->tell = memtell;
    ds->size = memsize;
    ds->flush = memflush;
    ds->close = memclose;

    ds->writable = true;

    // on init assume a size of 4-bytes-per-element and big-endian
    ds->elemsize = 4;
    ds->lsb = false;

    ds->minimize_requests_number = false;
    ds->memory_speedup = true;

    return ds;
}

int segy_mmap( segy_datasource* ds ) {
#ifndef HAVE_MMAP
    return SEGY_MMAP_INVALID;
#else
    /* don't re-map; i.e. multiple consecutive calls should be no-ops */
    if( ds->read == memread ) return SEGY_OK;

    /* make mmap available for file datasource only */
    if( ds->read != fileread ) return SEGY_MMAP_INVALID;

    long long fsize;
    int err = ds->size( ds, &fsize );

    if( err != 0 ) return SEGY_FSEEK_ERROR;

    memfile* mp = malloc( sizeof( memfile ) );
    if( !mp ) return SEGY_MEMORY_ERROR;

    const int prot = ds->writable ? PROT_READ | PROT_WRITE : PROT_READ;

    int fd = fileno( (FILE*) ds->stream );
    void* addr = mmap( NULL, fsize, prot, MAP_SHARED, fd, 0 );

    // cppcheck-suppress memleak
    if( addr == MAP_FAILED ) {
        free( mp );
        return SEGY_MMAP_ERROR;
    }

    // free FILE datasource resources
    ds->close( ds );

    mp->addr = addr;
    mp->cur = mp->addr;
    mp->size = fsize;

    /* repurpose ds */
    ds->stream = mp;

    ds->read = memread;
    ds->write = memwrite;
    ds->seek = memseek;
    ds->tell = memtell;
    ds->size = memsize;
    ds->flush = mmapflush;
    ds->close = mmapclose;

    ds->minimize_requests_number = false;
    ds->memory_speedup = true;

    return SEGY_OK;
#endif //HAVE_MMAP
}

int segy_flush( segy_datasource* ds ) {
    // flush is a no-op for read-only files
    if( !ds->writable ) return SEGY_OK;

    int flusherr = ds->flush( ds );
    if( flusherr != 0 ) return SEGY_DS_FLUSH_ERROR;

    return SEGY_OK;
}

static int segy_seek( segy_datasource* ds,
                      int trace,
                      long trace0,
                      int trace_bsize ) {

    trace_bsize += SEGY_TRACE_HEADER_SIZE;
    long long pos = (long long)trace0 + (trace * (long long)trace_bsize);

    int err = ds->seek( ds, pos, SEEK_SET );
    if( err != 0 ) return SEGY_DS_SEEK_ERROR;
    return SEGY_OK;
}

int segy_close( segy_datasource* ds ) {
    int err = segy_flush( ds);
    if( err != SEGY_OK ) return err;

    err = ds->close(ds);
    free( ds );
    if( err != 0 ) return SEGY_DS_CLOSE_ERROR;
    return SEGY_OK;
}

int segy_init_field_data( int field, segy_field_data* fd ) {
    if( field > 0 && field < SEGY_TRACE_HEADER_SIZE ) {
        fd->field_index = field;
        fd->field_offset = 0;
        fd->datatype = tr_field_type[fd->field_index];
    } else if( field > SEGY_TEXT_HEADER_SIZE && field < SEGY_TEXT_HEADER_SIZE + SEGY_BINARY_HEADER_SIZE ) {
        fd->field_index = field - SEGY_TEXT_HEADER_SIZE;
        fd->field_offset = SEGY_TEXT_HEADER_SIZE;
        fd->datatype = bin_field_type[fd->field_index];
    } else {
        return SEGY_INVALID_FIELD;
    }

    if( fd->datatype == SEGY_UNDEFINED_FIELD )
        return SEGY_INVALID_FIELD;
    return SEGY_OK;
}

int segy_get_field( const char* header, segy_field_data* fd ) {
    uint64_t val;
    switch ( fd->datatype ) {

        case SEGY_SIGNED_INTEGER_8_BYTE:
            memcpy( &(fd->value.i64), header + (fd->field_index -1), formatsize( fd->datatype ) );
            fd->value.i64 = be64toh( fd->value.i64 );
            return SEGY_OK;

        case SEGY_SIGNED_INTEGER_4_BYTE:
            memcpy( &(fd->value.i32), header + (fd->field_index -1), formatsize( fd->datatype ) );
            fd->value.i32 = be32toh( fd->value.i32 );
            return SEGY_OK;

        case SEGY_SIGNED_SHORT_2_BYTE:
            memcpy( &(fd->value.i16), header + (fd->field_index -1), formatsize( fd->datatype ) );
            fd->value.i16 = be16toh( fd->value.i16 );
            return SEGY_OK;

        case SEGY_SIGNED_CHAR_1_BYTE:
            memcpy( &(fd->value.i8), header + (fd->field_index -1), formatsize( fd->datatype ) );
            return SEGY_OK;

        case SEGY_UNSIGNED_INTEGER_8_BYTE:
            memcpy( &(fd->value.u64), header + (fd->field_index -1), formatsize( fd->datatype ) );
            fd->value.u64 = be64toh( fd->value.u64 );
            return SEGY_OK;

        case SEGY_UNSIGNED_INTEGER_4_BYTE:
            memcpy( &(fd->value.u32), header + (fd->field_index -1), formatsize( fd->datatype ) );
            fd->value.u32 = be32toh( fd->value.u32 );
            return SEGY_OK;

        case SEGY_UNSIGNED_SHORT_2_BYTE:
            memcpy( &(fd->value.u16), header + (fd->field_index -1), formatsize( fd->datatype ) );
            fd->value.u16 = be16toh( fd->value.u16 );
            return SEGY_OK;

        case SEGY_UNSIGNED_CHAR_1_BYTE:
            memcpy( &(fd->value.u8), header + (fd->field_index -1), formatsize( fd->datatype ) );
            return SEGY_OK;

        case SEGY_IEEE_FLOAT_8_BYTE:
            memcpy( &(fd->value.f64), header + (fd->field_index -1), formatsize( fd->datatype ) );
            memcpy( &val, &(fd->value.f64), sizeof( uint64_t ) );
            val = be64toh( val );
            memcpy( &(fd->value.f64), &val, sizeof( uint64_t ) );
            return SEGY_OK;

        default:
            return SEGY_INVALID_FIELD_DATATYPE;
    }
}

static int fd_get_int( const segy_field_data* fd, int* val ) {
    switch( fd->datatype ) {

        case SEGY_SIGNED_INTEGER_4_BYTE:
            *val = fd->value.i32;
            return SEGY_OK;

        case SEGY_SIGNED_SHORT_2_BYTE:
            *val = fd->value.i16;
            return SEGY_OK;

        case SEGY_SIGNED_CHAR_1_BYTE:
            *val = fd->value.i8;
            return SEGY_OK;

        case SEGY_UNSIGNED_SHORT_2_BYTE:
            *val = fd->value.u16;
            return SEGY_OK;

        case SEGY_UNSIGNED_CHAR_1_BYTE:
            *val = fd->value.u8;
            return SEGY_OK;

        default:
            return SEGY_INVALID_FIELD_DATATYPE;
    }
}

int segy_get_field_int( const char* header, int field, int* val ) {
    segy_field_data fd;
    int err = segy_init_field_data( field, &fd );
    if( err != SEGY_OK ) return err;

    err = segy_get_field(header, &fd);
    if ( err != SEGY_OK ) return err;
    return fd_get_int( &fd, val );
}

int segy_set_field( char* header, segy_field_data* fd ) {
    segy_field_value fv = fd->value;
    uint64_t val;
    switch ( fd->datatype ) {

        case SEGY_SIGNED_INTEGER_8_BYTE:
            fv.i64 = htobe64( fv.i64 );
            memcpy( header + (fd->field_index - 1), &(fv.i64), formatsize( fd->datatype ));
            return SEGY_OK;

        case SEGY_SIGNED_INTEGER_4_BYTE:
            fv.i32 = htobe32( fv.i32 );
            memcpy( header + (fd->field_index - 1), &(fv.i32), formatsize( fd->datatype ));
            return SEGY_OK;

        case SEGY_SIGNED_SHORT_2_BYTE:
            fv.i16 = htobe16( fv.i16 );
            memcpy( header + (fd->field_index - 1), &(fv.i16), formatsize( fd->datatype ));
            return SEGY_OK;

        case SEGY_SIGNED_CHAR_1_BYTE:
            memcpy( header + (fd->field_index - 1), &(fv.i8), formatsize( fd->datatype ));
            return SEGY_OK;

        case SEGY_UNSIGNED_INTEGER_8_BYTE:
            fv.u64 = htobe64( fv.u64 );
            memcpy( header + (fd->field_index - 1), &(fv.u64), formatsize( fd->datatype ));
            return SEGY_OK;

        case SEGY_UNSIGNED_INTEGER_4_BYTE:
            fv.u32 = htobe32( fv.u32 );
            memcpy( header + (fd->field_index - 1), &(fv.u32), formatsize( fd->datatype ));
            return SEGY_OK;

        case SEGY_UNSIGNED_SHORT_2_BYTE:
            fv.u16 = htobe16( fv.u16 );
            memcpy( header + (fd->field_index - 1), &(fv.u16), formatsize( fd->datatype ));
            return SEGY_OK;

        case SEGY_UNSIGNED_CHAR_1_BYTE:
            memcpy( header + (fd->field_index - 1), &(fv.u8), formatsize( fd->datatype ));
            return SEGY_OK;

        case SEGY_IEEE_FLOAT_8_BYTE:
            memcpy( &val, &(fv.f64), sizeof( uint64_t ) );
            val = htobe64( val );
            memcpy( &(fv.f64), &val, sizeof( uint64_t ) );
            memcpy( header + (fd->field_index - 1), &(fv.f64), formatsize( fd->datatype ));
            return SEGY_OK;

        default:
            return SEGY_INVALID_FIELD_DATATYPE;
    }
}

static int fd_set_int( segy_field_data* fd, int val ) {
    switch( fd->datatype ) {
        case SEGY_SIGNED_INTEGER_4_BYTE:
            fd->value.i32 = val;
            return SEGY_OK;

        case SEGY_SIGNED_SHORT_2_BYTE:
            if ( val > INT16_MAX || val < INT16_MIN )
                return SEGY_INVALID_FIELD_VALUE;
            fd->value.i16 = (int16_t)val;
            return SEGY_OK;

        case SEGY_SIGNED_CHAR_1_BYTE:
            if ( val > INT8_MAX || val < INT8_MIN )
                return SEGY_INVALID_FIELD_VALUE;
            fd->value.i8 = (int8_t)val;
            return SEGY_OK;

        case SEGY_UNSIGNED_SHORT_2_BYTE:
            if ( val > UINT16_MAX || val < 0 )
                return SEGY_INVALID_FIELD_VALUE;
            fd->value.u16 = (uint16_t)val;
            return SEGY_OK;

        case SEGY_UNSIGNED_CHAR_1_BYTE:
            if ( val > UINT8_MAX || val < 0 )
                return SEGY_INVALID_FIELD_VALUE;
            fd->value.u8 = (uint8_t)val;
            return SEGY_OK;

        default:
            return SEGY_INVALID_FIELD_DATATYPE;
    }
}

int segy_set_field_int( char* header, const int field, const int val ) {
    segy_field_data fd;
    int err = segy_init_field_data( field, &fd );
    if( err != SEGY_OK ) return err;

    err = fd_set_int( &fd, val );
    if( err != SEGY_OK ) return err;

    return segy_set_field( header, &fd );
}

static int slicelength( int start, int stop, int step ) {
    if( step == 0 ) return 0;

    if( ( step < 0 && stop >= start ) ||
        ( step > 0 && start >= stop ) ) return 0;

    if( step < 0 ) return (stop - start + 1) / step + 1;

    /*
     * cppcheck 1.84 introduced a false-positive on this as a result of
     * value-flow analysis not realising that step can *never* be zero here, as
     * it would trigger the early return.
     */
    // cppcheck-suppress zerodivcond
    return (stop - start - 1) / step + 1;
}

static int32_t bswap_header_word(int32_t f, int word_size) {
    if (word_size == 4)
        return bswap32(f);

    /*
     * The casts are here are necessary
     *
     * First, it must be converted to a *signed* short (to preserve negative
     * numbers). The narrowing is safe because the source is 2 bytes anyway.
     *
     * The behaviour when this is implicitly wrong was discovered in [1].
     *
     * Then, it must be byteswapped with bswap16. When using the shifts is
     * probably fine as the types are also cast in the macro and are
     * compatible, but the builtins have this signature [2]:
     *
     *  uint16_t __builtin_bswap16 (uint16_t x)
     *
     * which means that if the value is *negative* it will be interpreted as
     * unsigned and very much positive. When it is then implicitly converted
     * in the return type it is widened, and int32 can fit uint16 maximum just
     * fine.
     *
     * [1] https://github.com/equinor/segyio/issues/368
     * [2] http://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
     */
    return (int16_t) bswap16((int16_t) f);
}

int segy_field_forall( segy_datasource* ds,
                       int field,
                       int start,
                       int stop,
                       int step,
                       int* buf,
                       long trace0,
                       int trace_bsize ) {
    int err;
    // do a dummy-read of a zero-init'd buffer to check args
    int32_t f;
    char header[ SEGY_TRACE_HEADER_SIZE ] = { 0 };
    err = segy_get_field_int( header, field, &f );
    if( err != SEGY_OK ) return SEGY_INVALID_ARGS;

    int slicelen = slicelength( start, stop, step );

    const int zfield = field - 1;
    for( int i = start; slicelen > 0; i += step, ++buf, --slicelen ) {
        err = segy_seek( ds, i, trace0 + zfield, trace_bsize );
        if( err != SEGY_OK ) return err;
        err = ds->read( ds, header + zfield, sizeof( uint32_t ) );
        if( err != 0 ) return SEGY_DS_READ_ERROR;

        segy_field_data fd;
        err = segy_init_field_data(field, &fd);
        if ( err != SEGY_OK ) return err;
        err = segy_get_field( header, &fd );
        if( err != 0 ) return err;
        err = fd_get_int( &fd, &f );
        if( err != 0 ) return err;
        if (ds->lsb) f = bswap_header_word(f, formatsize( fd.datatype ));
        *buf = f;
    }

    return SEGY_OK;
}

static int bswap_bin( char* xs, int lsb ) {
    if( !lsb ) return SEGY_OK;

    const int bytes4[] = {
        SEGY_BIN_JOB_ID,
        SEGY_BIN_LINE_NUMBER,
        SEGY_BIN_REEL_NUMBER
    };

    const int bytes4_len = sizeof(bytes4) / sizeof(int);

    for( int i = 0; i < bytes4_len; ++i ) {
        const int offset = bytes4[ i ] - (HEADER_SIZE+1);
        bswap32_mem( xs + offset, xs + offset );
    }

    const int bytes2[] = {
        SEGY_BIN_ENSEMBLE_TRACES,
        SEGY_BIN_AUX_ENSEMBLE_TRACES,
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
        SEGY_BIN_EXT_HEADERS,
    };

    const int bytes2_len = sizeof( bytes2 ) / sizeof( int );

    for( int i = 0; i < bytes2_len; ++i ) {
        const int offset = bytes2[ i ] - (HEADER_SIZE+1);
        bswap16_mem( xs + offset, xs + offset );
    }

    return SEGY_OK;
}

int segy_binheader( segy_datasource* ds, char* buf ) {
    if( !ds ) return SEGY_INVALID_ARGS;

    int err = ds->seek( ds, SEGY_TEXT_HEADER_SIZE, SEEK_SET );
    if( err != 0 ) return SEGY_DS_SEEK_ERROR;

    err = ds->read( ds, buf, SEGY_BINARY_HEADER_SIZE );
    if( err != 0 ) return SEGY_DS_READ_ERROR;

    /* successful and file was lsb - swap to present as msb */
    return bswap_bin( buf, ds->lsb );
}

int segy_write_binheader( segy_datasource* ds, const char* buf ) {
    if( !ds->writable ) return SEGY_READONLY;

    char swapped[ SEGY_BINARY_HEADER_SIZE ];
    memcpy( swapped, buf, SEGY_BINARY_HEADER_SIZE );
    bswap_bin( swapped, ds->lsb );

    int err = ds->seek( ds, SEGY_TEXT_HEADER_SIZE, SEEK_SET );
    if( err != 0 ) return SEGY_DS_SEEK_ERROR;

    err = ds->write( ds, swapped, sizeof( swapped ) );
    if( err != 0 ) return SEGY_DS_WRITE_ERROR;

    return SEGY_OK;
}

int segy_format( const char* binheader ) {
    int format = 0;
    segy_get_field_int( binheader, SEGY_BIN_FORMAT, &format );
    return format;
}

int segy_set_format( segy_datasource* ds, int format ) {
    const int elemsize = formatsize( format );
    if( elemsize <= 0 ) return SEGY_INVALID_ARGS;
    ds->elemsize = elemsize;

    return SEGY_OK;
}

int segy_set_endianness( segy_datasource* ds, int endianness) {
    switch( endianness ) {
        case SEGY_LSB:
        case SEGY_MSB:
            break;
        default:
            return SEGY_INVALID_ARGS;
    }

    ds->lsb = endianness;
    return SEGY_OK;
}

int segy_samples( const char* binheader ) {
    int samples = 0;
    segy_get_field_int( binheader, SEGY_BIN_SAMPLES, &samples );
    samples = (int32_t)((uint16_t)samples);

    int ext_samples = 0;
    segy_get_field_int(binheader, SEGY_BIN_EXT_SAMPLES, &ext_samples);

    if (samples == 0 && ext_samples > 0)
        return ext_samples;

    /*
     * SEG-Y rev2 says that if this field is non-zero, the value in
     * SEGY_BIN_SAMPLES should be ignored, and this used instead. This seems
     * unreliable as the header words are not specified to be zero'd in the
     * unassigned section, so valid pre-rev2 files can have non-zero values
     * here.
     *
     * This means heuristics are necessary. Assume that if the extended word is
     * used, the revision flag is also appropriately set to >= 2. Negative
     * values are ignored, as it's likely just noise.
     */
    int revision = 0;
    segy_get_field_int(binheader, SEGY_BIN_SEGY_REVISION, &revision);
    if (revision >= 2 && ext_samples > 0)
        return ext_samples;

    return samples;
}

int segy_trace_bsize( int samples ) {
    assert( samples >= 0 );
    return segy_trsize( SEGY_IBM_FLOAT_4_BYTE, samples );
}

int segy_trsize( int format, int samples ) {
    const int elemsize = formatsize( format );
    if( elemsize < 0 ) return -1;
    return samples * elemsize;
}

long segy_trace0( const char* binheader ) {
    int extra_headers = 0;
    segy_get_field_int( binheader, SEGY_BIN_EXT_HEADERS, &extra_headers );

    return SEGY_TEXT_HEADER_SIZE + SEGY_BINARY_HEADER_SIZE +
           SEGY_TEXT_HEADER_SIZE * extra_headers;
}

static int bswap_th( char* xs, int lsb ) {
    if( !lsb ) return SEGY_OK;

    const int bytes4[] = {
        SEGY_TR_CDP_X,
        SEGY_TR_CDP_Y,
        SEGY_TR_CROSSLINE,
        SEGY_TR_ENERGY_SOURCE_POINT,
        SEGY_TR_ENSEMBLE,
        SEGY_TR_FIELD_RECORD,
        SEGY_TR_GROUP_WATER_DEPTH,
        SEGY_TR_GROUP_X,
        SEGY_TR_GROUP_Y,
        SEGY_TR_INLINE,
        SEGY_TR_NUMBER_ORIG_FIELD,
        SEGY_TR_NUM_IN_ENSEMBLE,
        SEGY_TR_OFFSET,
        SEGY_TR_RECV_DATUM_ELEV,
        SEGY_TR_RECV_GROUP_ELEV,
        SEGY_TR_SEQ_FILE,
        SEGY_TR_SEQ_LINE,
        SEGY_TR_SHOT_POINT,
        SEGY_TR_SOURCE_DATUM_ELEV,
        SEGY_TR_SOURCE_DEPTH,
        SEGY_TR_SOURCE_MEASURE_MANT,
        SEGY_TR_SOURCE_SURF_ELEV,
        SEGY_TR_SOURCE_WATER_DEPTH,
        SEGY_TR_SOURCE_X,
        SEGY_TR_SOURCE_Y,
        SEGY_TR_TRANSDUCTION_MANT,
    };

    const int bytes4_len = sizeof(bytes4) / sizeof(int);

    for( int i = 0; i < bytes4_len; ++i ) {
        const int offset = bytes4[ i ] - 1;
        bswap32_mem( xs + offset, xs + offset );
    }

    const int bytes2[] = {
        SEGY_TR_ALIAS_FILT_FREQ,
        SEGY_TR_ALIAS_FILT_SLOPE,
        SEGY_TR_COORD_UNITS,
        SEGY_TR_CORRELATED,
        SEGY_TR_DATA_USE,
        SEGY_TR_DAY_OF_YEAR,
        SEGY_TR_DELAY_REC_TIME,
        SEGY_TR_DEVICE_ID,
        SEGY_TR_ELEV_SCALAR,
        SEGY_TR_GAIN_TYPE,
        SEGY_TR_GAP_SIZE,
        SEGY_TR_GEOPHONE_GROUP_FIRST,
        SEGY_TR_GEOPHONE_GROUP_LAST,
        SEGY_TR_GEOPHONE_GROUP_ROLL1,
        SEGY_TR_GROUP_STATIC_CORR,
        SEGY_TR_GROUP_UPHOLE_TIME,
        SEGY_TR_HIGH_CUT_FREQ,
        SEGY_TR_HIGH_CUT_SLOPE,
        SEGY_TR_HOUR_OF_DAY,
        SEGY_TR_INSTR_GAIN_CONST,
        SEGY_TR_INSTR_INIT_GAIN,
        SEGY_TR_LAG_A,
        SEGY_TR_LAG_B,
        SEGY_TR_LOW_CUT_FREQ,
        SEGY_TR_LOW_CUT_SLOPE,
        SEGY_TR_MEASURE_UNIT,
        SEGY_TR_MIN_OF_HOUR,
        SEGY_TR_MUTE_TIME_END,
        SEGY_TR_MUTE_TIME_START,
        SEGY_TR_NOTCH_FILT_FREQ,
        SEGY_TR_NOTCH_FILT_SLOPE,
        SEGY_TR_OVER_TRAVEL,
        SEGY_TR_SAMPLE_COUNT,
        SEGY_TR_SAMPLE_INTER,
        SEGY_TR_SCALAR_TRACE_HEADER,
        SEGY_TR_SEC_OF_MIN,
        SEGY_TR_SHOT_POINT_SCALAR,
        SEGY_TR_SOURCE_ENERGY_DIR_VERT,
        SEGY_TR_SOURCE_ENERGY_DIR_XLINE,
        SEGY_TR_SOURCE_ENERGY_DIR_ILINE,
        SEGY_TR_SOURCE_GROUP_SCALAR,
        SEGY_TR_SOURCE_MEASURE_EXP,
        SEGY_TR_SOURCE_MEASURE_UNIT,
        SEGY_TR_SOURCE_STATIC_CORR,
        SEGY_TR_SOURCE_TYPE,
        SEGY_TR_SOURCE_UPHOLE_TIME,
        SEGY_TR_STACKED_TRACES,
        SEGY_TR_SUBWEATHERING_VELO,
        SEGY_TR_SUMMED_TRACES,
        SEGY_TR_SWEEP_FREQ_END,
        SEGY_TR_SWEEP_FREQ_START,
        SEGY_TR_SWEEP_LENGTH,
        SEGY_TR_SWEEP_TAPERLEN_END,
        SEGY_TR_SWEEP_TAPERLEN_START,
        SEGY_TR_SWEEP_TYPE,
        SEGY_TR_TAPER_TYPE,
        SEGY_TR_TIME_BASE_CODE,
        SEGY_TR_TOT_STATIC_APPLIED,
        SEGY_TR_TRACE_ID,
        SEGY_TR_TRANSDUCTION_EXP,
        SEGY_TR_TRANSDUCTION_UNIT,
        SEGY_TR_WEATHERING_VELO,
        SEGY_TR_WEIGHTING_FAC,
        SEGY_TR_YEAR_DATA_REC,
    };

    const int bytes2_len = sizeof( bytes2 ) / sizeof( int );

    for( int i = 0; i < bytes2_len; ++i ) {
        const int offset = bytes2[ i ] - 1;
        bswap16_mem( xs + offset, xs + offset );
    }

    return SEGY_OK;
}

int segy_traceheader( segy_datasource* ds,
                      int traceno,
                      char* buf,
                      long trace0,
                      int trace_bsize ) {

    int err = segy_seek( ds, traceno, trace0, trace_bsize );
    if( err != SEGY_OK ) return err;

    err = ds->read( ds, buf, SEGY_TRACE_HEADER_SIZE );
    if( err != 0 ) return SEGY_DS_READ_ERROR;

    return bswap_th( buf, ds->lsb );
}

int segy_write_traceheader( segy_datasource* ds,
                            int traceno,
                            const char* buf,
                            long trace0,
                            int trace_bsize ) {
    if( !ds->writable ) return SEGY_READONLY;

    int err = segy_seek( ds, traceno, trace0, trace_bsize );
    if( err != SEGY_OK ) return err;

    char swapped[SEGY_TRACE_HEADER_SIZE];
    memcpy( swapped, buf, SEGY_TRACE_HEADER_SIZE );
    bswap_th( swapped, ds->lsb );

    err = ds->write( ds, swapped, SEGY_TRACE_HEADER_SIZE );
    if( err != 0 ) return SEGY_DS_WRITE_ERROR;

    return SEGY_OK;
}

/*
 * Return the number of traces in the file. Stream position won't change after
 * this call.
 *
 * This function assumes that *all traces* are of the same size.
 */
int segy_traces( segy_datasource* ds,
                 int* traces,
                 long trace0,
                 int trace_bsize ) {

    if( trace0 < 0 ) return SEGY_INVALID_ARGS;

    long long size;
    int err = ds->size( ds, &size );
    if( err != 0 ) return SEGY_DS_ERROR;

    if( trace0 > size ) return SEGY_INVALID_ARGS;

    size -= trace0;
    trace_bsize += SEGY_TRACE_HEADER_SIZE;

    if( size % trace_bsize != 0 )
        return SEGY_TRACE_SIZE_MISMATCH;

    assert( size / trace_bsize <= (long long)INT_MAX );

    *traces = (int) (size / trace_bsize);
    return SEGY_OK;
}

int segy_sample_interval( segy_datasource* ds, float fallback, float* dt ) {

    char bin_header[ SEGY_BINARY_HEADER_SIZE ];
    char trace_header[ SEGY_TRACE_HEADER_SIZE ];

    int err = segy_binheader( ds, bin_header );
    if (err != 0) {
        return err;
    }

    const long trace0 = segy_trace0( bin_header );

    /* we don't need to figure out a trace size, since we're not advancing
     * beyond the first header */
    err = segy_traceheader( ds, 0, trace_header, trace0, 0 );
    if( err != 0 ) {
        return err;
    }

    int bindt = 0;
    int trdt = 0;

    segy_get_field_int( bin_header, SEGY_BIN_INTERVAL, &bindt );
    segy_get_field_int( trace_header, SEGY_TR_SAMPLE_INTER, &trdt );

    float binary_header_dt = (float) bindt;
    float trace_header_dt = (float) trdt;

    /*
     * 3 cases:
     * * When the trace header and binary header disagree on a (non-zero)
     *   sample interval; choose neither and opt for the fallback.
     * * When both sample intervals are zero: opt for the fallback.
     * * Otherwise, choose the interval from the non-zero header.
     */

    *dt = fallback;
    if( binary_header_dt <= 0 && trace_header_dt > 0 )
        *dt = trace_header_dt;

    if( trace_header_dt <= 0 && binary_header_dt > 0 )
        *dt = binary_header_dt;

    if( trace_header_dt == binary_header_dt && trace_header_dt > 0 )
        *dt = trace_header_dt;

    return SEGY_OK;
}

int segy_sample_indices( segy_datasource* ds,
                         float t0,
                         float dt,
                         int count,
                         float* buf ) {

    int err = segy_sample_interval( ds, dt, &dt );
    if( err != 0 ) {
        return err;
    }

    for( int i = 0; i < count; i++ ) {
        buf[i] = t0 + i * dt;
    }

    return SEGY_OK;
}

/*
 * Determine how a file is sorted. Expects the following three fields from the
 * trace header to guide sorting: the inline number `il`, the crossline
 * number `xl` and the offset number `tr_offset`.
 *
 * Iterates through trace headers and compare the three fields with the
 * fields from the previous header. If iline or crossline sorting is established
 * the method returns the sorting without reading through the rest of the file.
 *
 * A file is inline-sorted if inline is the last value to move. Likewise for
 * crossline sorted. If the file does not qualify as inline- or
 * crossline-sorted, it is unsorted. Exactly one of the three values should
 * increment from one trace to another for the file to be properly sorted.
 */

int segy_sorting( segy_datasource* ds,
                  int il,
                  int xl,
                  int tr_offset,
                  int* sorting,
                  long trace0,
                  int trace_bsize ) {
    int err;
    char traceheader[ SEGY_TRACE_HEADER_SIZE ];

    err = segy_traceheader( ds, 0, traceheader, trace0, trace_bsize );
    if( err != SEGY_OK ) return err;

    /* make sure field is valid, so we don't have to check errors later */
    const int fields[] = { il, xl, tr_offset };
    int len = sizeof( fields ) / sizeof( int );
    for( int i = 0; i < len; ++i ) {
        const int f = fields[ i ];
        if( f < 0 )
            return SEGY_INVALID_FIELD;
        if( f >= SEGY_TRACE_HEADER_SIZE )
            return SEGY_INVALID_FIELD;
        if( tr_field_type[ f ] == 0 )
            return SEGY_INVALID_FIELD;
    }

    int traces;
    err = segy_traces( ds, &traces, trace0, trace_bsize );
    if( err ) return err;

    if( traces == 1 ) {
        *sorting = SEGY_CROSSLINE_SORTING;
        return SEGY_OK;
    }

    int il_first = 0, il_next = 0, il_prev = 0;
    int xl_first = 0, xl_next = 0, xl_prev = 0;
    int of_first = 0, of_next = 0;

    segy_get_field_int( traceheader, il, &il_first );
    segy_get_field_int( traceheader, xl, &xl_first );
    segy_get_field_int( traceheader, tr_offset, &of_first );

    il_prev = il_first;
    xl_prev = xl_first;

    /* Iterating through traces, comparing il, xl, and offset values with the
     * values from the previous trace. Several cases is checked when
     * determining sorting:
     * If the offset have wrapped around and either il or xl is changed,
     * the sorting is xline or iline, respectivly.
     * If neither offset, il or xl changes, the file is unsorted.
     * If more than one value change from one trace to another, the file is
     * unsorted.
     */

    int traceno = 1;
    while ( traceno < traces ) {
        err = segy_traceheader( ds, traceno, traceheader, trace0, trace_bsize );
        if( err ) return err;
        ++traceno;

        segy_get_field_int( traceheader, il, &il_next );
        segy_get_field_int( traceheader, xl, &xl_next );
        segy_get_field_int( traceheader, tr_offset, &of_next );

        /* the exit condition - offset has wrapped around. */
        if( of_next == of_first ) {
            if( il_next == il_prev && xl_next != xl_prev ) {
                *sorting = SEGY_INLINE_SORTING;
                return SEGY_OK;
            }

            if( xl_next == xl_prev && il_next != il_prev ) {
                *sorting = SEGY_CROSSLINE_SORTING;
                return SEGY_OK;
            }

            *sorting = SEGY_UNKNOWN_SORTING;
            return SEGY_OK;
        }

        /* something else than offsets also moved, so this is not sorted */
        if( il_prev != il_next ) {
            *sorting = SEGY_UNKNOWN_SORTING;
            return SEGY_OK;
        }

        if( xl_prev != xl_next ) {
            *sorting = SEGY_UNKNOWN_SORTING;
            return SEGY_OK;
        }
    }

    *sorting = SEGY_CROSSLINE_SORTING;
    return SEGY_OK;
}

/*
 * Find the number of offsets. This is determined by inspecting the trace
 * headers [0,n) where n is the first trace where either the inline number or
 * the crossline number changes (which changes first depends on sorting, but is
 * irrelevant for this function).
 */
int segy_offsets( segy_datasource* ds,
                  int il,
                  int xl,
                  int traces,
                  int* out,
                  long trace0,
                  int trace_bsize ) {
    int err;
    int il0 = 0, il1 = 0, xl0 = 0, xl1 = 0;
    char header[ SEGY_TRACE_HEADER_SIZE ];
    int offsets = 0;

    if( traces == 1 ) {
        *out = 1;
        return SEGY_OK;
    }

    /*
     * check that field value is sane, so that we don't have to check
     * segy_get_field's error
     */
    if( tr_field_type[ il ] == 0 || tr_field_type[ xl ] == 0 )
        return SEGY_INVALID_FIELD;

    err = segy_traceheader( ds, 0, header, trace0, trace_bsize );
    if( err != 0 ) return SEGY_FREAD_ERROR;

    segy_get_field_int( header, il, &il0 );
    segy_get_field_int( header, xl, &xl0 );

    do {
        ++offsets;

        if( offsets == traces ) break;

        err = segy_traceheader( ds, offsets, header, trace0, trace_bsize );
        if( err != 0 ) return err;

        segy_get_field_int( header, il, &il1 );
        segy_get_field_int( header, xl, &xl1 );
    } while( il0 == il1 && xl0 == xl1 );

    *out = offsets;
    return SEGY_OK;
}

int segy_offset_indices( segy_datasource* ds,
                         int offset_field,
                         int offsets,
                         int* out,
                         long trace0,
                         int trace_bsize ) {
    int32_t x = 0;
    char header[ SEGY_TRACE_HEADER_SIZE ];

    if( tr_field_type[ offset_field ] == 0 )
        return SEGY_INVALID_FIELD;

    for( int i = 0; i < offsets; ++i ) {
        const int err = segy_traceheader( ds, i, header, trace0, trace_bsize );
        if( err != SEGY_OK ) return err;

        segy_get_field_int( header, offset_field, &x );
        *out++ = x;
    }

    return SEGY_OK;
}

static int segy_line_indices( segy_datasource* ds,
                              int field,
                              int traceno,
                              int stride,
                              int num_indices,
                              int* buf,
                              long trace0,
                              int trace_bsize ) {
    return segy_field_forall( ds,
                              field,
                              traceno,                          /* start */
                              traceno + (num_indices * stride), /* stop */
                              stride,                           /* step */
                              buf,
                              trace0,
                              trace_bsize );
}

static int count_lines( segy_datasource* ds,
                        int field,
                        int offsets,
                        int traces,
                        int* out,
                        long trace0,
                        int trace_bsize ) {

    int err;
    char header[ SEGY_TRACE_HEADER_SIZE ];
    err = segy_traceheader( ds, 0, header, trace0, trace_bsize );
    if( err != 0 ) return err;

    int first_lineno, first_offset, ln = 0, off = 0;

    err = segy_get_field_int( header, field, &first_lineno );
    if( err != 0 ) return err;

    err = segy_get_field_int( header, 37, &first_offset );
    if( err != 0 ) return err;

    int lines = 1;
    int curr = offsets;

    while( true ) {
        if( curr == traces ) break;
        if( curr >  traces ) return SEGY_NOTFOUND;

        err = segy_traceheader( ds, curr, header, trace0, trace_bsize );
        if( err != 0 ) return err;

        segy_get_field_int( header, field, &ln );
        segy_get_field_int( header, 37, &off );

        if( first_offset == off && ln == first_lineno ) break;

        curr += offsets;
        ++lines;
    }

    *out = lines;
    return SEGY_OK;
}

int segy_count_lines( segy_datasource* ds,
                      int field,
                      int offsets,
                      int* l1out,
                      int* l2out,
                      long trace0,
                      int trace_bsize ) {

    int traces;
    int err = segy_traces( ds, &traces, trace0, trace_bsize );
    if( err != 0 ) return err;

    /*
     * handle the case where there's only one trace (per offset) in the file,
     * and interpret is as a 1 line in each direction, with 1 trace (per
     * offset).
     */
    if( traces == offsets ) {
        *l1out = *l2out = 1;
        return SEGY_OK;
    }

    int l2count;
    err = count_lines( ds, field,
                           offsets,
                           traces,
                           &l2count,
                           trace0,
                           trace_bsize );
    if( err != 0 ) return err;

    const int line_length = l2count * offsets;
    const int l1count = traces / line_length;

    *l1out = l1count;
    *l2out = l2count;

    return SEGY_OK;
}

int segy_lines_count( segy_datasource* ds,
                      int il,
                      int xl,
                      int sorting,
                      int offsets,
                      int* il_count,
                      int* xl_count,
                      long trace0,
                      int trace_bsize ) {

    if( sorting == SEGY_UNKNOWN_SORTING ) return SEGY_INVALID_SORTING;

    int field;
    int l1out, l2out;

    if( sorting == SEGY_INLINE_SORTING ) field = xl;
    else field = il;

    int err = segy_count_lines( ds, field, offsets,
                                &l1out, &l2out,
                                trace0, trace_bsize );

    if( err != SEGY_OK ) return err;

    if( sorting == SEGY_INLINE_SORTING ) {
        *il_count = l1out;
        *xl_count = l2out;
    } else {
        *il_count = l2out;
        *xl_count = l1out;
    }

    return SEGY_OK;
}

/*
 * segy_*line_length is rather pointless as a computation, but serve a purpose
 * as an abstraction as the detail on how exactly a length is defined is usually uninteresting
 */
int segy_inline_length( int crossline_count ) {
    return crossline_count;
}

int segy_crossline_length( int inline_count ) {
    return inline_count;
}

int segy_inline_indices( segy_datasource* ds,
                         int il,
                         int sorting,
                         int inline_count,
                         int crossline_count,
                         int offsets,
                         int* buf,
                         long trace0,
                         int trace_bsize) {

    if( sorting == SEGY_INLINE_SORTING ) {
        int stride = crossline_count * offsets;
        return segy_line_indices( ds, il, 0, stride, inline_count, buf, trace0, trace_bsize );
    }

    if( sorting == SEGY_CROSSLINE_SORTING ) {
        return segy_line_indices( ds, il, 0, offsets, inline_count, buf, trace0, trace_bsize );
    }

    return SEGY_INVALID_SORTING;
}

int segy_crossline_indices( segy_datasource* ds,
                            int xl,
                            int sorting,
                            int inline_count,
                            int crossline_count,
                            int offsets,
                            int* buf,
                            long trace0,
                            int trace_bsize ) {

    if( sorting == SEGY_INLINE_SORTING ) {
        return segy_line_indices( ds, xl, 0, offsets, crossline_count, buf, trace0, trace_bsize );
    }

    if( sorting == SEGY_CROSSLINE_SORTING ) {
        int stride = inline_count * offsets;
        return segy_line_indices( ds, xl, 0, stride, crossline_count, buf, trace0, trace_bsize );
    }

    return SEGY_INVALID_SORTING;
}

static inline int subtr_seek( segy_datasource* ds,
                              int traceno,
                              int start,
                              int stop,
                              int elemsize,
                              long trace0,
                              int trace_bsize ) {
    /*
     * Optimistically assume that indices are correct by the time they're given
     * to subtr_seek.
     */
    const int min = start < stop ? start : stop + 1;
    assert( start >= 0 );
    assert( stop >= -1 );
    assert( abs(stop - start) * elemsize <= trace_bsize );

    // skip the trace header and skip everything before min
    trace0 += (min * elemsize) + SEGY_TRACE_HEADER_SIZE;
    return segy_seek( ds, traceno, trace0, trace_bsize );
}

static int reverse( void* buf, int elems, int elemsize ) {
    char* arr = (char*) buf;
    const int last = elems - 1;
    char tmp[ 8 ];
    for( int i = 0; i < elems / 2; ++i ) {
        memcpy( tmp, arr + i * elemsize, elemsize );
        memcpy( arr + i * elemsize, arr + (last - i) * elemsize, elemsize );
        memcpy( arr + (last - i) * elemsize, tmp, elemsize );
    }

    return SEGY_OK;
}

int segy_readtrace( segy_datasource* ds,
                    int traceno,
                    void* buf,
                    long trace0,
                    int trace_bsize ) {
    const int stop = trace_bsize / ds->elemsize;
    return segy_readsubtr( ds, traceno, 0, stop, 1, buf, NULL, trace0, trace_bsize );
}

static int bswap64vec( void* vec, long long len ) {
    char* begin = (char*) vec;
    const char* end = (char*) begin + len * sizeof(int64_t);

    for (char* xs = begin; xs != end; xs += sizeof(int64_t)) {
        bswap64_mem( xs, xs );
    }

    return SEGY_OK;
}

static int bswap32vec( void* vec, long long len ) {
    char* begin = (char*) vec;
    const char* end = (char*) begin + len * sizeof(int32_t);

    for( char* xs = begin; xs != end; xs += sizeof(int32_t) ) {
        bswap32_mem( xs, xs );
    }

    return SEGY_OK;
}

static int bswap24vec( void* vec, long long len ) {
    char* begin = (char*) vec;
    const char* end = (char*) begin + len * 3;

    for (char* xs = begin; xs != end; xs += 3) {
        uint8_t bits[3];
        memcpy(bits, xs, sizeof(bits));
        uint8_t tmp = bits[0];
        bits[0] = bits[2];
        bits[2] = tmp;
        memcpy(xs, bits, sizeof(bits));
    }

    return SEGY_OK;
}


static int bswap16vec( void* vec, long long len ) {
    char* begin = (char*) vec;
    const char* end = (char*) begin + len * sizeof(int16_t);

    for( char* xs = begin; xs != end; xs += sizeof(int16_t) ) {
        bswap16_mem( xs, xs );
    }

    return SEGY_OK;
}

int segy_readsubtr( segy_datasource* ds,
                    int traceno,
                    int start,
                    int stop,
                    int step,
                    void* buf,
                    void* rangebuf,
                    long trace0,
                    int trace_bsize ) {

    const int elems = abs( stop - start );
    const int elemsize = ds->elemsize;

    int err = subtr_seek( ds, traceno, start, stop, elemsize, trace0, trace_bsize );
    if( err != SEGY_OK ) return err;

    // most common case: step == abs(1), reading contiguously
    if( step == 1 || step == -1 ) {
        err = ds->read( ds, buf, elemsize * elems );
        if( err != 0 ) return SEGY_DS_READ_ERROR;

        if( ds->lsb ) {
            if( ds->elemsize == 8 ) bswap64vec( buf, elems );
            if( ds->elemsize == 4 ) bswap32vec( buf, elems );
            if( ds->elemsize == 3 ) bswap24vec( buf, elems );
            if( ds->elemsize == 2 ) bswap16vec( buf, elems );
        }

        if( step == -1 ) reverse( buf, elems, elemsize );

        return SEGY_OK;
    }

    // step != 1, i.e. do strided reads
    int defstart = start < stop ? 0 : elems - 1;
    const int slicelen = slicelength( start, stop, step );

    // step is the distance between elems to read, but now we're counting bytes
    step *= elemsize;
    char* dst = (char*)buf;

    if( !ds->minimize_requests_number ) {
        if( ds->memory_speedup ) {
            // separate "memory" path is used for better performance
            memfile* mp = (memfile*)ds->stream;
            const char* cur = (char*)mp->cur + elemsize * defstart;
            for( int i = 0; i < slicelen; cur += step, dst += elemsize, ++i ) {
                memcpy( dst, cur, elemsize );
            }
        } else {
            err = ds->seek( ds, elemsize * defstart, SEEK_CUR );
            if( err != 0 ) return SEGY_DS_SEEK_ERROR;

            for( int i = 0; i < slicelen; dst += elemsize, ++i ) {
                err = ds->read( ds, dst, elemsize );
                if( err != 0 ) return SEGY_DS_READ_ERROR;

                err = ds->seek( ds, step - elemsize, SEEK_CUR );
                if( err != 0 ) return SEGY_DS_SEEK_ERROR;
            }
        }

        if( ds->lsb ) {
            if( ds->elemsize == 8 ) bswap64vec( buf, slicelen );
            if( ds->elemsize == 4 ) bswap32vec( buf, slicelen );
            if( ds->elemsize == 3 ) bswap24vec( buf, slicelen );
            if( ds->elemsize == 2 ) bswap16vec( buf, slicelen );
        }

        return SEGY_OK;
    }

    /*
     * minimize requests number fallback: read the full chunk [start, stop) to
     * avoid multiple calls (expensive in case of fread, about 10x the cost of
     * a single read when reading every other trace). If rangebuf is NULL, the
     * caller has not supplied a buffer for us to use (likely if it's a
     * one-off, and we heap-alloc a buffer. This way the function is safer to
     * use, but with a significant performance penalty when no buffer is
     * supplied.
     */
    void* tracebuf = rangebuf ? rangebuf : malloc( elems * elemsize );
    if (!tracebuf) return SEGY_MEMORY_ERROR;

    err = ds->read( ds, tracebuf, elemsize * elems );
    if( err != 0 ) {
        if( !rangebuf ) free( tracebuf );
        return SEGY_DS_READ_ERROR;
    }

    const char* cur = (char*)tracebuf + elemsize * defstart;
    for( int i = 0; i < slicelen; cur += step, ++i, dst += elemsize )
        memcpy( dst, cur, elemsize );

    if( ds->lsb ) {
        if( ds->elemsize == 8 ) bswap64vec( buf, slicelen );
        if( ds->elemsize == 4 ) bswap32vec( buf, slicelen );
        if( ds->elemsize == 3 ) bswap24vec( buf, slicelen );
        if( ds->elemsize == 2 ) bswap16vec( buf, slicelen );
    }

    if( !rangebuf ) free( tracebuf );
    return SEGY_OK;
}

int segy_writetrace( segy_datasource* ds,
                     int traceno,
                     const void* buf,
                     long trace0,
                     int trace_bsize ) {

    const int stop = trace_bsize / ds->elemsize;
    return segy_writesubtr( ds, traceno, 0, stop, 1, buf, NULL, trace0, trace_bsize );
}


int segy_writesubtr( segy_datasource* ds,
                     int traceno,
                     int start,
                     int stop,
                     int step,
                     const void* buf,
                     void* rangebuf,
                     long trace0,
                     int trace_bsize ) {

    if( !ds->writable ) return SEGY_READONLY;

    const int elems = abs( stop - start );
    const int elemsize = ds->elemsize;
    const size_t range = elems * elemsize;

    int err = subtr_seek( ds, traceno, start, stop, elemsize, trace0, trace_bsize );
    if( err != SEGY_OK ) return err;


    if( step == 1 && !ds->lsb ) {
        /*
         * most common case: step == 1, writing contiguously
         * -1 is not covered here as it would require reversing the input buffer
         * (which is const), which in turn may require a memory allocation. It will
         * be handled by the stride-aware code path
         */

        err = ds->write( ds, buf, range );
        if( err != 0 ) return SEGY_DS_WRITE_ERROR;

        return SEGY_OK;
    }

    /*
     * contiguous, but require memory either because reversing, or byte
     * swapping
     */
    if( ( step == 1 || step == -1 ) && ds->lsb ) {
        void* tracebuf = rangebuf ? rangebuf : malloc( range );
        if (!tracebuf) return SEGY_MEMORY_ERROR;
        memcpy( tracebuf, buf, range );

        if( step == -1 ) reverse( tracebuf, elems, elemsize );
        if( ds->elemsize == 8 ) bswap64vec( tracebuf, elems );
        if( ds->elemsize == 4 ) bswap32vec( tracebuf, elems );
        if( ds->elemsize == 3 ) bswap24vec( tracebuf, elems );
        if( ds->elemsize == 2 ) bswap16vec( tracebuf, elems );

        err = ds->write( ds, tracebuf, range );
        if( !rangebuf ) free( tracebuf );
        if( err != 0 ) return SEGY_DS_WRITE_ERROR;
        return SEGY_OK;
    }

    // step != 1, i.e. do strided reads
    int defstart = start < stop ? 0 : elems - 1;
    int slicelen = slicelength( start, stop, step );

    step *= elemsize;

    void ( *bswap_mem )( char*, const char* );
    if( elemsize == 8 ) {
        bswap_mem = bswap64_mem;
    } else if( elemsize == 4 ) {
        bswap_mem = bswap32_mem;
    } else if( elemsize == 2 ) {
        bswap_mem = bswap16_mem;
    } else {
        return SEGY_INVALID_ARGS;
    }

    // step is the distance between elems, but we're counting bytes
    const char* src = (const char*)buf;

    if( !ds->minimize_requests_number ) {
        err = ds->seek( ds, elemsize * defstart, SEEK_CUR );
        if( err != 0 ) return SEGY_DS_SEEK_ERROR;

        if( !ds->lsb ) {
            // separate "memory" path is used for better performance
            if( ds->memory_speedup ) {
                memfile* mp = (memfile*)ds->stream;
                char* cur = (char*)mp->cur;
                for( ; slicelen > 0; cur += step, src += elemsize, --slicelen ) {
                    memcpy( cur, src, elemsize );
                }
            } else {
                for( ; slicelen > 0; src += elemsize, --slicelen ) {
                    err = ds->write( ds, src, elemsize );
                    if( err != 0 ) return SEGY_DS_WRITE_ERROR;

                    err = ds->seek( ds, step - elemsize, SEEK_CUR );
                    if( err != 0 ) return SEGY_DS_SEEK_ERROR;
                }
            }
        } else {
            char temp[8]; // allocate largest possible
            for( ; slicelen > 0; src += elemsize, --slicelen ) {
                bswap_mem( temp, src );

                err = ds->write( ds, temp, elemsize );
                if( err != 0 ) return SEGY_DS_WRITE_ERROR;

                err = ds->seek( ds, step - elemsize, SEEK_CUR );
                if( err != 0 ) return SEGY_DS_SEEK_ERROR;
            }
        }

        return SEGY_OK;
    }

    void* tracebuf = rangebuf ? rangebuf : malloc( range );
    if( !tracebuf ) return SEGY_MEMORY_ERROR;

    // like in readsubtr, read a larger chunk and then step through that
    err = ds->read( ds, tracebuf, range );
    if( err != 0 ) {
        free( tracebuf );
        return SEGY_DS_READ_ERROR;
    }
    /* rewind, because ds->read advances stream position */
    err = ds->seek( ds, -(long long)range, SEEK_CUR );
    if( err != 0 ) {
        if( !rangebuf ) free( tracebuf );
        return SEGY_DS_SEEK_ERROR;
    }

    char* cur = (char*)tracebuf + elemsize * defstart;
    if( !ds->lsb ) {
        for( ; slicelen > 0; cur += step, --slicelen, src += elemsize ) {
            memcpy( cur, src, elemsize );
        }
    } else {
        for( ; slicelen > 0; cur += step, --slicelen, src += elemsize ) {
            bswap_mem( cur, src );
        }
    }

    err = ds->write( ds, tracebuf, range );
    if( !rangebuf ) free( tracebuf );

    if( err != 0 ) return SEGY_DS_WRITE_ERROR;

    return SEGY_OK;
}

/*
 * The to/from native functions are aware of the underlying architecture and
 * the endianness of the input data (through the format enumerator).
 *
 * LSB - little endian (least-significant)
 * MSB - big endian (most-significant)
 *
 * For the 4-byte IEEE formats, there are the following scenarios:
 *
 *          +  HOST LSB | HOST MSB
 * FILE LSB |   no-op   |  bswap
 * FILE MSB |   bswap   |  no-op
 *
 * the ntohs/ntohl functions gracefully handle all FILE MSB, regardless of host
 * endianness. This does not work when host is MSB, because ntohl would be a
 * no-op for MSB platforms. Therefore, the final table is:
 *
 *          +  HOST LSB | HOST MSB
 * FILE LSB |   no-op   |  bswap
 * FILE MSB |   bswap   |  no-op
 */

static int segy_native_byteswap(int format, long long size, void* buf) {

    if (HOST_LSB) {
        const int elemsize = formatsize( format );

        switch (elemsize) {
            case 8: bswap64vec(buf, size); break;
            case 4: bswap32vec(buf, size); break;
            case 3: bswap24vec(buf, size); break;
            case 2: bswap16vec(buf, size); break;
            default:                       break;
        }
    }

    return SEGY_OK;
}

int segy_to_native( int format,
                    long long size,
                    void* buf ) {

    const int elemsize = formatsize( format );
    if( elemsize < 0 ) return SEGY_INVALID_ARGS;

    segy_native_byteswap( format, size, buf );

    char* dst = (char*)buf;
    if( format == SEGY_IBM_FLOAT_4_BYTE ) {
        for( long long i = 0; i < size; ++i )
            ibm_native( dst + i * elemsize );
    }

    return SEGY_OK;
}

int segy_from_native( int format,
                      long long size,
                      void* buf ) {

    const int elemsize = formatsize( format );
    if( elemsize < 0 ) return SEGY_INVALID_ARGS;

    char* dst = (char*)buf;
    if( format == SEGY_IBM_FLOAT_4_BYTE ) {
        for( long long i = 0; i < size; ++i )
            native_ibm( dst + i * elemsize );
    }

    return segy_native_byteswap( format, size, buf );
}

/*
 * Determine the position of the element `x` in `xs`.
 * Returns -1 if the value cannot be found
 */
static int index_of( int x,
                     const int* xs,
                     int sz ) {
    for( int i = 0; i < sz; i++ ) {
        if( xs[i] == x )
            return i;
    }

    return -1;
}

/*
 * Read the inline or crossline `lineno`. If it's an inline or crossline
 * depends on the parameters. The line has a length of `line_length` traces,
 * `offsets` are the number of offsets in this file, and `buf` must be of
 * (minimum) `line_length*samples_per_trace` size.  Reads every `stride` trace,
 * starting at the trace specified by the *position* of the value `lineno` in
 * `linenos`. If `lineno` isn't present in `linenos`, SEGY_MISSING_LINE_INDEX
 * will be returned.
 *
 * If reading a trace fails, this function will return whatever error
 * segy_readtrace returns.
 */
int segy_read_line( segy_datasource* ds,
                    int line_trace0,
                    int line_length,
                    int stride,
                    int offsets,
                    void* buf,
                    long trace0,
                    int trace_bsize ) {

    char* dst = (char*) buf;
    stride *= offsets;

    for( ; line_length--; line_trace0 += stride, dst += trace_bsize ) {
        int err = segy_readtrace( ds, line_trace0, dst, trace0, trace_bsize );
        if( err != 0 ) return err;
    }

    return SEGY_OK;
}

/*
 * Write the inline or crossline `lineno`. If it's an inline or crossline
 * depends on the parameters. The line has a length of `line_length` traces,
 * and `buf` must be of (minimum) `line_length*samples_per_trace` size.  Reads
 * every `stride` trace, starting at the trace specified by the *position* of
 * the value `lineno` in `linenos`. If `lineno` isn't present in `linenos`,
 * SEGY_MISSING_LINE_INDEX will be returned.
 *
 * If reading a trace fails, this function will return whatever error
 * segy_readtrace returns.
 */
int segy_write_line( segy_datasource* ds,
                     int line_trace0,
                     int line_length,
                     int stride,
                     int offsets,
                     const void* buf,
                     long trace0,
                     int trace_bsize ) {
    if( !ds->writable ) return SEGY_READONLY;

    const char* src = (const char*) buf;
    stride *= offsets;

    for( ; line_length--; line_trace0 += stride, src += trace_bsize ) {
        int err = segy_writetrace( ds, line_trace0, src, trace0, trace_bsize );
        if( err != 0 ) return err;
    }

    return SEGY_OK;
}

int segy_line_trace0( int lineno,
                      int line_length,
                      int stride,
                      int offsets,
                      const int* linenos,
                      int linenos_sz,
                      int* traceno ) {

    int index = index_of( lineno, linenos, linenos_sz );

    if( index < 0 ) return SEGY_MISSING_LINE_INDEX;

    if( stride == 1 ) index *= line_length;

    *traceno = index * offsets;

    return SEGY_OK;
}

int segy_inline_stride( int sorting,
                        int inline_count,
                        int* stride ) {
    switch( sorting ) {
        case SEGY_CROSSLINE_SORTING:
            *stride = inline_count;
            return SEGY_OK;

        case SEGY_INLINE_SORTING:
            *stride = 1;
            return SEGY_OK;

        default:
            return SEGY_INVALID_SORTING;
    }
}

int segy_crossline_stride( int sorting,
                           int crossline_count,
                           int* stride ) {
    switch( sorting ) {
        case SEGY_CROSSLINE_SORTING:
            *stride = 1;
            return SEGY_OK;

        case SEGY_INLINE_SORTING:
            *stride = crossline_count;
            return SEGY_OK;

        default:
            return SEGY_INVALID_SORTING;
    }
}

int segy_read_textheader( segy_datasource* ds, char *buf) {
    return segy_read_ext_textheader(ds, -1, buf );
}

int segy_read_ext_textheader( segy_datasource* ds, int pos, char *buf) {
    if( pos < -1 ) return SEGY_INVALID_ARGS;
    if( !ds ) return SEGY_FSEEK_ERROR;

    const long offset = pos == -1 ? 0 :
                        SEGY_TEXT_HEADER_SIZE + SEGY_BINARY_HEADER_SIZE +
                        (pos * SEGY_TEXT_HEADER_SIZE);

    int err = ds->seek( ds, offset, SEEK_SET );
    if( err != 0 ) return SEGY_DS_SEEK_ERROR;

    char localbuf[SEGY_TEXT_HEADER_SIZE + 1] = { 0 };
    err = ds->read( ds, localbuf, SEGY_TEXT_HEADER_SIZE );
    if( err != 0 ) return SEGY_DS_READ_ERROR;

    encode( buf, localbuf, e2a, SEGY_TEXT_HEADER_SIZE );
    return SEGY_OK;
}

int segy_write_textheader( segy_datasource* ds, int pos, const char* buf ) {
    if( !ds->writable ) return SEGY_READONLY;

    int err;
    char mbuf[ SEGY_TEXT_HEADER_SIZE ];

    if( pos < 0 ) return SEGY_INVALID_ARGS;

    encode( mbuf, buf, a2e, SEGY_TEXT_HEADER_SIZE );

    const long offset = pos == 0
                      ? 0
                      : SEGY_TEXT_HEADER_SIZE + SEGY_BINARY_HEADER_SIZE +
                        ((pos-1) * SEGY_TEXT_HEADER_SIZE);

    err = ds->seek( ds, offset, SEEK_SET );
    if( err != 0 ) return SEGY_DS_SEEK_ERROR;

    err = ds->write( ds, mbuf, SEGY_TEXT_HEADER_SIZE );
    if( err != 0 ) return SEGY_DS_WRITE_ERROR;

    return SEGY_OK;
}

int segy_textheader_size( void ) {
    return SEGY_TEXT_HEADER_SIZE + 1;
}

int segy_binheader_size( void ) {
    return SEGY_BINARY_HEADER_SIZE;
}

static int scaled_cdp( segy_datasource* ds,
                       int traceno,
                       float* cdpx,
                       float* cdpy,
                       long trace0,
                       int trace_bsize ) {
    int x, y;
    int scalar;
    char trheader[ SEGY_TRACE_HEADER_SIZE ];

    int err = segy_traceheader( ds, traceno, trheader, trace0, trace_bsize );
    if( err != 0 ) return err;

    err = segy_get_field_int( trheader, SEGY_TR_CDP_X, &x );
    if( err != 0 ) return err;
    err = segy_get_field_int( trheader, SEGY_TR_CDP_Y, &y );
    if( err != 0 ) return err;
    err = segy_get_field_int( trheader, SEGY_TR_SOURCE_GROUP_SCALAR, &scalar );
    if( err != 0 ) return err;

    float scale = (float) scalar;
    if( scalar == 0 ) scale = 1.0;
    if( scalar < 0 )  scale = -1.0f / scale;

    *cdpx = x * scale;
    *cdpy = y * scale;

    return SEGY_OK;
}

int segy_rotation_cw( segy_datasource* ds,
                      int line_length,
                      int stride,
                      int offsets,
                      const int* linenos,
                      int linenos_sz,
                      float* rotation,
                      long trace0,
                      int trace_bsize ) {

    struct coord { float x, y; } nw, sw;

    int err;
    int traceno;
    err = segy_line_trace0( linenos[0], line_length,
                                        stride,
                                        offsets,
                                        linenos,
                                        linenos_sz,
                                        &traceno );
    if( err != 0 ) return err;

    err = scaled_cdp( ds, traceno, &sw.x, &sw.y, trace0, trace_bsize );
    if( err != 0 ) return err;

    /* read the last trace in the line */
    traceno += (line_length - 1) * stride * offsets;
    err = scaled_cdp( ds, traceno, &nw.x, &nw.y, trace0, trace_bsize );
    if( err != 0 ) return err;

    float x = nw.x - sw.x;
    float y = nw.y - sw.y;
    double radians = x || y ? atan2( x, y ) : 0;
    if( radians < 0 ) radians += 2 * acos(-1);

    *rotation = (float) radians;
    return SEGY_OK;
}
