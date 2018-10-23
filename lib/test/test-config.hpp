#ifndef SEGYIO_TEST_CONFIG_HPP
#define SEGYIO_TEST_CONFIG_HPP
#include <segyio/segy.h>

struct testcfg {
    void apply( segy_file* );

    static testcfg& config();

    bool memmap = false;
};

#endif // SEGYIO_TEST_CONFIG_HPP
