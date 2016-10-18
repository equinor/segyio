#include <errno.h>
#include <stdint.h>
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

    const char* filename = mxArrayToString( prhs[ 0 ] );
    FILE* fp = segyfopen( prhs[ 0 ], "rb" );

    int field = mxGetScalar( prhs[ 1 ] );

    struct segy_file_format fmt = filefmt( fp );

    plhs[0] = mxCreateNumericMatrix( 1, fmt.traces, mxINT32_CLASS, mxREAL );
    int32_t* out = mxGetData( plhs[ 0 ] );

    char header[ SEGY_TRACE_HEADER_SIZE ];

    for( size_t i = 0; i < fmt.traces; ++i ) {
        int32_t f;
        err = segy_traceheader( fp, i, header, fmt.trace0, fmt.trace_bsize );

        if( err != 0 ) {
            msg1 = "segy:get_header:segy_traceheader";
            msg2 = strerror( errno );
            goto cleanup;
        }

        err = segy_get_field( header, field, &f );

        if( err != 0 ) {
            msg1 = "segy:get_header:segy_get_field";
            msg2 = "Error reading header field.";
            goto cleanup;
        }

        out[ i ] = f;
    }

    fclose( fp );

    plhs[ 1 ] = mxCreateDoubleScalar( fmt.traces );
    return;

cleanup:
    fclose( fp );

cleanup_fopen:
    mexErrMsgIdAndTxt( msg1, msg2 );
}
