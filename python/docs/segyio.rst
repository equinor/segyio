segyio
======

.. currentmodule:: segyio

Open and create
---------------
.. autofunction:: segyio.open
.. autofunction:: segyio.create

Addressing modes
----------------

.. automodule:: segyio.segy

Trace modes
~~~~~~~~~~~
.. autoattribute:: segyio.SegyFile.trace
.. autoattribute:: segyio.SegyFile.header

Line modes
~~~~~~~~~~
.. autoattribute:: segyio.SegyFile.iline
.. autoattribute:: segyio.SegyFile.xline
.. autoattribute:: segyio.SegyFile.fast
.. autoattribute:: segyio.SegyFile.slow

Composite modes
~~~~~~~~~~~~~~~
.. autoattribute:: segyio.SegyFile.depth_slice
.. autoattribute:: segyio.SegyFile.gather

Text and binary modes
~~~~~~~~~~~~~~~~~~~~~
.. autoattribute:: segyio.SegyFile.text
.. autoattribute:: segyio.SegyFile.bin

Methods
-------
.. automethod:: segyio.SegyFile.attributes
.. automethod:: segyio.SegyFile.mmap
.. automethod:: segyio.SegyFile.flush
.. automethod:: segyio.SegyFile.close

Attributes
----------
.. autoattribute:: segyio.SegyFile.ilines
.. autoattribute:: segyio.SegyFile.xlines
.. autoattribute:: segyio.SegyFile.samples
.. autoattribute:: segyio.SegyFile.offsets

.. autoattribute:: segyio.SegyFile.tracecount
.. autoattribute:: segyio.SegyFile.ext_headers
.. autoattribute:: segyio.SegyFile.unstructured
.. autoattribute:: segyio.SegyFile.sorting
.. autoattribute:: segyio.SegyFile.format

Tools
-----
.. automodule:: segyio.tools
    :members:
    :undoc-members:
    :show-inheritance:

Constants
---------
.. autoclass:: segyio.TraceField
    :members:
    :undoc-members:
    :member-order: bysource
.. autoclass:: segyio.BinField
    :members:
    :undoc-members:
    :member-order: bysource
.. autoclass:: segyio.su
    :members:
    :undoc-members:
    :member-order: bysource
.. autoclass:: segyio.TraceSortingFormat
    :members:
    :undoc-members:
    :member-order: bysource
.. autoclass:: segyio.SegySampleFormat
    :members:
    :undoc-members:
    :member-order: bysource
