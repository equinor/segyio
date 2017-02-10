#ifndef SEGYIO_SEGYUTIL_H
#define SEGYIO_SEGYUTIL_H

#include <stdlib.h>
#include <string.h>

#include <mex.h>

#include <segyio/segy.h>

typedef struct {
    char* filename;

    unsigned int sample_format;

    unsigned int* crossline_indexes;
    unsigned int crossline_count;

    unsigned int* inline_indexes;
    unsigned int inline_count;

    unsigned int offset_count;

    double* sample_indexes;
    int sample_count;

    int trace_sorting_format;

    unsigned int il_stride;
    unsigned int xl_stride;
    unsigned int first_trace_pos;
    unsigned int trace_bsize;

} SegySpec;

int segyCreateSpec(SegySpec* spec, const char* file, unsigned int inline_field, unsigned int crossline_field, double t0, double dt);

void recreateSpec(SegySpec* spec, const mxArray* mex_spec);

struct segy_file_format {
    int samples;
    long trace0;
    unsigned int trace_bsize;
    size_t traces;
    int format;
};

struct segy_file_format buffmt( const char* );
struct segy_file_format filefmt( segy_file* );
segy_file* segyfopen( const mxArray* filename, const char* mode );

#endif //SEGYIO_SEGYUTIL_H
