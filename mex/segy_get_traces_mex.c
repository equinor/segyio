#include <errno.h>
#include <string.h>

#include <segyio/segy.h>
#include "segyutil.h"

#include "matrix.h"
#include "mex.h"

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {

    char* msg1;
    char* msg2;
    int err;

    FILE* fp = segyfopen( prhs[ 0 ], "rb" );
    int first_trace = mxGetScalar( prhs[ 1 ] );
    int last_trace  = mxGetScalar( prhs[ 2 ] );
    int notype      = mxGetScalar( prhs[ 3 ] );

    struct segy_file_format fmt = filefmt( fp );

    char binary[ SEGY_BINARY_HEADER_SIZE ];
    err = segy_binheader( fp, binary );
    if( err != 0 ) {
        msg1 = "segy:get_traces:binary";
        msg2 = strerror( errno );
        goto cleanup;
    }

    if( last_trace != -1 && last_trace < fmt.traces )
        fmt.traces = (last_trace + 1);

    fmt.traces -= first_trace;

    plhs[0] = mxCreateNumericMatrix( fmt.samples, fmt.traces, mxSINGLE_CLASS, mxREAL );
    float* out = mxGetData( plhs[ 0 ] );

    if( first_trace > fmt.traces ) {
        msg1 = "segy:get_traces:bounds";
        msg2 = "first trace must be smaller than last trace";
        goto cleanup;
    }

    for( size_t i = first_trace; i < fmt.traces; ++i ) {
        err = segy_readtrace( fp, i, out, fmt.trace0, fmt.trace_bsize );
        out += fmt.samples;

        if( err != 0 ) {
            msg1 = "segy:get_traces:segy_readtrace";
            msg2 = strerror( errno );
            goto cleanup;
        }
    }

    fclose( fp );

    if( notype != -1 )
        fmt.format = notype;

    segy_to_native( fmt.format, fmt.samples * fmt.traces, mxGetData( plhs[ 0 ] ) );

    int interval;
    segy_get_bfield( binary, BIN_Interval, &interval );
    plhs[ 1 ] = mxCreateDoubleScalar( interval );
    plhs[ 2 ] = mxCreateDoubleScalar( fmt.format );

    return;

cleanup:
    fclose( fp );

cleanup_fopen:
    mexErrMsgIdAndTxt( msg1, msg2 );
}
