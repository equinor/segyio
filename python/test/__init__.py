# -*- coding: utf-8 -*-

import shutil
import pytest

def tmpfiles(*files):
    def tmpfiles_decorator(func):
        def func_wrapper(tmpdir):
            for f in files:
                shutil.copy(f, str(tmpdir))
            func(tmpdir)
        return func_wrapper
    return tmpfiles_decorator


@pytest.fixture
def small(tmpdir):
    shutil.copy("test-data/small.sgy", str(tmpdir))
    return tmpdir / "small.sgy"


@pytest.fixture
def smallps(tmpdir):
    shutil.copy("test-data/small-ps.sgy", str(tmpdir))
    return tmpdir / "small-ps.sgy"