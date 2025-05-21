from field import Field


class TraceHeader:
    TRACE_HEADER_SIZE = 240

    def __init__(self, data: bytes | None = None, offset: int = 0, endian: str = ">"):
        assert endian in [">", "<"]  # big or little endian

        if data is None:
            data = bytearray(TraceHeader.TRACE_HEADER_SIZE)
        assert isinstance(data, (bytes, bytearray)), "Data must be bytes or bytearray"
        assert (
            len(data) == TraceHeader.TRACE_HEADER_SIZE
        ), f"Expected {TraceHeader.TRACE_HEADER_SIZE}, got {len(data)}"

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
            self.pos == TraceHeader.TRACE_HEADER_SIZE
        ), f"Expected {TraceHeader.TRACE_HEADER_SIZE}, got {self.pos}"

    @property
    def defined_fields(self):
        return [
            ["i", "SeqLine"],
            ["i", "SeqFile"],
            ["i", "FieldRecord"],
            ["i", "NumberOrigField"],
            ["i", "EnergySourcePoint"],
            ["i", "Ensemble"],
            ["i", "NumInEnsemble"],
            ["h", "TraceId"],
            ["h", "SummedTraces"],
            ["h", "StackedTraces"],
            ["h", "DataUse"],
            ["i", "Offset"],
            ["i", "RecvGroupElev"],
            ["i", "SourceSurfElev"],
            ["i", "SourceDepth"],
            ["i", "RecvDatumElev"],
            ["i", "SourceDatumElev"],
            ["i", "SourceWaterDepth"],
            ["i", "GroupWaterDepth"],
            ["h", "ElevScalar"],
            ["h", "SourceGroupScalar"],
            ["i", "SourceX"],
            ["i", "SourceY"],
            ["i", "GroupX"],
            ["i", "GroupY"],
            ["h", "CoordUnits"],
            ["h", "WeatheringVelo"],
            ["h", "SubweatheringVelo"],
            ["h", "SourceUpholeTime"],
            ["h", "GroupUpholeTime"],
            ["h", "SourceStaticCorr"],
            ["h", "GroupStaticCorr"],
            ["h", "TotStaticApplied"],
            ["h", "LagA"],
            ["h", "LagB"],
            ["h", "DelayRecTime"],
            ["h", "MuteTimeStart"],
            ["h", "MuteTimeEnd"],
            ["H", "SampleCount"],
            ["h", "SampleInter"],
            ["h", "GainType"],
            ["h", "InstrGainConst"],
            ["h", "InstrInitGain"],
            ["h", "Correlated"],
            ["h", "SweepFreqStart"],
            ["h", "SweepFreqEnd"],
            ["h", "SweepLength"],
            ["h", "SweepType"],
            ["h", "SweepTaperLenStart"],
            ["h", "SweepTaperLenEnd"],
            ["h", "TaperType"],
            ["h", "AliasFiltFreq"],
            ["h", "AliasFiltSlope"],
            ["h", "NotchFiltFreq"],
            ["h", "NotchFiltSlope"],
            ["h", "LowCutFreq"],
            ["h", "HighCutFreq"],
            ["h", "LowCutSlope"],
            ["h", "HighCutSlope"],
            ["h", "YearDataRec"],
            ["h", "DayOfYear"],
            ["h", "HourOfDay"],
            ["h", "MinOfHour"],
            ["h", "SecOfMin"],
            ["h", "TimeBaseCode"],
            ["h", "WeightingFac"],
            ["h", "GeophoneGroupRoll1"],
            ["h", "GeophoneGroupFirst"],
            ["h", "GeophoneGroupLast"],
            ["h", "GapSize"],
            ["h", "OverTravel"],
            ["i", "CdpX"],
            ["i", "CdpY"],
            ["i", "Inline"],
            ["i", "Crossline"],
            ["i", "ShotPoint"],
            ["h", "ShotPointScalar"],
            ["h", "MeasureUnit"],
            ["i", "TransductionMant"],
            ["h", "TransductionExp"],
            ["h", "TransductionUnit"],
            ["h", "DeviceId"],
            ["h", "ScalarTraceHeader"],
            ["h", "SourceType"],
            ["h", "SourceEnergyDirVert"],
            ["h", "SourceEnergyDirXline"],
            ["h", "SourceEnergyDirIline"],
            ["i", "SourceMeasureMant"],
            ["h", "SourceMeasureExp"],
            ["h", "SourceMeasureUnit"],
            ["i", "Unassigned1"],
            ["i", "Unassigned2"],
        ]

    def __str__(self):
        out_data = (
            f"Trace Header [{TraceHeader.TRACE_HEADER_SIZE}] Offset[{self.offset}]:\n"
        )
        out_data += f"{' Offset':<8} {'Length':<7} {'Field Name':<35} : Value\n"
        total_size = 1
        for field in self.fields:
            out_data += (
                f"{total_size.__str__().center(8)} {field.length.__str__().center(9)}"
                + f"{field.name.ljust(34)} : {self.values[field.name]}\n"
            )
            total_size += field.length
        return out_data

    def dump_data(self):
        out_data = b""
        total_size = 3201
        for field in self.fields:
            total_size += field.length
            out_data += field.f_struct.pack(self.values[field.name])
        return out_data
