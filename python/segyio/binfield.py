import warnings
from . import Enum

class TracesWrapper:
    def __get__(self, instance, owner):
        warnings.warn(
            "Traces is deprecated and will be removed in a future version."
            "Please use EnsembleTraces instead.",
            DeprecationWarning,
            stacklevel=2
        )
        return 3213

class AuxTracesWrapper:
    def __get__(self, instance, owner):
        warnings.warn(
            "AuxTraces is deprecated and will be removed in a future version."
            "Please use AuxEnsembleTraces instead.",
            DeprecationWarning,
            stacklevel=2
        )
        return 3215

class ExtTracesWrapper:
    def __get__(self, instance, owner):
        warnings.warn(
            "ExtTraces is deprecated and will be removed in a future version."
            "Please use ExtEnsembleTraces instead.",
            DeprecationWarning,
            stacklevel=2
        )
        return 3261

class ExtAuxTracesWrapper:
    def __get__(self, instance, owner):
        warnings.warn(
            "ExtAuxTraces is deprecated and will be removed in a future version."
            "Please use ExtAuxEnsembleTraces instead.",
            DeprecationWarning,
            stacklevel=2
        )
        return 3265


class BinField(Enum):
    """Trace header field enumerator

    See also
    -------
    segyio.su : Seismic unix aliases for header fields
    """

    JobID = 3201
    LineNumber = 3205
    ReelNumber = 3209
    Traces = TracesWrapper()  # Deprecated, use EnsembleTraces
    EnsembleTraces = 3213
    AuxTraces = AuxTracesWrapper()  # Deprecated, use AuxEnsembleTraces
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
    ExtTraces = ExtTracesWrapper()  # Deprecated, use ExtEnsembleTraces
    ExtEnsembleTraces = 3261
    ExtAuxTraces = ExtAuxTracesWrapper()  # Deprecated, use ExtAuxEnsembleTraces
    ExtAuxEnsembleTraces = 3265
    ExtSamples = 3269
    ExtSamplesOriginal = 3289
    ExtEnsembleFold = 3293
    Unassigned1 = 3301
    SEGYRevision = 3501
    SEGYRevisionMinor = 3502
    TraceFlag = 3503
    ExtendedHeaders = 3505
    Unassigned2 = 3507

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
    'ExtSamplesOriginal'        : 3289,
    'ExtEnsembleFold'           : 3293,
    'Unassigned1'               : 3301,
    'SEGYRevision'              : 3501,
    'SEGYRevisionMinor'         : 3502,
    'TraceFlag'                 : 3503,
    'ExtendedHeaders'           : 3505,
    'Unassigned2'               : 3507,
}
