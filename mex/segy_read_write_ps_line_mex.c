#include <string.h>
#include "mex.h"
#include "matrix.h"

#include "segyutil.h"

#include <segyio/segy.h>

/* The gateway function */
void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {

    if( nrhs < 5 || nrhs > 6 ) goto ERROR;
    bool read = nrhs == 5;

    const mxArray* mx_spec = prhs[0];
    const mxArray* mx_index = prhs[1];
    const mxArray* mx_line_length = prhs[2];
    const mxArray* mx_line_indexes = prhs[3];
    const mxArray* mx_stride = prhs[4];

    SegySpec spec;
    recreateSpec(&spec, mx_spec);

    size_t index = (size_t)mxGetScalar(mx_index);

    uint32_t line_length = (uint32_t)mxGetScalar(mx_line_length);

    uint32_t* line_indexes = (uint32_t*)mxGetData(mx_line_indexes);
    int n = mxGetN(mx_line_indexes);
    int m = mxGetM(mx_line_indexes);
    uint32_t line_count = (n>m)? n:m;

    uint32_t stride = (uint32_t)mxGetScalar(mx_stride);
    int offsets = spec.offset_count;

    segy_file* fp;

    int line_trace0;
    int errc = segy_line_trace0( index, line_length, stride, offsets, line_indexes, line_count, &line_trace0 );
    if( errc != SEGY_OK ) {
        mexErrMsgIdAndTxt( "segy:get_ps_line:wrong_line_number",
                           "Specified line number is not in cube." );
        return;
    }

    const char* mode = read ? "rb" : "r+b";
    fp = segy_open( spec.filename, mode );
    if( !fp ) {
        mexErrMsgIdAndTxt( "segy:get:ps_line:file",
                           "unable to open file" );
        return;
    }

    mwSize dims[] = { spec.sample_count, spec.offset_count, line_length };

    const size_t tr_size = SEGY_TRACE_HEADER_SIZE + spec.trace_bsize;

    if( read ) {
        plhs[0] = mxCreateNumericArray(3, dims, mxSINGLE_CLASS, mxREAL );
        float* buf = (float*) mxGetData(plhs[0]);

        for( int i = 0; i < offsets; ++i ) {
            errc = segy_read_line( fp,
                                   line_trace0,
                                   line_length,
                                   stride,
                                   offsets,
                                   buf + (spec.sample_count * line_length * i),
                                   spec.first_trace_pos + (i * tr_size),
                                   spec.trace_bsize );

            if( errc != 0 ) goto CLEANUP;
        }

        errc = segy_to_native( spec.sample_format,
                               offsets * line_length * spec.sample_count,
                               buf );

        if( errc != 0 ) goto CLEANUP;
    }
    else {
        const mxArray* mx_data = prhs[5];
        float* buf = (float*) mxGetData(mx_data);

        errc = segy_from_native( spec.sample_format,
                                 offsets * line_length * spec.sample_count,
                                 buf );

        if( errc != 0 ) goto CLEANUP;

        for( int i = 0; i < offsets; ++i ) {
            errc = segy_write_line( fp,
                                    line_trace0,
                                    line_length,
                                    stride,
                                    offsets,
                                    buf + (spec.sample_count * line_length * i),
                                    spec.first_trace_pos + (i * tr_size),
                                    spec.trace_bsize );
            if( errc != 0 ) goto CLEANUP;
        }

        errc = segy_to_native( spec.sample_format,
                               offsets * line_length * spec.sample_count,
                               buf );
        if( errc != 0 ) goto CLEANUP;
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
