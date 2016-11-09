#ifndef SEGYIO_SEGYUTIL_H
#define SEGYIO_SEGYUTIL_H

#include <stdlib.h>
#include <string.h>

#include <mex.h>

#include <segyio/segy.h>

#include "spec/segyspec.h"


void recreateSpec(SegySpec* spec, const mxArray* mex_spec);

struct segy_file_format {
    unsigned int samples;
    long trace0;
    unsigned int trace_bsize;
    size_t traces;
    int format;
};

struct segy_file_format buffmt( const char* );
struct segy_file_format filefmt( segy_file* );
segy_file* segyfopen( const mxArray* filename, const char* mode );

#endif //SEGYIO_SEGYUTIL_H
