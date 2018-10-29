#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <getopt.h>

#include <segyio/segy.h>

namespace {

static struct option long_options[] = {
    { "ext",        required_argument,    0, 'e' },
    { "samples",    required_argument,    0, 's' },
    { "samplesize", required_argument,    0, 'F' },
    { "format",     required_argument,    0, 'f' },
    { "help",       no_argument,          0, 'h' },
    { 0, 0, 0, 0 }
};

int help( int errc = EXIT_SUCCESS ) {
    auto& out = errc == EXIT_SUCCESS ? std::cout : std::cerr;

    out << "usage: flip-endianness [OPTS...] IN OUT\n\n"
        << "swap endianness of values. this program is only intended\n"
        << "for testing segyio, and is not supported.\n"
        << "This program does not flip values of unassigned header words\n"
        << "\n"
        << "options: \n"
        << "-e, --ext N           external headers\n"
        << "-s, --samples N       samples-per-trace\n"
        << "-F, --samplesize N    sample size (default: 4)\n"
        << "-f, --format [id]     sample size from format\n"
        << "                      formats: ibm ieee byte short int\n"
        << "--help                this text\n"
        ;
    return errc;
}

int size_from_format( const std::string& fmt ) {
    if( fmt == "ibm" )   return 4;
    if( fmt == "ieee" )  return 4;
    if( fmt == "byte" )  return 1;
    if( fmt == "short" ) return 2;
    if( fmt == "int" )   return 4;

    std::cerr << "unknown format '" << fmt
              << "', expected one of: "
              << "ibm iee byte short int\n";
    std::exit( EXIT_FAILURE );
}

void flip_binary_header( char* xs ) {
    std::vector< int > sizes;
    sizes.insert( sizes.end(), 3, 4 );
    sizes.insert( sizes.end(), 24, 2 );

    for( auto size : sizes ) {
        std::reverse( xs, xs + size );
        xs += size;
    }

    xs += 240;

    for( auto size : { 2, 2, 2 } ) {
        std::reverse( xs, xs + size );
        xs += size;
    }
}

void flip_trace_header( char* xs ) {
    std::vector< int > sizes;
    sizes.insert( sizes.end(), 7,  4);
    sizes.insert( sizes.end(), 4,  2);
    sizes.insert( sizes.end(), 8,  4);
    sizes.insert( sizes.end(), 2,  2);
    sizes.insert( sizes.end(), 4,  4);
    sizes.insert( sizes.end(), 46, 2);
    sizes.insert( sizes.end(), 5,  4);
    sizes.insert( sizes.end(), 2,  2);
    sizes.insert( sizes.end(), 1,  4);
    sizes.insert( sizes.end(), 5,  2);
    sizes.insert( sizes.end(), 1,  4);
    sizes.insert( sizes.end(), 1,  2);
    sizes.insert( sizes.end(), 1,  4);
    sizes.insert( sizes.end(), 2,  2);

    for( auto size : sizes ) {
        std::reverse( xs, xs + size );
        xs += size;
    }
}

void flip_trace_data( char* xs, int samples, int samplesize ) {
    for( int i = 0; i < samples; ++i ) {
        std::reverse( xs, xs + samplesize );
        xs += samplesize;
    }
}

}

