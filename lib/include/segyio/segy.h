#ifndef SEGYIO_SEGY_H
#define SEGYIO_SEGY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SEGY_BINARY_HEADER_SIZE 400
#define SEGY_TEXT_HEADER_SIZE 3200
#define SEGY_TRACE_HEADER_SIZE 240

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum {
    SEGY_UNDEFINED_FIELD = 0,
    SEGY_IBM_FLOAT_4_BYTE = 1,
    SEGY_SIGNED_INTEGER_4_BYTE = 2,
    SEGY_SIGNED_SHORT_2_BYTE = 3,
    SEGY_FIXED_POINT_WITH_GAIN_4_BYTE = 4, // Obsolete
    SEGY_IEEE_FLOAT_4_BYTE = 5,
    SEGY_IEEE_FLOAT_8_BYTE = 6,
    SEGY_SIGNED_CHAR_3_BYTE = 7,
    SEGY_SIGNED_INTEGER_3_BYTE = 7,
    SEGY_SIGNED_CHAR_1_BYTE = 8,
    SEGY_SIGNED_INTEGER_8_BYTE = 9,
    SEGY_UNSIGNED_INTEGER_4_BYTE = 10,
    SEGY_UNSIGNED_SHORT_2_BYTE = 11,
    SEGY_UNSIGNED_INTEGER_8_BYTE = 12,
    SEGY_UNSIGNED_INTEGER_3_BYTE = 15,
    SEGY_UNSIGNED_CHAR_1_BYTE = 16,
    SEGY_NOT_IN_USE_1 = 19,
    SEGY_NOT_IN_USE_2 = 20,
} SEGY_FORMAT;

typedef enum {
    SEGY_ENTRY_TYPE_UNDEFINED   = 0,
    SEGY_ENTRY_TYPE_INT2        = 1,
    SEGY_ENTRY_TYPE_INT4        = 2,
    SEGY_ENTRY_TYPE_INT8        = 3,
    SEGY_ENTRY_TYPE_UINT1       = 4,
    SEGY_ENTRY_TYPE_UINT2       = 5,
    SEGY_ENTRY_TYPE_UINT4       = 6,
    SEGY_ENTRY_TYPE_UINT8       = 7,
    SEGY_ENTRY_TYPE_IBMFP       = 8,
    SEGY_ENTRY_TYPE_IEEE32      = 9,
    SEGY_ENTRY_TYPE_IEEE64      = 10,
    SEGY_ENTRY_TYPE_LINETRC     = 11,
    SEGY_ENTRY_TYPE_REELTRC     = 12,
    SEGY_ENTRY_TYPE_LINETRC8    = 13,
    SEGY_ENTRY_TYPE_REELTRC8    = 14,
    SEGY_ENTRY_TYPE_COOR4       = 15,
    SEGY_ENTRY_TYPE_ELEV4       = 16,
    SEGY_ENTRY_TYPE_TIME2       = 17,
    SEGY_ENTRY_TYPE_SPNUM4      = 18,
    SEGY_ENTRY_TYPE_SCALE6_MANT = 19,
    SEGY_ENTRY_TYPE_SCALE6_EXP  = 20,
    SEGY_ENTRY_TYPE_STRING8     = 21,
} SEGY_ENTRY_TYPE;

