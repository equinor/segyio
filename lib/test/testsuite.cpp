#define CATCH_CONFIG_RUNNER
#include <catch/catch.hpp>

#include <segyio/segy.h>

#include "test-config.hpp"

void testcfg::apply( segy_file* fp ) {
    if( this->memmap ) {
    #ifdef HAVE_MMAP
        REQUIRE( segy_mmap( fp ) == SEGY_OK );
    #endif //HAVE_MMAP
    }
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
          ;
      session.cli( cli );

      const int errc = session.applyCommandLine( argc, argv );
      if( errc )
          return errc;

      return session.run();
}
