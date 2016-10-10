#include <stdlib.h>
#include <string.h>
#include <segyio/segy.h>
#include "unittest.h"

void ebcdic2ascii( const char*, char* );
void ascii2ebcdic( const char*, char* );

void testEbcdicConversion() {
    char* expected = (char*) "Hello there!";
    char str[] = "\xc8\x85\x93\x93\x96\x40\xa3\x88\x85\x99\x85\x4f";

    char result[strlen(expected)];

    ebcdic2ascii(str, result);
    assertTrue(strcmp(result, expected) == 0, "Converted string did not match the expected result!");

    ascii2ebcdic(result, result);
    assertTrue(strcmp(result, str) == 0, "Converted string did not match the expected result!");
}

void testEbcdicTable() {
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
    for(unsigned char i = 0; i < 255; i++) {
    }
}

void testConversionAllocation() {
    char* expected = (char*) "Hello there!";
    char str[] = "\xc8\x85\x93\x93\x96\x40\xa3\x88\x85\x99\x85\x4f";

    char result[strlen(str) + 1];
    ebcdic2ascii(str, result);
    assertTrue(strcmp(result, expected) == 0, "Converted string did not match the expected result!");

    ascii2ebcdic(expected, result);
    assertTrue(strcmp(result, str) == 0, "Converted string did not match the expected result!");
}

#define MAX 1000000 /* number of iterations */
#define IBM_EPS 4.7683738e-7 /* worst case error */

void ibm2ieee(void* to, const void* from, int len);
void ieee2ibm(void* to, const void* from, int len);

static void check(float f1, double * epsm) {
    int exp;
    float f2;
    double eps;
    unsigned ibm1, ibm2;

    frexp(f1, &exp);
    ieee2ibm(&ibm1, &f1, 1);
    ibm2ieee(&f2, &ibm1, 1);
    ieee2ibm(&ibm2, &f2, 1);

    assertTrue(memcmp(&ibm1, &ibm2, sizeof ibm1) == 0, "The content of two memory areas were not identical!");
    //printf("Error: %08x <=> %08x\n", *(unsigned*) &ibm1, *(unsigned*) &ibm2);

    eps = ldexp(fabs(f1 - f2), -exp);
    if (eps > *epsm) {
        *epsm = eps;
    }

    assertTrue(eps < IBM_EPS, "Difference over conversion larger than allowed epsilon!");
    //printf("Error: %.8g != %.8g\n", f1, f2);
}

void testIBMFloat() {
    int i;
    float f1;

    double epsm = 0.0;
    for (i = 0; i < MAX; i++) {
        f1 = rand();
        check(f1, &epsm);
        check(-f1, &epsm);
    }
    assertClose(4.17233e-07, epsm, 1e-06);
}

int to_int16( const char* );
int to_int32( const char* );

int16_t from_int16( int16_t );
int32_t from_int32( int32_t );

void test_integer_round_trip() {
    /* this test probably only works as expected on intel/x86 */
    /* this is what data looks like when read from segy */
    char buf16[ 2 ] = { 000, 001 }; /* 1 */
    /* unsigned to avoid overflow warning on 0257 */
    unsigned char buf32[ 4 ] = { 0, 0, 011, 0257 };

    int i1 = to_int16( buf16 );
    int i2479 = to_int32( (char*)buf32 );
    assertTrue( i1 == 1, "Expected SEGY two's complement 2-byte 1 => 1" );
    assertTrue( i2479 == 2479, "Expected SEGY two's complement 4-byte 2479 => 2479" );

    int16_t round_int16 = from_int16( 1 );
    int32_t round_int32 = from_int32( 2479 );

    assertTrue( memcmp( &round_int16, buf16, sizeof( round_int16 ) ) == 0,
                "int16 did not survive round trip" );
    assertTrue( memcmp( &round_int32, buf32, sizeof( round_int32 ) ) == 0,
                "int32 did not survive round trip" );
}

int main() {
    testEbcdicConversion();
    testEbcdicTable();
    testConversionAllocation();
    testIBMFloat();
    test_integer_round_trip();
    exit(0);
}
