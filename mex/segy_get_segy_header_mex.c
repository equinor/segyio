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

    segy_file* fp = segyfopen( prhs[ 0 ], "rb" );
    char* textheader = mxMalloc( segy_textheader_size() );
    err = segy_read_textheader( fp, textheader );

    if( err != 0 ) {
        msg1 = "segy:text_header:os";
        msg2 = strerror( errno );
        goto cleanup;
    }
    plhs[ 0 ] = mxCreateString( textheader );

    mwSize dims[ 1 ] = { segy_binheader_size() };
    plhs[ 1 ] = mxCreateCharArray( 1, dims );
    err = segy_binheader( fp, mxGetData( plhs[ 1 ] ) );

    if( err != 0 ) {
        msg1 = "segy:binary_header:os";
        msg2 = strerror( errno );
        goto cleanup;
    }

    mxFree( textheader );
    return;

cleanup:
    segy_close( fp );
    mexErrMsgIdAndTxt( msg1, msg2 );
}
