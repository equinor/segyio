#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apputils.h"
#include <segyio/segy.h>

int errmsg( int errcode, const char* msg ) {
    if( !msg ) return errcode;

    fputs( msg,  stderr );
    fputc( '\n', stderr );
    return errcode;
}

int errmsg2( int errcode, const char* prelude, const char* msg ) {
    if( !prelude ) return errmsg( errcode, msg );

    fputs( prelude, stderr );
    fputc( ':', stderr );
    fputc( ' ', stderr );
    return errmsg( errcode, msg );
}

int parseint( const char* str, int* x ) {
    char* endptr;
    *x = strtol( str, &endptr, 10 );

    if( *endptr != '\0' ) return 1;
    if( *x < 0 ) return 2;
    return 0;
}

int bfield( const char* header, int field ) {
    int32_t f;
    int err = segy_get_bfield( header, field, &f );

    if( err ) return -1;
    return f;
}

int trfield( const char* header, int field ) {
    int32_t f;
    int err = segy_get_field( header, field, &f );

    if( err ) return -1;
    return f;
}

int printversion( const char* name ) {
    printf( "%s (segyio version %d.%d)\n", name, segyio_MAJOR, segyio_MINOR );
    return 0;
}
