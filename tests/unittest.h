#ifndef SEGYIO_UNITTEST_H
#define SEGYIO_UNITTEST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

void testAssertionFailed(const char *message, const char *file, int line) {
    fprintf(stderr, "Assertion failed in file: %s on line: %d\n", file, line);
    if (strlen(message) > 0) {
        fprintf(stderr, message);
    }
    exit(1);
}

#define assertTrue(value, message) _testAssertTrue((value), (message), __FILE__, __LINE__)

void _testAssertTrue(bool value, const char *message, const char *file, int line) {
    if (!value) {
        if (strlen(message) == 0) {
            message = "The expression did not evaluate to true!";
        }
        testAssertionFailed(message, file, line);
    }
}

#define assertClose(expected, actual, eps) _testAssertClose((expected), (actual), (eps), __FILE__, __LINE__)

void _testAssertClose(float expected, float actual, float eps, const char *file, int line) {
    float diff = fabsf(expected-actual);
    if (!(diff <= eps)) {
        char message[1000];
        sprintf(message, "Expected: %f, Actual: %f, diff: %f, eps: %f\n", expected, actual, diff, eps);
        testAssertionFailed(message, file, line);
    }
}



#endif //SEGYIO_UNITTEST_H
