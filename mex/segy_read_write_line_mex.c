#include <string.h>
#include "mex.h"
#include "matrix.h"

#include "segyutil.h"

#include <segyio/segy.h>

/* The gateway function */
void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {

    bool read;

    if (nrhs == 6) {
        read = true;
    }
    else if (nrhs == 7) {
        read = false;
    }
    else {
        goto ERROR;
    }

    const mxArray* mx_spec = prhs[0];
    const mxArray* mx_index = prhs[1];
    const mxArray* mx_line_length = prhs[2];
    const mxArray* mx_line_indexes = prhs[3];
    const mxArray* mx_stride = prhs[4];
    const mxArray* mx_offsets = prhs[5];

    SegySpec spec;
    recreateSpec(&spec,mx_spec);

    size_t index = (size_t)mxGetScalar(mx_index);

    uint32_t line_length = (uint32_t)mxGetScalar(mx_line_length);

    uint32_t* line_indexes = (uint32_t*)mxGetData(mx_line_indexes);
    int n = mxGetN(mx_line_indexes);
    int m = mxGetM(mx_line_indexes);
    uint32_t line_count = (n>m)? n:m;

    uint32_t stride = (uint32_t)mxGetScalar(mx_stride);
    int32_t offsets = (int32_t)mxGetScalar(mx_offsets);

    segy_file* fp;

    int line_trace0;
    int errc = segy_line_trace0( index, line_length, stride, offsets, line_indexes, line_count, &line_trace0 );
    if (errc != 0) {
        goto CLEANUP;
    }

    if (read) {
        fp = segy_open( spec.filename, "rb" );
        if (fp == NULL) {
            goto CLEANUP;
        }

        plhs[0] = mxCreateNumericMatrix(spec.sample_count, line_length, mxSINGLE_CLASS, mxREAL);
        float *data_ptr = (float *) mxGetData(plhs[0]);

        errc = segy_read_line( fp, line_trace0, line_length, stride, offsets, data_ptr, spec.first_trace_pos, spec.trace_bsize );
        if (errc != 0) {
            goto CLEANUP;
        }

        errc = segy_to_native( spec.sample_format, line_length * spec.sample_count, data_ptr );
        if (errc != 0) {
            goto CLEANUP;
        }
    }
    else {
        fp = segy_open( spec.filename, "r+b" );
        if (fp == NULL) {
            goto CLEANUP;
        }

        const mxArray* mx_data = prhs[6];

        float *data_ptr = (float *) mxGetData(mx_data);

        errc = segy_from_native( spec.sample_format, line_length * spec.sample_count, data_ptr );
        if (errc != 0) {
            goto CLEANUP;
        }

        errc = segy_write_line( fp, line_trace0, line_length, stride, offsets, data_ptr, spec.first_trace_pos, spec.trace_bsize );
        if (errc != 0) {
            goto CLEANUP;
        }

        errc = segy_to_native( spec.sample_format, line_length * spec.sample_count, data_ptr );
        if (errc != 0) {
            goto CLEANUP;
        }
    }

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
