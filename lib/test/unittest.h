#ifndef SEGYIO_UNITTEST_H
#define SEGYIO_UNITTEST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

static void testAssertionFailed(const char *message, const char *file, int line) {
    fprintf(stderr, "Assertion failed in file: %s on line: %d\n", file, line);
    if (strlen(message) > 0) {
        fprintf(stderr, "%s", message);
    }
    exit(1);
}

#define assertTrue(value, message) testAssertTrue((value), (message), __FILE__, __LINE__)

static void testAssertTrue(bool value, const char *message, const char *file, int line) {
    if (!value) {
        if (!message) {
            message = "The expression did not evaluate to true!";
        }
        testAssertionFailed(message, file, line);
    }
}

#define assertClose(expected, actual, eps) testAssertClose((expected), (actual), (eps), __FILE__, __LINE__)

static void testAssertClose(double expected, double actual, double eps, const char *file, int line) {
    double diff = fabs(expected-actual);
    if (diff > eps) {
        char message[1000];
        sprintf(message, "Expected: %f, Actual: %f, diff: %f, eps: %f\n", expected, actual, diff, eps);
        testAssertionFailed(message, file, line);
    }
}



#endif //SEGYIO_UNITTEST_H
