import sys
import segyio
import numpy as np
import matplotlib.pyplot as plt
from pylab import *
import time
 
def main():
    if len( sys.argv ) < 2:
        sys.exit("Usage: segyview.py [file]")


    filename = sys.argv[1]
    data = None
    ion()

    with segyio.open( filename, "r" ) as f:
         data = f.xline[1428]
         #plt.colorbar()
         imgplot = plt.imshow(data, vmin=1500, vmax=1600)
         plt.show(False)
         for i in range(1429,1440):
              data = f.xline[i]
              imgplot.set_data(data)
              plt.draw()
              plt.pause(0.1) #Note this correction

    plt.show()


if __name__ == '__main__':
    main()
