#include <segyio/segyio.hpp>

int main() {
    // Open a SEG-Y file to avoid error about unused internal functions.
    segyio::basic_volume< segyio::readonly > tmp(
        segyio::path{ "test-data/small.sgy" },
        segyio::config{}
    );
    return 0;
}
