#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <segyio/segy.h>
#include "segyutil.h"

#include "matrix.h"
#include "mex.h"

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {

    char* msg1 = "";
    char* msg2 = "";
    int err;

    const char* filename = mxArrayToString( prhs[ 0 ] );
    segy_file* fp = segyfopen( prhs[ 0 ], "rb" );

    if( !fp ) goto cleanup;

    int field = mxGetScalar( prhs[ 1 ] );

    struct segy_file_format fmt = filefmt( fp );

    plhs[0] = mxCreateNumericMatrix( 1, fmt.traces, mxINT32_CLASS, mxREAL );
    int* out = mxGetData( plhs[ 0 ] );


    err = segy_field_forall( fp, field,
                             0, fmt.traces, 1, /* start, stop, step */
                             out, fmt.trace0, fmt.trace_bsize );

    int no = errno;
    segy_close( fp );

    if( err != SEGY_OK )
        mexErrMsgIdAndTxt( "segy:get_header:forall", strerror( errno ) );

    plhs[ 1 ] = mxCreateDoubleScalar( fmt.traces );
    return;

cleanup:
    mexErrMsgIdAndTxt( "segy:get_header:fopen", strerror( errno ) );
}
