#ifndef SEGYIO_TEST_CONFIG_HPP
#define SEGYIO_TEST_CONFIG_HPP
#include <segyio/segy.h>

#include <string>

struct testcfg {
    /*
     * If one of the supported files, get -lsb version of file path
     */
    std::string apply( const char* );

    /*
     * call testcfg::config().apply(fp) to enable both mmap and lsb
     * configurations for this test.
     *
     * If only one of those are supported/useful/sensical (e.g. there is no
     * -lsb file), call the feature directly testcfg::config().mmap( fp );
     */
    void mmap( segy_file* );
    void lsb( segy_file* );
    void apply( segy_file* );

    static testcfg& config();

    bool memmap = false;
    bool lsbit = false;
};

#endif // SEGYIO_TEST_CONFIG_HPP
