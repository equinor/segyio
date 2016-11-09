#include <segyio/segy.h>
#include "segyutil.h"

#include "mex.h"

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {

    segy_file* fp = segyfopen( prhs[ 0 ], "rb" );
    struct segy_file_format fmt = filefmt( fp );
    segy_close( fp );

    plhs[0] = mxCreateDoubleScalar( fmt.traces );
}
