#include <stdlib.h>

#include "segyio/segy.h"
#include "segyspec.h"

static char* copyString(const char* path) {
    size_t size = strlen(path) + 1;
    char* path_copy = malloc(size);
    memcpy(path_copy, path, size);
    return path_copy;
}


int segyCreateSpec(SegySpec* spec, const char* file, unsigned int inline_field, unsigned int crossline_field, double t0) {

    int errc = 0;

    FILE* fp = fopen( file, "r" );
    if (fp == NULL) {
        fprintf(stderr, "Unable to open file: '%s'\n", file);
        return -1;
    }

    spec->sample_indexes = NULL;
    spec->inline_indexes = NULL;
    spec->crossline_indexes = NULL;

    char header[ SEGY_BINARY_HEADER_SIZE ];
    errc = segy_binheader( fp, header );
    if (errc!=0) {
        goto CLEANUP;
    }

    spec->filename = copyString(file);
    spec->sample_format = segy_format( header );
    spec->sample_count = segy_samples( header );

    spec->sample_indexes = malloc(sizeof(double) * spec->sample_count);
    errc = segy_sample_indexes(fp, spec->sample_indexes, t0, spec->sample_count);
    if (errc != 0) {
        goto CLEANUP;
    }

    const long trace0 = segy_trace0( header );

    spec->trace_bsize = segy_trace_bsize( segy_samples( header ) );
    size_t traces;
    errc = segy_traces(fp, &traces, trace0, spec->trace_bsize);
    if (errc != 0) {
        goto CLEANUP;
    }

    errc = segy_offsets(fp, inline_field, crossline_field, traces, &spec->offset_count, trace0, spec->trace_bsize);
    if (errc != 0) {
        goto CLEANUP;
    }

    errc = segy_sorting(fp, inline_field, crossline_field, &spec->trace_sorting_format, trace0, spec->trace_bsize);
    if (errc != 0) {
        goto CLEANUP;
    }

    unsigned int* l1;
    unsigned int* l2;
    unsigned int field;
    if (spec->trace_sorting_format == INLINE_SORTING) {
        field = crossline_field;
        l1 = &spec->inline_count;
        l2 = &spec->crossline_count;
    } else if (spec->trace_sorting_format == CROSSLINE_SORTING) {
        field = inline_field;
        l2 = &spec->inline_count;
        l1 = &spec->crossline_count;
    } else {
        fprintf(stderr, "Unknown sorting\n");
        goto CLEANUP;
    }

    errc = segy_count_lines(fp, field, spec->offset_count, l1, l2, trace0, spec->trace_bsize);
    if (errc != 0) {
        goto CLEANUP;
    }

    spec->inline_indexes = malloc(sizeof(unsigned int) * spec->inline_count);
    spec->crossline_indexes = malloc(sizeof(unsigned int) * spec->crossline_count);

    errc = segy_inline_indices(fp, inline_field, spec->trace_sorting_format,
                        spec->inline_count, spec->crossline_count, spec->offset_count, spec->inline_indexes,
                        trace0, spec->trace_bsize);
    if (errc != 0) {
        goto CLEANUP;
    }

    errc = segy_crossline_indices(fp, crossline_field, spec->trace_sorting_format,
                        spec->inline_count, spec->crossline_count, spec->offset_count, spec->crossline_indexes,
                        trace0, spec->trace_bsize);
    if (errc != 0) {
        goto CLEANUP;
    }

    spec->first_trace_pos = segy_trace0( header );

    errc = segy_inline_stride(spec->trace_sorting_format, spec->inline_count, &spec->il_stride);
    if (errc != 0) {
        goto CLEANUP;
    }

    errc = segy_crossline_stride(spec->trace_sorting_format, spec->crossline_count, &spec->xl_stride);
    if (errc != 0) {
        goto CLEANUP;
    }

    fclose(fp);

    return 0;

    CLEANUP:
    if (spec->crossline_indexes != NULL)
        free(spec->crossline_indexes);
    if (spec->inline_indexes != NULL)
        free(spec->inline_indexes);
    if (spec->sample_indexes != NULL)
        free(spec->sample_indexes);
    free(spec->filename);

    fclose(fp);

    return errc;
}
