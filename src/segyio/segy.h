#ifndef SEGYIO_SEGY_H
#define SEGYIO_SEGY_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define SEGY_BINARY_HEADER_SIZE 400
#define SEGY_TEXT_HEADER_SIZE 3200
#define SEGY_TRACE_HEADER_SIZE 240

/*
 * About signatures:
 * If a function returns `int` you can assume the return value is an error
 * code. 0 will always indicate success. If a function returns something else
 * than an int it's typically an operation that cannot fail assuming the passed
 * buffer is of the correct size. Any exceptions will be clearly stated.
 *
 * Function signatures are typically:
 * 1) input parameters
 * 2) output parameters
 * 3) low-level file structure information
 *
 * Output parameters are non-const pointers, input parameters are const
 * pointers or plain values. All functions are namespace-prefix'd with segy_.
 * Some functions return values, notably the family concerned with the binary
 * header such as segy_trace0, that should be used in consecutive segy function
 * calls that use the same name for one of its parameters.
 */

/* binary header operations */
/*
 * The binheader buffer passed to these functions must be of *at least*
 * `segy_binheader_size`.
 */
unsigned int segy_binheader_size();
int segy_binheader( FILE*, char* buf );
int segy_write_binheader( FILE*, const char* buf );
unsigned int segy_samples( const char* binheader );
/* exception: the int returned is an enum, SEGY_SORTING, not an error code */
int segy_format( const char* binheader );
int segy_get_field( const char* traceheader, int field, int* f );
int segy_get_bfield( const char* binheader, int field, int* f );
int segy_set_field( char* traceheader, int field, int val );
int segy_set_bfield( char* binheader, int field, int val );

unsigned segy_trace_bsize( unsigned int samples );
/* byte-offset of the first trace header. */
long segy_trace0( const char* binheader );
/* number of traces in this file */
int segy_traces( FILE*, size_t*, long trace0, unsigned int trace_bsize );

int segy_sample_indexes(FILE* fp, double* buf, double t0, size_t count);

/* text header operations */
int segy_read_textheader(FILE *, char *buf);
unsigned int segy_textheader_size();
int segy_write_textheader( FILE*, unsigned int pos, const char* buf );

/* Read the trace header at `traceno` into `buf`. */
int segy_traceheader( FILE*,
                      unsigned int traceno,
                      char* buf,
                      long trace0,
                      unsigned int trace_bsize );

/* Read the trace header at `traceno` into `buf`. */
int segy_write_traceheader( FILE*,
                            unsigned int traceno,
                            const char* buf,
                            long trace0,
                            unsigned int trace_bsize );

/*
 * Number of traces in this file. The sorting type will be written to `sorting`
 * if the function can figure out how the file is sorted. If not, some error
 * code will be returned and `out` will not be modified.
 */
int segy_sorting( FILE*,
                  int il,
                  int xl,
                  int* sorting,
                  long trace0,
                  unsigned int trace_bsize );

/*
 * Number of offsets in this file, written to `offsets`. 1 if a 3D data set, >1
 * if a 4D data set.
 */
int segy_offsets( FILE*,
                  int il,
                  int xl,
                  unsigned int traces,
                  unsigned int* offsets,
                  long trace0,
                  unsigned int trace_bsize );

/*
 * read/write traces. Does not manipulate the buffers at all, i.e. in order to
 * make sense of the read trace it must be converted to native floats, and the
 * buffer sent to write must be converted to target float.
 */
int segy_readtrace( FILE*,
                    unsigned int traceno,
                    float* buf,
                    long trace0,
                    unsigned int trace_bsize );

int segy_writetrace( FILE*,
                     unsigned int traceno,
                     const float* buf,
                     long trace0,
                     unsigned int trace_bsize );

/* convert to/from native float from segy formats (likely IBM or IEEE) */
int segy_to_native( int format,
                    unsigned int size,
                    float* buf );

int segy_from_native( int format,
                      unsigned int size,
                      float* buf );

int segy_read_line( FILE* fp,
                    unsigned int line_trace0,
                    unsigned int line_length,
                    unsigned int stride,
                    float* buf,
                    long trace0,
                    unsigned int trace_bsize );

int segy_write_line( FILE* fp,
                     unsigned int line_trace0,
                     unsigned int line_length,
                     unsigned int stride,
                     float* buf,
                     long trace0,
                     unsigned int trace_bsize );

