r"""
01. Reading pre-stack data
==========================
This tutorial explains how to read a SEGY file with unstructured data.
"""

import matplotlib.pyplot as plt
import segyio

###############################################################################
# Let's first define the file name and open it with `segyio`

segyfile = 'viking_small.segy'

f = segyio.open(segyfile, ignore_geometry=True)

###############################################################################
# Set up a figure and plot a few shot gathers

clip = 1e+2
vmin, vmax = -clip, clip

# Figure
figsize=(20, 20)
fig, axs = plt.subplots(nrows=1, ncols=1, figsize=figsize, facecolor='w', edgecolor='k',
                       squeeze=False,
                       sharex=True)
axs = axs.ravel()
im = axs[0].imshow(f.trace.raw[:].T, cmap=plt.cm.seismic, vmin=vmin, vmax=vmax)

###############################################################################
# Close the file

f.close()
