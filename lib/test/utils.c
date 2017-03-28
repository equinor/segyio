#include <stdlib.h>
#include <string.h>
#include <segyio/segy.h>
#include <segyio/util.h>
#include "unittest.h"

static void testEbcdicConversion() {
    char expected[] = "Hello there!";
    char str[] = "\xc8\x85\x93\x93\x96\x40\xa3\x88\x85\x99\x85\x4f";

    char result[ sizeof( expected ) ];

    ebcdic2ascii(str, result);
    assertTrue(strcmp(result, expected) == 0, "Converted string did not match the expected result!");

    ascii2ebcdic(result, result);
    assertTrue(strcmp(result, str) == 0, "Converted string did not match the expected result!");
}

static void testEbcdicTable() {
    char ascii[256];
    for (unsigned char i = 0; i < 255; i++) {
        ascii[i] = (char) (i + 1);
    }
    ascii[255] = 0;

    char ebcdic[256];
    ascii2ebcdic((const char*) ascii, ebcdic);

    char result_ascii[256];
    ebcdic2ascii(ebcdic, result_ascii);

    assertTrue(strcmp(ascii, result_ascii) == 0, "Conversion from EBCDIC to ASCII to EBCDIC failed!");
}

static void testConversionAllocation() {
    char expected[] = "Hello there!";
    char str[] = "\xc8\x85\x93\x93\x96\x40\xa3\x88\x85\x99\x85\x4f";

    char result[ sizeof( str ) ];
    ebcdic2ascii(str, result);
    assertTrue(strcmp(result, expected) == 0, "Converted string did not match the expected result!");

    ascii2ebcdic(expected, result);
    assertTrue(strcmp(result, str) == 0, "Converted string did not match the expected result!");
}

#define MAX 1000000 /* number of iterations */
#define IBM_EPS 4.7683738e-7 /* worst case error */

static void check(float f1, double * epsm) {
    int exp;
    float f2;
    double eps;
    unsigned ibm1, ibm2;

    frexp(f1, &exp);
    ieee2ibm(&ibm1, &f1);
    ibm2ieee(&f2, &ibm1);
    ieee2ibm(&ibm2, &f2);

    assertTrue(memcmp(&ibm1, &ibm2, sizeof ibm1) == 0, "The content of two memory areas were not identical!");
    //printf("Error: %08x <=> %08x\n", *(unsigned*) &ibm1, *(unsigned*) &ibm2);

    eps = ldexp(fabs(f1 - f2), -exp);
    if (eps > *epsm) {
        *epsm = eps;
    }

    assertTrue(eps < IBM_EPS, "Difference over conversion larger than allowed epsilon!");
    //printf("Error: %.8g != %.8g\n", f1, f2);
}

static void testIBMFloat() {
    double epsm = 0.0;
    for( int i = 0; i < MAX; i++) {
        const float f1 = rand();
        check(f1, &epsm);
        check(-f1, &epsm);
    }
    assertClose(4.17233e-07, epsm, 1e-06);
}

int main() {
    testEbcdicConversion();
    testEbcdicTable();
    testConversionAllocation();
    testIBMFloat();
    exit(0);
}
