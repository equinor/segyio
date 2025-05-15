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
    EnsembleTraces = 3213
    AuxEnsembleTraces = 3215
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
    ExtEnsembleTraces = 3261
    ExtAuxEnsembleTraces = 3265
    ExtSamples = 3269
    ExtInterval = 3273
    ExtIntervalOriginal = 3281
    ExtSamplesOriginal = 3289
    ExtEnsembleFold = 3293
    IntConstant = 3297
    Unassigned1 = 3301
    SEGYRevision = 3501
    SEGYRevisionMinor = 3502
    TraceFlag = 3503
    ExtendedHeaders = 3505
    MaxAdditionalTraceHeaders = 3507
    SurveyType = 3509
    TimeBasisCode = 3511
    NrTracesInStream = 3513
    FirstTraceOffset = 3521
    NrTrailerRecords = 3529
    Unassigned2 = 3533

keys = {
    'JobID'                     : 3201,
    'LineNumber'                : 3205,
    'ReelNumber'                : 3209,
    'EnsembleTraces'            : 3213,
    'AuxEnsembleTraces'         : 3215,
    'Interval'                  : 3217,
    'IntervalOriginal'          : 3219,
    'Samples'                   : 3221,
    'SamplesOriginal'           : 3223,
    'Format'                    : 3225,
    'EnsembleFold'              : 3227,
    'SortingCode'               : 3229,
    'VerticalSum'               : 3231,
    'SweepFrequencyStart'       : 3233,
    'SweepFrequencyEnd'         : 3235,
    'SweepLength'               : 3237,
    'Sweep'                     : 3239,
    'SweepChannel'              : 3241,
    'SweepTaperStart'           : 3243,
    'SweepTaperEnd'             : 3245,
    'Taper'                     : 3247,
    'CorrelatedTraces'          : 3249,
    'BinaryGainRecovery'        : 3251,
    'AmplitudeRecovery'         : 3253,
    'MeasurementSystem'         : 3255,
    'ImpulseSignalPolarity'     : 3257,
    'VibratoryPolarity'         : 3259,
    'ExtEnsembleTraces'         : 3261,
    'ExtAuxEnsembleTraces'      : 3265,
    'ExtSamples'                : 3269,
    'ExtInterval'               : 3273,
    'ExtIntervalOriginal'       : 3281,
    'ExtSamplesOriginal'        : 3289,
    'ExtEnsembleFold'           : 3293,
    'IntConstant'               : 3297,
    'Unassigned1'               : 3301,
    'SEGYRevision'              : 3501,
    'SEGYRevisionMinor'         : 3502,
    'TraceFlag'                 : 3503,
    'ExtendedHeaders'           : 3505,
    'MaxAdditionalTraceHeaders' : 3507,
    'SurveyType'                : 3509,
    'TimeBasisCode'             : 3511,
    'NrTracesInStream'          : 3513,
    'FirstTraceOffset'          : 3521,
    'NrTrailerRecords'          : 3529,
    'Unassigned2'               : 3533,
}
