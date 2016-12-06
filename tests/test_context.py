import os
import tempfile

import shutil

import errno


class TestContext(object):
    def __init__(self, name="unnamed", cleanup=True):
        super(TestContext, self).__init__()
        self._name = name
        self._cwd = os.getcwd()
        self._cleanup = cleanup

    def __enter__(self):
        temp_path = tempfile.mkdtemp("_%s" % self._name)
        os.chdir(temp_path)
        self._temp_path = os.getcwd()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        os.chdir(self._cwd)
        if self._cleanup:
            shutil.rmtree(self._temp_path)

    def copy_file(self, source, target="."):
        if not os.path.isabs(source):
            source = os.path.join(self._cwd, source)

        source_dir, source_filename = os.path.split(source)

        target = os.path.join(self._temp_path, target, source_filename)
        try:
            os.makedirs(os.path.dirname(target))
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
        shutil.copyfile(source, target)

    @property
    def temp_path(self):
        return self._temp_path

    @property
    def cwd(self):
        return self._cwd