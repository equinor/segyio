#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <spec/segyspec.h>
#include <segyio/segy.h>


static void testSegyInspection() {
    const char *path = "test-data/small.sgy";
    double t0 = 1111.0;

    SegySpec spec;

    segyCreateSpec(&spec, path, INLINE_3D, CROSSLINE_3D, t0);

    assertTrue(spec.sample_format == IBM_FLOAT_4_BYTE, "Expected the float format to be IBM Float");

    assertTrue(strcmp(spec.filename, path) == 0, "The paths did not match");

    assertTrue(spec.offset_count == 1, "Expected offset to be 1");

    assertTrue(spec.trace_sorting_format == INLINE_SORTING, "Expected sorting to be INLINE_SORTING");


    assertTrue(spec.sample_count == 50, "Expected sample count to be 50");

    for(int i = 0; i < spec.sample_count; i++) {
        double t = t0 + i * 4.0;
        assertTrue(spec.sample_indexes[i] == t, "Sample index not equal to expected value");
    }

    assertTrue(spec.inline_count == 5, "Expect inline count to be 5");
    for(int i = 0; i < spec.inline_count; i++) {
        unsigned int il = spec.inline_indexes[i];
        assertTrue(il >= 1 && il <= 5, "Expected inline index value to be between [1, 5]");
    }

    assertTrue(spec.crossline_count == 5, "Expect crossline count to be 5");
    for(int i = 0; i < spec.crossline_count; i++) {
        unsigned int xl = spec.crossline_indexes[i];
        assertTrue(xl >= 20 && xl <= 24, "Expected crossline index value to be between [20, 24]");
    }

    if (spec.crossline_indexes != NULL)
        free(spec.crossline_indexes);
    if (spec.inline_indexes != NULL)
        free(spec.inline_indexes);
    if (spec.sample_indexes != NULL)
        free(spec.sample_indexes);

    free(spec.filename);

}

static void testAlloc(){
    const char *path = "test-data/small.sgy";
    double t0 = 1111.0;
    SegySpec spec;
    segyCreateSpec(&spec, path, INLINE_3D, CROSSLINE_3D, t0);

    if (spec.crossline_indexes != NULL)
        free(spec.crossline_indexes);
    if (spec.inline_indexes != NULL)
        free(spec.inline_indexes);
    if (spec.sample_indexes != NULL)
        free(spec.sample_indexes);

    free(spec.filename);

}

int main() {
    testAlloc();
    testSegyInspection();
    exit(0);
}

