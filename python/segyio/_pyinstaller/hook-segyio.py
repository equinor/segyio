"""PyInstaller hook for segyio.

Windows wheels are repaired with delvewheel, which places the DLLs that
``_segyio`` depends on (e.g. the MSVC runtime) in a ``segyio.libs`` directory
next to the package and patches ``segyio/__init__.py`` to register it with
``os.add_dll_directory``. PyInstaller does not scan that directory on its own,
so without this hook the frozen application fails at import time with

    ImportError: DLL load failed while importing _segyio

On platforms where no ``segyio.libs`` directory exists this is a no-op.
"""

from PyInstaller.utils.hooks import collect_delvewheel_libs_directory

datas, binaries = collect_delvewheel_libs_directory("segyio")
