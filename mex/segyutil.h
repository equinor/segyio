#ifndef SEGYIO_SEGYUTIL_H
#define SEGYIO_SEGYUTIL_H

#include <stdlib.h>
#include <string.h>

#include <mex.h>

#include <segyio/segy.h>

typedef struct {
    char* filename;

    int sample_format;

    int* crossline_indexes;
    int crossline_count;

    int* inline_indexes;
    int inline_count;

    int offset_count;

    float* sample_indices;
    int sample_count;

    int trace_sorting_format;

    int il_stride;
    int xl_stride;
    long first_trace_pos;
    int trace_bsize;

} SegySpec;

int segyCreateSpec(SegySpec* spec, const char* file, unsigned int inline_field, unsigned int crossline_field, float t0, float dt);

void recreateSpec(SegySpec* spec, const mxArray* mex_spec);

struct segy_file_format {
    int samples;
    long trace0;
    int trace_bsize;
    int traces;
    int format;
};

struct segy_file_format buffmt( const char* );
struct segy_file_format filefmt( segy_file* );
segy_file* segyfopen( const mxArray* filename, const char* mode );

#endif //SEGYIO_SEGYUTIL_H
