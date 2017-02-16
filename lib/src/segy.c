#ifdef HAVE_MMAP
  #define _POSIX_SOURCE
  #include <sys/mman.h>
#endif //HAVE_MMAP

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#elif HAVE_ARPA_INET_H
#include <arpa/inet.h>
#elif HAVE_WINSOCK2_H
#include <winsock2.h>
#endif

#ifdef HAVE_SYS_STAT_H
  #include <sys/types.h>
  #include <sys/stat.h>
#endif //HAVE_SYS_STAT_H

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <segyio/segy.h>
#include <segyio/util.h>

static unsigned char a2e[256] = {
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

static unsigned char e2a[256] = {
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

void ebcdic2ascii( const char* ebcdic, char* ascii ) {
    while( *ebcdic != '\0' )
        *ascii++ = (char)e2a[ (unsigned char) *ebcdic++ ];

    *ascii = '\0';
}

void ascii2ebcdic( const char* ascii, char* ebcdic ) {
    while (*ascii != '\0')
        *ebcdic++ = (char)a2e[(unsigned char) *ascii++];

    *ebcdic = '\0';
}

void ibm2ieee( void* to, const void* from ) {
    uint32_t fr;      /* fraction */
    int exp;          /* exponent */
    uint32_t sgn;     /* sign */

    memcpy( &fr, from, sizeof( uint32_t ) );
    /* split into sign, exponent, and fraction */
    fr = ntohl( fr ); /* pick up value */
    sgn = fr >> 31; /* save sign */
    fr <<= 1; /* shift sign out */
    exp = fr >> 25; /* save exponent */
    fr <<= 7; /* shift exponent out */

    if (fr == 0) { /* short-circuit for zero */
        exp = 0;
        goto done;
    }

    /* adjust exponent from base 16 offset 64 radix point before first digit
     * to base 2 offset 127 radix point after first digit
     * (exp - 64) * 4 + 127 - 1 == exp * 4 - 256 + 126 == (exp << 2) - 130 */
    exp = (exp << 2) - 130;

    /* (re)normalize */
    while (fr < 0x80000000) { /* 3 times max for normalized input */
        --exp;
        fr <<= 1;
    }

    if (exp <= 0) { /* underflow */
        if (exp < -24) /* complete underflow - return properly signed zero */
            fr = 0;
        else /* partial underflow - return denormalized number */
            fr >>= -exp;
        exp = 0;
    } else if (exp >= 255) { /* overflow - return infinity */
        fr = 0;
        exp = 255;
    } else { /* just a plain old number - remove the assumed high bit */
        fr <<= 1;
    }

done:
    /* put the pieces back together and return it */
    fr = (fr >> 9) | (exp << 23) | (sgn << 31);
    memcpy( to, &fr, sizeof( uint32_t ) );
}

void ieee2ibm( void* to, const void* from ) {
    uint32_t fr;      /* fraction */
    int exp;          /* exponent */
    uint32_t sgn;     /* sign */

    /* split into sign, exponent, and fraction */
    memcpy( &fr, from, sizeof( uint32_t ) ); /* pick up value */
    sgn = fr >> 31; /* save sign */
    fr <<= 1; /* shift sign out */
    exp = fr >> 24; /* save exponent */
    fr <<= 8; /* shift exponent out */

    if (exp == 255) { /* infinity (or NAN) - map to largest */
        fr = 0xffffff00;
        exp = 0x7f;
        goto done;
    }
    else if (exp > 0) /* add assumed digit */
        fr = (fr >> 1) | 0x80000000;
    else if (fr == 0) /* short-circuit for zero */
        goto done;

    /* adjust exponent from base 2 offset 127 radix point after first digit
     * to base 16 offset 64 radix point before first digit */
    exp += 130;
    fr >>= -exp & 3;
    exp = (exp + 3) >> 2;

    /* (re)normalize */
    while (fr < 0x10000000) { /* never executed for normalized input */
        --exp;
        fr <<= 4;
    }

done:
    /* put the pieces back together and return it */
    fr = htonl( (fr >> 8) | (exp << 24) | (sgn << 31) );
    memcpy( to, &fr, sizeof( uint32_t ) );
}

/* Lookup table for field sizes. All values not explicitly set are 0 */
static int field_size[] = {
    [SEGY_TR_CDP_X                  ] = 4,
    [SEGY_TR_CDP_Y                  ] = 4,
    [SEGY_TR_CROSSLINE              ] = 4,
    [SEGY_TR_ENERGY_SOURCE_POINT    ] = 4,
    [SEGY_TR_ENSEMBLE               ] = 4,
    [SEGY_TR_FIELD_RECORD           ] = 4,
    [SEGY_TR_GROUP_WATER_DEPTH      ] = 4,
    [SEGY_TR_GROUP_X                ] = 4,
    [SEGY_TR_GROUP_Y                ] = 4,
    [SEGY_TR_INLINE                 ] = 4,
    [SEGY_TR_NUMBER_ORIG_FIELD      ] = 4,
    [SEGY_TR_NUM_IN_ENSEMBLE        ] = 4,
    [SEGY_TR_OFFSET                 ] = 4,
    [SEGY_TR_RECV_DATUM_ELEV        ] = 4,
    [SEGY_TR_RECV_GROUP_ELEV        ] = 4,
    [SEGY_TR_SEQ_FILE               ] = 4,
    [SEGY_TR_SEQ_LINE               ] = 4,
    [SEGY_TR_SHOT_POINT             ] = 4,
    [SEGY_TR_SOURCE_DATUM_ELEV      ] = 4,
    [SEGY_TR_SOURCE_DEPTH           ] = 4,
    [SEGY_TR_SOURCE_ENERGY_DIR_MANT ] = 4,
    [SEGY_TR_SOURCE_MEASURE_MANT    ] = 4,
    [SEGY_TR_SOURCE_SURF_ELEV       ] = 4,
    [SEGY_TR_SOURCE_X               ] = 4,
    [SEGY_TR_SOURCE_Y               ] = 4,
    [SEGY_TR_TRANSDUCTION_MANT      ] = 4,
    [SEGY_TR_UNASSIGNED1            ] = 4,
    [SEGY_TR_UNASSIGNED2            ] = 4,

    [SEGY_TR_ALIAS_FILT_FREQ        ] = 2,
    [SEGY_TR_ALIAS_FILT_SLOPE       ] = 2,
    [SEGY_TR_COORD_UNITS            ] = 2,
    [SEGY_TR_CORRELATED             ] = 2,
    [SEGY_TR_DATA_USE               ] = 2,
    [SEGY_TR_DAY_OF_YEAR            ] = 2,
    [SEGY_TR_DELAY_REC_TIME         ] = 2,
    [SEGY_TR_DEVICE_ID              ] = 2,
    [SEGY_TR_ELEV_SCALAR            ] = 2,
    [SEGY_TR_GAIN_TYPE              ] = 2,
    [SEGY_TR_GAP_SIZE               ] = 2,
    [SEGY_TR_GEOPHONE_GROUP_FIRST   ] = 2,
    [SEGY_TR_GEOPHONE_GROUP_LAST    ] = 2,
    [SEGY_TR_GEOPHONE_GROUP_ROLL1   ] = 2,
    [SEGY_TR_GROUP_STATIC_CORR      ] = 2,
    [SEGY_TR_GROUP_UPHOLE_TIME      ] = 2,
    [SEGY_TR_HIGH_CUT_FREQ          ] = 2,
    [SEGY_TR_HIGH_CUT_SLOPE         ] = 2,
    [SEGY_TR_HOUR_OF_DAY            ] = 2,
    [SEGY_TR_INSTR_GAIN_CONST       ] = 2,
    [SEGY_TR_INSTR_INIT_GAIN        ] = 2,
    [SEGY_TR_LAG_A                  ] = 2,
    [SEGY_TR_LAG_B                  ] = 2,
    [SEGY_TR_LOW_CUT_FREQ           ] = 2,
    [SEGY_TR_LOW_CUT_SLOPE          ] = 2,
    [SEGY_TR_MEASURE_UNIT           ] = 2,
    [SEGY_TR_MIN_OF_HOUR            ] = 2,
    [SEGY_TR_MUTE_TIME_END          ] = 2,
    [SEGY_TR_MUTE_TIME_START        ] = 2,
    [SEGY_TR_NOTCH_FILT_FREQ        ] = 2,
    [SEGY_TR_NOTCH_FILT_SLOPE       ] = 2,
    [SEGY_TR_OVER_TRAVEL            ] = 2,
    [SEGY_TR_SAMPLE_COUNT           ] = 2,
    [SEGY_TR_SAMPLE_INTER           ] = 2,
    [SEGY_TR_SCALAR_TRACE_HEADER    ] = 2,
    [SEGY_TR_SEC_OF_MIN             ] = 2,
    [SEGY_TR_SHOT_POINT_SCALAR      ] = 2,
    [SEGY_TR_SOURCE_ENERGY_DIR_EXP  ] = 2,
    [SEGY_TR_SOURCE_GROUP_SCALAR    ] = 2,
    [SEGY_TR_SOURCE_MEASURE_EXP     ] = 2,
    [SEGY_TR_SOURCE_MEASURE_UNIT    ] = 2,
    [SEGY_TR_SOURCE_STATIC_CORR     ] = 2,
    [SEGY_TR_SOURCE_TYPE            ] = 2,
    [SEGY_TR_SOURCE_UPHOLE_TIME     ] = 2,
    [SEGY_TR_SOURCE_WATER_DEPTH     ] = 2,
    [SEGY_TR_STACKED_TRACES         ] = 2,
    [SEGY_TR_SUBWEATHERING_VELO     ] = 2,
    [SEGY_TR_SUMMED_TRACES          ] = 2,
    [SEGY_TR_SWEEP_FREQ_END         ] = 2,
    [SEGY_TR_SWEEP_FREQ_START       ] = 2,
    [SEGY_TR_SWEEP_LENGTH           ] = 2,
    [SEGY_TR_SWEEP_TAPERLEN_END     ] = 2,
    [SEGY_TR_SWEEP_TAPERLEN_START   ] = 2,
    [SEGY_TR_SWEEP_TYPE             ] = 2,
    [SEGY_TR_TAPER_TYPE             ] = 2,
    [SEGY_TR_TIME_BASE_CODE         ] = 2,
    [SEGY_TR_TOT_STATIC_APPLIED     ] = 2,
    [SEGY_TR_TRACE_ID               ] = 2,
    [SEGY_TR_TRANSDUCTION_EXP       ] = 2,
    [SEGY_TR_TRANSDUCTION_UNIT      ] = 2,
    [SEGY_TR_WEATHERING_VELO        ] = 2,
    [SEGY_TR_WEIGHTING_FAC          ] = 2,
    [SEGY_TR_YEAR_DATA_REC          ] = 2,
};

#define HEADER_SIZE SEGY_TEXT_HEADER_SIZE

/*
 * Supporting same byte offsets as in the segy specification, i.e. from the
 * start of the *text header*, not the binary header.
 */
static int bfield_size[] = {
    [- HEADER_SIZE + SEGY_BIN_JOB_ID                ] = 4,
    [- HEADER_SIZE + SEGY_BIN_LINE_NUMBER           ] = 4,
    [- HEADER_SIZE + SEGY_BIN_REEL_NUMBER           ] = 4,

    [- HEADER_SIZE + SEGY_BIN_TRACES                ] = 2,
    [- HEADER_SIZE + SEGY_BIN_AUX_TRACES            ] = 2,
    [- HEADER_SIZE + SEGY_BIN_INTERVAL              ] = 2,
    [- HEADER_SIZE + SEGY_BIN_INTERVAL_ORIG         ] = 2,
    [- HEADER_SIZE + SEGY_BIN_SAMPLES               ] = 2,
    [- HEADER_SIZE + SEGY_BIN_SAMPLES_ORIG          ] = 2,
    [- HEADER_SIZE + SEGY_BIN_FORMAT                ] = 2,
    [- HEADER_SIZE + SEGY_BIN_ENSEMBLE_FOLD         ] = 2,
    [- HEADER_SIZE + SEGY_BIN_SORTING_CODE          ] = 2,
    [- HEADER_SIZE + SEGY_BIN_VERTICAL_SUM          ] = 2,
    [- HEADER_SIZE + SEGY_BIN_SWEEP_FREQ_START      ] = 2,
    [- HEADER_SIZE + SEGY_BIN_SWEEP_FREQ_END        ] = 2,
    [- HEADER_SIZE + SEGY_BIN_SWEEP_LENGTH          ] = 2,
    [- HEADER_SIZE + SEGY_BIN_SWEEP                 ] = 2,
    [- HEADER_SIZE + SEGY_BIN_SWEEP_CHANNEL         ] = 2,
    [- HEADER_SIZE + SEGY_BIN_SWEEP_TAPER_START     ] = 2,
    [- HEADER_SIZE + SEGY_BIN_SWEEP_TAPER_END       ] = 2,
    [- HEADER_SIZE + SEGY_BIN_TAPER                 ] = 2,
    [- HEADER_SIZE + SEGY_BIN_CORRELATED_TRACES     ] = 2,
    [- HEADER_SIZE + SEGY_BIN_BIN_GAIN_RECOVERY     ] = 2,
    [- HEADER_SIZE + SEGY_BIN_AMPLITUDE_RECOVERY    ] = 2,
    [- HEADER_SIZE + SEGY_BIN_MEASUREMENT_SYSTEM    ] = 2,
    [- HEADER_SIZE + SEGY_BIN_IMPULSE_POLARITY      ] = 2,
    [- HEADER_SIZE + SEGY_BIN_VIBRATORY_POLARITY    ] = 2,
    [- HEADER_SIZE + SEGY_BIN_SEGY_REVISION         ] = 2,
    [- HEADER_SIZE + SEGY_BIN_TRACE_FLAG            ] = 2,
    [- HEADER_SIZE + SEGY_BIN_EXT_HEADERS           ] = 2,

    [- HEADER_SIZE + SEGY_BIN_UNASSIGNED1           ] = 0,
    [- HEADER_SIZE + SEGY_BIN_UNASSIGNED2           ] = 0,
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
#ifdef HAVE_FSTATI64
    // this means we're on windows where fstat is unreliable for filesizes >2G
    // because long is only 4 bytes
    struct _stati64 st;
    const int err = _fstati64( fileno( fp ), &st );
#else
    struct stat st;
    const int err = fstat( fileno( fp ), &st );
#endif

    if( err != 0 ) return SEGY_FSEEK_ERROR;
    *size = st.st_size;
    return SEGY_OK;
}
#endif //HAVE_SYS_STAT_H

/*
 * addr is NULL if mmap is not found under compilation or if the file is
 * not requested mmap'd. If so, the fallback code path of FILE* is taken
 */
struct segy_file_handle {
    void* addr;
    void* cur;
    FILE* fp;
    size_t fsize;
    char mode[ 4 ];
};

segy_file* segy_open( const char* path, const char* mode ) {

    if( !path || !mode ) return NULL;

    // append a 'b' if it is not passed by the user; not a problem on unix, but
    // windows and other platforms fail without it
    char binary_mode[ 4 ] = { 0 };
    strncpy( binary_mode, mode, 3 );

    size_t mode_len = strlen( binary_mode );
    if( binary_mode[ mode_len - 1 ] != 'b' ) binary_mode[ mode_len ] = 'b';

     // Account for invalid mode. On unix this is fine, but windows crashes the
     // process if mode is invalid
    if( !strstr( "rb" "wb" "ab" "r+b" "w+b" "a+b", binary_mode ) )
        return NULL;

    FILE* fp = fopen( path, binary_mode );

    if( !fp ) return NULL;

    segy_file* file = calloc( 1, sizeof( segy_file ) );

    if( !file ) {
        fclose( fp );
        return NULL;
    }

    file->fp = fp;
    strcpy( file->mode, binary_mode );

    return file;
}

int segy_mmap( segy_file* fp ) {
#ifndef HAVE_MMAP
    return SEGY_MMAP_INVALID;
#else

    long long fsize;
    int err = file_size( fp->fp, &fsize );

    if( err != 0 ) return SEGY_FSEEK_ERROR;
    fp->fsize = fsize;

    bool rw = strstr( fp->mode, "+" ) || strstr( fp->mode, "w" );
    const int prot =  rw ? PROT_READ | PROT_WRITE : PROT_READ;

    int fd = fileno( fp->fp );
    void* addr = mmap( NULL, fp->fsize, prot, MAP_SHARED, fd, 0 );

    if( addr == MAP_FAILED )
        return SEGY_MMAP_ERROR;

    fp->addr = fp->cur = addr;
    return SEGY_OK;
#endif //HAVE_MMAP
}

int segy_flush( segy_file* fp, bool async ) {
    int syncerr = 0;

#ifdef HAVE_MMAP
    if( fp->addr ) {
        int flag = async ? MS_ASYNC : MS_SYNC;
        syncerr = msync( fp->addr, fp->fsize, flag );
    }
#endif //HAVE_MMAP

    if( syncerr != 0 ) return syncerr;

    int flusherr = fflush( fp->fp );

    if( flusherr != 0 ) return SEGY_FWRITE_ERROR;

    return SEGY_OK;
}

long long segy_ftell( segy_file* fp ) {
#ifdef HAVE_FSTATI64
    // assuming we're on windows. This function is a little rough, but only
    // meant for testing - it's not a part of the public interface.
    return _ftelli64( fp->fp );
#else
    assert( sizeof( long ) == sizeof( long long ) );
    return ftell( fp->fp );
#endif
}

int segy_close( segy_file* fp ) {
    int err = segy_flush( fp, false );

#ifdef HAVE_MMAP
    if( !fp->addr ) goto no_mmap;

    err = munmap( fp->addr, fp->fsize );
    if( err != 0 )
        err = SEGY_MMAP_ERROR;

no_mmap:
#endif //HAVE_MMAP

    fclose( fp->fp );
    free( fp );
    return err;
}

static int get_field( const char* header,
                      const int* table,
                      int field,
                      int32_t* f ) {

    const int bsize = table[ field ];
    uint32_t buf32 = 0;
    uint16_t buf16 = 0;

    switch( bsize ) {
        case 4:
            memcpy( &buf32, header + (field - 1), 4 );
            *f = (int32_t)ntohl( buf32 );
            return SEGY_OK;

        case 2:
            memcpy( &buf16, header + (field - 1), 2 );
            *f = (int16_t)ntohs( buf16 );
            return SEGY_OK;

        case 0:
        default:
            return SEGY_INVALID_FIELD;
    }
}

int segy_get_field( const char* traceheader, int field, int* f ) {
    if( field < 0 || field >= SEGY_TRACE_HEADER_SIZE )
        return SEGY_INVALID_FIELD;

    return get_field( traceheader, field_size, field, f );
}

int segy_get_bfield( const char* binheader, int field, int32_t* f ) {
    field -= SEGY_TEXT_HEADER_SIZE;

    if( field < 0 || field >= SEGY_BINARY_HEADER_SIZE )
        return SEGY_INVALID_FIELD;

    return get_field( binheader, bfield_size, field, f );
}

static int set_field( char* header, const int* table, int field, int32_t val ) {
    const int bsize = table[ field ];

    uint32_t buf32;
    uint16_t buf16;

    switch( bsize ) {
        case 4:
            buf32 = htonl( (uint32_t)val );
            memcpy( header + (field - 1), &buf32, sizeof( buf32 ) );
            return SEGY_OK;

        case 2:
            buf16 = htons( (uint16_t)val );
            memcpy( header + (field - 1), &buf16, sizeof( buf16 ) );
            return SEGY_OK;

        case 0:
        default:
            return SEGY_INVALID_FIELD;
    }
}

int segy_set_field( char* traceheader, int field, int val ) {
    if( field < 0 || field >= SEGY_TRACE_HEADER_SIZE )
        return SEGY_INVALID_FIELD;

    return set_field( traceheader, field_size, field, val );
}

int segy_set_bfield( char* binheader, int field, int val ) {
    field -= SEGY_TEXT_HEADER_SIZE;

    if( field < 0 || field >= SEGY_BINARY_HEADER_SIZE )
        return SEGY_INVALID_FIELD;

    return set_field( binheader, bfield_size, field, val );
}

int segy_binheader( segy_file* fp, char* buf ) {
    if(fp == NULL) {
        return SEGY_INVALID_ARGS;
    }

    const int err = fseek( fp->fp, SEGY_TEXT_HEADER_SIZE, SEEK_SET );
    if( err != 0 ) return SEGY_FSEEK_ERROR;

    const size_t read_count = fread( buf, 1, SEGY_BINARY_HEADER_SIZE, fp->fp );
    if( read_count != SEGY_BINARY_HEADER_SIZE )
        return SEGY_FREAD_ERROR;

    return SEGY_OK;
}

int segy_write_binheader( segy_file* fp, const char* buf ) {
    if(fp == NULL) {
        return SEGY_INVALID_ARGS;
    }

    const int err = fseek( fp->fp, SEGY_TEXT_HEADER_SIZE, SEEK_SET );
    if( err != 0 ) return SEGY_FSEEK_ERROR;

    const size_t writec = fwrite( buf, 1, SEGY_BINARY_HEADER_SIZE, fp->fp );
    if( writec != SEGY_BINARY_HEADER_SIZE )
        return SEGY_FWRITE_ERROR;

    return SEGY_OK;
}

int segy_format( const char* buf ) {
    int format;
    segy_get_bfield( buf, SEGY_BIN_FORMAT, &format );
    return format;
}

int segy_samples( const char* buf ) {
    int32_t samples;
    segy_get_bfield( buf, SEGY_BIN_SAMPLES, &samples );
    return samples;
}

int segy_trace_bsize( int samples ) {
    assert( samples >= 0 );
    /* Hard four-byte float assumption */
    return samples * 4;
}

long segy_trace0( const char* binheader ) {
    int extra_headers;
    segy_get_bfield( binheader, SEGY_BIN_EXT_HEADERS, &extra_headers );

    return SEGY_TEXT_HEADER_SIZE + SEGY_BINARY_HEADER_SIZE +
           SEGY_TEXT_HEADER_SIZE * extra_headers;
}

int segy_seek( segy_file* fp,
               int trace,
               long trace0,
               int trace_bsize ) {

    trace_bsize += SEGY_TRACE_HEADER_SIZE;
    long long pos = (long long)trace0 + (trace * (long long)trace_bsize);

    if( fp->addr ) {
        if( (size_t)pos >= fp->fsize ) return SEGY_FSEEK_ERROR;

        fp->cur = (char*)fp->addr + pos;
        return SEGY_OK;
    }

    int err = SEGY_OK;
    if( sizeof( long ) == sizeof( long long ) ) {
        err = fseek( fp->fp, pos, SEEK_SET );
    } else {
        /*
         * If long is 32bit on our platform (hello, windows), we do skips according
         * to LONG_MAX and seek relative to our cursor rather than absolute on file
         * begin.
         */
        rewind( fp->fp );
        while( pos >= LONG_MAX && err == SEGY_OK ) {
            err = fseek( fp->fp, LONG_MAX, SEEK_CUR );
            pos -= LONG_MAX;
        }

        if( err != 0 ) return SEGY_FSEEK_ERROR;

        err = fseek( fp->fp, pos, SEEK_CUR );
    }

    if( err != 0 ) return SEGY_FSEEK_ERROR;
    return SEGY_OK;
}

int segy_traceheader( segy_file* fp,
                      int traceno,
                      char* buf,
                      long trace0,
                      int trace_bsize ) {

    const int err = segy_seek( fp, traceno, trace0, trace_bsize );
    if( err != 0 ) return err;

    if( fp->addr ) {
        memcpy( buf, fp->cur, SEGY_TRACE_HEADER_SIZE );
        return SEGY_OK;
    }

    const size_t readc = fread( buf, 1, SEGY_TRACE_HEADER_SIZE, fp->fp );

    if( readc != SEGY_TRACE_HEADER_SIZE )
        return SEGY_FREAD_ERROR;

    return SEGY_OK;
}

int segy_write_traceheader( segy_file* fp,
                            int traceno,
                            const char* buf,
                            long trace0,
                            int trace_bsize ) {

    const int err = segy_seek( fp, traceno, trace0, trace_bsize );
    if( err != 0 ) return err;

    if( fp->addr ) {
        memcpy( fp->cur, buf, SEGY_TRACE_HEADER_SIZE );
        return SEGY_OK;
    }

    const size_t writec = fwrite( buf, 1, SEGY_TRACE_HEADER_SIZE, fp->fp );

    if( writec != SEGY_TRACE_HEADER_SIZE )
        return SEGY_FWRITE_ERROR;

    return SEGY_OK;
}

/*
 * Return the number of traces in the file. The file pointer won't change after
 * this call unless fseek itself fails.
 *
 * This function assumes that *all traces* are of the same size.
 */
int segy_traces( segy_file* fp,
                 int* traces,
                 long trace0,
                 int trace_bsize ) {

    long long fsize;
    int err = file_size( fp->fp, &fsize );
    if( err != 0 ) return err;

    if( trace0 > fsize ) return SEGY_INVALID_ARGS;

    trace_bsize += SEGY_TRACE_HEADER_SIZE;
    const int trace_data_size = fsize - trace0;

    if( trace_data_size % trace_bsize != 0 )
        return SEGY_TRACE_SIZE_MISMATCH;

    *traces = trace_data_size / trace_bsize;
    return SEGY_OK;
}

int segy_sample_interval( segy_file* fp, float* dt ) {

    char bin_header[ SEGY_BINARY_HEADER_SIZE ];
    char trace_header[ SEGY_TRACE_HEADER_SIZE ];

    int err = segy_binheader( fp, bin_header );
    if (err != 0) {
        return err;
    }

    const long trace0 = segy_trace0( bin_header );
    int samples = segy_samples( bin_header );
    const int trace_bsize = segy_trace_bsize( samples );

    err = segy_traceheader(fp, 0, trace_header, trace0, trace_bsize);
    if (err != 0) {
        return err;
    }

    // microseconds: us
    int binary_header_dt_us;
    int trace_header_dt_us;

    segy_get_bfield(bin_header, SEGY_BIN_INTERVAL, &binary_header_dt_us);
    segy_get_field(trace_header, SEGY_TR_SAMPLE_INTER, &trace_header_dt_us);

    // milliseconds: ms
    double binary_header_dt_ms = binary_header_dt_us/1000.0;
    double trace_header_dt_ms = trace_header_dt_us/1000.0;

    if (trace_header_dt_us==0 && binary_header_dt_us==0) {
        //noop
    } else if (binary_header_dt_us == 0) {
        *dt = trace_header_dt_ms;
    } else if (trace_header_dt_us == 0) {
        *dt = binary_header_dt_ms;
    } else if (trace_header_dt_us == binary_header_dt_us) {
        *dt = trace_header_dt_ms;
    }

    return 0;

}

int segy_sample_indices( segy_file* fp,
                         float t0,
                         float dt,
                         int count,
                         float* buf ) {

    int err = segy_sample_interval(fp, &dt);
    if (err != 0) {
        return err;
    }

    for( int i = 0; i < count; i++ ) {
        buf[i] = t0 + i * dt;
    }

    return SEGY_OK;
}

/*
 * Determine how a file is sorted. Expects the following two fields from the
 * trace header to guide sorting: the inline number `il` and the crossline
 * number `xl`.
 *
 * Inspects trace headers 0 and 1 and compares these two fields in the
 * respective trace headers. If the first two traces are components of the same
 * inline, header[0].ilnum should be equal to header[1].ilnum, similarly for
 * crosslines. If neither match, the sorting is considered unknown.
 */
int segy_sorting( segy_file* fp,
                  int il,
                  int xl,
                  int* sorting,
                  long trace0,
                  int trace_bsize ) {
    int err;
    char traceheader[ SEGY_TRACE_HEADER_SIZE ];

    err = segy_traceheader( fp, 0, traceheader, trace0, trace_bsize );
    if( err != SEGY_OK ) return err;

    if( il < 0 || il >= SEGY_TRACE_HEADER_SIZE )
        return SEGY_INVALID_FIELD;

    if( xl < 0 || xl >= SEGY_TRACE_HEADER_SIZE )
        return SEGY_INVALID_FIELD;

    /* make sure field is valid, so we don't have to check errors later */
    if( field_size[ il ] == 0 || field_size[ xl ] == 0 )
        return SEGY_INVALID_FIELD;

    int il0, xl0, il1, xl1, off0, off1;

    segy_get_field( traceheader, il, &il0 );
    segy_get_field( traceheader, xl, &xl0 );
    segy_get_field( traceheader, SEGY_TR_OFFSET, &off0 );

    int traces;
    err = segy_traces( fp, &traces, trace0, trace_bsize );
    if( err != 0 ) return err;
    int traceno = 1;

    do {
        err = segy_traceheader( fp, traceno, traceheader, trace0, trace_bsize );
        if( err != SEGY_OK ) return err;

        segy_get_field( traceheader, il, &il1 );
        segy_get_field( traceheader, xl, &xl1 );
        segy_get_field( traceheader, SEGY_TR_OFFSET, &off1 );
        ++traceno;
    } while( off0 != off1 && traceno < traces );

    /*
     * sometimes files come with Mx1, 1xN or even 1x1 geometries. When this is
     * the case we look at the last trace and compare it to the first. If these
     * numbers match we define the sorting direction as the non-1 dimension
     */
    err = segy_traceheader( fp, traces - 1, traceheader, trace0, trace_bsize );
    if( err != SEGY_OK ) return err;

    int il_last, xl_last;
    segy_get_field( traceheader, il, &il_last );
    segy_get_field( traceheader, xl, &xl_last );

    if     ( il0 == il_last ) *sorting = SEGY_CROSSLINE_SORTING;
    else if( xl0 == xl_last ) *sorting = SEGY_INLINE_SORTING;
    else if( il0 == il1 )     *sorting = SEGY_INLINE_SORTING;
    else if( xl0 == xl1 )     *sorting = SEGY_CROSSLINE_SORTING;
    else return SEGY_INVALID_SORTING;

    return SEGY_OK;
}

/*
 * Find the number of offsets. This is determined by inspecting the trace
 * headers [0,n) where n is the first trace where either the inline number or
 * the crossline number changes (which changes first depends on sorting, but is
 * irrelevant for this function).
 */
int segy_offsets( segy_file* fp,
                  int il,
                  int xl,
                  int traces,
                  int* out,
                  long trace0,
                  int trace_bsize ) {
    int err;
    int il0, il1, xl0, xl1;
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
    if( field_size[ il ] == 0 || field_size[ xl ] == 0 )
        return SEGY_INVALID_FIELD;

    err = segy_traceheader( fp, 0, header, trace0, trace_bsize );
    segy_get_field( header, il, &il0 );
    segy_get_field( header, xl, &xl0 );

    do {
        ++offsets;

        if( offsets == traces ) break;

        err = segy_traceheader( fp, offsets, header, trace0, trace_bsize );
        if( err != 0 ) return err;

        segy_get_field( header, il, &il1 );
        segy_get_field( header, xl, &xl1 );
    } while( il0 == il1 && xl0 == xl1 );

    *out = offsets;
    return SEGY_OK;
}

int segy_offset_indices( segy_file* fp,
                         int offset_field,
                         int offsets,
                         int* out,
                         long trace0,
                         int trace_bsize ) {
    int err = 0;
    int32_t x = 0;
    char header[ SEGY_TRACE_HEADER_SIZE ];

    if( field_size[ offset_field ] == 0 )
        return SEGY_INVALID_FIELD;

    for( int i = 0; i < offsets; ++i ) {
        err = segy_traceheader( fp, i, header, trace0, trace_bsize );
        if( err != SEGY_OK ) return err;

        segy_get_field( header, offset_field, &x );
        *out++ = x;
    }

    return SEGY_OK;
}

static int segy_line_indices( segy_file* fp,
                              int field,
                              int traceno,
                              int stride,
                              int num_indices,
                              int* buf,
                              long trace0,
                              int trace_bsize ) {

    if( field_size[ field ] == 0 )
        return SEGY_INVALID_FIELD;

    char header[ SEGY_TRACE_HEADER_SIZE ];
    for( ; num_indices--; traceno += stride, ++buf ) {

        int err = segy_traceheader( fp, traceno, header, trace0, trace_bsize );
        if( err != 0 ) return SEGY_FREAD_ERROR;

        segy_get_field( header, field, buf );
    }

    return SEGY_OK;
}

static int count_lines( segy_file* fp,
                        int field,
                        int offsets,
                        int* out,
                        long trace0,
                        int trace_bsize ) {

    int err;
    char header[ SEGY_TRACE_HEADER_SIZE ];
    err = segy_traceheader( fp, 0, header, trace0, trace_bsize );
    if( err != 0 ) return err;

    int first_lineno, first_offset, ln, off;

    err = segy_get_field( header, field, &first_lineno );
    if( err != 0 ) return err;

    err = segy_get_field( header, 37, &first_offset );
    if( err != 0 ) return err;

    int lines = 1;
    int curr = offsets;

    while( true ) {
        err = segy_traceheader( fp, curr, header, trace0, trace_bsize );
        if( err != 0 ) return err;

        segy_get_field( header, field, &ln );
        segy_get_field( header, 37, &off );

        if( first_offset == off && ln == first_lineno ) break;

        curr += offsets;
        ++lines;
    }

    *out = lines;
    return SEGY_OK;
}

int segy_count_lines( segy_file* fp,
                      int field,
                      int offsets,
                      int* l1out,
                      int* l2out,
                      long trace0,
                      int trace_bsize ) {

    int err;
    int l2count;
    err = count_lines( fp, field, offsets, &l2count, trace0, trace_bsize );
    if( err != 0 ) return err;

    int traces;
    err = segy_traces( fp, &traces, trace0, trace_bsize );
    if( err != 0 ) return err;

    const int line_length = l2count * offsets;
    const int l1count = traces / line_length;

    *l1out = l1count;
    *l2out = l2count;

    return SEGY_OK;
}

int segy_lines_count( segy_file* fp,
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

    int err = segy_count_lines( fp, field, offsets,
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

int segy_inline_indices( segy_file* fp,
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
        return segy_line_indices( fp, il, 0, stride, inline_count, buf, trace0, trace_bsize );
    }

    if( sorting == SEGY_CROSSLINE_SORTING ) {
        return segy_line_indices( fp, il, 0, offsets, inline_count, buf, trace0, trace_bsize );
    }

    return SEGY_INVALID_SORTING;
}

int segy_crossline_indices( segy_file* fp,
                            int xl,
                            int sorting,
                            int inline_count,
                            int crossline_count,
                            int offsets,
                            int* buf,
                            long trace0,
                            int trace_bsize ) {

    if( sorting == SEGY_INLINE_SORTING ) {
        return segy_line_indices( fp, xl, 0, offsets, crossline_count, buf, trace0, trace_bsize );
    }

    if( sorting == SEGY_CROSSLINE_SORTING ) {
        int stride = inline_count * offsets;
        return segy_line_indices( fp, xl, 0, stride, crossline_count, buf, trace0, trace_bsize );
    }

    return SEGY_INVALID_SORTING;
}

static inline int subtr_seek( segy_file* fp,
                              int traceno,
                              int fst,
                              int lst,
                              long trace0,
                              int trace_bsize ) {
    /*
     * Optimistically assume that indices are correct by the time they're given
     * to subtr_seek.
     */
    assert( fst > lst || fst < 0 );
    assert( sizeof( float ) == 4 );
    assert( (lst - fst) * sizeof( float ) <= trace_bsize );

    int err;
    // skip the trace header and skip everything before fst.
    trace0 += (fst * sizeof( float )) + SEGY_TRACE_HEADER_SIZE;
    return segy_seek( fp, traceno, trace0, trace_bsize );
}


int segy_readtrace( segy_file* fp,
                    int traceno,
                    float* buf,
                    long trace0,
                    int trace_bsize ) {
    const int lst = trace_bsize / sizeof( float );
    return segy_readsubtr( fp, traceno, 0, lst, buf, trace0, trace_bsize );
}

int segy_readsubtr( segy_file* fp,
                    int traceno,
                    int fst,
                    int lst,
                    float* buf,
                    long trace0,
                    int trace_bsize ) {

    int err = subtr_seek( fp, traceno, fst, lst, trace0, trace_bsize );
    if( err != SEGY_OK ) return err;

    assert( trace_bsize >= 0 );
    const size_t bsize = (size_t) trace_bsize;

    if( fp->addr ) {
        memcpy( buf, fp->cur, sizeof( float ) * ( lst - fst ) );
        return SEGY_OK;
    }

    const size_t readc = fread( buf, sizeof( float ), lst - fst, fp->fp );
    if( readc != lst - fst ) return SEGY_FREAD_ERROR;

    return SEGY_OK;
}

int segy_writetrace( segy_file* fp,
                     int traceno,
                     const float* buf,
                     long trace0,
                     int trace_bsize ) {

    const int lst = trace_bsize / sizeof( float );
    return segy_writesubtr( fp, traceno, 0, lst, buf, trace0, trace_bsize );
}

int segy_writesubtr( segy_file* fp,
                     int traceno,
                     int fst,
                     int lst,
                     const float* buf,
                     long trace0,
                     int trace_bsize ) {

    int err = subtr_seek( fp, traceno, fst, lst, trace0, trace_bsize );
    if( err != SEGY_OK ) return err;

    assert( trace_bsize >= 0 );
    const size_t bsize = (size_t) trace_bsize;

    if( fp->addr ) {
        memcpy( fp->cur, buf, sizeof( float ) * ( lst - fst ) );
        return SEGY_OK;
    }

    const size_t writec = fwrite( buf, sizeof( float ), lst - fst, fp->fp );
    if( writec != lst - fst ) return SEGY_FWRITE_ERROR;

    return SEGY_OK;
}

int segy_to_native( int format,
                    int size,
                    float* buf ) {

    assert( sizeof( float ) == sizeof( uint32_t ) );

    uint32_t u;
    if( format == SEGY_IEEE_FLOAT_4_BYTE ) {
        while( size-- ) {
            memcpy( &u, buf, sizeof( float ) );
            u = ntohl( u );
            memcpy( buf++, &u, sizeof( float ) );
        }
    }
    else {
        while( size-- ) {
            ibm2ieee( &u, buf );
            memcpy( buf++, &u, sizeof( float ) );
        }
    }

    return SEGY_OK;
}

int segy_from_native( int format,
                      int size,
                      float* buf ) {

    assert( sizeof( float ) == sizeof( uint32_t ) );

    uint32_t u;
    if( format == SEGY_IEEE_FLOAT_4_BYTE ) {
        while( size-- ) {
            memcpy( &u, buf, sizeof( float ) );
            u = htonl( u );
            memcpy( buf++, &u, sizeof( float ) );
        }
    }
    else {
        while( size-- ) {
            ieee2ibm( &u, buf );
            memcpy( buf++, &u, sizeof( float ) );
        }
    }

    return SEGY_OK;
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
int segy_read_line( segy_file* fp,
                    int line_trace0,
                    int line_length,
                    int stride,
                    int offsets,
                    float* buf,
                    long trace0,
                    int trace_bsize ) {

    assert( sizeof( float ) == sizeof( int32_t ) );
    assert( trace_bsize % 4 == 0 );
    const int trace_data_size = trace_bsize / 4;

    stride *= offsets;

    for( ; line_length--; line_trace0 += stride, buf += trace_data_size ) {
        int err = segy_readtrace( fp, line_trace0, buf, trace0, trace_bsize );
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
int segy_write_line( segy_file* fp,
                     int line_trace0,
                     int line_length,
                     int stride,
                     int offsets,
                     const float* buf,
                     long trace0,
                     int trace_bsize ) {

    assert( sizeof( float ) == sizeof( int32_t ) );
    assert( trace_bsize % 4 == 0 );
    const int trace_data_size = trace_bsize / 4;

    line_trace0 *= offsets;
    stride *= offsets;

    for( ; line_length--; line_trace0 += stride, buf += trace_data_size ) {
        int err = segy_writetrace( fp, line_trace0, buf, trace0, trace_bsize );
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

int segy_read_textheader( segy_file* fp, char *buf) { //todo: Missing position/index support
    if(fp == NULL) {
        return SEGY_FSEEK_ERROR;
    }
    rewind( fp->fp );

    const size_t read = fread( buf, 1, SEGY_TEXT_HEADER_SIZE, fp->fp );
    if( read != SEGY_TEXT_HEADER_SIZE ) return SEGY_FREAD_ERROR;

    buf[ SEGY_TEXT_HEADER_SIZE ] = '\0';
    ebcdic2ascii( buf, buf );
    return SEGY_OK;
}

int segy_write_textheader( segy_file* fp, int pos, const char* buf ) {
    int err;
    char mbuf[ SEGY_TEXT_HEADER_SIZE + 1 ];

    if( pos < 0 ) return SEGY_INVALID_ARGS;

    // TODO: reconsider API, allow non-zero terminated strings
    ascii2ebcdic( buf, mbuf );

    const long offset = pos == 0
                      ? 0
                      : SEGY_TEXT_HEADER_SIZE + SEGY_BINARY_HEADER_SIZE +
                        ((pos-1) * SEGY_TEXT_HEADER_SIZE);

    err = fseek( fp->fp, offset, SEEK_SET );
    if( err != 0 ) return SEGY_FSEEK_ERROR;

    size_t writec = fwrite( mbuf, 1, SEGY_TEXT_HEADER_SIZE, fp->fp );
    if( writec != SEGY_TEXT_HEADER_SIZE )
        return SEGY_FWRITE_ERROR;

    return SEGY_OK;
}

int segy_textheader_size() {
    return SEGY_TEXT_HEADER_SIZE + 1;
}

unsigned int segy_binheader_size() {
    return SEGY_BINARY_HEADER_SIZE;
}