typedef enum {
    SEGY_TR_SEQ_LINE                = 1,
    SEGY_TR_SEQ_FILE                = 5,
    SEGY_TR_FIELD_RECORD            = 9,
    SEGY_TR_NUMBER_ORIG_FIELD       = 13,
    SEGY_TR_ENERGY_SOURCE_POINT     = 17,
    SEGY_TR_ENSEMBLE                = 21,
    SEGY_TR_NUM_IN_ENSEMBLE         = 25,
    SEGY_TR_TRACE_ID                = 29,
    SEGY_TR_SUMMED_TRACES           = 31,
    SEGY_TR_STACKED_TRACES          = 33,
    SEGY_TR_DATA_USE                = 35,
    SEGY_TR_OFFSET                  = 37,
    SEGY_TR_RECV_GROUP_ELEV         = 41,
    SEGY_TR_SOURCE_SURF_ELEV        = 45,
    SEGY_TR_SOURCE_DEPTH            = 49,
    SEGY_TR_RECV_DATUM_ELEV         = 53,
    SEGY_TR_SOURCE_DATUM_ELEV       = 57,
    SEGY_TR_SOURCE_WATER_DEPTH      = 61,
    SEGY_TR_GROUP_WATER_DEPTH       = 65,
    SEGY_TR_ELEV_SCALAR             = 69,
    SEGY_TR_SOURCE_GROUP_SCALAR     = 71,
    SEGY_TR_SOURCE_X                = 73,
    SEGY_TR_SOURCE_Y                = 77,
    SEGY_TR_GROUP_X                 = 81,
    SEGY_TR_GROUP_Y                 = 85,
    SEGY_TR_COORD_UNITS             = 89,
    SEGY_TR_WEATHERING_VELO         = 91,
    SEGY_TR_SUBWEATHERING_VELO      = 93,
    SEGY_TR_SOURCE_UPHOLE_TIME      = 95,
    SEGY_TR_GROUP_UPHOLE_TIME       = 97,
    SEGY_TR_SOURCE_STATIC_CORR      = 99,
    SEGY_TR_GROUP_STATIC_CORR       = 101,
    SEGY_TR_TOT_STATIC_APPLIED      = 103,
    SEGY_TR_LAG_A                   = 105,
    SEGY_TR_LAG_B                   = 107,
    SEGY_TR_DELAY_REC_TIME          = 109,
    SEGY_TR_MUTE_TIME_START         = 111,
    SEGY_TR_MUTE_TIME_END           = 113,
    SEGY_TR_SAMPLE_COUNT            = 115,
    SEGY_TR_SAMPLE_INTER            = 117,
    SEGY_TR_GAIN_TYPE               = 119,
    SEGY_TR_INSTR_GAIN_CONST        = 121,
    SEGY_TR_INSTR_INIT_GAIN         = 123,
    SEGY_TR_CORRELATED              = 125,
    SEGY_TR_SWEEP_FREQ_START        = 127,
    SEGY_TR_SWEEP_FREQ_END          = 129,
    SEGY_TR_SWEEP_LENGTH            = 131,
    SEGY_TR_SWEEP_TYPE              = 133,
    SEGY_TR_SWEEP_TAPERLEN_START    = 135,
    SEGY_TR_SWEEP_TAPERLEN_END      = 137,
    SEGY_TR_TAPER_TYPE              = 139,
    SEGY_TR_ALIAS_FILT_FREQ         = 141,
    SEGY_TR_ALIAS_FILT_SLOPE        = 143,
    SEGY_TR_NOTCH_FILT_FREQ         = 145,
    SEGY_TR_NOTCH_FILT_SLOPE        = 147,
    SEGY_TR_LOW_CUT_FREQ            = 149,
    SEGY_TR_HIGH_CUT_FREQ           = 151,
    SEGY_TR_LOW_CUT_SLOPE           = 153,
    SEGY_TR_HIGH_CUT_SLOPE          = 155,
    SEGY_TR_YEAR_DATA_REC           = 157,
    SEGY_TR_DAY_OF_YEAR             = 159,
    SEGY_TR_HOUR_OF_DAY             = 161,
    SEGY_TR_MIN_OF_HOUR             = 163,
    SEGY_TR_SEC_OF_MIN              = 165,
    SEGY_TR_TIME_BASE_CODE          = 167,
    SEGY_TR_WEIGHTING_FAC           = 169,
    SEGY_TR_GEOPHONE_GROUP_ROLL1    = 171,
    SEGY_TR_GEOPHONE_GROUP_FIRST    = 173,
    SEGY_TR_GEOPHONE_GROUP_LAST     = 175,
    SEGY_TR_GAP_SIZE                = 177,
    SEGY_TR_OVER_TRAVEL             = 179,
    SEGY_TR_CDP_X                   = 181,
    SEGY_TR_CDP_Y                   = 185,
    SEGY_TR_INLINE                  = 189,
    SEGY_TR_CROSSLINE               = 193,
    SEGY_TR_SHOT_POINT              = 197,
    SEGY_TR_SHOT_POINT_SCALAR       = 201,
    SEGY_TR_MEASURE_UNIT            = 203,
    SEGY_TR_TRANSDUCTION_MANT       = 205,
    SEGY_TR_TRANSDUCTION_EXP        = 209,
    SEGY_TR_TRANSDUCTION_UNIT       = 211,
    SEGY_TR_DEVICE_ID               = 213,
    SEGY_TR_SCALAR_TRACE_HEADER     = 215,
    SEGY_TR_SOURCE_TYPE             = 217,
    SEGY_TR_SOURCE_ENERGY_DIR_VERT  = 219,
    SEGY_TR_SOURCE_ENERGY_DIR_XLINE = 221,
    SEGY_TR_SOURCE_ENERGY_DIR_ILINE = 223,
    SEGY_TR_SOURCE_MEASURE_MANT     = 225,
    SEGY_TR_SOURCE_MEASURE_EXP      = 229,
    SEGY_TR_SOURCE_MEASURE_UNIT     = 231,
    SEGY_TR_UNASSIGNED1             = 233,
    SEGY_TR_UNASSIGNED2             = 237
} SEGY_FIELD;

