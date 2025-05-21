from trace_header import TraceHeader


class TraceEnsemble:
    def __init__(self, trace_header: TraceHeader, data: bytes, trace_ensemble_nr: int):
        self.trace_ensemble_nr = trace_ensemble_nr
        self.trace_header = trace_header
        self.data = data

    def __str__(self, nr: int | None = None):
        out_data = f"\nTrace Ensemble: {self.trace_ensemble_nr}:\n"
        out_data += self.trace_header.__str__()
        out_data += f"{(TraceHeader.TRACE_HEADER_SIZE +1).__str__().center(8)} "
        out_data += f"{len(self.data).__str__().ljust(8)} "
        out_data += (
            f"{('RAW DATA').ljust(34)} : {self.data[0:min(20, len(self.data))]}\n\n"
        )
        return out_data
