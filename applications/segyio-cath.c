#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <unistd.h>

#include "apputils.c"
#include <segyio/segy.h>

static int help() {
    puts( "Usage: segyio-cath [OPTION]... [FILE]...\n"
          "Concatenate the textual header(s) from FILE(s) to standard output.\n"
          "\n"
          "-n, --num        the textual header to show, starts at 0\n"
          "-a, --all        all textual headers\n"
          "-s, --strict     abort if a header or file is not found\n"
          "                 primarily meant for shell scripts\n"
          "-S, --nonstrict  ignore missing headers\n"
          "                 this is the default behaviour\n"
          "    --version    output version information and exit\n"
          "    --help       display this help and exit\n"
          "\n"
          "By default, only the non-extended header is printed, which is\n"
          "equivalent to --num 0\n"
        );
    return 0;
}

static int ext_headers( segy_file* fp ) {
    char binary[ SEGY_BINARY_HEADER_SIZE ];
    int err = segy_binheader( fp, binary );

    if( err ) return -1;

    int32_t ext;
    err = segy_get_bfield( binary, SEGY_BIN_EXT_HEADERS, &ext );
    if( err ) return -2;
    return ext;
}

static void print_header( const char* header ) {
    for( int line = 0, ch = 0; line < 40; ++line ) {
        for( int c = 0; c < 80; ++c, ++ch ) putchar( header[ ch ] );
        putchar( '\n' );
    }
}

int main( int argc, char** argv ) {

    static int all = false;
    static int strict = false;
    static int version = 0;
    static struct option long_options[] = {
        { "num",        required_argument,    0,        'n' },
        { "all",        no_argument,          &all,      1  },
        { "strict",     no_argument,          &strict,   1  },
        { "nonstrict",  no_argument,          &strict,   0  },
        { "version",    no_argument,          &version,  1  },
        { "help",       no_argument,          0,        'h' },
        { 0, 0, 0, 0 }
    };

    int num_alloc_sz = 32;
    int* num   = calloc( sizeof( int ), num_alloc_sz );
    int num_sz = 0;

    while( true ) {
        int option_index = 0;
        int c = getopt_long( argc, argv, "n:asS",
                             long_options, &option_index );

        if( c == -1 ) break;

        char* endptr;
        switch( c ) {
            case  0:  break;
            case 'h': return help();
            case 's': strict  = 1; break;
            case 'S': strict  = 0; break;
            case 'a': all     = 1; break;
            case 'n':
                if( version ) break;

                if( num_sz == num_alloc_sz - 1 ) {
                    num_alloc_sz *= 2;
                    int* re = realloc( num, num_alloc_sz * sizeof( int ) );
                    if( !re ) return errmsg( errno, "Unable to alloc" );
                    num = re;
                }

                num[ num_sz ] = strtol( optarg, &endptr, 10 );
                if( *endptr != '\0' )
                    return errmsg( EINVAL, "num must be an integer" );
                if( num[ num_sz ] < 0 )
                    return errmsg( EINVAL, "num must be non-negative" );
                break;

            default: return help();
        }
    }

    if( version ) return printversion( "segyio-cath" );

    char header[ SEGY_TEXT_HEADER_SIZE + 1 ] = { 0 };

    for( int i = optind; i < argc; ++i ) {
        segy_file* fp = segy_open( argv[ i ], "r" );

        if( !fp ) fprintf( stderr, "segyio-cath: %s: No such file or directory\n",
                           argv[ i ] );

        if( !fp && strict ) return errmsg( errno, NULL );
        if( !fp ) continue;

        if( num_sz == 0 ) num_sz = 1;

        const int exts = ext_headers( fp );
        if( exts < 0 )
            return errmsg( errno, "Unable to read binary header" );

        if( all ) {
            /* just create the list 0,1,2... as if it was passed explicitly */
            if( exts >= num_alloc_sz ) {
                num_alloc_sz = exts * 2;
                int* re = realloc( num, num_alloc_sz * sizeof( int ) );
                if( !re ) return errmsg( errno, "Unable to alloc" );
                num = re;
            }

            num_sz = exts + 1;
            num[ 0 ] = 0;
            for( int j = 0; j < exts; ++j )
                num[ j + 1 ] = j;
        }

        for( int j = 0; j < num_sz; ++j ) {
            if( strict && ( num[ j ] < 0 || num[ j ] > exts ) )
                return errmsg( EINVAL, "Header index out of range" );
            if( num[ j ] < 0 || num[ j ] > exts ) continue;

            int err = num[ j ] == 0
                ? segy_read_textheader( fp, header )
                : segy_read_ext_textheader( fp, num[ j ] - 1, header );

            if( err != 0 ) return errmsg( errno, "Unable to read header" );
            print_header( header );
        }

        segy_close( fp );
    }

    free( num );
    return 0;
}
