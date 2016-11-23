from matplotlib import gridspec
from matplotlib.figure import Figure
from matplotlib.axes import Axes


class LayoutFigure(Figure):
    def __init__(self, width=11.7, height=8.3, dpi=100, tight_layout=True, **kwargs):
        super(LayoutFigure, self).__init__(figsize=(width, height), dpi=dpi, tight_layout=tight_layout, **kwargs)

        self._axes = []
        """ :type: list[Axes] """

    def set_plot_layout(self, layout_spec):
        grid_spec = gridspec.GridSpec(*layout_spec['dims'])

        for axes in self._axes:
            self.delaxes(axes)

        self._axes = [self.add_subplot(grid_spec[sub_spec]) for sub_spec in layout_spec['grid']]

    def index(self, axes):
        """
        :param axes: The Axes instance to find the index of.
        :type axes: Axes
        :rtype: int
        """
        return self._axes.index(axes)

    def layout_axes(self):
        """ :rtype: list[Axes] """
        return list(self._axes)
