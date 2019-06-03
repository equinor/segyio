# -*- coding: utf-8 -*-

import shutil
import pytest

import py
import os
testdata = py.path.local(os.path.abspath('test-data/'))

def tmpfiles(*files):
    def tmpfiles_decorator(func):
        def func_wrapper(tmpdir):
            for f in files:
                shutil.copy(str(f), str(tmpdir))
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
