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
    # fetch a newer version pypi. Files newer than 3.13.3 are not available
    # for manylinux1, so pin that. However, cmake < 3.14 is not packaged for python 3.8
    python -m pip install \
        "cmake < 3.14; python_version < '3.8'" \
        "cmake; python_version >= '3.8'" \
        scikit-build

    mkdir build-centos5
    pushd build-centos5

    cmake --version
    cmake -DBUILD_PYTHON=OFF \
          -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_SHARED_LIBS=ON \
          -DCMAKE_INSTALL_NAME_DIR=/usr/local/lib \
          ..

    if [ -n "$IS_OSX" ]; then
        sudo make install;
    else
        make install;
    fi

    popd

    # clean dirty files from python/, otherwise it picks up the one built
    # outside docker and symbols will be too recent for auditwheel.
    # setuptools_scm really *really* expects a .git-directory. As the wheel
    # building process does its work in /tmp, setuptools_scm crashes because it
    # cannot find the .git dir. Leave version.py so that setuptools can obtain
    # the version from it
    git clean -dxf python --exclude python/segyio/version.py
}
