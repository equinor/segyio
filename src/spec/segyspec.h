#ifndef SEGYIO_SEGYSPEC_H
#define SEGYIO_SEGYSPEC_H

#include <string.h>

typedef struct {
    char* filename;

    unsigned int sample_format;

    unsigned int* crossline_indexes;
    unsigned int crossline_count;

    unsigned int* inline_indexes;
    unsigned int inline_count;

    unsigned int offset_count;

    double* sample_indexes;
    unsigned int sample_count;

    unsigned int trace_sorting_format;

    unsigned int il_stride;
    unsigned int xl_stride;
    unsigned int first_trace_pos;
    unsigned int trace_bsize;

} SegySpec;

int segyCreateSpec(SegySpec* spec, const char* file, unsigned int inline_field, unsigned int crossline_field, double t0);

#endif //SEGYIO_SEGYSPEC_H
