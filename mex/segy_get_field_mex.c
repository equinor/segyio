#include <errno.h>
#include <string.h>

#include <segyio/segy.h>
#include "segyutil.h"

#include "matrix.h"
#include "mex.h"

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {

    const char* bin = mxGetData( prhs[ 0 ] );
    const int field = mxGetScalar( prhs[ 1 ] );
    int f;

    int err = segy_get_field( bin, field, &f );

    if( err == SEGY_INVALID_FIELD )
        mexErrMsgIdAndTxt( "segy:get_field:invalid_field",
                           "Invalid field value/header offset" );

    plhs[ 0 ] = mxCreateDoubleScalar( f );
}
