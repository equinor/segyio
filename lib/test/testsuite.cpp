#define CATCH_CONFIG_RUNNER
#include <catch/catch.hpp>

#include <segyio/segy.h>

#include "test-config.hpp"

std::string testcfg::apply( const char* path ) {
    std::string p(path);

    if (p.find("test-data/Format") != std::string::npos) {
        if (this->lsbit) return p + "lsb.sgy";
        else             return p + "msb.sgy";
    }

    if( !this->lsbit ) return path;

    if( p == "test-data/small.sgy" )
        return "test-data/small-lsb.sgy";

    if( p == "test-data/f3.sgy" )
        return "test-data/f3-lsb.sgy";

    return path;
}

void testcfg::mmap( segy_file* fp ) {
    if( this->memmap ) {
    #ifdef HAVE_MMAP
        REQUIRE( segy_mmap( fp ) == SEGY_OK );
    #endif //HAVE_MMAP
    }
}

void testcfg::lsb( segy_file* fp ) {
    if( this->lsbit ) {
        REQUIRE( segy_set_format( fp, SEGY_LSB ) == SEGY_OK );
    }
}

void testcfg::apply( segy_file* fp ) {
    this->mmap( fp );
    this->lsb( fp );
}

testcfg& testcfg::config() {
    static testcfg s;
    return s;
}

int main( int argc, char** argv ) {
      Catch::Session session;

      auto& cfg = testcfg::config();

      using namespace Catch::clara;
      auto cli = session.cli()
          | Opt( cfg.memmap ) ["--test-mmap"] ("run with memory mapped files")
          | Opt( cfg.lsbit )  ["--test-lsb"]  ("run with LSB files")
          ;
      session.cli( cli );

      const int errc = session.applyCommandLine( argc, argv );
      if( errc )
          return errc;

      return session.run();
}