typedef enum {
    SEGY_BIN_JOB_ID                     = 3201,
    SEGY_BIN_LINE_NUMBER                = 3205,
    SEGY_BIN_REEL_NUMBER                = 3209,
    SEGY_BIN_ENSEMBLE_TRACES            = 3213,
    SEGY_BIN_AUX_ENSEMBLE_TRACES        = 3215,
    SEGY_BIN_INTERVAL                   = 3217,
    SEGY_BIN_INTERVAL_ORIG              = 3219,
    SEGY_BIN_SAMPLES                    = 3221,
    SEGY_BIN_SAMPLES_ORIG               = 3223,
    SEGY_BIN_FORMAT                     = 3225,
    SEGY_BIN_ENSEMBLE_FOLD              = 3227,
    SEGY_BIN_SORTING_CODE               = 3229,
    SEGY_BIN_VERTICAL_SUM               = 3231,
    SEGY_BIN_SWEEP_FREQ_START           = 3233,
    SEGY_BIN_SWEEP_FREQ_END             = 3235,
    SEGY_BIN_SWEEP_LENGTH               = 3237,
    SEGY_BIN_SWEEP                      = 3239,
    SEGY_BIN_SWEEP_CHANNEL              = 3241,
    SEGY_BIN_SWEEP_TAPER_START          = 3243,
    SEGY_BIN_SWEEP_TAPER_END            = 3245,
    SEGY_BIN_TAPER                      = 3247,
    SEGY_BIN_CORRELATED_TRACES          = 3249,
    SEGY_BIN_BIN_GAIN_RECOVERY          = 3251,
    SEGY_BIN_AMPLITUDE_RECOVERY         = 3253,
    SEGY_BIN_MEASUREMENT_SYSTEM         = 3255,
    SEGY_BIN_IMPULSE_POLARITY           = 3257,
    SEGY_BIN_VIBRATORY_POLARITY         = 3259,
    SEGY_BIN_EXT_ENSEMBLE_TRACES        = 3261,
    SEGY_BIN_EXT_AUX_ENSEMBLE_TRACES    = 3265,
    SEGY_BIN_EXT_SAMPLES                = 3269,
    SEGY_BIN_EXT_INTERVAL               = 3273,
    SEGY_BIN_EXT_INTERVAL_ORIG          = 3281,
    SEGY_BIN_EXT_SAMPLES_ORIG           = 3289,
    SEGY_BIN_EXT_ENSEMBLE_FOLD          = 3293,
    SEGY_BIN_INTEGER_CONSTANT           = 3297, // Expected value 16909060 (decimal). Used to detect order of bytes
    SEGY_BIN_UNASSIGNED1                = 3301,
    SEGY_BIN_SEGY_REVISION              = 3501,
    SEGY_BIN_SEGY_REVISION_MINOR        = 3502,
    SEGY_BIN_TRACE_FLAG                 = 3503,
    SEGY_BIN_EXT_HEADERS                = 3505,
    SEGY_BIN_MAX_ADDITIONAL_TR_HEADERS  = 3507,
    SEGY_BIN_SURVEY_TYPE                = 3509,
    SEGY_BIN_TIME_BASIS_CODE            = 3511,
    SEGY_BIN_NR_TRACES_IN_STREAM        = 3513,
    SEGY_BIN_FIRST_TRACE_OFFSET         = 3521,
    SEGY_BIN_NR_TRAILER_RECORDS         = 3529,
    SEGY_BIN_UNASSIGNED2                = 3533,
} SEGY_BINFIELD;


