class DepthPlane:

    def __init__(self, samples, sorting, read_fn):
        self.samples = samples
        self.sorting = sorting
        self.read_fn = read_fn

    def __getitem__(self, depth):
        if isinstance(depth, int):
            return self.read_fn(self.sorting, depth)
        raise TypeError("Expected an int as index")

    def __len__(self):
        return len(self.samples)

    def __iter__(self):
        for i in xrange(self.samples):
            yield self[i]
