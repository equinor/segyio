#!/bin/sh

function run_tests {
    set -x
    python -c "import segyio; print(segyio.__version__)"
    python ../python/examples/scan_min_max.py ../test-data/small.sgy
}


function pre_build {
    set -e
    if [ -n "$IS_OSX" ]; then return; fi
    if [ -d build-centos5 ]; then return; fi

    # the cmakes available in yum for centos5 are too old (latest 2.11.x), so
    # fetch a newer version pypi
    python -m pip install cmake

    mkdir build-centos5
    pushd build-centos5

    cmake --version
    cmake .. -DBUILD_PYTHON=OFF -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
    make install
    popd
}
