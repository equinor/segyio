from PyQt4.QtCore import QSize
from PyQt4.QtGui import QComboBox, QPixmap, qRgb
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QImage

import numpy as np
from matplotlib.cm import ScalarMappable


class ColormapCombo(QComboBox):
    def __init__(self, color_maps=None, parent=None):
        QComboBox.__init__(self, parent)
        self.setMaxVisibleItems(10)
        self.setStyleSheet("QComboBox { combobox-popup: 0; }")

        if color_maps is None:
            color_maps = self._type_sorted_color_maps()

        self.setMinimumWidth(170)
        self.setMaximumWidth(170)
        self.setMinimumHeight(30)

        self.setIconSize(QSize(128, 16))

        icon_width = 256
        icon_height = 16
        values = np.linspace(0, 1, icon_width)
        color_indexes = np.linspace(0, 255, icon_width, dtype=np.uint8)
        color_indexes = np.tile(color_indexes, icon_height)
        image = QImage(color_indexes.data, icon_width, icon_height, QImage.Format_Indexed8)

        for index, item in enumerate(color_maps):
            self.addItem(item)
            pix_map = self._create_icon(item, image, values)
            self.setItemData(index, "", Qt.DisplayRole)
            self.setItemData(index, item, Qt.ToolTipRole)
            self.setItemData(index, pix_map, Qt.DecorationRole)

    def _create_icon(self, color_map_name, image, values):
        """"
        :type color_map_name: str
        :type image: QImage
        :type values: np.ndarray
        """

        color_map = ScalarMappable(cmap=color_map_name)
        rgba = color_map.to_rgba(values, bytes=True)

        color_table = [qRgb(c[0], c[1], c[2]) for c in rgba]
        image.setColorTable(color_table)

        return QPixmap.fromImage(image).scaledToWidth(128)

    def _type_sorted_color_maps(self):
        cmaps = [('Perceptually Uniform Sequential', ['viridis', 'inferno', 'plasma', 'magma']),
                 ('Sequential', ['Blues', 'BuGn', 'BuPu',
                                 'GnBu', 'Greens', 'Greys', 'Oranges', 'OrRd',
                                 'PuBu', 'PuBuGn', 'PuRd', 'Purples', 'RdPu',
                                 'Reds', 'YlGn', 'YlGnBu', 'YlOrBr', 'YlOrRd']),
                 ('Sequential (2)', ['afmhot', 'autumn', 'bone', 'cool',
                                     'copper', 'gist_heat', 'gray', 'hot',
                                     'pink', 'spring', 'summer', 'winter']),
                 ('Diverging', ['BrBG', 'bwr', 'coolwarm', 'PiYG', 'PRGn', 'PuOr',
                                'RdBu', 'RdGy', 'RdYlBu', 'RdYlGn', 'Spectral', 'seismic']),
                 ('Qualitative', ['Accent', 'Dark2', 'Paired', 'Pastel1',
                                  'Pastel2', 'Set1', 'Set2', 'Set3']),
                 ('Miscellaneous', ['gist_earth', 'terrain', 'ocean', 'gist_stern',
                                    'brg', 'CMRmap', 'cubehelix',
                                    'gnuplot', 'gnuplot2', 'gist_ncar',
                                    'nipy_spectral', 'jet', 'rainbow',
                                    'gist_rainbow', 'hsv', 'flag', 'prism'])
                 ]

        # color_maps = sorted(m for m in cm.datad if not m.endswith("_r"))

        color_maps = []

        for cm_group in cmaps:
            color_maps.extend(cm_group[1])

        return color_maps

    def itemText(self, index):
        return str(self.itemData(index, Qt.ToolTipRole).toString())
