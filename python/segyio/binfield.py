from . import Enum

class BinField(Enum):
    """Trace header field enumerator

    See also
    -------
    segyio.su : Seismic unix aliases for header fields
    """

    JobID = 3201
    LineNumber = 3205
    ReelNumber = 3209
    Traces = 3213
    AuxTraces = 3215
    Interval = 3217
    IntervalOriginal = 3219
    Samples = 3221
    SamplesOriginal = 3223
    Format = 3225
    EnsembleFold = 3227
    SortingCode = 3229
    VerticalSum = 3231
    SweepFrequencyStart = 3233
    SweepFrequencyEnd = 3235
    SweepLength = 3237
    Sweep = 3239
    SweepChannel = 3241
    SweepTaperStart = 3243
    SweepTaperEnd = 3245
    Taper = 3247
    CorrelatedTraces = 3249
    BinaryGainRecovery = 3251
    AmplitudeRecovery = 3253
    MeasurementSystem = 3255
    ImpulseSignalPolarity = 3257
    VibratoryPolarity = 3259
    ExtTraces = 3261
    ExtAuxTraces = 3265
    ExtSamples = 3269
    ExtSamplesOriginal = 3289
    ExtEnsembleFold = 3293
    Unassigned1 = 3261
    SEGYRevision = 3501
    SEGYRevisionMinor = 3502
    TraceFlag = 3503
    ExtendedHeaders = 3505
    Unassigned2 = 3507

keys = {
    'JobID'                 : 3201,
    'LineNumber'            : 3205,
    'ReelNumber'            : 3209,
    'Traces'                : 3213,
    'AuxTraces'             : 3215,
    'Interval'              : 3217,
    'IntervalOriginal'      : 3219,
    'Samples'               : 3221,
    'SamplesOriginal'       : 3223,
    'Format'                : 3225,
    'EnsembleFold'          : 3227,
    'SortingCode'           : 3229,
    'VerticalSum'           : 3231,
    'SweepFrequencyStart'   : 3233,
    'SweepFrequencyEnd'     : 3235,
    'SweepLength'           : 3237,
    'Sweep'                 : 3239,
    'SweepChannel'          : 3241,
    'SweepTaperStart'       : 3243,
    'SweepTaperEnd'         : 3245,
    'Taper'                 : 3247,
    'CorrelatedTraces'      : 3249,
    'BinaryGainRecovery'    : 3251,
    'AmplitudeRecovery'     : 3253,
    'MeasurementSystem'     : 3255,
    'ImpulseSignalPolarity' : 3257,
    'VibratoryPolarity'     : 3259,
    'ExtTraces'             : 3261,
    'ExtAuxTraces'          : 3265,
    'ExtSamples'            : 3269,
    'ExtSamplesOriginal'    : 3289,
    'ExtEnsembleFold'       : 3293,
    'Unassigned1'           : 3301,
    'SEGYRevision'          : 3501,
    'SEGYRevisionMinor'     : 3502,
    'TraceFlag'             : 3503,
    'ExtendedHeaders'       : 3505,
    'Unassigned2'           : 3507,
}
