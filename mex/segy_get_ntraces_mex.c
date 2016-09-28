#include <segyio/segy.h>
#include "segyutil.h"

#include "mex.h"

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {

    FILE* fp = segyfopen( prhs[ 0 ], "r" );
    struct segy_file_format fmt = filefmt( fp );
    fclose( fp );

    plhs[0] = mxCreateDoubleScalar( fmt.traces );
}
