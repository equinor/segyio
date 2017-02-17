#ifndef SEGYIO_UTILS_H
#define SEGYIO_UTILS_H

#include <stdio.h>

/*
 * Functions that are internal implementations detail, but exposed to the
 * testing utilities. These functions won't show up in the installed headers.
 */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct segy_file_handle;

void ebcdic2ascii( const char* ebcdic, char* ascii );
void ascii2ebcdic( const char* ascii, char* ebcdic );
void ibm2ieee(void* to, const void* from);
void ieee2ibm(void* to, const void* from);
int segy_seek( struct segy_file_handle*, int, long, int );
long long segy_ftell( struct segy_file_handle* );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //SEGYIO_UTILS_H
