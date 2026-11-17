"""PyInstaller integration.

Exposes the directory containing segyio's PyInstaller hook so that PyInstaller
discovers it automatically via the ``pyinstaller40`` entry point declared in
``pyproject.toml``. See
https://pyinstaller.org/en/stable/hooks.html#providing-pyinstaller-hooks-with-your-package
"""

import os


def get_hook_dirs():
    return [os.path.dirname(__file__)]
