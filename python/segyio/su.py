from segyio import Enum
from segyio import TraceField as tf
from segyio import BinField as bf

class su(Enum):
    """ Since v1.1
        Seismic Unix style aliases for binary and trace header fields.

    About names:
        Seismic Unix does not have names for all possible fields, as it came
        around during an early revision of SEG-Y, and names for these fields
        are created by segyio in their absence. If Seismic Unix starts
        providing names for these fields, they will be added to these alies,
        and in conflicts take presedence after some time. If there are no
        conflicts, segyio names are considered for deprecation on a
        case-by-case basis, but will most likely be supported along with the
        Seismic Unix name.
    """
# trace
    tracl  = tf.TRACE_SEQUENCE_LINE
    tracr  = tf.TRACE_SEQUENCE_FILE
    fldr   = tf.FieldRecord
    tracf  = tf.TraceNumber
    ep     = tf.EnergySourcePoint
    cdp    = tf.CDP
    cdpt   = tf.CDP_TRACE
    trid   = tf.TraceIdentificationCode
    nvs    = tf.NSummedTraces
    nhs    = tf.NStackedTraces
    duse   = tf.DataUse
    offset = tf.offset
    gelev  = tf.ReceiverGroupElevation
    selev  = tf.SourceSurfaceElevation
    sdepth = tf.SourceDepth
    gdel   = tf.ReceiverDatumElevation
    sdel   = tf.SourceDatumElevation
    swdep  = tf.SourceWaterDepth
    gwdep  = tf.GroupWaterDepth
    scalel = tf.ElevationScalar
    scalco = tf.SourceGroupScalar
    sx     = tf.SourceX
    sy     = tf.SourceY
    gx     = tf.GroupX
    gy     = tf.GroupY
    counit = tf.CoordinateUnits
    wevel  = tf.WeatheringVelocity
    swevel = tf.SubWeatheringVelocity
    sut    = tf.SourceUpholeTime
    gut    = tf.GroupUpholeTime
    sstat  = tf.SourceStaticCorrection
    gstat  = tf.GroupStaticCorrection
    tstat  = tf.TotalStaticApplied
    laga   = tf.LagTimeA
    lagb   = tf.LagTimeB
    delrt  = tf.DelayRecordingTime
    muts   = tf.MuteTimeStart
    mute   = tf.MuteTimeEND
    ns     = tf.TRACE_SAMPLE_COUNT
    dt     = tf.TRACE_SAMPLE_INTERVAL
    gain   = tf.GainType
    igc    = tf.InstrumentGainConstant
    igi    = tf.InstrumentInitialGain
    corr   = tf.Correlated
    sfs    = tf.SweepFrequencyStart
    sfe    = tf.SweepFrequencyEnd
    slen   = tf.SweepLength
    styp   = tf.SweepType
    stat   = tf.SweepTraceTaperLengthStart
    stae   = tf.SweepTraceTaperLengthEnd
    tatyp  = tf.TaperType
    afilf  = tf.AliasFilterFrequency
    afils  = tf.AliasFilterSlope
    nofilf = tf.NotchFilterFrequency
    nofils = tf.NotchFilterSlope
    lcf    = tf.LowCutFrequency
    hcf    = tf.HighCutFrequency
    lcs    = tf.LowCutSlope
    hcs    = tf.HighCutSlope
    year   = tf.YearDataRecorded
    day    = tf.DayOfYear
    hour   = tf.HourOfDay
    minute = tf.MinuteOfHour
    sec    = tf.SecondOfMinute
    timbas = tf.TimeBaseCode
    trwf   = tf.TraceWeightingFactor
    grnors = tf.GeophoneGroupNumberRoll1
    grnofr = tf.GeophoneGroupNumberFirstTraceOrigField
    grnlof = tf.GeophoneGroupNumberLastTraceOrigField
    gaps   = tf.GapSize
    otrav  = tf.OverTravel
    cdpx   = tf.CDP_X
    cdpy   = tf.CDP_Y
    iline  = tf.INLINE_3D
    xline  = tf.CROSSLINE_3D
    sp     = tf.ShotPoint
    scalsp = tf.ShotPointScalar
    trunit = tf.TraceValueMeasurementUnit
    tdcm   = tf.TransductionConstantMantissa
    tdcp   = tf.TransductionConstantPower
    tdunit = tf.TransductionUnit
    triden = tf.TraceIdentifier
    sctrh  = tf.ScalarTraceHeader
    stype  = tf.SourceType
    sedm   = tf.SourceEnergyDirectionMantissa
    sede   = tf.SourceEnergyDirectionExponent
    smm    = tf.SourceMeasurementMantissa
    sme    = tf.SourceMeasurementExponent
    smunit = tf.SourceMeasurementUnit
    uint1  = tf.UnassignedInt1
    uint2  = tf.UnassignedInt2

# binary
    jobid  = bf.JobID
    lino   = bf.LineNumber
    reno   = bf.ReelNumber
    ntrpr  = bf.Traces
    nart   = bf.AuxTraces
    hdt    = bf.Interval
    dto    = bf.IntervalOriginal
    hns    = bf.Samples
    nso    = bf.SamplesOriginal
    format = bf.Format
    fold   = bf.EnsembleFold
    tsort  = bf.SortingCode
    vscode = bf.VerticalSum
    hsfs   = bf.SweepFrequencyStart
    hsfe   = bf.SweepFrequencyEnd
    hslen  = bf.SweepLength
    hstyp  = bf.Sweep
    schn   = bf.SweepChannel
    hstas  = bf.SweepTaperStart
    hstae  = bf.SweepTaperEnd
    htatyp = bf.Taper
    hcorr  = bf.CorrelatedTraces
    bgrcv  = bf.BinaryGainRecovery
    rcvm   = bf.AmplitudeRecovery
    mfeet  = bf.MeasurementSystem
    polyt  = bf.ImpulseSignalPolarity
    vpol   = bf.VibratoryPolarity
    unas1  = bf.Unassigned1
    rev    = bf.SEGYRevision
    trflag = bf.TraceFlag
    exth   = bf.ExtendedHeaders
    unas2  = bf.Unassigned2
