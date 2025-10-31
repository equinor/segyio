#include <string.h>
#include "mex.h"
#include "matrix.h"

#include "segyutil.h"

#include <segyio/segy.h>

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {

    int errc = SEGY_OK;
    if (nrhs != 4) {
        goto ERROR;
    }

    const mxArray* mx_spec = prhs[0];
    const mxArray* mx_offset = prhs[1];
    const mxArray* mx_il_word = prhs[2];
    const mxArray* mx_xl_word = prhs[3];

    SegySpec spec;
    recreateSpec(&spec, mx_spec);

    int offset = (int)mxGetScalar(mx_offset);
    int il = (int)mxGetScalar(mx_il_word);
    int xl = (int)mxGetScalar(mx_xl_word);

    segy_file* fp = segy_open( spec.filename, "rb" );

    if( !fp ) {
        errc = SEGY_FOPEN_ERROR;
        goto ERROR;
    };

    fp->metadata.format = spec.sample_format;
    fp->metadata.trace0 = spec.first_trace_pos;
    fp->metadata.trace_bsize = spec.trace_bsize;
    fp->metadata.samplecount = spec.sample_count;

    plhs[0] = mxCreateNumericMatrix(1, spec.offset_count, mxINT32_CLASS, mxREAL);
    int* int_offsets = mxMalloc(sizeof( int ) * spec.offset_count);

    errc = segy_offset_indices(fp, offset, spec.offset_count,
                               int_offsets);

    if( errc != SEGY_OK ) goto CLEANUP;

    int32_t* plhs0 = (int32_t*)mxGetData(plhs[0]);
    for( int i = 0; i < spec.offset_count; ++i )
        plhs0[i] = int_offsets[i];

    mxFree( int_offsets );
    segy_close(fp);
    return;

    CLEANUP:
    segy_close(fp);
    ERROR:
    {
        int nfields = 1;
        const char *fnames[nfields];
        fnames[0] = "error";
        plhs[0] = mxCreateStructMatrix(0,0, nfields, fnames);
        mxSetFieldByNumber(plhs[0], 0, 0, mxCreateDoubleScalar(errc));
    }
}
