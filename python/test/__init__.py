import shutil


def tmpfiles(*files):
    def tmpfiles_decorator(func):
        def func_wrapper(tmpdir):
            for f in files:
                shutil.copy(str(f), str(tmpdir))
            func(tmpdir)
        return func_wrapper
    return tmpfiles_decorator
