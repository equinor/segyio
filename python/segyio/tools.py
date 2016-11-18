import segyio


def get_dt(file):
    dt = segyio._segyio.get_dt(file.xfd)

    return dt['dt']