/*
 * Count inlines and crosslines. Use this function to determine how large buffer
 * the functions `segy_inline_indices` and `segy_crossline_indices` expect.  If
 * the file is sorted on inlines, `field` should the trace header field for the
 * crossline number, and the inline number if the file is sorted on crosslines.
 * If the file is sorted on inlines, `l1out` will contain the number of
 * inlines, and `l2out` crosslines, and the other way around if the file is
 * sorted on crosslines.
 *
 * `offsets` is the number of offsets in the file and be found with
 * `segy_offsets`.
 */
int segy_count_lines( FILE*,
                      int field,
                      unsigned int offsets,
                      unsigned int* l1out,
                      unsigned int* l2out,
                      long trace0,
                      unsigned int trace_bsize );
/*
 * Find the `line_length` for the inlines. Assumes all inlines, crosslines and
 * traces don't vary in length. `offsets` can be found with `segy_offsets`, and
 * `traces` can be found with `segy_traces`.
 *
 * `inline_count` and `crossline_count` are the two values obtained with
 * `segy_count_lines`.
 */
int segy_inline_length( int sorting,
                        unsigned int traces,
                        unsigned int crossline_count,
                        unsigned int offsets,
                        unsigned int* line_length );

int segy_crossline_length( int sorting,
                           unsigned int traces,
                           unsigned int inline_count,
                           unsigned int offsets,
                           unsigned int* line_length );

/*
 * Find the indices of the inlines and write to `buf`. `offsets` are the number
 * of offsets for this file as returned by `segy_offsets`
 */
int segy_inline_indices( FILE*,
                         int il,
                         int sorting,
                         unsigned int inline_count,
                         unsigned int crossline_count,
                         unsigned int offsets,
                         unsigned int* buf,
                         long trace0,
                         unsigned int trace_bsize );

int segy_crossline_indices( FILE*,
                            int xl,
                            int sorting,
                            unsigned int inline_count,
                            unsigned int crossline_count,
                            unsigned int offsets,
                            unsigned int* buf,
                            long trace0,
                            unsigned int trace_bsize );

/*
 * Find the first `traceno` of the line `lineno`. `linenos` should be the line
 * indices returned by `segy_inline_indices` or `segy_crossline_indices`. The
 * stride depends on the sorting and is given by `segy_inline_stride` or
 * `segy_crossline_stride`. `line_length` is the length, i.e. traces per line,
 * given by `segy_inline_length` or `segy_crossline_length`.
 *
 * To read/write an inline, read `line_length` starting at `traceno`,
 * incrementing `traceno` with `stride` `line_length` times.
 */
int segy_line_trace0( unsigned int lineno,
                      unsigned int line_length,
                      unsigned int stride,
                      const unsigned int* linenos,
                      const unsigned int linenos_sz,
                      unsigned int* traceno );

/*
 * Find the stride needed for an inline/crossline traversal.
 */
int segy_inline_stride( int sorting,
                        unsigned int inline_count,
                        unsigned int* stride );

int segy_crossline_stride( int sorting,
                           unsigned int crossline_count,
                           unsigned int* stride );

