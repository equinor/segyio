#ifndef SEGYIO_APPUTILS_H
#define SEGYIO_APPUTILS_H

int errmsg( int errcode, const char* msg );
int errmsg2( int errcode, const char* prelude, const char* msg );
int parseint( const char* str, int* x );
int printversion( const char* name );

#endif //SEGYIO_APPUTILS_H
