from cwrap import BaseCEnum


class TraceField(BaseCEnum):
    TYPE_NAME = "TraceField"

    TRACE_SEQUENCE_LINE = None
    TRACE_SEQUENCE_FILE = None
    FieldRecord = None
    TraceNumber = None
    EnergySourcePoint = None
    CDP = None
    CDP_TRACE = None
    TraceIdentificationCode = None
    NSummedTraces = None
    NStackedTraces = None
    DataUse = None
    offset = None
    ReceiverGroupElevation = None
    SourceSurfaceElevation = None
    SourceDepth = None
    ReceiverDatumElevation = None
    SourceDatumElevation = None
    SourceWaterDepth = None
    GroupWaterDepth = None
    ElevationScalar = None
    SourceGroupScalar = None
    SourceX = None
    SourceY = None
    GroupX = None
    GroupY = None
    CoordinateUnits = None
    WeatheringVelocity = None
    SubWeatheringVelocity = None
    SourceUpholeTime = None
    GroupUpholeTime = None
    SourceStaticCorrection = None
    GroupStaticCorrection = None
    TotalStaticApplied = None
    LagTimeA = None
    LagTimeB = None
    DelayRecordingTime = None
    MuteTimeStart = None
    MuteTimeEND = None
    TRACE_SAMPLE_COUNT = None
    TRACE_SAMPLE_INTERVAL = None
    GainType = None
    InstrumentGainConstant = None
    InstrumentInitialGain = None
    Correlated = None
    SweepFrequencyStart = None
    SweepFrequencyEnd = None
    SweepLength = None
    SweepType = None
    SweepTraceTaperLengthStart = None
    SweepTraceTaperLengthEnd = None
    TaperType = None
    AliasFilterFrequency = None
    AliasFilterSlope = None
    NotchFilterFrequency = None
    NotchFilterSlope = None
    LowCutFrequency = None
    HighCutFrequency = None
    LowCutSlope = None
    HighCutSlope = None
    YearDataRecorded = None
    DayOfYear = None
    HourOfDay = None
    MinuteOfHour = None
    SecondOfMinute = None
    TimeBaseCode = None
    TraceWeightingFactor = None
    GeophoneGroupNumberRoll1 = None
    GeophoneGroupNumberFirstTraceOrigField = None
    GeophoneGroupNumberLastTraceOrigField = None
    GapSize = None
    OverTravel = None
    CDP_X = None
    CDP_Y = None
    INLINE_3D = None
    CROSSLINE_3D = None
    ShotPoint = None
    ShotPointScalar = None
    TraceValueMeasurementUnit = None
    TransductionConstantMantissa = None
    TransductionConstantPower = None
    TransductionUnit = None
    TraceIdentifier = None
    ScalarTraceHeader = None
    SourceType = None
    SourceEnergyDirectionMantissa = None
    SourceEnergyDirectionExponent = None
    SourceMeasurementMantissa = None
    SourceMeasurementExponent = None
    SourceMeasurementUnit = None
    UnassignedInt1 = None
    UnassignedInt2 = None

