#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include "segy.h"

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
        *ascii++ = e2a[ (unsigned char) *ebcdic++ ];

    *ascii = '\0';
}

void ascii2ebcdic( const char* ascii, char* ebcdic ) {
    while (*ascii != '\0')
        *ebcdic++ = a2e[(unsigned char) *ascii++];

    *ebcdic = '\0';
}

void ibm2ieee(void* to, const void* from, int len) {
    register unsigned fr; /* fraction */
    register int exp; /* exponent */
    register int sgn; /* sign */

    for (; len-- > 0; to = (char*) to + 4, from = (char*) from + 4) {
        /* split into sign, exponent, and fraction */
        fr = ntohl(*(int32_t*) from); /* pick up value */
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
        *(unsigned*) to = (fr >> 9) | (exp << 23) | (sgn << 31);
    }
}

void ieee2ibm(void* to, const void* from, int len) {
    register unsigned fr; /* fraction */
    register int exp; /* exponent */
    register int sgn; /* sign */

    for (; len-- > 0; to = (char*) to + 4, from = (char*) from + 4) {
        /* split into sign, exponent, and fraction */
        fr = *(unsigned*) from; /* pick up value */
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
        fr = (fr >> 8) | (exp << 24) | (sgn << 31);
        *(unsigned*) to = htonl(fr);
    }
}

static void flipEndianness32(char* data, unsigned int count) {
    for( unsigned int i = 0; i < count; i += 4) {
        char a = data[i];
        char b = data[i + 1];
        data[i] = data[i + 3];
        data[i + 1] = data[i + 2];
        data[i + 2] = b;
        data[i + 3] = a;
    }
}

/* Lookup table for field sizes. All values not explicitly set are 0 */
static int field_size[] = {
    [CDP]                                     =  4,
    [CDP_TRACE]                               =  4,
    [CDP_X]                                   =  4,
    [CDP_Y]                                   =  4,
    [CROSSLINE_3D]                            =  4,
    [EnergySourcePoint]                       =  4,
    [FieldRecord]                             =  4,
    [GroupWaterDepth]                         =  4,
    [GroupX]                                  =  4,
    [GroupY]                                  =  4,
    [INLINE_3D]                               =  4,
    [offset]                                  =  4,
    [ReceiverDatumElevation]                  =  4,
    [ReceiverGroupElevation]                  =  4,
    [ShotPoint]                               =  4,
    [SourceDatumElevation]                    =  4,
    [SourceDepth]                             =  4,
    [SourceEnergyDirectionMantissa]           =  4,
    [SourceMeasurementExponent]               =  4,
    [SourceSurfaceElevation]                  =  4,
    [SourceWaterDepth]                        =  4,
    [SourceX]                                 =  4,
    [SourceY]                                 =  4,
    [TraceNumber]                             =  4,
    [TRACE_SEQUENCE_FILE]                     =  4,
    [TRACE_SEQUENCE_LINE]                     =  4,
    [TransductionConstantMantissa]            =  4,
    [UnassignedInt1]                          =  4,
    [UnassignedInt2]                          =  4,

    [AliasFilterFrequency]                    =  2,
    [AliasFilterSlope]                        =  2,
    [CoordinateUnits]                         =  2,
    [Correlated]                              =  2,
    [DataUse]                                 =  2,
    [DayOfYear]                               =  2,
    [DelayRecordingTime]                      =  2,
    [ElevationScalar]                         =  2,
    [GainType]                                =  2,
    [GapSize]                                 =  2,
    [GeophoneGroupNumberFirstTraceOrigField]  =  2,
    [GeophoneGroupNumberLastTraceOrigField]   =  2,
    [GeophoneGroupNumberRoll1]                =  2,
    [GroupStaticCorrection]                   =  2,
    [GroupUpholeTime]                         =  2,
    [HighCutFrequency]                        =  2,
    [HighCutSlope]                            =  2,
    [HourOfDay]                               =  2,
    [InstrumentGainConstant]                  =  2,
    [InstrumentInitialGain]                   =  2,
    [LagTimeA]                                =  2,
    [LagTimeB]                                =  2,
    [LowCutFrequency]                         =  2,
    [LowCutSlope]                             =  2,
    [MinuteOfHour]                            =  2,
    [MuteTimeEND]                             =  2,
    [MuteTimeStart]                           =  2,
    [NotchFilterFrequency]                    =  2,
    [NotchFilterSlope]                        =  2,
    [NStackedTraces]                          =  2,
    [NSummedTraces]                           =  2,
    [OverTravel]                              =  2,
    [ScalarTraceHeader]                       =  2,
    [SecondOfMinute]                          =  2,
    [ShotPointScalar]                         =  2,
    [SourceEnergyDirectionExponent]           =  2,
    [SourceGroupScalar]                       =  2,
    [SourceMeasurementMantissa]               =  2,
    [SourceMeasurementUnit]                   =  2,
    [SourceStaticCorrection]                  =  2,
    [SourceType]                              =  2,
    [SourceUpholeTime]                        =  2,
    [SubWeatheringVelocity]                   =  2,
    [SweepFrequencyEnd]                       =  2,
    [SweepFrequencyStart]                     =  2,
    [SweepLength]                             =  2,
    [SweepTraceTaperLengthEnd]                =  2,
    [SweepTraceTaperLengthStart]              =  2,
    [SweepType]                               =  2,
    [TaperType]                               =  2,
    [TimeBaseCode]                            =  2,
    [TotalStaticApplied]                      =  2,
    [TraceIdentificationCode]                 =  2,
    [TraceIdentifier]                         =  2,
    [TRACE_SAMPLE_COUNT]                      =  2,
    [TRACE_SAMPLE_INTERVAL]                   =  2,
    [TraceValueMeasurementUnit]               =  2,
    [TraceWeightingFactor]                    =  2,
    [TransductionConstantPower]               =  2,
    [TransductionUnit]                        =  2,
    [WeatheringVelocity]                      =  2,
    [YearDataRecorded]                        =  2,
};