typedef enum {
    SEGY_MSB = 0,
    SEGY_LSB = 1,
} SEGY_ENDIANNESS;

typedef enum {
    SEGY_EBCDIC = 0,
    SEGY_ASCII = 1,
} SEGY_ENCODING;

typedef enum {
    SEGY_UNKNOWN_SORTING = 0,
    SEGY_CROSSLINE_SORTING = 1,
    SEGY_INLINE_SORTING = 2,
} SEGY_SORTING;

typedef enum {
    SEGY_OK = 0,
    SEGY_FOPEN_ERROR,
    SEGY_FSEEK_ERROR,
    SEGY_FREAD_ERROR,
    SEGY_FWRITE_ERROR,
    SEGY_INVALID_FIELD,
    SEGY_INVALID_FIELD_DATATYPE,
    SEGY_INVALID_FIELD_VALUE,
    SEGY_INVALID_SORTING,
    SEGY_MISSING_LINE_INDEX,
    SEGY_INVALID_OFFSETS,
    SEGY_TRACE_SIZE_MISMATCH,
    SEGY_INVALID_ARGS,
    SEGY_MMAP_ERROR,
    SEGY_MMAP_INVALID,
    SEGY_READONLY,
    SEGY_NOTFOUND,
    SEGY_MEMORY_ERROR,
    SEGY_DS_FLUSH_ERROR,
    SEGY_DS_CLOSE_ERROR,
    // values are duplicated until enum is properly cleaned
    SEGY_DS_READ_ERROR = SEGY_FREAD_ERROR,
    SEGY_DS_WRITE_ERROR = SEGY_FWRITE_ERROR,
    SEGY_DS_SEEK_ERROR = SEGY_FSEEK_ERROR,
    SEGY_DS_ERROR = SEGY_FREAD_ERROR,
} SEGY_ERROR;


/*
 * Represents an abstract data source which supports common file operations,
 * described by function pointers and `writable` attribute.
 *
 * `void* stream` points to a concrete data object and segy_datasource is just a
 * library-adjusted wrapper over it. Function implementations can cast it to the
 * actual type and perform required operations on it. All functions implementing
 * function pointers are expected to return 0 on success and non-0 value on
 * error. Unless specified, they should not change stream position.
 *
 * `elemsize` and `lsb` describe format of the data in the datasource and are
 * required to interpret trace/header data correctly. `minimize_requests_number`
 * is an algorithm parameter.
 */
struct segy_datasource {
    /* Pointer to the actual finite stream of data. */
    void* stream;

    /* Reads `size` number of bytes from the `self` stream into the `buffer`.
     * Advances the stream position by read number of bytes. If requested number
     * of bytes can not be read, implementation must return non-0 error code.
     */
    int ( *read )( struct segy_datasource* self, void* buffer, size_t size );

    /* Writes `size` number of bytes from the `buffer` into the `self` stream.
     * Advances the stream position by read number of bytes. If requested number
     * of bytes can not be written, implementation must return non-0 error code.
     */
    int ( *write )( struct segy_datasource* self, const void* buffer, size_t size );

    /* Sets `self` stream position. Position is relative according to the
     * `whence` parameter, where 0 is "`offset` is relative to the stream
     * start", 1 is "`offset` is relative to the current position" and 2 is
     * "`offset` is relative to the stream end".
     */
    int ( *seek )( struct segy_datasource* self, long long offset, int whence );

    /* Gets current `self` stream position and stores result in `out`.*/
    int ( *tell )( struct segy_datasource* self, long long* out );

    /* Gets `self` stream size and stores result in `out`.*/
    int ( *size )( struct segy_datasource* self, long long* out );

    /* Flushes `self` stream contents. */
    int ( *flush )( struct segy_datasource* self );

