#include <string.h>
#include "mex.h"

#include <segyio/segy.h>
#include "segyutil.h"

mxArray *createPLHSStruct() {
    int nfields = 11;
    const char *fnames[nfields];

    fnames[0] = "filename";
    fnames[1] = "sample_format";
    fnames[2] = "crossline_indexes";
    fnames[3] = "inline_indexes";
    fnames[4] = "sample_indexes";
    fnames[5] = "trace_sorting_format";
    fnames[6] = "offset_count";
    fnames[7] = "first_trace_pos";
    fnames[8] = "il_stride";
    fnames[9] = "xl_stride";
    fnames[10] = "trace_bsize";

    mxArray *plhs = mxCreateStructMatrix(1,1,nfields,fnames);

    return plhs;
}


void checkInputOutputSizes(int nlhs, int nrhs ) {

    /* check for proper number of arguments */
    if(nrhs!=5) {
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:nrhs","Four inputs required.");
    }
    if(nlhs!=1) {
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:nlhs","One output required.");
    }

}

void checkInputOutput(int nlhs, mxArray **plhs,
                      int nrhs, const mxArray **prhs) {

    checkInputOutputSizes(nlhs, nrhs);

    /* First input must be a string */
    if ( mxIsChar(prhs[0]) != 1) {
        mexErrMsgIdAndTxt("SegyIo:segyspec:inputNotString",
                          "Input must be a string.");
    }

    /* First input must be a row vector */
    if (mxGetM(prhs[0])!=1) {
        mexErrMsgIdAndTxt("SegyIo:segyspec:inputNotVector",
                          "Input must be a row vector.");
    }

    /* make sure the second input argument is int */
    if( !mxIsNumeric(prhs[1]) ||
        mxGetNumberOfElements(prhs[1])!=1 ) {
        mexErrMsgIdAndTxt("SegyIo:segyspec:notScalar","Input multiplier must be a numeric.");
    }

    /* make sure the third input argument is int */
    if( !mxIsNumeric(prhs[2]) ||
        mxGetNumberOfElements(prhs[2])!=1 ) {
        mexErrMsgIdAndTxt("SegyIo:segyspec:notScalar","Input multiplier must be a numeric.");
    }

    /* make sure the fourth input argument is double */
    if( !mxIsDouble(prhs[3]) ||
        mxGetNumberOfElements(prhs[3])!=1 ) {
        mexErrMsgIdAndTxt("SegyIo:segyspec:notScalar","Input multiplier must be a double.");
    }

    /* make sure the fifth input argument is double */
    if( !mxIsDouble(prhs[4]) ||
        mxGetNumberOfElements(prhs[4])!=1 ) {
        mexErrMsgIdAndTxt("SegyIo:segyspec:notScalar","Input multiplier must be a double.");
    }


}

/* The gateway function */
void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[]) {

    plhs[0] = createPLHSStruct();

    checkInputOutput(nlhs, plhs, nrhs, prhs);
    char *filename = mxArrayToString(prhs[0]);

    int il = (int)mxGetScalar(prhs[1]);
    int xl = (int)mxGetScalar(prhs[2]);
    double t0 = mxGetScalar(prhs[3]);
    double dt = mxGetScalar(prhs[4]);

    SegySpec spec;
    int errc = segyCreateSpec(&spec, filename, il, xl, t0 * 1000.0, dt);
    if (errc != 0) {
        goto FAILURE;
    }
    mxSetFieldByNumber(plhs[0], 0, 0, mxCreateString(spec.filename));

    mxSetFieldByNumber(plhs[0], 0, 1, mxCreateDoubleScalar(spec.sample_format));

    mxArray *crossline_indexes = mxCreateDoubleMatrix(spec.crossline_count, 1, mxREAL);
    double *crossline_indexes_ptr = mxGetPr(crossline_indexes);
    for (int i = 0; i < spec.crossline_count; i++) {
        crossline_indexes_ptr[i] = spec.crossline_indexes[i];
    }
    mxSetFieldByNumber(plhs[0], 0, 2, crossline_indexes);

    mxArray *inline_indexes = mxCreateDoubleMatrix(spec.inline_count, 1, mxREAL);
    double *inline_indexes_ptr = mxGetPr(inline_indexes);
    for (int i = 0; i < spec.inline_count; i++) {
        inline_indexes_ptr[i] = spec.inline_indexes[i];
    }
    mxSetFieldByNumber(plhs[0], 0, 3, inline_indexes);

    mxArray *mx_sample_indexes = mxCreateDoubleMatrix(spec.sample_count,1, mxREAL);
    double *mx_sample_indexes_ptr = mxGetPr(mx_sample_indexes);
    for (int i = 0; i < spec.sample_count; i++) {
        mx_sample_indexes_ptr[i] = spec.sample_indices[i];
    }
    mxSetFieldByNumber(plhs[0], 0, 4, mx_sample_indexes);

    mxSetFieldByNumber(plhs[0], 0, 5, mxCreateDoubleScalar(spec.trace_sorting_format));
    mxSetFieldByNumber(plhs[0], 0, 6, mxCreateDoubleScalar(spec.offset_count));
    mxSetFieldByNumber(plhs[0], 0, 7, mxCreateDoubleScalar(spec.first_trace_pos));
    mxSetFieldByNumber(plhs[0], 0, 8, mxCreateDoubleScalar(spec.il_stride));
    mxSetFieldByNumber(plhs[0], 0, 9, mxCreateDoubleScalar(spec.xl_stride));
    mxSetFieldByNumber(plhs[0], 0, 10, mxCreateDoubleScalar(spec.trace_bsize));

    if (spec.crossline_indexes != NULL)
        free(spec.crossline_indexes);
    if (spec.inline_indexes != NULL)
        free(spec.inline_indexes);
    free(spec.sample_indices);
    if (spec.filename != NULL)
        free(spec.filename);
    mxFree(filename);

    return;

    FAILURE:
    {
        int nfields = 1;
        const char *fnames[nfields];
        fnames[0] = "error";
        plhs[0] = mxCreateStructMatrix(0,0, nfields, fnames);
    }
    mxFree(filename);

}