int main( int argc, char** argv ) {
    int ext = 0;
    int samples = 0;
    int samplesize = 4;

    while( true ) {
        int option_index = 0;
        int c = getopt_long( argc, argv, "e:s:F:f:",
                             long_options, &option_index );

        if( c == -1 ) break;

        switch( c ) {
            case  0:  break;
            case 'e': std::exit( help() );
            case 't': ext = std::stoi( optarg ); break;
            case 's': samples = std::stoi( optarg ); break;
            case 'F': samplesize = std::stoi( optarg ); break;
            case 'f': samplesize = size_from_format( optarg ); break;
            default: break;
        }
    }

    if( argc - optind != 2 ) {
        std::exit( help( EXIT_FAILURE ) );
    }

    if( std::strcmp( argv[ optind ], argv[ optind + 1 ] ) == 0 ) {
        std::cerr << "output file cannot be the same as input file\n";
        std::exit( EXIT_FAILURE );
    }

    auto* in  = std::fopen( argv[ optind ], "rb" );
    if( !in ) {
        std::perror( "unable to open input file" );
        std::exit( EXIT_FAILURE );
    }

    auto* out = std::fopen( argv[ optind + 1 ], "wb" );
    if( !out ) {
        std::perror( "unable to open output file" );
        std::exit( EXIT_FAILURE );
    }

    const auto trsize = samples * samplesize;
    if( trsize <= 0 ) {
        std::cerr << "trace size must be non-negative (was "
                  << trsize << ")\n";
        std::exit( EXIT_FAILURE );
    }

    std::vector< char > buffer(
        std::max( SEGY_TEXT_HEADER_SIZE, trsize )
    );

    /* copy textual header */
    auto sz = std::fread( buffer.data(), 1, SEGY_TEXT_HEADER_SIZE, in );
    if( sz != SEGY_TEXT_HEADER_SIZE ) {
        std::perror( "error reading text header" );
        std::exit( EXIT_FAILURE );
    }
    sz = std::fwrite( buffer.data(), 1, SEGY_TEXT_HEADER_SIZE, out );
    if( sz != SEGY_TEXT_HEADER_SIZE ) {
        std::perror( "error writing text header" );
        std::exit( EXIT_FAILURE );
    }

    sz = std::fread( buffer.data(), 1, SEGY_BINARY_HEADER_SIZE, in );
    if( sz != SEGY_BINARY_HEADER_SIZE ) {
        std::perror( "error reading binary header" );
        std::exit( EXIT_FAILURE );
    }
    flip_binary_header( buffer.data() );
    sz = std::fwrite( buffer.data(), 1, SEGY_BINARY_HEADER_SIZE, out );
    if( sz != SEGY_BINARY_HEADER_SIZE ) {
        std::perror( "error writing binary header" );
        std::exit( EXIT_FAILURE );
    }

    for( int i = 0; i < ext; ++i ) {
        sz = std::fread( buffer.data(), 1, SEGY_TEXT_HEADER_SIZE, in );
        if( sz != SEGY_TEXT_HEADER_SIZE ) {
            std::perror( "error reading ext text header" );
            std::exit( EXIT_FAILURE );
        }
        sz = std::fwrite( buffer.data(), 1, SEGY_TEXT_HEADER_SIZE, out );
        if( sz != SEGY_TEXT_HEADER_SIZE ) {
            std::perror( "error writing ext text header" );
            std::exit( EXIT_FAILURE );
        }
    }

    while( true ) {
        /*
         * read-flip-write trace header
         */
        sz = std::fread( buffer.data(), 1, SEGY_TRACE_HEADER_SIZE, in );
        if( sz == 0 && std::feof( in ) ) break;
        if( sz != SEGY_TRACE_HEADER_SIZE ) {
            std::perror( "error reading trace header" );
            std::exit( EXIT_FAILURE );
        }
        flip_trace_header( buffer.data() );
        sz = std::fwrite( buffer.data(), 1, SEGY_TRACE_HEADER_SIZE, out );
        if( sz != SEGY_TRACE_HEADER_SIZE ) {
            std::perror( "error writing trace header" );
            std::exit( EXIT_FAILURE );
        }

        /*
         * read-flip-write trace data
         */
        sz = std::fread( buffer.data(), 1, trsize, in );
        if( sz != std::size_t( trsize ) ) {
            std::perror( "error reading trace header" );
            std::exit( EXIT_FAILURE );
        }
        flip_trace_data( buffer.data(), samples, samplesize );
        sz = std::fwrite( buffer.data(), 1, trsize, out );
        if( sz != std::size_t( trsize ) ) {
            std::perror( "error writing trace header" );
            std::exit( EXIT_FAILURE );
        }
    }

    std::fclose( in );
    std::fclose( out );
}