#define HEADER_SIZE SEGY_TEXT_HEADER_SIZE

/*
 * Supporting same byte offsets as in the segy specification, i.e. from the
 * start of the *text header*, not the binary header.
 */
static int bfield_size[] = {
    [- HEADER_SIZE + BIN_JobID]                 =  4,
    [- HEADER_SIZE + BIN_LineNumber]            =  4,
    [- HEADER_SIZE + BIN_ReelNumber]            =  4,

    [- HEADER_SIZE + BIN_Traces]                =  2,
    [- HEADER_SIZE + BIN_AuxTraces]             =  2,
    [- HEADER_SIZE + BIN_Interval]              =  2,
    [- HEADER_SIZE + BIN_IntervalOriginal]      =  2,
    [- HEADER_SIZE + BIN_Samples]               =  2,
    [- HEADER_SIZE + BIN_SamplesOriginal]       =  2,
    [- HEADER_SIZE + BIN_Format]                =  2,
    [- HEADER_SIZE + BIN_EnsembleFold]          =  2,
    [- HEADER_SIZE + BIN_SortingCode]           =  2,
    [- HEADER_SIZE + BIN_VerticalSum]           =  2,
    [- HEADER_SIZE + BIN_SweepFrequencyStart]   =  2,
    [- HEADER_SIZE + BIN_SweepFrequencyEnd]     =  2,
    [- HEADER_SIZE + BIN_SweepLength]           =  2,
    [- HEADER_SIZE + BIN_Sweep]                 =  2,
    [- HEADER_SIZE + BIN_SweepChannel]          =  2,
    [- HEADER_SIZE + BIN_SweepTaperStart]       =  2,
    [- HEADER_SIZE + BIN_SweepTaperEnd]         =  2,
    [- HEADER_SIZE + BIN_Taper]                 =  2,
    [- HEADER_SIZE + BIN_CorrelatedTraces]      =  2,
    [- HEADER_SIZE + BIN_BinaryGainRecovery]    =  2,
    [- HEADER_SIZE + BIN_AmplitudeRecovery]     =  2,
    [- HEADER_SIZE + BIN_MeasurementSystem]     =  2,
    [- HEADER_SIZE + BIN_ImpulseSignalPolarity] =  2,
    [- HEADER_SIZE + BIN_VibratoryPolarity]     =  2,

    [- HEADER_SIZE + BIN_Unassigned1]           =  0,

    [- HEADER_SIZE + BIN_SEGYRevision]          =  2,
    [- HEADER_SIZE + BIN_TraceFlag]             =  2,
    [- HEADER_SIZE + BIN_ExtendedHeaders]       =  2,

    [- HEADER_SIZE + BIN_Unassigned2]           =  0,
};

