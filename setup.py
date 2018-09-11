#!/usr/bin/env python

import os
import sys
from setuptools import setup, Extension

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

if 'win' in sys.platform:
    extra_libs = []
else:
    extra_libs = ['m']

def getversion():
    # if this is a tarball distribution, the .git-directory won't be avilable
    # and setuptools_scm will crash hard. good tarballs are built with a
    # pre-generated version.py, so parse that and extract version from it
    #
    # set the SEGYIO_NO_GIT_VER environment variable to ignore a version from
    # git (useful when building for debian or other distributions)
    if not 'SEGYIO_NO_GIT_VER' in os.environ and os.path.isdir(src('.git')):
        return {
            'use_scm_version': {
                'root': src(''),
                'write_to': src('python/segyio/version.py')
            }
        }


    pkgversion = { 'version': '0.0.0' }
    versionfile = src('python/segyio/version.py')

    if not os.path.exists(versionfile):
        return pkgversion

    import ast
    with open(versionfile) as f:
        root = ast.parse(f.read())

    for node in ast.walk(root):
        if not isinstance(node, ast.Assign): continue
        if len(node.targets) == 1 and node.targets[0].id == 'version':
            pkgversion['version'] = node.value.s

    return pkgversion

setup(name='segyio',
      description='Simple & fast IO for SEG-Y files',
      long_description=long_description,
      author='Statoil ASA',
      author_email='fg_gpl@statoil.com',
      url='https://github.com/Statoil/segyio',
      package_dir={'' : src('python')},
      packages=['segyio', 'segyio.su'],
      package_data={ 'segyio': ['segyio.dll'], },
      license='LGPL-3.0',
      ext_modules=[Extension('segyio._segyio',
        sources=[src('python/segyio/segyio.cpp')],
        include_dirs=[src('lib/include')],
        libraries=['segyio'] + extra_libs
        )],
      platforms='any',
      install_requires=['numpy >=1.10'],
      setup_requires=['setuptools >=28', 'setuptools_scm', 'pytest-runner'],
      tests_require=['pytest'],
      classifiers=[
          'Development Status :: 5 - Production/Stable',
          'Environment :: Other Environment',
          'Intended Audience :: Developers',
          'Intended Audience :: Science/Research',
          'License :: OSI Approved :: GNU Lesser General Public License v3 or later (LGPLv3+)',
          'Natural Language :: English',
          'Programming Language :: Python',
          'Programming Language :: Python :: 2.7',
          'Programming Language :: Python :: 3.5',
          'Programming Language :: Python :: 3.6',
          'Topic :: Scientific/Engineering',
          'Topic :: Scientific/Engineering :: Physics',
          'Topic :: Software Development :: Libraries',
          'Topic :: Utilities'
      ],
      **getversion()
      )
