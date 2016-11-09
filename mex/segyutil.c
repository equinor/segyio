#include <errno.h>
#include <malloc.h>
#include <string.h>

#include "segyio/segy.h"
#include "segyutil.h"

static int getMaxDim(mxArray* arr){
    int n = mxGetN(arr);
    int m = mxGetM(arr);

    int max = m;
    if (n>m) max = n;

    return max;
}

void recreateSpec(SegySpec *spec, const mxArray* mex_spec) {

    spec->filename = mxArrayToString(mxGetProperty(mex_spec, 0, "filename"));
    spec->sample_format = (unsigned int)mxGetScalar(mxGetProperty(mex_spec, 0, "sample_format"));
    spec->trace_sorting_format = (unsigned int)mxGetScalar(mxGetProperty(mex_spec, 0, "trace_sorting_format"));
    spec->offset_count = (unsigned int)mxGetScalar(mxGetProperty(mex_spec, 0, "offset_count"));
    spec->first_trace_pos = (unsigned int)mxGetScalar(mxGetProperty(mex_spec, 0, "first_trace_pos"));
    spec->il_stride = (unsigned int)mxGetScalar(mxGetProperty(mex_spec, 0, "il_stride"));
    spec->xl_stride = (unsigned int)mxGetScalar(mxGetProperty(mex_spec, 0, "xl_stride"));
    spec->trace_bsize = (unsigned int)mxGetScalar(mxGetProperty(mex_spec, 0, "trace_bsize"));

    mxArray* crossline_indexes = mxGetProperty(mex_spec, 0, "crossline_indexes");
    spec->crossline_count = getMaxDim(crossline_indexes);
    spec->crossline_indexes = mxGetData(crossline_indexes);

    mxArray* inline_indexes = mxGetProperty(mex_spec, 0, "inline_indexes");
    spec->inline_count = getMaxDim(inline_indexes);
    spec->inline_indexes = mxGetData(inline_indexes);

    mxArray* sample_indexes = mxGetProperty(mex_spec, 0, "sample_indexes");
    spec->sample_count = getMaxDim(sample_indexes);
    spec->sample_indexes = mxGetData(sample_indexes);

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