/*
 * to/from_int32 are to be considered internal functions, but have external
 * linkage so that tests can hook into them. They're not declared in the header
 * files, and use of this internal interface is at user's own risk, i.e. it may
 * change without notice.
 */
int to_int32( const char* buf ) {
    int32_t value;
    memcpy( &value, buf, sizeof( value ) );
    return ((value >> 24) & 0xff)
         | ((value << 8)  & 0xff0000)
         | ((value >> 8)  & 0xff00)
         | ((value << 24) & 0xff000000);
}

int to_int16( const char* buf ) {
    int16_t value;
    memcpy( &value, buf, sizeof( value ) );
    return ((value >> 8) & 0x00ff)
         | ((value << 8) & 0xff00);
}

/* from native int to segy int. fixed-width ints used as byte buffers */
int32_t from_int32( int32_t buf ) {
    int32_t value = 0;
    memcpy( &value, &buf, sizeof( value ) );
    return ((value >> 24) & 0xff)
         | ((value << 8)  & 0xff0000)
         | ((value >> 8)  & 0xff00)
         | ((value << 24) & 0xff000000);
}

int16_t from_int16( int16_t buf ) {
    int16_t value = 0;
    memcpy( &value, &buf, sizeof( value ) );
    return ((value >> 8) & 0x00ff)
         | ((value << 8) & 0xff00);
}

