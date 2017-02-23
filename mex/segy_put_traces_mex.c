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

    segy_file* fp = segyfopen( prhs[ 0 ], "r+b" );
    plhs[ 0 ] = mxDuplicateArray( prhs[ 1 ] );
    int first_trace = mxGetScalar( prhs[ 2 ] );
    int last_trace  = mxGetScalar( prhs[ 3 ] );
    int notype      = mxGetScalar( prhs[ 4 ] );

    struct segy_file_format fmt = filefmt( fp );

    if( notype != -1 )
        fmt.format = notype;

    // if last_trace was defaulted we assign it to the last trace in the file
    if( last_trace == -1 )
        last_trace = fmt.traces - 1;

    int traces = 1 + (last_trace - first_trace);
    long long bufsize = (long long)fmt.samples * traces;

    if( first_trace > last_trace ) {
        msg1 = "segy:get_traces:bounds";
        msg2 = "first trace must be smaller than last trace";
        goto cleanup;
    }

    float* out      = mxGetData( plhs[ 0 ] );
    segy_from_native( fmt.format, fmt.samples * fmt.traces, out );

    float* itr = out;
    for( int i = first_trace; i <= last_trace; ++i ) {
        err = segy_writetrace( fp, i, itr, fmt.trace0, fmt.trace_bsize );
        itr += fmt.samples;

        if( err != 0 ) {
            msg1 = "segy:put_traces:segy_writetrace";
            msg2 = strerror( errno );
            fmt.traces = i;
            goto cleanup;
        }
    }

    segy_close( fp );
    segy_to_native( fmt.format, bufsize, out );
    plhs[ 1 ] = mxCreateDoubleScalar( fmt.format );

    return;

cleanup:
    segy_close( fp );
    segy_to_native( fmt.format, bufsize, out );

    mexErrMsgIdAndTxt( msg1, msg2 );
}
