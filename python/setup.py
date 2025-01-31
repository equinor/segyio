import os
import sys
import skbuild
import setuptools

long_description = """
=======
SEGY IO
=======

https://segyio.readthedocs.io

Introduction
------------

Segyio is a small LGPL licensed C library for easy interaction with SEG Y
formatted seismic data, with language bindings for Python and Matlab. Segyio is
an attempt to create an easy-to-use, embeddable, community-oriented library for
seismic applications. Features are added as they are needed; suggestions and
contributions of all kinds are very welcome.

Feature summary
---------------
 * A low-level C interface with few assumptions; easy to bind to other
   languages.
 * Read and write binary and textual headers.
 * Read and write traces, trace headers.
 * Easy to use and native-feeling python interface with numpy integration.

Project goals
-------------

Segyio does necessarily attempt to be the end-all of SEG-Y interactions;
rather, we aim to lower the barrier to interacting with SEG-Y files for
embedding, new applications or free-standing programs.

Additionally, the aim is not to support the full standard or all exotic (but
correctly) formatted files out there. Some assumptions are made, such as:

 * All traces in a file are assumed to be of the same sample size.
 * It is assumed all lines have the same number of traces.

The writing functionality in Segyio is largely meant to *modify* or adapt
files. A file created from scratch is not necessarily a to-spec SEG-Y file, as
we only necessarily write the header fields segyio needs to make sense of the
geometry. It is still highly recommended that SEG-Y files are maintained and
written according to specification, but segyio does not mandate this.

"""

def src(x):
    root = os.path.dirname( __file__ )
    return os.path.abspath(os.path.join(root, x))

if 'MAKEFLAGS' in os.environ:
    # if setup.py is called from cmake, it reads and uses the MAKEFLAGS
    # environment variable, which in turn gets picked up on by scikit-build.
    # However, scikit-build uses make install to move the built .so to the
    # right object, still in the build tree. This make invocation honours
    # DESTDIR, which leads to unwanted items in the destination tree.
    #
    # If the MAKEFLAGS env var is set, remove DESTDIR from it.
    #
    # Without this: make install DESTDIR=/tmp
    # /tmp/src/segyio/python/_skbuild/linux-x86_64-3.5/cmake-install/segyio/_segyio.so
    # /tmp/usr/local/lib/python2.7/site-packages/segyio/_segyio.so
    #
    # with this the _skbuild install is gone
    makeflags = os.environ['MAKEFLAGS']
    flags = makeflags.split(' ')
    flags = [flag for flag in flags if not flag.startswith('DESTDIR=')]
    os.environ['MAKEFLAGS'] = ' '.join(flags)

skbuild.setup(
    name = 'segyio',
    description = 'Simple & fast IO for SEG-Y files',
    long_description = long_description,
    author = 'Equinor ASA',
    author_email = 'jokva@equinor.com',
    url = 'https://github.com/equinor/segyio',
    packages = ['segyio', 'segyio.su'],
    package_data = { 'segyio': ['segyio.dll'], },
    license = 'LGPL-3.0',
    platforms = 'any',
    install_requires = ['numpy >= 1.10'],
    setup_requires = [
        'setuptools >= 28',
        'scikit-build',
    ],
    cmake_args = [
        # we can safely pass OSX_DEPLOYMENT_TARGET as it's ignored on
        # everything not OS X. We depend on C++11, which makes our minimum
        # supported OS X release 10.9
        '-DCMAKE_OSX_DEPLOYMENT_TARGET=10.9',
    ],
    classifiers = [
        'Development Status :: 5 - Production/Stable',
        'Environment :: Other Environment',
        'Intended Audience :: Developers',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: GNU Lesser General Public License v3 or later (LGPLv3+)',
        'Natural Language :: English',
        'Programming Language :: Python',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
        'Programming Language :: Python :: 3.13',
        'Topic :: Scientific/Engineering',
        'Topic :: Scientific/Engineering :: Physics',
        'Topic :: Software Development :: Libraries',
        'Topic :: Utilities'
    ],
)
