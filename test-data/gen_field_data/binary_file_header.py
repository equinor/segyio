from field import Field
from format import FormatType, FormatSize


class BinaryFileHeader:
    BINARY_FILE_HEADER_SIZE = 400

    def __init__(
        self, data: bytes | None = None, offset: int = 3200, endian: str = ">"
    ):
        assert endian in [">", "<"]  # big or little endian

        if data is None:
            data = bytearray(BinaryFileHeader.BINARY_FILE_HEADER_SIZE)
        assert isinstance(data, (bytes, bytearray)), "Data must be bytes or bytearray"
        assert len(data) == BinaryFileHeader.BINARY_FILE_HEADER_SIZE

        self.offset = offset
        self.data = data
        self.pos = 0
        self.fields = []
        self.values = {}
        self.pos_value = {}
        for [f_d_type, f_name] in self.defined_fields:
            self.fields.append(Field(f_name, f_d_type, endian))

        for field in self.fields:
            self.pos_value[field.name] = [
                self.offset + self.pos,
                self.offset + self.pos + field.length,
                field.type,
            ]
            self.values[field.name] = field.f_struct.unpack(
                data[self.pos : self.pos + field.length]
            )[0]
            self.pos += field.length

        assert (
            self.pos == BinaryFileHeader.BINARY_FILE_HEADER_SIZE
        ), f"Expected {BinaryFileHeader.BINARY_FILE_HEADER_SIZE}, got {self.pos}"

    @property
    def defined_fields(self):
        return [
            ["i", "JobID"],
            ["i", "LineNumber"],
            ["i", "ReelNumber"],
            ["h", "EnsembleTraces"],
            ["h", "AuxEnsembleTraces"],
            ["h", "Interval"],
            ["h", "IntervalOriginal"],
            ["H", "Samples"],
            ["H", "SamplesOriginal"],
            ["H", "Format"],
            ["h", "EnsembleFold"],
            ["h", "SortingCode"],
            ["h", "VerticalSum"],
            ["h", "SweepFrequencyStart"],
            ["h", "SweepFrequencyEnd"],
            ["h", "SweepLength"],
            ["h", "Sweep"],
            ["h", "SweepChannel"],
            ["h", "SweepTaperStart"],
            ["h", "SweepTaperEnd"],
            ["h", "Taper"],
            ["h", "CorrelatedTraces"],
            ["h", "BinaryGainRecovery"],
            ["h", "AmplitudeRecovery"],
            ["h", "MeasurementSystem"],
            ["h", "ImpulseSignalPolarity"],
            ["h", "VibratoryPolarity"],
            ["i", "ExtEnsembleTraces"],
            ["i", "ExtAuxEnsembleTraces"],
            ["i", "ExtSamples"],
            ["d", "ExtInterval"],
            ["d", "ExtIntervalOriginal"],
            ["i", "ExtSamplesOriginal"],
            ["i", "ExtEnsembleFold"],
            ["i", "IntegerConstant"],
            ["200s", "Unassigned1"],
            ["B", "SEGYRevision"],
            ["B", "SEGYRevisionMinor"],
            ["h", "FixedTraceFlag"],
            ["h", "ExtendedHeaders"],
            ["h", "MaxAdditionalTraceHeaders"],
            ["h", "SurveyType"],
            ["h", "TimeBasisCode"],
            ["Q", "NrTracesInStream"],
            ["Q", "FirstTraceOffset"],
            ["i", "NrTrailerRecords"],
            ["68s", "Unassigned2"],
        ]

    def __str__(self):
        out_data = (
            f"\nBinary File Header [{BinaryFileHeader.BINARY_FILE_HEADER_SIZE}]:\n"
        )
        out_data += f"{' Offset':<8} {'Length':<7} {'Field Name':<35} : Value\n"
        total_size = 3201
        for field in self.fields:
            out_data += (
                f"{total_size.__str__().center(8)} {field.length.__str__().center(9)}"
                + f"{field.name.ljust(34)} : {self.values[field.name]}\n"
            )
            total_size += field.length
        return out_data

    def dump_data(self):
        out_data = b""
        for field in self.fields:
            out_data += field.f_struct.pack(self.values[field.name])
        return out_data

    @property
    def format_size(self):
        return FormatSize[FormatType[self.values["Format"]]]