    /* Closes `self` stream. */
    int ( *close )( struct segy_datasource* self );

    /* Is datasource writable */
    bool writable;

    /* Size of each element in trace */
    int elemsize;

    /* True if stream uses little-endian byte order, false if big-endian
     * (SEG-Y_r2.1 section "3.3. Number Formats"). Pairwise byte-swapped
     * ordering is not supported.
     */
    bool lsb;

    /* EBCDIC or ASCII encoding. All text-reading functions should consult this
     * property to determine string encoding.
     */
    int encoding;

    /* Setting for trace algorithms.
     *
     * If true, unnecessary data would be read into memory/written to the
     * destination in order to avoid many requests to datasource.
     *
     * If false, algorithms would prioritize reading/writing minimal possible
     * amount of data at the cost of many requests.
     */
    bool minimize_requests_number;

    /* Performance hack for in-memory datasources.
     *
     * If true, algorithms would be free to break encapsulation by assuming that
     * stream is a "memfile" struct. It leads to improved performance for
     * certain operations.
     */
    bool memory_speedup;
};

typedef struct segy_datasource segy_datasource;

typedef struct segy_datasource segy_file;

typedef union {
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
} segy_field_value;

typedef struct {
    segy_field_value value;
    uint8_t datatype;
} segy_field_data;

typedef struct {
    SEGY_ENTRY_TYPE entry_type;
    bool requires_nonzero_value; //use only if field value is not zero
} segy_entry_definition;


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

segy_file* segy_open( const char* path, const char* mode );
int segy_mmap( segy_datasource* );
segy_datasource* segy_memopen( unsigned char* addr, size_t size );

int segy_flush( segy_datasource* );
int segy_close( segy_datasource* );

/* binary header operations */
/*
 * The binheader buffer passed to these functions must be of *at least*
 * `segy_binheader_size`. Returns size, not an error code.
 */
int segy_binheader_size( void );
int segy_binheader( segy_datasource*, char* buf );
int segy_write_binheader( segy_datasource*, const char* buf );
/*
 * exception: the int returned is the number of samples (the segy standard only
 * allocates 2 octets for this, so it comfortably sits inside an int. If the
 * value is negative, it is converted to an unsigned 16-bit integer, allowing
 * up to 65536 samples per trace.
 */
int segy_samples( const char* binheader );
/*
 * infer the interval between traces. this function tries to read the interval
 * from the binary header and the first trace header, and will fall back to the
 * `fallback` argument.
 */
int segy_sample_interval( segy_datasource*, float fallback , float* dt );

/* exception: the int returned is an enum, SEGY_FORMAT, not an error code */
int segy_format( const char* binheader );
/* override the assumed format of the samples.
 *
 * by default, segyio assumes a 4-byte float format (usually IBM float). The
 * to/from native functions take this parameter explicitly, but functions like
 * read_subtrace requires the size of each element.
 *
 * if this function is not called, for backwards compatibility reasons, the
 * format is always assumed to be IBM float.
 *
 * The binary header is not implicitly queried, because it's often broken and
 * unreliable with this information - however, if the header IS considered to
 * be reliable, the result of `segy_format` can be passed to this function.
 */
int segy_set_format( segy_datasource*, int format );

/* set file as LSB/MSB (little/big endian)
 *
 * The binary header is not implicitly queried, because it's often broken and
 * unreliable with this information. This is subject to a change.
 *
 * By default, segyio assumes files are MSB. However, some files (seismic unix,
 * SEG-Y rev2) are LSB. *all* functions returning bytes in segyio will output
 * MSB, regardless of the properties of the underlying file.
 */
int segy_set_endianness( segy_datasource*, int opt );

/* sets file to be EBCDIC/ASCII encoded. If no valid encoding is provided, one
 * is automatically decided by consulting first text header character. If in
 * doubt, EBCDIC encoding is assumed.
 */
int segy_set_encoding( segy_datasource*, int opt );

/* Binary header layout map. Indicies (offsets) are 0-based. */
const segy_entry_definition* segy_binheader_map( void );
/* Default trace header layout map. Indicies (offsets) are 0-based. */
const segy_entry_definition* segy_traceheader_default_map( void );