TraceField.addEnum("TRACE_SEQUENCE_LINE", 1)
TraceField.addEnum("TRACE_SEQUENCE_FILE", 5)
TraceField.addEnum("FieldRecord", 9)
TraceField.addEnum("TraceNumber", 13)
TraceField.addEnum("EnergySourcePoint", 17)
TraceField.addEnum("CDP", 21)
TraceField.addEnum("CDP_TRACE", 25)
TraceField.addEnum("TraceIdentificationCode", 29)
TraceField.addEnum("NSummedTraces", 31)
TraceField.addEnum("NStackedTraces", 33)
TraceField.addEnum("DataUse", 35)
TraceField.addEnum("offset", 37)
TraceField.addEnum("ReceiverGroupElevation", 41)
TraceField.addEnum("SourceSurfaceElevation", 45)
TraceField.addEnum("SourceDepth", 49)
TraceField.addEnum("ReceiverDatumElevation", 53)
TraceField.addEnum("SourceDatumElevation", 57)
TraceField.addEnum("SourceWaterDepth", 61)
TraceField.addEnum("GroupWaterDepth", 65)
TraceField.addEnum("ElevationScalar", 69)
TraceField.addEnum("SourceGroupScalar", 71)
TraceField.addEnum("SourceX", 73)
TraceField.addEnum("SourceY", 77)
TraceField.addEnum("GroupX", 81)
TraceField.addEnum("GroupY", 85)
TraceField.addEnum("CoordinateUnits", 89)
TraceField.addEnum("WeatheringVelocity", 91)
TraceField.addEnum("SubWeatheringVelocity", 93)
TraceField.addEnum("SourceUpholeTime", 95)
TraceField.addEnum("GroupUpholeTime", 97)
TraceField.addEnum("SourceStaticCorrection", 99)
TraceField.addEnum("GroupStaticCorrection", 101)
TraceField.addEnum("TotalStaticApplied", 103)
TraceField.addEnum("LagTimeA", 105)
TraceField.addEnum("LagTimeB", 107)
TraceField.addEnum("DelayRecordingTime", 109)
TraceField.addEnum("MuteTimeStart", 111)
TraceField.addEnum("MuteTimeEND", 113)
TraceField.addEnum("TRACE_SAMPLE_COUNT", 115)
TraceField.addEnum("TRACE_SAMPLE_INTERVAL", 117)
TraceField.addEnum("GainType", 119)
TraceField.addEnum("InstrumentGainConstant", 121)
TraceField.addEnum("InstrumentInitialGain", 123)
TraceField.addEnum("Correlated", 125)
TraceField.addEnum("SweepFrequencyStart", 127)
TraceField.addEnum("SweepFrequencyEnd", 129)
TraceField.addEnum("SweepLength", 131)
TraceField.addEnum("SweepType", 133)
TraceField.addEnum("SweepTraceTaperLengthStart", 135)
TraceField.addEnum("SweepTraceTaperLengthEnd", 137)
TraceField.addEnum("TaperType", 139)
TraceField.addEnum("AliasFilterFrequency", 141)
TraceField.addEnum("AliasFilterSlope", 143)
TraceField.addEnum("NotchFilterFrequency", 145)
TraceField.addEnum("NotchFilterSlope", 147)
TraceField.addEnum("LowCutFrequency", 149)
TraceField.addEnum("HighCutFrequency", 151)
TraceField.addEnum("LowCutSlope", 153)
TraceField.addEnum("HighCutSlope", 155)
TraceField.addEnum("YearDataRecorded", 157)
TraceField.addEnum("DayOfYear", 159)
TraceField.addEnum("HourOfDay", 161)
TraceField.addEnum("MinuteOfHour", 163)
TraceField.addEnum("SecondOfMinute", 165)
TraceField.addEnum("TimeBaseCode", 167)
TraceField.addEnum("TraceWeightingFactor", 169)
TraceField.addEnum("GeophoneGroupNumberRoll1", 171)
TraceField.addEnum("GeophoneGroupNumberFirstTraceOrigField", 173)
TraceField.addEnum("GeophoneGroupNumberLastTraceOrigField", 175)
TraceField.addEnum("GapSize", 177)
TraceField.addEnum("OverTravel", 179)
TraceField.addEnum("CDP_X", 181)
TraceField.addEnum("CDP_Y", 185)
TraceField.addEnum("INLINE_3D", 189)
TraceField.addEnum("CROSSLINE_3D", 193)
TraceField.addEnum("ShotPoint", 197)
TraceField.addEnum("ShotPointScalar", 201)
TraceField.addEnum("TraceValueMeasurementUnit", 203)
TraceField.addEnum("TransductionConstantMantissa", 205)
TraceField.addEnum("TransductionConstantPower", 209)
TraceField.addEnum("TransductionUnit", 211)
TraceField.addEnum("TraceIdentifier", 213)
TraceField.addEnum("ScalarTraceHeader", 215)
TraceField.addEnum("SourceType", 217)
TraceField.addEnum("SourceEnergyDirectionMantissa", 219)
TraceField.addEnum("SourceEnergyDirectionExponent", 223)
TraceField.addEnum("SourceMeasurementMantissa", 225)
TraceField.addEnum("SourceMeasurementExponent", 229)
TraceField.addEnum("SourceMeasurementUnit", 231)
TraceField.addEnum("UnassignedInt1", 233)
TraceField.addEnum("UnassignedInt2", 237)