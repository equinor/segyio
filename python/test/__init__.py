# -*- coding: utf-8 -*-

import shutil
import pytest

import py
import os
testdata = py.path.local(os.path.abspath('../test-data/'))

def tmpfiles(*files):
    def tmpfiles_decorator(func):
        def func_wrapper(tmpdir):
            for f in files:
                # Make sure that this is always py.path because then we can
                # always strpath on it. Otherwise, calling str on it may give
                # weird results on some systems
                f = py.path.local(f)
                shutil.copy(f.strpath, str(tmpdir))
            func(tmpdir)
        return func_wrapper
    return tmpfiles_decorator


@pytest.fixture
def small(tmpdir):
    shutil.copy(str(testdata / 'small.sgy'), str(tmpdir))
    return tmpdir / "small.sgy"


@pytest.fixture
def smallps(tmpdir):
    shutil.copy(str(testdata / 'small-ps.sgy'), str(tmpdir))
    return tmpdir / "small-ps.sgy"
