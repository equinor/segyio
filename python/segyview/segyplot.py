from matplotlib.ticker import FuncFormatter, MaxNLocator
import matplotlib.patches as patches

import matplotlib

class SegyPlot(object):

    """
    Plots a segy slice and line indicators on the provided axes.
    """

    def __init__(self, slices, indexes, axes, cmap='seismic', x_axis_indexes=None, y_axis_indexes=None,
                 display_horizontal_indicator=False,
                 display_vertical_indicator=False, v_min_max=None):

        self.slices = slices
        self.indexes = indexes
        self.cmap = cmap

        self.vmin, self.vmax = v_min_max or (None, None)

        self.plane_height = len(self.slices[self.indexes[0]][0])
        self.plane_width = len(self.slices[self.indexes[0]][:])

        self.x_axis_name, self.x_axis_indexes = x_axis_indexes or (None, None)
        self.y_axis_name, self.y_axis_indexes = y_axis_indexes or (None, None)

        self.slice_axes = axes
        self.slice_axes.tick_params(axis='both', labelsize=8)

        if self.x_axis_indexes is not None:
            def x_axis_label_formatter(val, position):
                if 0 <= val < len(self.x_axis_indexes):
                    return self.x_axis_indexes[int(val)]
                return ''

            self.slice_axes.set_xlabel(self.x_axis_name, fontsize=8)
            self.slice_axes.get_xaxis().set_major_formatter(FuncFormatter(x_axis_label_formatter))
            self.slice_axes.get_xaxis().set_major_locator(MaxNLocator(20))  # max 20 ticks are shown

        if self.y_axis_indexes is not None:
            def y_axis_label_formatter(val, position):
                if 0 <= val < len(self.y_axis_indexes):
                    return self.y_axis_indexes[int(val)]
                return ''

            self.slice_axes.set_ylabel(self.y_axis_name, fontsize=8)
            self.slice_axes.get_yaxis().set_major_formatter(FuncFormatter(y_axis_label_formatter))
            self.slice_axes.get_yaxis().set_major_locator(MaxNLocator(10))  # max 20 ticks are shown

        self.im = self.slice_axes.imshow(slices[indexes[0]].T,
                                         interpolation="nearest",
                                         aspect="auto",
                                         cmap=self.cmap,
                                         vmin=self.vmin,
                                         vmax=self.vmax)


        if display_vertical_indicator:
            self.vertical_indicator_rect = self.slice_axes.add_patch(
                patches.Rectangle(
                    (-0.5, -0.5),
                    1,
                    self.plane_height,
                    fill=False,
                    alpha=1,
                    color='black',
                    linestyle='dashed',
                    linewidth=0.5,

                )
            )

        if display_horizontal_indicator:
            self.horizontal_indicator_rect = self.slice_axes.add_patch(
                patches.Rectangle(
                    (-0.5, -0.5),
                    self.plane_width,
                    1,
                    fill=False,
                    alpha=1,
                    color='black',
                    linestyle='dashed',
                    linewidth=0.5
                )
            )

        self.disabled_overlay = self.slice_axes.add_patch(
            patches.Rectangle(
                (-0.5, -0.5),  # (x,y)
                len(self.slices[self.indexes[0]][0]),
                len(self.slices[self.indexes[0]][0]),
                alpha=0.5,
                color='gray',
                visible=False
            )
        )

    def set_min_max(self, v_min_max):
        self.vmin, self.vmax = v_min_max or (None, None)
        self.im.set_clim(self.vmin,self.vmax)



    def set_colormap(self, cmap):
        self.cmap = cmap
        self.im.set_cmap(cmap)

    def update_image(self, index):
        self.im.set_data(self.slices[index].T)

    def set_vertical_line_indicator(self, line_index):
        if self.x_axis_indexes is not None:
            self.vertical_indicator_rect.set_x(self.x_axis_indexes.index(line_index) - 0.5)

    def set_horizontal_line_indicator(self, line_index):
        if self.y_axis_indexes is not None:
            line_index = self.y_axis_indexes.index(line_index)
        self.horizontal_indicator_rect.set_y(line_index - 0.5)

    def enable_overlay(self):
        self.disabled_overlay.set_visible(True)

    def disable_overlay(self):
        self.disabled_overlay.set_visible(False)

class ColorBarPlot(object):
    def __init__(self, axes, cmap=None, v_min_max=None):
        print(v_min_max)
        self.axes = axes
        self.cmap = cmap

        min, max = v_min_max
        norm = matplotlib.colors.Normalize(vmin=min, vmax=max)

        self.colorbar = matplotlib.colorbar.ColorbarBase(self.axes, cmap=cmap, norm=norm)

    def set_cmap(self, cmap):
        self.colorbar.set_cmap(str(cmap))
        self.colorbar.draw_all()

    def set_min_max(self, min_max):
        min, max = min_max
        self.colorbar.set_clim(min, max)
        self.colorbar.draw_all()

    def mouse_clicked(self, evt):
        if evt.inaxes is not None:
            print(evt)