static int get_field( const char* header,
                      const int* table,
                      int field,
                      int* f ) {

    const int bsize = table[ field ];

    switch( bsize ) {
        case 4:
            *f = to_int32( header + (field - 1) );
            return SEGY_OK;

        case 2:
            *f = to_int16( header + (field - 1) );
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

int segy_get_bfield( const char* binheader, int field, int* f ) {
    field -= SEGY_TEXT_HEADER_SIZE;

    if( field < 0 || field >= SEGY_BINARY_HEADER_SIZE )
        return SEGY_INVALID_FIELD;

    return get_field( binheader, bfield_size, field, f );
}

static int set_field( char* header, const int* table, int field, int val ) {
    const int bsize = table[ field ];

    int32_t buf32;
    int16_t buf16;

    switch( bsize ) {
        case 4:
            buf32 = from_int32( val );
            memcpy( header + (field - 1), &buf32, sizeof( buf32 ) );
            return SEGY_OK;

        case 2:
            buf16 = from_int16( val );
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

int segy_binheader( FILE* fp, char* buf ) {
    if(fp == NULL) {
        return SEGY_INVALID_ARGS;
    }

    const int err = fseek( fp, SEGY_TEXT_HEADER_SIZE, SEEK_SET );
    if( err != 0 ) return SEGY_FSEEK_ERROR;

    const size_t read_count = fread( buf, 1, SEGY_BINARY_HEADER_SIZE, fp);
    if( read_count != SEGY_BINARY_HEADER_SIZE )
        return SEGY_FREAD_ERROR;

    return SEGY_OK;
}

int segy_write_binheader( FILE* fp, const char* buf ) {
    if(fp == NULL) {
        return SEGY_INVALID_ARGS;
    }

    const int err = fseek( fp, SEGY_TEXT_HEADER_SIZE, SEEK_SET );
    if( err != 0 ) return SEGY_FSEEK_ERROR;

    const size_t writec = fwrite( buf, 1, SEGY_BINARY_HEADER_SIZE, fp);
    if( writec != SEGY_BINARY_HEADER_SIZE )
        return SEGY_FWRITE_ERROR;

    return SEGY_OK;
}

int segy_format( const char* buf ) {
    int format;
    segy_get_bfield( buf, BIN_Format, &format );
    return format;
}

unsigned int segy_samples( const char* buf ) {
    int samples;
    segy_get_bfield( buf, BIN_Samples, &samples );
    return (unsigned int) samples;
}

unsigned int segy_trace_bsize( unsigned int samples ) {
    /* Hard four-byte float assumption */
    return samples * 4;
}

long segy_trace0( const char* binheader ) {
    int extra_headers;
    segy_get_bfield( binheader, BIN_ExtendedHeaders, &extra_headers );

    return SEGY_TEXT_HEADER_SIZE + SEGY_BINARY_HEADER_SIZE +
           SEGY_TEXT_HEADER_SIZE * extra_headers;
}

int segy_seek( FILE* fp,
               unsigned int trace,
               long trace0,
               unsigned int trace_bsize ) {

    trace_bsize += SEGY_TRACE_HEADER_SIZE;
    const long pos = trace0 + ( (long)trace * (long)trace_bsize );
    const int err = fseek( fp, pos, SEEK_SET );
    if( err != 0 ) return SEGY_FSEEK_ERROR;
    return SEGY_OK;
}

int segy_traceheader( FILE* fp,
                      unsigned int traceno,
                      char* buf,
                      long trace0,
                      unsigned int trace_bsize ) {

    const int err = segy_seek( fp, traceno, trace0, trace_bsize );
    if( err != 0 ) return err;

    const size_t readc = fread( buf, 1, SEGY_TRACE_HEADER_SIZE, fp );

    if( readc != SEGY_TRACE_HEADER_SIZE )
        return SEGY_FREAD_ERROR;

    return SEGY_OK;
}

int segy_write_traceheader( FILE* fp,
                            unsigned int traceno,
                            const char* buf,
                            long trace0,
                            unsigned int trace_bsize ) {

    const int err = segy_seek( fp, traceno, trace0, trace_bsize );
    if( err != 0 ) return err;

    const size_t writec = fwrite( buf, 1, SEGY_TRACE_HEADER_SIZE, fp );

    if( writec != SEGY_TRACE_HEADER_SIZE )
        return SEGY_FWRITE_ERROR;

    return SEGY_OK;
}

/*
 * Determine the file size in bytes. If this function succeeds, the file
 * pointer will be reset to wherever it was before this call. If this call
 * fails for some reason, the return value is 0 and the file pointer location
 * will be determined by the behaviour of fseek.
 */
static int file_size( FILE* fp, size_t* size ) {
    const long prev_pos = ftell( fp );

    int err = fseek( fp, 0, SEEK_END );
    if( err != 0 ) return SEGY_FSEEK_ERROR;

    const size_t sz = ftell( fp );
    err = fseek( fp, prev_pos, SEEK_SET );
    if( err != 0 ) return SEGY_FSEEK_ERROR;

    *size = sz;
    return SEGY_OK;
}

/*
 * Return the number of traces in the file. The file pointer won't change after
 * this call unless fseek itself fails.
 *
 * This function assumes that *all traces* are of the same size.
 */
int segy_traces( FILE* fp, size_t* traces, long trace0, unsigned int trace_bsize ) {

    size_t fsize;
    int err = file_size( fp, &fsize );
    if( err != 0 ) return err;

    trace_bsize += SEGY_TRACE_HEADER_SIZE;
    const size_t trace_data_size = fsize - trace0;

    if( trace_data_size % trace_bsize != 0 )
        return SEGY_TRACE_SIZE_MISMATCH;

    *traces = trace_data_size / trace_bsize;
    return SEGY_OK;
}

static int segy_sample_interval(FILE* fp, double* dt) {

    char bin_header[ SEGY_BINARY_HEADER_SIZE ];
    char trace_header[SEGY_TRACE_HEADER_SIZE];

    int err = segy_binheader( fp, bin_header );
    if (err != 0) {
        return err;
    }

    const long trace0 = segy_trace0( bin_header );
    unsigned int samples = segy_samples( bin_header );
    const size_t trace_bsize = segy_trace_bsize( samples );

    err = segy_traceheader(fp, 0, trace_header, trace0, trace_bsize);
    if (err != 0) {
        return err;
    }

    int _binary_header_dt;
    segy_get_bfield(bin_header, BIN_Interval, &_binary_header_dt);
    double binary_header_dt = _binary_header_dt/1000.0;

    int _trace_header_dt;
    segy_get_field(trace_header, TRACE_SAMPLE_INTERVAL, &_trace_header_dt);
    double trace_header_dt = _trace_header_dt/1000.0;


    *dt = trace_header_dt;

    if (binary_header_dt == 0.0 && trace_header_dt == 0.0) {
        fprintf(stderr, "Trace sampling rate in SEGY header and trace header set to 0.0. Will default to 4 ms.\n");
        *dt = 4.0;
    } else if (binary_header_dt == 0.0) {
        fprintf(stderr, "Trace sampling rate in SEGY header set to 0.0. Will use trace sampling rate of: %f\n", trace_header_dt);
        *dt = trace_header_dt;
    } else if (trace_header_dt == 0.0) {
        fprintf(stderr, "Trace sampling rate in trace header set to 0.0. Will use SEGY header sampling rate of: %f\n", binary_header_dt);
        *dt = binary_header_dt;
    } else if (trace_header_dt != binary_header_dt) {
        fprintf(stderr, "Trace sampling rate in SEGY header and trace header are not equal. Will use SEGY header sampling rate of: %f\n",
                binary_header_dt);
        *dt = binary_header_dt;
    }

    return 0;

}

int segy_sample_indexes(FILE* fp, double* buf, double t0, size_t count) {

    double dt;
    int err = segy_sample_interval(fp, &dt);
    if (err != 0) {
        return err;
    }

    for (int i = 0; i < count; i++) {
        buf[i] = t0 + i * dt;
    }

    return 0;

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
int segy_sorting( FILE* fp,
                  int il,
                  int xl,
                  int* sorting,
                  long trace0,
                  unsigned int trace_bsize ) {
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
    segy_get_field( traceheader, offset, &off0 );

    int traceno = 1;

    do {
        err = segy_traceheader( fp, traceno, traceheader, trace0, trace_bsize );
        if( err != SEGY_OK ) return err;

        segy_get_field( traceheader, il, &il1 );
        segy_get_field( traceheader, xl, &xl1 );
        segy_get_field( traceheader, offset, &off1 );
        ++traceno;
    } while( off0 != off1 );

    // todo: Should expect at least xline, inline or offset to change by one?
    if     ( il0 == il1 ) *sorting = INLINE_SORTING;
    else if( xl0 == xl1 ) *sorting = CROSSLINE_SORTING;
    else return SEGY_INVALID_SORTING;

    return SEGY_OK;
}

/*
 * Find the number of offsets. This is determined by inspecting the trace
 * headers [0,n) where n is the first trace where either the inline number or
 * the crossline number changes (which changes first depends on sorting, but is
 * irrelevant for this function).
 */
int segy_offsets( FILE* fp,
                  int il,
                  int xl,
                  unsigned int traces,
                  unsigned int* out,
                  long trace0,
                  unsigned int trace_bsize ) {
    int err;
    int il0, il1, xl0, xl1;
    char header[ SEGY_TRACE_HEADER_SIZE ];
    unsigned int offsets = 0;

    do {
        err = segy_traceheader( fp, offsets, header, trace0, trace_bsize );
        if( err != 0 ) return err;

        /*
         * check that field value is sane, so that we don't have to check
         * segy_get_field's error
         */
        if( field_size[ il ] == 0 || field_size[ xl ] == 0 )
            return SEGY_INVALID_FIELD;

        segy_get_field( header, il, &il0 );
        segy_get_field( header, xl, &xl0 );

        err = segy_traceheader( fp, offsets + 1, header, trace0, trace_bsize );
        if( err != 0 ) return err;
        segy_get_field( header, il, &il1 );
        segy_get_field( header, xl, &xl1 );

        ++offsets;
    } while( il0 == il1 && xl0 == xl1 && offsets < traces );

    if( offsets >= traces ) return SEGY_INVALID_OFFSETS;

    *out = offsets;
    return SEGY_OK;
}

int segy_line_indices( FILE* fp,
                       int field,
                       unsigned int traceno,
                       unsigned int stride,
                       unsigned int num_indices,
                       unsigned int* buf,
                       long trace0,
                       unsigned int trace_bsize ) {

    if( field_size[ field ] == 0 )
        return SEGY_INVALID_FIELD;

    char header[ SEGY_TRACE_HEADER_SIZE ];
    for( ; num_indices--; traceno += stride, ++buf ) {

        int err = segy_traceheader( fp, traceno, header, trace0, trace_bsize );
        if( err != 0 ) return SEGY_FREAD_ERROR;

        segy_get_field( header, field, (int*)buf );
    }

    return SEGY_OK;
}

static int count_lines( FILE* fp,
                        int field,
                        unsigned int offsets,
                        unsigned int* out,
                        long trace0,
                        unsigned int trace_bsize ) {

    int err;
    char header[ SEGY_TRACE_HEADER_SIZE ];
    err = segy_traceheader( fp, 0, header, trace0, trace_bsize );
    if( err != 0 ) return err;

    int first_lineno, first_offset, ln, off;

    err = segy_get_field( header, field, &first_lineno );
    if( err != 0 ) return err;

    err = segy_get_field( header, 37, &first_offset );
    if( err != 0 ) return err;

    unsigned int lines = 1;
    unsigned int curr = offsets;

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

int segy_count_lines( FILE* fp,
                      int field,
                      unsigned int offsets,
                      unsigned int* l1out,
                      unsigned int* l2out,
                      long trace0,
                      unsigned int trace_bsize ) {

    int err;
    unsigned int l2count;
    err = count_lines( fp, field, offsets, &l2count, trace0, trace_bsize );
    if( err != 0 ) return err;

    size_t traces;
    err = segy_traces( fp, &traces, trace0, trace_bsize );
    if( err != 0 ) return err;

    const unsigned int line_length = l2count * offsets;
    const unsigned int l1count = traces / line_length;

    *l1out = l1count;
    *l2out = l2count;

    return SEGY_OK;
}

int segy_inline_length( int sorting,
                        unsigned int traces,
                        unsigned int crossline_count,
                        unsigned int offsets,
                        unsigned int* line_length ) {

    if( sorting == INLINE_SORTING ) {
        *line_length = crossline_count;
        return SEGY_OK;
    }

    if( sorting == CROSSLINE_SORTING ) {
        *line_length = traces / (crossline_count * offsets);
        return SEGY_OK;
    }

    return SEGY_INVALID_SORTING;
}

int segy_crossline_length( int sorting,
                           unsigned int traces,
                           unsigned int inline_count,
                           unsigned int offsets,
                           unsigned int* line_length ) {

    if( sorting == INLINE_SORTING ) {
        *line_length = inline_count;
        return SEGY_OK;
    }

    if( sorting == CROSSLINE_SORTING ) {
        *line_length = traces / (inline_count * offsets);
        return SEGY_OK;
    }

    return SEGY_INVALID_SORTING;
}

int segy_inline_indices( FILE* fp,
                         int il,
                         int sorting,
                         unsigned int inline_count,
                         unsigned int crossline_count,
                         unsigned int offsets,
                         unsigned int* buf,
                         long trace0,
                         unsigned int trace_bsize) {
    int err;

    if( sorting == INLINE_SORTING ) {
        size_t traces;
        err = segy_traces( fp, &traces, trace0, trace_bsize );
        if( err != 0 ) return err;

        unsigned int stride = crossline_count * offsets;
        return segy_line_indices( fp, il, 0, stride, inline_count, buf, trace0, trace_bsize );
    }

    if( sorting == CROSSLINE_SORTING ) {
        return segy_line_indices( fp, il, 0, offsets, inline_count, buf, trace0, trace_bsize );
    }

    return SEGY_INVALID_SORTING;
}

int segy_crossline_indices( FILE* fp,
                            int xl,
                            int sorting,
                            unsigned int inline_count,
                            unsigned int crossline_count,
                            unsigned int offsets,
                            unsigned int* buf,
                            long trace0,
                            unsigned int trace_bsize ) {

    int err;

    if( sorting == INLINE_SORTING ) {
        return segy_line_indices( fp, xl, 0, offsets, crossline_count, buf, trace0, trace_bsize );
    }

    if( sorting == CROSSLINE_SORTING ) {
        size_t traces;
        err = segy_traces( fp, &traces, trace0, trace_bsize );
        if( err != 0 ) return err;

        unsigned int stride = inline_count * offsets;
        return segy_line_indices( fp, xl, 0, stride, crossline_count, buf, trace0, trace_bsize );
    }

    return SEGY_INVALID_SORTING;
}


static int skip_traceheader( FILE* fp ) {
    const int err = fseek( fp, SEGY_TRACE_HEADER_SIZE, SEEK_CUR );
    if( err != 0 ) return SEGY_FSEEK_ERROR;
    return SEGY_OK;
}

int segy_readtrace( FILE* fp,
                    unsigned int traceno,
                    float* buf,
                    long trace0,
                    unsigned int trace_bsize ) {
    int err;
    err = segy_seek( fp, traceno, trace0, trace_bsize );
    if( err != 0 ) return err;

    err = skip_traceheader( fp );
    if( err != 0 ) return err;

    const size_t readc = fread( buf, 1, trace_bsize, fp );
    if( readc != trace_bsize ) return SEGY_FREAD_ERROR;

    return SEGY_OK;

}

int segy_writetrace( FILE* fp,
                     unsigned int traceno,
                     const float* buf,
                     long trace0,
                     unsigned int trace_bsize ) {

    int err;
    err = segy_seek( fp, traceno, trace0, trace_bsize );
    if( err != 0 ) return err;

    err = skip_traceheader( fp );
    if( err != 0 ) return err;

    const size_t writec = fwrite( buf, 1, trace_bsize, fp );
    if( writec != trace_bsize )
        return SEGY_FWRITE_ERROR;
    return SEGY_OK;
}

int segy_to_native( int format,
                    unsigned int size,
                    float* buf ) {

    if( format == IEEE_FLOAT_4_BYTE )
        flipEndianness32( (char*)buf, size * sizeof( float ) );
    else
        ibm2ieee( buf, buf, size );

    return SEGY_OK;
}

int segy_from_native( int format,
                      unsigned int size,
                      float* buf ) {

    if( format == IEEE_FLOAT_4_BYTE )
        flipEndianness32( (char*)buf, size * sizeof( float ) );
    else
        ieee2ibm( buf, buf, size );

    return SEGY_OK;
}

/*
 * Determine the position of the element `x` in `xs`.
 * Returns -1 if the value cannot be found
 */
static long index_of( unsigned int x,
                      const unsigned int* xs,
                      unsigned int sz ) {
    for( unsigned int i = 0; i < sz; i++ ) {
        if( xs[i] == x )
            return i;
    }

    return -1;
}

/*
 * Read the inline or crossline `lineno`. If it's an inline or crossline
 * depends on the parameters. The line has a length of `line_length` traces,
 * and `buf` must be of (minimum) `line_length*samples_per_trace` size.  Reads
 * every `stride` trace, starting at the trace specified by the *position* of
 * the value `lineno` in `linenos`. If `lineno` isn't present in `linenos`,
 * SEGY_MISSING_LINE_INDEX will be returned.
 *
 * If reading a trace fails, this function will return whatever error
 * segy_readtrace returns.
 */
int segy_read_line( FILE* fp,
                    unsigned int line_trace0,
                    unsigned int line_length,
                    unsigned int stride,
                    float* buf,
                    long trace0,
                    unsigned int trace_bsize ) {

    const size_t trace_data_size = trace_bsize / 4;

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
int segy_write_line( FILE* fp,
                     unsigned int line_trace0,
                     unsigned int line_length,
                     unsigned int stride,
                     float* buf,
                     long trace0,
                     unsigned int trace_bsize ) {

    const size_t trace_data_size = trace_bsize / 4;

    for( ; line_length--; line_trace0 += stride, buf += trace_data_size ) {
        int err = segy_writetrace( fp, line_trace0, buf, trace0, trace_bsize );
        if( err != 0 ) return err;
    }

    return SEGY_OK;
}


int segy_line_trace0( unsigned int lineno,
                      unsigned int line_length,
                      unsigned int stride,
                      const unsigned int* linenos,
                      const unsigned int linenos_sz,
                      unsigned int* traceno ) {

    long index = index_of( lineno, linenos, linenos_sz );

    if( index < 0 ) return SEGY_MISSING_LINE_INDEX;

    if( stride == 1 ) index *= line_length;

    *traceno = index;

    return SEGY_OK;
}

int segy_inline_stride( int sorting,
                        unsigned int inline_count,
                        unsigned int* stride ) {
    switch( sorting ) {
        case CROSSLINE_SORTING:
            *stride = inline_count;
            return SEGY_OK;

        case INLINE_SORTING:
            *stride = 1;
            return SEGY_OK;

        default:
            return SEGY_INVALID_SORTING;
    }
}

int segy_crossline_stride( int sorting,
                           unsigned int crossline_count,
                           unsigned int* stride ) {
    switch( sorting ) {
        case CROSSLINE_SORTING:
            *stride = 1;
            return SEGY_OK;

        case INLINE_SORTING:
            *stride = crossline_count;
            return SEGY_OK;

        default:
            return SEGY_INVALID_SORTING;
    }
}

int segy_read_textheader(FILE *fp, char *buf) { //todo: Missing position/index support
    if(fp == NULL) {
        return SEGY_FSEEK_ERROR;
    }
    rewind( fp );

    const size_t read = fread( buf, 1, SEGY_TEXT_HEADER_SIZE, fp );
    if( read != SEGY_TEXT_HEADER_SIZE ) return SEGY_FREAD_ERROR;

    buf[ SEGY_TEXT_HEADER_SIZE ] = '\0';
    ebcdic2ascii( buf, buf );
    return SEGY_OK;
}

int segy_write_textheader( FILE* fp, unsigned int pos, const char* buf ) {
    int err;
    char mbuf[ SEGY_TEXT_HEADER_SIZE + 1 ];

    // TODO: reconsider API, allow non-zero terminated strings
    ascii2ebcdic( buf, mbuf );

    const long offset = pos == 0
                      ? 0
                      : SEGY_TEXT_HEADER_SIZE + SEGY_BINARY_HEADER_SIZE +
                        ((pos-1) * SEGY_TEXT_HEADER_SIZE);

    err = fseek( fp, offset, SEEK_SET );
    if( err != 0 ) return SEGY_FSEEK_ERROR;

    size_t writec = fwrite( mbuf, 1, SEGY_TEXT_HEADER_SIZE, fp );
    if( writec != SEGY_TEXT_HEADER_SIZE )
        return SEGY_FWRITE_ERROR;

    return SEGY_OK;
}

unsigned int segy_textheader_size() {
    return SEGY_TEXT_HEADER_SIZE + 1;
}

unsigned int segy_binheader_size() {
    return SEGY_BINARY_HEADER_SIZE;
}