/* Gets field datatype from field id. Depending on field value, binary or trace
 * mapping table would be used.
 * The int returned is the field datatype, not an error code.
 */
int segy_field_datatype( int field, const segy_entry_definition* mapping );

/* Reads one trace field data from given 0-based header. 0-based
 * offset-to-entry-definition mapping should correspond to provided header.
 * Field parameter should be 1-based offset, as in, e.g., SEGY_FIELD array.
 */
int segy_get_tracefield( const char* header,
                         const segy_entry_definition* mapping,
                         int field,
                         segy_field_data* fd );

/* Writes one trace field data to given 0-based header. 0-based
 * offset-to-entry-definition mapping should correspond to provided header.
 * Field parameter should be 1-based offset, as in, e.g., SEGY_FIELD array.
 */
int segy_set_tracefield( char* header,
                         const segy_entry_definition* mapping,
                         int field,
                         segy_field_data fd );

/* Reads one binary field data from given 0-based header. Field parameter should
 * be 3201-based offset, as in SEGY_BINFIELD array.
 */
int segy_get_binfield( const char* header,
                       int field,
                       segy_field_data* fd );

/* Writes one binary field data to given 0-based header. Field parameter should
 * be 3201-based offset, as in SEGY_BINFIELD array.
 */
int segy_set_binfield( char* header,
                       int field,
                       segy_field_data fd );

/* Legacy function compliant only with rev 1.0 specification. Intention is to
 * deprecate it and remove it in the future. */
int segy_get_tracefield_int( const char* header, int field, int* f );
/* Legacy function compliant only with rev 1.0 specification. Intention is to
 * deprecate it and remove it in the future. */
int segy_set_tracefield_int( char* header, int field, int val );
/* Legacy function compliant only with rev 1.0 specification. Intention is to
 * deprecate it and remove it in the future. */
int segy_get_binfield_int( const char* header, int field, int* f );
/* Legacy function compliant only with rev 1.0 specification. Intention is to
 * deprecate it and remove it in the future. */
int segy_set_binfield_int( char* header, int field, int val );



int segy_field_forall( segy_datasource*,
                       int field,
                       int start,
                       int stop,
                       int step,
                       int* buf,
                       long trace0,
                       int trace_bsize );

/*
 * exception: segy_trace_bsize computes the size of the traces in bytes. Cannot
 * fail. Equivalent to segy_trsize(SEGY_IBM_FLOAT_4_BYTE, samples);
 */
int segy_trace_bsize( int samples );
/*
 * segy_trsize computes the size of a trace in bytes, determined by the trace
 * format. If format is unknown, invalid, or unsupported, this function returns
 * a negative value. If `samples` is zero or negative, the result is undefined.
 */
int segy_trsize( int format, int samples );
/* byte-offset of the first trace header. */
long segy_trace0( const char* binheader );
/*
 * number of traces in this file.
 * if this function fails, the input argument is not modified.
 */
int segy_traces( segy_datasource*, int*, long trace0, int trace_bsize );

int segy_sample_indices( segy_datasource*,
                         float t0,
                         float dt,
                         int count,
                         float* buf );

/* text header operations */
/*
 * buf in all read and write functions should be minimum segy_textheader_size()
 * in size
 *
 * all read_textheader function outputs are zero-terminated C strings. It is
 * assumed input is ebcdic encoded.
 */
int segy_read_textheader( segy_datasource*, char *buf);
/*
 * segy_textheader_size() returns is a size hint for C-string style buffers,
 * and includes space for a terminating null byte.
 */
int segy_textheader_size( void );
/*
 * read the extended textual headers. `pos = 0` gives the first *extended*
 * header, i.e. the first textual header following the binary header.
 * Behaviour is undefined if the file does not have extended headers
 */
int segy_read_ext_textheader( segy_datasource*, int pos, char* buf );

/*
 * Write the text header. `pos` is regular array indexing, i.e. pos = 0 is the
 * regular text header, 1 is the first extended textual header. This is *NOT*
 * the same behaviour as read_ext_textheader.
 *
 * The asymmetry in the interface is unfortunate, but a consequence of there
 * only being support for non-extended headers for a while. The old behaviour
 * is preserved for backwards compatibility.
 *
 * Like the read-textheader functions, the input text should be in ascii and
 * will be automatically encoded to ebcdic.
 */
