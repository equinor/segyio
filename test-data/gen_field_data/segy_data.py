from textual_file_header import TextualFileHeader
from binary_file_header import BinaryFileHeader
from trace_ensemble import TraceEnsemble
from trace_header import TraceHeader


class SEGYData:
    def __init__(
        self,
        textual_file_header: TextualFileHeader,
        binary_file_header: BinaryFileHeader,
        trace_ensembles: list[TraceEnsemble],
        file_name: str = None,
    ):
        self.file_name = file_name
        self.textual_file_header = textual_file_header
        self.binary_file_header = binary_file_header
        self.trace_ensembles = trace_ensembles

    def to_file(self, file_name: str):
        # Create a new file with the same name as the original
        with open(file_name, "wb") as f:
            f.write(self.textual_file_header.data)
            f.write(self.binary_file_header.dump_data())
            for trace_ensemble in self.trace_ensembles:
                f.write(trace_ensemble.trace_header.dump_data())
                f.write(trace_ensemble.data)

    def __str__(self):
        out_data = ""
        out_data += self.textual_file_header.__str__()
        out_data += self.binary_file_header.__str__()
        for i, trace_ensemble in enumerate(self.trace_ensembles):
            out_data += trace_ensemble.__str__()
        return out_data

    @staticmethod
    def from_file(file_name: str):

        with open(file_name, "rb") as f:
            data = f.read()

        pos = 0
        # Read the textual file header
        tfh = data[pos : pos + TextualFileHeader.TEXTUAL_FILE_HEADER_SIZE]
        textual_file_header = TextualFileHeader(tfh)
        pos += TextualFileHeader.TEXTUAL_FILE_HEADER_SIZE

        # Read the binary file header
        bfh = data[pos : pos + BinaryFileHeader.BINARY_FILE_HEADER_SIZE]
        binary_file_header = BinaryFileHeader(bfh)
        pos += BinaryFileHeader.BINARY_FILE_HEADER_SIZE

        # Read trace ensembles
        trace_ensembles = []
        for i in range(binary_file_header.values["ExtEnsembleTraces"]):

            th_data = data[pos : pos + TraceHeader.TRACE_HEADER_SIZE]
            trace_header = TraceHeader(th_data, pos)
            pos += TraceHeader.TRACE_HEADER_SIZE
            sample_data = data[pos : pos + binary_file_header.values["ExtSamples"] * 4]
            pos += binary_file_header.values["ExtSamples"] * 4
            trace_ensembles.append(TraceEnsemble(trace_header, sample_data, i))

        return SEGYData(
            textual_file_header=textual_file_header,
            binary_file_header=binary_file_header,
            trace_ensembles=trace_ensembles,
            file_name=file_name,
        )
