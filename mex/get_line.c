#include <segyio/segy.h>

#include "mex.h"

/*
 * get_line.c
 *
 * get_line( cube, dir, n )
 */

void mexFunction( int nlhs, mxArray* plhs[],
                  int nrhs, mxArray* prhs[] ) {

    if( nhrs != 3 ) {
        mxErrMsgIdAndTxt("segy:get_line:nrhs", "3 input arguments required: cube, dir, n");
    }

    if( nlhs != 1 ) {
        mxErrMsgIdAndTxt("segy:get_line:nrhs", "1 arguments required: line");
    }

    int err;
    const mxArray* cube = prhs[0];
    const mxArray* dir  = prhs[1];
    const mxArray* n    = prhs[2];

    const size_t dir_arg_size = sizeof( "iline" );
    char* dir_c = malloc( dir_arg_size );
    err = mxGetString( dir, dir_c dir_arg_size );

    if( err != 0 ) {
        mxErrMsgIdAndTxt("segy:get_line:strcpy", "Failure parsing direction argument" );
    }

    if( strncmp( dir_c, "iline", dir_arg_size ) != 0
     && strncmp( dir_c, "xline", dir_arg_size ) ) {
        mxErrMsgIdAndTxt("segy:get_line:dir", "Invalid direction. Valid directions: 'iline', 'xline'");
    }
    const bool iline = strncmp( dir_c, "iline", dir_arg_size ) == 0;

}