int segy_write_textheader( segy_datasource*, int pos, const char* buf );

/* Read the trace header at `traceno` into `buf`. */
int segy_traceheader( segy_datasource*,
                      int traceno,
                      char* buf,
                      long trace0,
                      int trace_bsize );

/* Read the trace header at `traceno` into `buf`. */
int segy_write_traceheader( segy_datasource*,
                            int traceno,
                            const char* buf,
                            long trace0,
                            int trace_bsize );

/*
 * The sorting type will be written to `sorting` if the function can figure out
 * how the file is sorted.
 */
int segy_sorting( segy_datasource*,
                  int il,
                  int xl,
                  int tr_offset,
                  int* sorting,
                  long trace0,
                  int trace_bsize );

/*
 * Number of offsets in this file, written to `offsets`. 1 if a 3D data set, >1
 * if a 4D data set.
 */
int segy_offsets( segy_datasource*,
                  int il,
                  int xl,
                  int traces,
                  int* out,
                  long trace0,
                  int trace_bsize );

/*
 * The names of the individual offsets. `out` must be a buffer of
 * `segy_offsets` elements.
 */
int segy_offset_indices( segy_datasource*,
                         int offset_field,
                         int offsets,
                         int* out,
                         long trace0,
                         int trace_bsize );

/*
 * read/write traces. does not convert data from on-disk representation to
 * native formats, so this data can not be used directly on most systems (intel
 * in particular). use to/from native to convert to native representations.
 */
int segy_readtrace( segy_datasource*,
                    int traceno,
                    void* buf,
                    long trace0,
                    int trace_bsize );

int segy_writetrace( segy_datasource*,
                     int traceno,
                     const void* buf,
                     long trace0,
                     int trace_bsize );

/*
 * read/write sub traces, with the same assumption and requirements as
 * segy_readtrace. start and stop are *indices*, not byte offsets, so
 * segy_readsubtr(ds, traceno, 10, 12, ...) reads samples 10 through 12, and
 * not bytes 10 through 12.
 *
 * start and stop are in the range [start,stop), so start=0, stop=5, step=2
 * yields [0, 2, 4], whereas stop=4 yields [0, 2]
 *
 * When step is negative, the subtrace will be read in reverse. If step is
 * negative and [0,n) is desired, pass use -1 for stop. Other negative values
 * are undefined. If the range [n, m) where m is larger than the samples is
 * considered undefined. Any [n, m) where distance(n,m) > samples is undefined.
 *
 * The parameter rangebuf is a pointer to a buffer of at least abs(stop-start)
 * size. This is largely intended for script-C boundaries. In code paths where
 * step is not 1 or -1, and mmap is not activated, these functions will
 * *allocate* a buffer to read data from file in chunks. This is a significant
 * speedup over multiple fread calls, at the cost of a clunkier interface. This
 * is a tradeoff, since this function is often called in an inner loop. If
 * you're fine with these functions allocating and freeing this buffer for you,
 * rangebuf can be NULL.
 */
int segy_readsubtr( segy_datasource*,
                    int traceno,
                    int start,
                    int stop,
                    int step,
                    void* buf,
                    void* rangebuf,
                    long trace0,
                    int trace_bsize );

int segy_writesubtr( segy_datasource*,
                     int traceno,
                     int start,
                     int stop,
                     int step,
                     const void* buf,
                     void* rangebuf,
                     long trace0,
                     int trace_bsize );

/*
 * convert to/from native float from segy formats (likely IBM or IEEE).  Size
 * parameter is long long because it needs to know the number of *samples*,
 * which can be very large for bulk conversion of a collection of traces.
 *
 * to/from native are unaware of the host architecture, and always assume MSB
 * layout. However, the read/write functions of segyio are aware, so as long as
 * only segyio functions are used, you do not need to care about the endianenss
 * of your platform. Some care must be taken, because you need to explicitly
 * tell segyio if your file uses LSB.
 */
int segy_to_native( int format,
                    long long size,
                    void* buf );

int segy_from_native( int format,
                      long long size,
                      void* buf );

