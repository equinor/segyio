import shutil


def tmpfiles(*files):
    def tmpfiles_decorator(func):
        def func_wrapper(tmpdir):
            for f in files:
                shutil.copy(str(f), str(tmpdir))
        return func_wrapper
    return tmpfiles_decorator
