from cwrap import BaseCEnum

class BinField(BaseCEnum):
    TYPE_NAME = "SEGY_BINFIELD"

    JobID = None
    LineNumber = None
    ReelNumber = None
    Traces = None
    AuxTraces = None
    Interval = None
    IntervalOriginal = None
    Samples = None
    SamplesOriginal = None
    Format = None
    EnsembleFold = None
    SortingCode = None
    VerticalSum = None
    SweepFrequencyStart = None
    SweepFrequencyEnd = None
    SweepLength = None
    Sweep = None
    SweepChannel = None
    SweepTaperStart = None
    SweepTaperEnd = None
    Taper = None
    CorrelatedTraces = None
    BinaryGainRecovery = None
    AmplitudeRecovery = None
    MeasurementSystem = None
    ImpulseSignalPolarity = None
    VibratoryPolarity = None
    Unassigned1 = None
    SEGYRevision = None
    TraceFlag = None
    ExtendedHeaders = None
    Unassigned2 = None

BinField.addEnum("JobID", 3201)
BinField.addEnum("LineNumber", 3205)
BinField.addEnum("ReelNumber", 3209)
BinField.addEnum("Traces", 3213)
BinField.addEnum("AuxTraces", 3215)
BinField.addEnum("Interval", 3217)
BinField.addEnum("IntervalOriginal", 3219)
BinField.addEnum("Samples", 3221)
BinField.addEnum("SamplesOriginal", 3223)
BinField.addEnum("Format", 3225)
BinField.addEnum("EnsembleFold", 3227)
BinField.addEnum("SortingCode", 3229)
BinField.addEnum("VerticalSum", 3231)
BinField.addEnum("SweepFrequencyStart", 3233)
BinField.addEnum("SweepFrequencyEnd", 3235)
BinField.addEnum("SweepLength", 3237)
BinField.addEnum("Sweep", 3239)
BinField.addEnum("SweepChannel", 3241)
BinField.addEnum("SweepTaperStart", 3243)
BinField.addEnum("SweepTaperEnd", 3245)
BinField.addEnum("Taper", 3247)
BinField.addEnum("CorrelatedTraces", 3249)
BinField.addEnum("BinaryGainRecovery", 3251)
BinField.addEnum("AmplitudeRecovery", 3253)
BinField.addEnum("MeasurementSystem", 3255)
BinField.addEnum("ImpulseSignalPolarity", 3257)
BinField.addEnum("VibratoryPolarity", 3259)
BinField.addEnum("Unassigned1", 3261)
BinField.addEnum("SEGYRevision", 3501)
BinField.addEnum("TraceFlag", 3503)
BinField.addEnum("ExtendedHeaders", 3505)
BinField.addEnum("Unassigned2", 3507)
