#include <errno.h>
#include <malloc.h>
#include <string.h>

#include <segyio/segy.h>
#include "segyutil.h"

static char* copyString(const char* path) {
    size_t size = strlen(path) + 1;
    char* path_copy = malloc(size);
    memcpy(path_copy, path, size);
    return path_copy;
}


int segyCreateSpec(SegySpec* spec, const char* file, unsigned int inline_field, unsigned int crossline_field, float t0, float dt) {

    int errc = 0;

    segy_file* fp = segy_open( file, "rb" );
    if (fp == NULL) {
        fprintf(stderr, "Unable to open file: '%s'\n", file);
        return -1;
    }

    spec->sample_indices = NULL;
    spec->inline_indexes = NULL;
    spec->crossline_indexes = NULL;

    char header[ SEGY_BINARY_HEADER_SIZE ];
    errc = segy_binheader( fp, header );
    if (errc!=0) {
        goto CLEANUP;
    }

    spec->filename = copyString(file);
    spec->sample_format = segy_format( header );
    spec->sample_count = segy_samples( header );

    spec->sample_indices = malloc(sizeof(float) * spec->sample_count);
    errc = segy_sample_indices(fp, t0, dt, spec->sample_count, spec->sample_indices );
    if (errc != 0) {
        goto CLEANUP;
    }

    const long trace0 = segy_trace0( header );

    spec->trace_bsize = segy_trace_bsize( segy_samples( header ) );
    int traces;
    errc = segy_traces(fp, &traces, trace0, spec->trace_bsize);
    if (errc != 0) {
        goto CLEANUP;
    }

    errc = segy_offsets(fp, inline_field, crossline_field, traces, &spec->offset_count, trace0, spec->trace_bsize);
    if (errc != 0) {
        goto CLEANUP;
    }

    errc = segy_sorting(fp, inline_field,
                            crossline_field,
                            SEGY_TR_OFFSET,
                            &spec->trace_sorting_format,
                            trace0, spec->trace_bsize);
    if (errc != 0) {
        goto CLEANUP;
    }

    int* l1;
    int* l2;
    int field;
    if (spec->trace_sorting_format == SEGY_INLINE_SORTING) {
        field = crossline_field;
        l1 = &spec->inline_count;
        l2 = &spec->crossline_count;
    } else if (spec->trace_sorting_format == SEGY_CROSSLINE_SORTING) {
        field = inline_field;
        l2 = &spec->inline_count;
        l1 = &spec->crossline_count;
    } else {
        fprintf(stderr, "Unknown sorting\n");
        goto CLEANUP;
    }

    errc = segy_count_lines(fp, field, spec->offset_count, l1, l2, trace0, spec->trace_bsize);
    if (errc != 0) {
        goto CLEANUP;
    }

    spec->inline_indexes = malloc(sizeof(int) * spec->inline_count);
    spec->crossline_indexes = malloc(sizeof(int) * spec->crossline_count);

    errc = segy_inline_indices(fp, inline_field, spec->trace_sorting_format,
                        spec->inline_count, spec->crossline_count, spec->offset_count, spec->inline_indexes,
                        trace0, spec->trace_bsize);
    if (errc != 0) {
        goto CLEANUP;
    }

    errc = segy_crossline_indices(fp, crossline_field, spec->trace_sorting_format,
                        spec->inline_count, spec->crossline_count, spec->offset_count, spec->crossline_indexes,
                        trace0, spec->trace_bsize);
    if (errc != 0) {
        goto CLEANUP;
    }

    spec->first_trace_pos = segy_trace0( header );

    errc = segy_inline_stride(spec->trace_sorting_format, spec->inline_count, &spec->il_stride);
    if (errc != 0) {
        goto CLEANUP;
    }

    errc = segy_crossline_stride(spec->trace_sorting_format, spec->crossline_count, &spec->xl_stride);
    if (errc != 0) {
        goto CLEANUP;
    }

    segy_close(fp);

    return 0;

    CLEANUP:
    if (spec->crossline_indexes != NULL)
        free(spec->crossline_indexes);
    if (spec->inline_indexes != NULL)
        free(spec->inline_indexes);
    free(spec->sample_indices);
    free(spec->filename);

    segy_close(fp);

    return errc;
}

static int getMaxDim(mxArray* arr){
    int n = mxGetN(arr);
    int m = mxGetM(arr);

    int max = m;
    if (n>m) max = n;

    return max;
}

void recreateSpec(SegySpec *spec, const mxArray* mex_spec) {

    spec->filename = mxArrayToString(mxGetProperty(mex_spec, 0, "filename"));
    spec->sample_format = (int)mxGetScalar(mxGetProperty(mex_spec, 0, "sample_format"));
    spec->trace_sorting_format = (int)mxGetScalar(mxGetProperty(mex_spec, 0, "trace_sorting_format"));
    spec->offset_count = (int)mxGetScalar(mxGetProperty(mex_spec, 0, "offset_count"));
    spec->first_trace_pos = (int)mxGetScalar(mxGetProperty(mex_spec, 0, "first_trace_pos"));
    spec->il_stride = (int)mxGetScalar(mxGetProperty(mex_spec, 0, "il_stride"));
    spec->xl_stride = (int)mxGetScalar(mxGetProperty(mex_spec, 0, "xl_stride"));
    spec->trace_bsize = (int)mxGetScalar(mxGetProperty(mex_spec, 0, "trace_bsize"));

    mxArray* crossline_indexes = mxGetProperty(mex_spec, 0, "crossline_indexes");
    spec->crossline_count = getMaxDim(crossline_indexes);
    spec->crossline_indexes = mxGetData(crossline_indexes);

    mxArray* inline_indexes = mxGetProperty(mex_spec, 0, "inline_indexes");
    spec->inline_count = getMaxDim(inline_indexes);
    spec->inline_indexes = mxGetData(inline_indexes);

    mxArray* sample_indexes = mxGetProperty(mex_spec, 0, "sample_indexes");
    spec->sample_count = getMaxDim(sample_indexes);
    spec->sample_indices = mxGetData(sample_indexes);

}

struct segy_file_format buffmt( const char* binary ) {
    struct segy_file_format fmt;
    fmt.samples = segy_samples( binary );
    fmt.trace_bsize = segy_trace_bsize( fmt.samples );
    fmt.trace0 = segy_trace0( binary );
    fmt.format = segy_format( binary );
    fmt.traces = 0;

    return fmt;
}

struct segy_file_format filefmt( segy_file* fp ) {
    char binary[SEGY_BINARY_HEADER_SIZE];
    int err = segy_binheader( fp, binary );

    if( err != 0 )
        mexErrMsgIdAndTxt( "segy:c:filemft", strerror( errno ) );

    struct segy_file_format fmt = buffmt( binary );

    err = segy_traces( fp, &fmt.traces, fmt.trace0, fmt.trace_bsize );
    if( err == 0 ) return fmt;

    const char* msg1 = "segy:c:filefmt";
    const char* msg2;

    if( err == SEGY_TRACE_SIZE_MISMATCH )
        msg2 = "Number of traces not consistent with file size. File corrupt?";
    else
        msg2 = strerror( errno );

    mexErrMsgIdAndTxt( msg1, msg2 );
}

segy_file* segyfopen( const mxArray* filename, const char* mode ) {
    const char* fname = mxArrayToString( filename );

    segy_file* fp = segy_open( fname, mode );
    int err = errno;

    mxFree( (void*)fname );

    if( !fp )
        mexErrMsgIdAndTxt( "segy:c:fopen", strerror( errno ) );

    return fp;
}
