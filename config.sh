function run_tests {
    set -x
    python -c "import segyio; print(segyio.__version__)"
    python ../python/examples/scan_min_max.py ../test-data/small.sgy
}


function pre_build {
    if [ -n "$IS_OSX" ]; then return; fi
    if [ -d build-centos5 ]; then return; fi

    # the cmakes available in yum for centos5 are too old (latest 2.11.x), so
    # do a dirty hack and get a pre-compiled i386-binary from cmake.org and run
    # it. it's only necessary in the multilinux docker container and hopefully
    # only until multilinux2 images are released

    mkdir build-centos5
    pushd build-centos5
    export cmake=cmake-2.8.12.2-Linux-i386
    wget --no-check-certificate https://cmake.org/files/v2.8/cmake-2.8.12.2-Linux-i386.tar.gz
    tar xzvf $cmake.tar.gz
    ./$cmake/bin/cmake --version
    ./$cmake/bin/cmake .. -DBUILD_PYTHON=OFF -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
    make install
    popd
}