typedef enum {
    TRACE_SEQUENCE_LINE = 1,
    TRACE_SEQUENCE_FILE = 5,
    FieldRecord = 9,
    TraceNumber = 13,
    EnergySourcePoint = 17,
    CDP = 21,
    CDP_TRACE = 25,
    TraceIdentificationCode = 29,
    NSummedTraces = 31,
    NStackedTraces = 33,
    DataUse = 35,
    offset = 37,
    ReceiverGroupElevation = 41,
    SourceSurfaceElevation = 45,
    SourceDepth = 49,
    ReceiverDatumElevation = 53,
    SourceDatumElevation = 57,
    SourceWaterDepth = 61,
    GroupWaterDepth = 65,
    ElevationScalar = 69,
    SourceGroupScalar = 71,
    SourceX = 73,
    SourceY = 77,
    GroupX = 81,
    GroupY = 85,
    CoordinateUnits = 89,
    WeatheringVelocity = 91,
    SubWeatheringVelocity = 93,
    SourceUpholeTime = 95,
    GroupUpholeTime = 97,
    SourceStaticCorrection = 99,
    GroupStaticCorrection = 101,
    TotalStaticApplied = 103,
    LagTimeA = 105,
    LagTimeB = 107,
    DelayRecordingTime = 109,
    MuteTimeStart = 111,
    MuteTimeEND = 113,
    TRACE_SAMPLE_COUNT = 115,
    TRACE_SAMPLE_INTERVAL = 117,
    GainType = 119,
    InstrumentGainConstant = 121,
    InstrumentInitialGain = 123,
    Correlated = 125,
    SweepFrequencyStart = 127,
    SweepFrequencyEnd = 129,
    SweepLength = 131,
    SweepType = 133,
    SweepTraceTaperLengthStart = 135,
    SweepTraceTaperLengthEnd = 137,
    TaperType = 139,
    AliasFilterFrequency = 141,
    AliasFilterSlope = 143,
    NotchFilterFrequency = 145,
    NotchFilterSlope = 147,
    LowCutFrequency = 149,
    HighCutFrequency = 151,
    LowCutSlope = 153,
    HighCutSlope = 155,
    YearDataRecorded = 157,
    DayOfYear = 159,
    HourOfDay = 161,
    MinuteOfHour = 163,
    SecondOfMinute = 165,
    TimeBaseCode = 167,
    TraceWeightingFactor = 169,
    GeophoneGroupNumberRoll1 = 171,
    GeophoneGroupNumberFirstTraceOrigField = 173,
    GeophoneGroupNumberLastTraceOrigField = 175,
    GapSize = 177,
    OverTravel = 179,
    CDP_X = 181,
    CDP_Y = 185,
    INLINE_3D = 189,
    CROSSLINE_3D = 193,
    ShotPoint = 197,
    ShotPointScalar = 201,
    TraceValueMeasurementUnit = 203,
    TransductionConstantMantissa = 205,
    TransductionConstantPower = 209,
    TransductionUnit = 211,
    TraceIdentifier = 213,
    ScalarTraceHeader = 215,
    SourceType = 217,
    SourceEnergyDirectionMantissa = 219,
    SourceEnergyDirectionExponent = 223,
    SourceMeasurementMantissa = 225,
    SourceMeasurementExponent = 229,
    SourceMeasurementUnit = 231,
    UnassignedInt1 = 233,
    UnassignedInt2 = 237

} SEGY_FIELD;

typedef enum {
    BIN_JobID = 3201,
    BIN_LineNumber = 3205,
    BIN_ReelNumber = 3209,
    BIN_Traces = 3213,
    BIN_AuxTraces = 3215,
    BIN_Interval = 3217,
    BIN_IntervalOriginal = 3219,
    BIN_Samples = 3221,
    BIN_SamplesOriginal = 3223,
    BIN_Format = 3225,
    BIN_EnsembleFold = 3227,
    BIN_SortingCode = 3229,
    BIN_VerticalSum = 3231,
    BIN_SweepFrequencyStart = 3233,
    BIN_SweepFrequencyEnd = 3235,
    BIN_SweepLength = 3237,
    BIN_Sweep = 3239,
    BIN_SweepChannel = 3241,
    BIN_SweepTaperStart = 3243,
    BIN_SweepTaperEnd = 3245,
    BIN_Taper = 3247,
    BIN_CorrelatedTraces = 3249,
    BIN_BinaryGainRecovery = 3251,
    BIN_AmplitudeRecovery = 3253,
    BIN_MeasurementSystem = 3255,
    BIN_ImpulseSignalPolarity = 3257,
    BIN_VibratoryPolarity = 3259,
    BIN_Unassigned1 = 3261,
    BIN_SEGYRevision = 3501,
    BIN_TraceFlag = 3503,
    BIN_ExtendedHeaders = 3505,
    BIN_Unassigned2 = 3507,
} SEGY_BINFIELD;

typedef enum {
    IBM_FLOAT_4_BYTE = 1,
    SIGNED_INTEGER_4_BYTE = 2,
    SIGNED_SHORT_2_BYTE = 3,
    FIXED_POINT_WITH_GAIN_4_BYTE = 4, // Obsolete
    IEEE_FLOAT_4_BYTE = 5,
    NOT_IN_USE_1 = 6,
    NOT_IN_USE_2 = 7,
    SIGNED_CHAR_1_BYTE = 8
} SEGY_FORMAT;

typedef enum {
    UNKNOWN_SORTING = 0,
    CROSSLINE_SORTING = 1,
    INLINE_SORTING = 2,

} SEGY_SORTING;

typedef enum {
    SEGY_OK = 0,
    SEGY_FOPEN_ERROR,
    SEGY_FSEEK_ERROR,
    SEGY_FREAD_ERROR,
    SEGY_FWRITE_ERROR,
    SEGY_INVALID_FIELD,
    SEGY_INVALID_SORTING,
    SEGY_MISSING_LINE_INDEX,
    SEGY_INVALID_OFFSETS,
    SEGY_TRACE_SIZE_MISMATCH,
    SEGY_INVALID_ARGS
} SEGY_ERROR;


#endif //SEGYIO_SEGY_H