int segy_read_line( segy_datasource* ds,
                    int line_trace0,
                    int line_length,
                    int stride,
                    int offsets,
                    void* buf,
                    long trace0,
                    int trace_bsize );

int segy_write_line( segy_datasource* ds,
                    int line_trace0,
                    int line_length,
                    int stride,
                    int offsets,
                    const void* buf,
                    long trace0,
                    int trace_bsize );

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
 *
 * If the file has only 1 trace (or, for pre-stack files, 1-trace-per-offset),
 * segyio considers this as 1 line in each direction.
 */
int segy_count_lines( segy_datasource*,
                      int field,
                      int offsets,
                      int* l1out,
                      int* l2out,
                      long trace0,
                      int trace_bsize );

/*
 * Alternative interface for segy_count_lines. If you have information about
 * sorting this is easier to use, but requires both the inline and crossline
 * header field positions. Does the argument shuffling needed to call
 * segy_count_lines.
 */
int segy_lines_count( segy_datasource*,
                      int il,
                      int xl,
                      int sorting,
                      int offsets,
                      int* il_count,
                      int* xl_count,
                      long trace0,
                      int trace_bsize );
/*
 * Find the `line_length` for the inlines. Assumes all inlines, crosslines and
 * traces don't vary in length.
 *
 * `inline_count` and `crossline_count` are the two values obtained with
 * `segy_count_lines`.
 *
 * These functions cannot fail and return the length, not an error code.
 */
int segy_inline_length(int crossline_count);

int segy_crossline_length(int inline_count);

/*
 * Find the indices of the inlines and write to `buf`. `offsets` are the number
 * of offsets for this file as returned by `segy_offsets`
 */
int segy_inline_indices( segy_datasource*,
                         int il,
                         int sorting,
                         int inline_count,
                         int crossline_count,
                         int offsets,
                         int* buf,
                         long trace0,
                         int trace_bsize );

int segy_crossline_indices( segy_datasource*,
                            int xl,
                            int sorting,
                            int inline_count,
                            int crossline_count,
                            int offsets,
                            int* buf,
                            long trace0,
                            int trace_bsize );

/*
 * Find the first `traceno` of the line `lineno`. `linenos` should be the line
 * indices returned by `segy_inline_indices` or `segy_crossline_indices`. The
 * stride depends on the sorting and is given by `segy_inline_stride` or
 * `segy_crossline_stride`. `offsets` is given by `segy_offsets` function, and
 * is the number of offsets in this file (1 for post stack data). `line_length`
 * is the length, i.e. traces per line, given by `segy_inline_length` or
 * `segy_crossline_length`.
 *
 * To read/write an inline, read `line_length` starting at `traceno`,
 * incrementing `traceno` with `stride` `line_length` times.
 */
int segy_line_trace0( int lineno,
                      int line_length,
                      int stride,
                      int offsets,
                      const int* linenos,
                      int linenos_sz,
                      int* traceno );

/*
 * Find the `rotation` of the survey in radians.
 *
 * Returns the clock-wise rotation around north, i.e. the angle between the
 * first line given and north axis. In this context, north is the direction
 * that yields a higher CDP-Y coordinate, and east is the direction that yields
 * a higher CDP-X coordinate.
 *
 * N
 * |
 * |
 * | +
 * | |~~/``````/
 * | | /------/
 * | |/,,,,,,/
 * |
 * +--------------- E
 *
 *
 * When the survey is as depicted, and the first line is starting in the
 * south-west corner and goes north, the angle (~~) is < pi/4. If the first
 * line is parallel with equator moving east, the angle is pi/2.
 *
 * The return value is in the domain [0, 2pi)
 */
int segy_rotation_cw( segy_datasource*,
                      int line_length,
                      int stride,
                      int offsets,
                      const int* linenos,
                      int linenos_sz,
                      float* rotation,
                      long trace0,
                      int trace_bsize );

/*
 * Find the stride needed for an inline/crossline traversal.
 */
int segy_inline_stride( int sorting,
                        int inline_count,
                        int* stride );

int segy_crossline_stride( int sorting,
                           int crossline_count,
                           int* stride );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //SEGYIO_SEGY_H
