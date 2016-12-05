import segyio


def dt(f, fallback_dt=4):
    return segyio._segyio.get_dt(f.xfd, fallback_dt)
