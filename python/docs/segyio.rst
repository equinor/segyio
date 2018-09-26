Open and create
===============

.. autofunction:: segyio.open
.. autofunction:: segyio.create

File handle
===========

.. autoclass:: segyio.SegyFile()
    :member-order: groupwise

Addressing
==========

.. currentmodule:: segyio

Data trace
----------
.. autoclass:: segyio.trace.Trace()
    :special-members: __getitem__, __setitem__, __len__, __contains__, __iter__

.. autoclass:: segyio.trace.RawTrace()
    :special-members: __getitem__, __setitem__, __len__, __contains__, __iter__

.. autoclass:: segyio.trace.RefTrace()
    :special-members: __getitem__, __setitem__, __len__, __contains__, __iter__

Trace header and attributes
---------------------------
.. autoclass:: segyio.trace.Header()
    :special-members: __getitem__, __setitem__, __len__, __contains__, __iter__

.. autoclass:: segyio.trace.Attributes()
    :special-members: __getitem__, __setitem__, __len__, __contains__, __iter__

Data line
---------
.. autoclass:: segyio.line.Line()
    :special-members: __getitem__, __setitem__, __len__, __contains__, __iter__

Line header
-----------
.. autoclass:: segyio.line.HeaderLine()
    :special-members: __getitem__, __setitem__, __len__, __contains__, __iter__

Gather
------
.. autoclass:: segyio.gather.Gather()
    :special-members: __getitem__

Depth
-----
.. autoclass:: segyio.depth.Depth()
    :special-members: __getitem__, __setitem__, __len__, __contains__, __iter__

Text
----
.. autoclass:: segyio.trace.Text()
    :special-members: __getitem__, __setitem__, __len__, __contains__, __iter__

Trace and binary header
=======================
.. autoclass:: segyio.field.Field()
    :special-members: __getitem__, __setitem__, __delitem__, __len__, __contains__, __iter__

Tools
=====

.. automodule:: segyio.tools
    :members:
    :undoc-members:
    :show-inheritance:

Constants
=========

Trace header keys
-----------------
.. autoclass:: segyio.TraceField
    :members:
    :undoc-members:
    :member-order: bysource

Binary header keys
------------------
.. autoclass:: segyio.BinField
    :members:
    :undoc-members:
    :member-order: bysource

Seismic Unix keys
-----------------
.. autoclass:: segyio.su
    :members:
    :undoc-members:
    :member-order: bysource

Sorting and formats
-------------------
.. autoclass:: segyio.TraceSortingFormat
    :members:
    :undoc-members:
    :member-order: bysource
.. autoclass:: segyio.SegySampleFormat
    :members:
    :undoc-members:
    :member-order: bysource
