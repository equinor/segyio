classdef Segy
% add comment here
    properties
        spec
    end

    methods(Static)

        function obj = readInLine(spec, index)
            obj = segy_read_write_line_mex(spec, index, max(size(spec.crossline_indexes)), spec.inline_indexes, spec.il_stride, spec.offset_count);
        end

        function obj = readCrossLine(spec, index)
            obj = segy_read_write_line_mex(spec, index, max(size(spec.inline_indexes)), spec.crossline_indexes, spec.xl_stride, spec.offset_count);
        end

        function obj = writeCrossLine(spec, data, index)
            segy_read_write_line_mex(spec, index, max(size(spec.inline_indexes)), spec.crossline_indexes, spec.xl_stride, spec.offset_count, data);
            obj = data;
        end

        function obj = writeInLine(spec, data, index)
            segy_read_write_line_mex(spec, index, max(size(spec.crossline_indexes)), spec.inline_indexes, spec.il_stride, spec.offset_count, data);
            obj = data;
        end

        function data = get_line(cube, dir, n)
            if strcmpi(dir, 'iline')
                data = segy_read_write_line_mex(cube, n, max(size(cube.crossline_indexes)), cube.inline_indexes, cube.il_stride, cube.offset_count);

            elseif strcmpi(dir, 'xline')
                data = segy_read_write_line_mex(cube, n, max(size(cube.inline_indexes)), cube.crossline_indexes, cube.xl_stride, cube.offset_count);

            else
                error('Only iline and xline are valid directions.');
            end
        end

        function data = put_line(cube, data, dir, n)
            if strcmpi(dir, 'iline')
                segy_read_write_line_mex(cube, n, max(size(cube.crossline_indexes)), cube.inline_indexes, cube.il_stride, cube.offset_count, data);

            elseif strcmpi(dir, 'xline')
                segy_read_write_line_mex(cube, n, max(size(cube.inline_indexes)), cube.crossline_indexes, cube.xl_stride, cube.offset_count, data );

            else
                error('Only iline and xline are valid directions.');
            end
        end

        % Goal:
        %   Fast reading of trace header words in segy file.
        %
        % Algorithm:
        %   Use keyword as specified in available_headers. If byte location is
        %   known byte2headerword converts to keyword.
        %
        % Inputs:
        %   filename    Filename of segyfile
        %   headword    Name of header word to read (example: 'cdp')
        %   notype      Optional. If number format is different than set in segy
        %               header this can be set by notype. Valid numbers are 1,2,3,5
        %               and 8 as spesified by SEG-Y rev 1 standard.
        %
        % Output:
        %   name                  meaning
        %   tr_heads              Header values
        function [tr_heads, notrace] = get_header(filename, headword, notype)
            if exist(filename, 'file') ~= 2
                error('File does not exist')
            end
            % notype isn't really use so we ignore it (for interface compatilibty)

            if ischar(headword)
                headword = TraceField.(headword);
            end

            headword = int32(headword);
            [x, y] = segy_get_header_mex(filename, headword);
            tr_heads = x;
            notrace = y;
        end

        % [data,dt,notype] = get_traces(filename,n1,n2,notype)
        %
        % Goal:
        %   Fast reading of traces in segy volume. Not for front-end use. Use
        %   get_line / get_slice / get_subcube instead.
        %
        % Algorithm:
        %
        %
        % Inputs:
        %   filename    Filename of segyfile
        %   n1          (Optional) First trace. If no first and last trace is
        %               specified, all file is read.
        %   n2          (Optional) Last trace.
        %   notype      Optional. If number format is different than set in segy
        %               header this can be set by notype. Valid numbers are 1,2,3,5
        %               and 8 as spesified by SEG-Y rev 1 standard.
        %
        % Output:
        %   data        Traces read from file
        %   dt          Sample interval
        %   notype      Number format
        function [data, dt, notype] = get_traces( filename, n1, n2, notype )
            if exist(filename, 'file') ~= 2
                error('File does not exist')
            end

            if nargin < 2
                n1 = 1;
            end
            if nargin < 3
                n2 = 0;
            end

            if nargin < 4
                notype = -1;
            end

            [data, dt, notype] = segy_get_traces_mex( filename, n1 - 1, n2 - 1, notype );
        end

        %function ntraces = get_ntraces(filename);
        %
        % Goal
        %   Count the number of traces in a segy file
        %
        % Inputs:
        %   filename    Filename of segyfile
        %
        % Output:
        %   notrace     number of traces
        %               return 0 in case of error
        function notrace = get_ntraces( filename )
            if exist(filename, 'file') ~= 2
                error('File does not exist')
            end
            notrace = segy_get_ntraces_mex( filename );
        end

        % Goal:
        %   Interpret segy cube as a 3D cube and save information needed to access
        %   the segy file as a cube in terms of inline and crossline numbers.
        %
        % inputs:
        %   filename    filename of segy file
        %   il_word     bytenumber or header word for inline number
        %   xl_word     bytenumber or header word for crossline number
        %   t0          Time (ms) / depth (m) of first sample. Optional (default = 0)
        %
        % output:
        %   segycube    struct needed to access segy file as a cube.

        function segycube = interpret_segycube(filename, il_word, xl_word, t0)
            if exist(filename, 'file') ~= 2
                error('File does not exist')
            end

            if nargin < 4
                t0      = 0;
            end
            if nargin < 3
                xl_word = TraceField.Crossline3D;
            end
            if nargin < 2
                il_word = TraceField.Inline3D;
            end

            % for compatibility with old code; if argument is passed as a
            % string, first convert to an enum, then pass that enum to the
            % constructor
            if ischar(il_word)
                il_word = TraceField.(il_word);
            end

            if ischar(xl_word)
                xl_word = TraceField.(xl_word);
            end

            segycube = SegySpec(filename, il_word, xl_word, t0);
        end

        % Goal:
        %   Interpret segy cube as a 3D cube and save information needed to access
        %   the segy file as a cube in terms of inline and crossline numbers.
        %
        % inputs:
        %   name        meaning
        %   filename    filename of segy file
        %   offset      bytenumber or header word for offset number
        %   il_word     bytenumber or header word for inline number
        %   xl_word     bytenumber or header word for crossline number
        %   t0          Time (ms) / depth (m) of first sample. Optional (default = 0)
        %
        % output:
        %   name        meaning
        %   segycube    struct needed to access segy file as a cube. Used by
        %               get_ps_line.m, put_ps_line.m and possibly friends
        function segycube = parse_ps_segycube(filename, offset, il_word, xl_word, t0)
            if ~exist('offset', 'var') || isempty(offset)
                offset = TraceField.offset;
            end
            if ~exist('il_word', 'var') || isempty(il_word)
                il_word = TraceField.Inline3D;
            end
            if ~exist('xl_word', 'var') || isempty(xl_word)
                xl_word = TraceField.Crossline3D;
            end
            if ~exist('t0', 'var') || isempty(t0)
                t0 = 0;
            end

            if ischar(il_word)
                il_word = TraceField.(il_word);
            end

            if ischar(xl_word)
                xl_word = TraceField.(xl_word);
            end

            if ischar(offset)
                offset = TraceField.(offset);
            end

            offset  = int32(offset);
            il_word = int32(il_word);
            xl_word = int32(xl_word);
            t0      = double(t0);

            segycube = Segy.interpret_segycube(filename, il_word, xl_word, t0);
            offsets = segy_get_offsets_mex(segycube, offset, il_word, xl_word);
            segycube.offset = offsets;
        end

        % Goal:
        %   Read an inline / crosline from a cube.
        %
        % Inputs:
        %   cube        Data as an interpreted segy cube from
        %               'interpret_segycube.m'
        %   dir         Direction of desired line (iline / xline) as a string
        %   n           Inline / crossline number
        %
        % Output:
        %   data        Extracted line
        function data = get_ps_line(cube, dir, n)
            if nargin < 3
                error('Too few arguments. Usage: Segy.get_ps_line(cube, dir, n)');
            end

            if strcmpi(dir, 'iline')
                len  = max(size(cube.crossline_indexes));
                ix   = cube.inline_indexes;
                st   = cube.il_stride;
            elseif strcmpi(dir, 'xline')
                len  = max(size(cube.inline_indexes));
                ix   = cube.crossline_indexes;
                st   = cube.xl_stride;
            else
                error('Only iline and xline are valid directions.');
            end

            tmp = segy_read_write_ps_line_mex(cube, n, len, ix, st);
            tmp = reshape(tmp, [], cube.offset_count);
            nt = length(cube.t);
            data = permute(reshape(tmp, nt, size(tmp, 1)/nt, []), [1 3 2]);
        end

        function data = put_ps_line(cube, data, dir, n)
            if nargin < 4
                error('Too few arguments. Usage: Segy.put_ps_line(cube, data, dir, n)');
            end

            if strcmpi(dir, 'iline')
                len  = max(size(cube.crossline_indexes));
                ix   = cube.inline_indexes;
                st   = cube.il_stride;
            elseif strcmpi(dir, 'xline')
                len  = max(size(cube.inline_indexes));
                ix   = cube.crossline_indexes;
                st   = cube.xl_stride;
            else
                error('Only iline and xline are valid directions.');
            end

            tmp = permute( data, [1 3 2] );
            segy_read_write_ps_line_mex( cube, n, len, ix, st, tmp );
        end

        function data = get_cube(sc)
            data = Segy.get_traces(sc.filename);

            if sc.trace_sorting_format == TraceSortingFormat.iline
                data = reshape( data, size( sc.sample_indexes, 1 ), size( sc.xline, 1 ), size( sc.iline, 1 ) );
            elseif sc.trace_sorting_format == TraceSortingFormat.xline
                data = reshape( data, size( sc.sample_indexes, 1 ), size( sc.iline, 1 ), size( sc.xline, 1 ) );
            else
                warning('Sorting was not set properly. Data returned as single long line');
            end
        end

        function [data, notype] = put_traces(filename, data, n1, n2, notype)
            if exist(filename, 'file') ~= 2
                error('File does not exist')
            end

            if nargin < 2
                error('Too few arguments. Usage: put_traces( filename, data, (optional): n1, n2, notype)')
            end

            if nargin < 3
                n1 = 1;
            end

            if nargin < 4
                n2 = 0;
            end

            if nargin < 5
                notype = -1;
            end

            % matlab uses 1-indexing, but C wants its positions 0-indexed.
            [data, notype] = segy_put_traces_mex( filename, data, n1 - 1, n2 - 1, notype );
        end


        % function SegyHeader = get_segy_header(filename)
        %
        % Goal:
        %   Read segy header. Extended textual headers are not read
        %
        % Inputs:
        %   filename    Filename of segyfile
        %
        % Output:
        %   SegyHeader  Struct with entire segy header
        function SegyHeader = get_segy_header(filename)
            if exist(filename, 'file') ~= 2
                error('File does not exist')
            end

            [ebcdic, bin] = segy_get_segy_header_mex( filename );

            SegyHeader.ebcdic = ebcdic;
            SegyHeader.JobIdNumber = segy_get_bfield_mex( bin, 3201 );
            SegyHeader.LineNumber = segy_get_bfield_mex( bin, 3205 );
            SegyHeader.ReelNumber = segy_get_bfield_mex( bin, 3209 );
            SegyHeader.NumberOfTracesPerEnsemble = segy_get_bfield_mex( bin, 3213 );
            SegyHeader.NumberOfAuxTracesPerEnsemble = segy_get_bfield_mex( bin, 3215 );
            SegyHeader.SampleInterval = segy_get_bfield_mex( bin, 3217 );
            SegyHeader.SampleIntervalOriginal = segy_get_bfield_mex( bin, 3219 );
            SegyHeader.NumberOfSamples = segy_get_bfield_mex( bin, 3221 );
            SegyHeader.NumberOfSamplesOriginal = segy_get_bfield_mex( bin, 3223 );

            SegyHeader.SampleFormat = 0;
            switch segy_get_bfield_mex( bin, 3225 )
                case 1
                    SegyHeader.SampleFormat = 'IBM32';
                case 2
                    SegyHeader.SampleFormat = 'INT32';
                case 3
                    SegyHeader.SampleFormat = 'INT16';
                case 4
                    SegyHeader.SampleFormat = 'Obsolete';
                case 5
                    SegyHeader.SampleFormat = 'IEEE32';
                case 6
                    SegyHeader.SampleFormat = 'NotUsed';
                case 7
                    SegyHeader.SampleFormat = 'NotUsed';
                case 8
                    SegyHeader.SampleFormat = 'INT8';
            end

            SegyHeader.EnsembleFold = segy_get_bfield_mex( bin, 3227 );

            SegyHeader.TraceSortingCode = 0;
            switch segy_get_bfield_mex( bin, 3229 )
                case -1
                    SegyHeader.TraceSortingCode = 'Other';
                case 0
                    SegyHeader.TraceSortingCode = 'Unknown';
                case 1
                    SegyHeader.TraceSortingCode = 'AsRecorded';
                case 2
                    SegyHeader.TraceSortingCode = 'CDP';
                case 3
                    SegyHeader.TraceSortingCode = 'SingleFoldContinuousProfile';
                case 4
                    SegyHeader.TraceSortingCode = 'HorizontallyStacked';
                case 5
                    SegyHeader.TraceSortingCode = 'CommonSourcePoint';
                case 6
                    SegyHeader.TraceSortingCode = 'CommonReceiverPoint';
                case 7
                    SegyHeader.TraceSortingCode = 'CommonOffsetPoint';
                case 8
                    SegyHeader.TraceSortingCode = 'CommonMidPoint';
                case 9
                    SegyHeader.TraceSortingCode = 'CommonConversionPoint';
            end

            SegyHeader.VerticalSumCode = [num2str(segy_get_bfield_mex( bin, 3231 )), '_Sum'];
            SegyHeader.SweepFrequencyAtStart = segy_get_bfield_mex( bin, 3233 );
            SegyHeader.SweepFrequencyAtEnd = segy_get_bfield_mex( bin, 3235 );
            SegyHeader.SweepLength = segy_get_bfield_mex( bin, 3237 );
            SegyHeader.SweepTypeCode = 0;
            switch segy_get_bfield_mex( bin, 3239 )
                case 1
                    SegyHeader.SweepTypeCode = 'Linear';
                case 2
                    SegyHeader.SweepTypeCode = 'Parabolic';
                case 3
                    SegyHeader.SweepTypeCode = 'Exponential';
                case 4
                    SegyHeader.SweepTypeCode = 'Other';
            end

            SegyHeader.TraceNoOfSweepChannel = segy_get_bfield_mex( bin, 3241 );
            SegyHeader.SweepTraceTaperLenghtStart = segy_get_bfield_mex( bin, 3243 );
            SegyHeader.SweepTraceTaperLenghtEnd = segy_get_bfield_mex( bin, 3245 );

            SegyHeader.TaperType = 0;
            switch segy_get_bfield_mex( bin, 3247 )
                    case 1
                        SegyHeader.TaperType = 'Linear';
                    case 2
                        SegyHeader.TaperType = 'Cos^2';
                    case 3
                        SegyHeader.TaperType = 'Other';
            end

            SegyHeader.CorrelatedDataTraces = 0;
            switch segy_get_bfield_mex( bin, 3249 )
                case 1
                    SegyHeader.CorrelatedDataTraces = 'No';
                case 2
                    SegyHeader.CorrelatedDataTraces = 'Yes';
            end

            SegyHeader.BinaryGainRecovered = 0;
            switch segy_get_bfield_mex( bin, 3251 )
                case 1
                    SegyHeader.BinaryGainRecovered = 'Yes';
                case 2
                    SegyHeader.BinaryGainRecovered = 'No';
            end

            SegyHeader.AmplitudeRecoveryMethod = 0;
            switch segy_get_bfield_mex( bin, 3253 )
                case 1
                    SegyHeader.AmplitudeRecoveryMethod = 'None';
                case 2
                    SegyHeader.AmplitudeRecoveryMethod = 'SphericalDivergence';
                case 3
                    SegyHeader.AmplitudeRecoveryMethod = 'AGC';
                case 4
                    SegyHeader.AmplitudeRecoveryMethod = 'Other';
            end

            SegyHeader.MeasurementSystem = 0;
            switch segy_get_bfield_mex( bin, 3255 )
                case 1
                    SegyHeader.MeasurementSystem = 'Meter';
                case 2
                    SegyHeader.MeasurementSystem = 'Feet';
            end

            SegyHeader.ImpulseSignalPolarity = 0;
            switch segy_get_bfield_mex( bin, 3257 )
                case 1
                    SegyHeader.ImpulseSignalPolarity = 'IncreasePressureNegativeNumber';
                case 2
                    SegyHeader.ImpulseSignalPolarity = 'IncreasePressurePositiveNumber';
            end

            SegyHeader.VibratorPolarityCode = 0;
            switch segy_get_bfield_mex( bin, 3259 )
                case 1
                    SegyHeader.VibratorPolarityCode = '337.5-22.5';
                case 2
                    SegyHeader.VibratorPolarityCode = '22.5-67.5';
                case 3
                    SegyHeader.VibratorPolarityCode = '67.5-112.5';
                case 4
                    SegyHeader.VibratorPolarityCode = '112.5-157.5';
                case 5
                    SegyHeader.VibratorPolarityCode = '157.5-202.5';
                case 6
                    SegyHeader.VibratorPolarityCode = '202.5-247.5';
                case 7
                    SegyHeader.VibratorPolarityCode = '247.5-292.5';
                case 8
                    SegyHeader.VibratorPolarityCode = '292.5-337.5';
            end

            SegyHeader.FormatRevisionNumber = segy_get_bfield_mex( bin, 3501 );
            SegyHeader.FixedLengthTraceFlag = segy_get_bfield_mex( bin, 3503 );
            SegyHeader.NumberOfExtTextHeaders = segy_get_bfield_mex( bin, 3505 );
        end

        % [tr_heads,notrace] = get_trace_header(filename,itrace);
        %
        % Goal:
        %   Read the full trace header of the trace itrace
        %
        % Inputs:
        %   filename    Filename of segyfile
        %   itrace      trace number
        %
        % Output:
        %   tr_heads    Header values
        %   notrace     number of traces in segy file
        %
        function [tr_heads, notrace] = get_trace_header(filename, itrace)
            [header, notrace] = segy_get_trace_header_mex( filename, itrace );

            % read trace header
            tr_heads.TraceSequenceLine                      = segy_get_field_mex( header, 1 );
            tr_heads.TraceSequenceFile                      = segy_get_field_mex( header, 5 );
            tr_heads.FieldRecord                            = segy_get_field_mex( header, 9 );
            tr_heads.TraceNumber                            = segy_get_field_mex( header, 13 );
            tr_heads.EnergySourcePoint                      = segy_get_field_mex( header, 17 );
            tr_heads.cdp                                    = segy_get_field_mex( header, 21 );
            tr_heads.cdpTrace                               = segy_get_field_mex( header, 25 );
            tr_heads.TraceIdenitifactionCode                = segy_get_field_mex( header, 29 );
            tr_heads.NSummedTraces                          = segy_get_field_mex( header, 31 );
            tr_heads.NStackedTraces                         = segy_get_field_mex( header, 33 );
            tr_heads.DataUse                                = segy_get_field_mex( header, 35 );
            tr_heads.offset                                 = segy_get_field_mex( header, 37 );
            tr_heads.ReceiverGroupElevation                 = segy_get_field_mex( header, 41 );
            tr_heads.SourceSurfaceElevation                 = segy_get_field_mex( header, 45 );
            tr_heads.SourceDepth                            = segy_get_field_mex( header, 49 );
            tr_heads.ReceiverDatumElevation                 = segy_get_field_mex( header, 53 );
            tr_heads.SourceDatumElevation                   = segy_get_field_mex( header, 57 );
            tr_heads.SourceWaterDepth                       = segy_get_field_mex( header, 61 );
            tr_heads.GroupWaterDepth                        = segy_get_field_mex( header, 65 );
            tr_heads.ElevationScalar                        = segy_get_field_mex( header, 69 );
            tr_heads.SourceGroupScalar                      = segy_get_field_mex( header, 71 );
            tr_heads.SourceX                                = segy_get_field_mex( header, 73 );
            tr_heads.SourceY                                = segy_get_field_mex( header, 77 );
            tr_heads.GroupX                                 = segy_get_field_mex( header, 81 );
            tr_heads.GroupY                                 = segy_get_field_mex( header, 85 );
            tr_heads.CoordinateUnits                        = segy_get_field_mex( header, 89 );
            tr_heads.WeatheringVelocity                     = segy_get_field_mex( header, 91 );
            tr_heads.SubWeatheringVelocity                  = segy_get_field_mex( header, 93 );
            tr_heads.SourceUpholeTime                       = segy_get_field_mex( header, 95 );
            tr_heads.GroupUpholeTime                        = segy_get_field_mex( header, 97 );
            tr_heads.SourceStaticCorrection                 = segy_get_field_mex( header, 99 );
            tr_heads.GroupStaticCorrection                  = segy_get_field_mex( header, 101 );
            tr_heads.TotalStaticApplied                     = segy_get_field_mex( header, 103 );
            tr_heads.LagTimeA                               = segy_get_field_mex( header, 105 );
            tr_heads.LagTimeB                               = segy_get_field_mex( header, 107 );
            tr_heads.DelayRecordingTime                     = segy_get_field_mex( header, 109 );
            tr_heads.MuteTimeStart                          = segy_get_field_mex( header, 111 );
            tr_heads.MuteTimeEND                            = segy_get_field_mex( header, 113 );
            tr_heads.ns                                     = segy_get_field_mex( header, 115 );
            tr_heads.dt                                     = segy_get_field_mex( header, 117 );
            tr_heads.GainType                               = segy_get_field_mex( header, 119 );
            tr_heads.InstrumentGainConstant                 = segy_get_field_mex( header, 121 );
            tr_heads.InstrumentInitialGain                  = segy_get_field_mex( header, 123 );
            tr_heads.Correlated                             = segy_get_field_mex( header, 125 );
            tr_heads.SweepFrequenceStart                    = segy_get_field_mex( header, 127 );
            tr_heads.SweepFrequenceEnd                      = segy_get_field_mex( header, 129 );
            tr_heads.SweepLength                            = segy_get_field_mex( header, 131 );
            tr_heads.SweepType                              = segy_get_field_mex( header, 133 );
            tr_heads.SweepTraceTaperLengthStart             = segy_get_field_mex( header, 135 );
            tr_heads.SweepTraceTaperLengthEnd               = segy_get_field_mex( header, 137 );
            tr_heads.TaperType                              = segy_get_field_mex( header, 139 );
            tr_heads.AliasFilterFrequency                   = segy_get_field_mex( header, 141 );
            tr_heads.AliasFilterSlope                       = segy_get_field_mex( header, 143 );
            tr_heads.NotchFilterFrequency                   = segy_get_field_mex( header, 145 );
            tr_heads.NotchFilterSlope                       = segy_get_field_mex( header, 147 );
            tr_heads.LowCutFrequency                        = segy_get_field_mex( header, 149 );
            tr_heads.HighCutFrequency                       = segy_get_field_mex( header, 151 );
            tr_heads.LowCutSlope                            = segy_get_field_mex( header, 153 );
            tr_heads.HighCutSlope                           = segy_get_field_mex( header, 155 );
            tr_heads.YearDataRecorded                       = segy_get_field_mex( header, 157 );
            tr_heads.DayOfYear                              = segy_get_field_mex( header, 159 );
            tr_heads.HourOfDay                              = segy_get_field_mex( header, 161 );
            tr_heads.MinuteOfHour                           = segy_get_field_mex( header, 163 );
            tr_heads.SecondOfMinute                         = segy_get_field_mex( header, 165 );
            tr_heads.TimeBaseCode                           = segy_get_field_mex( header, 167 );
            tr_heads.TraceWeightningFactor                  = segy_get_field_mex( header, 169 );
            tr_heads.GeophoneGroupNumberRoll1               = segy_get_field_mex( header, 171 );
            tr_heads.GeophoneGroupNumberFirstTraceOrigField = segy_get_field_mex( header, 173 );
            tr_heads.GeophoneGroupNumberLastTraceOrigField  = segy_get_field_mex( header, 175 );
            tr_heads.GapSize                                = segy_get_field_mex( header, 177 );
            tr_heads.OverTravel                             = segy_get_field_mex( header, 179 );
            tr_heads.cdpX                                   = segy_get_field_mex( header, 181 );
            tr_heads.cdpY                                   = segy_get_field_mex( header, 185 );
            tr_heads.Inline3D                               = segy_get_field_mex( header, 189 );
            tr_heads.Crossline3D                            = segy_get_field_mex( header, 193 );
            tr_heads.ShotPoint                              = segy_get_field_mex( header, 197 );
            tr_heads.ShotPointScalar                        = segy_get_field_mex( header, 201 );
            tr_heads.TraceValueMeasurementUnit              = segy_get_field_mex( header, 203 );
            tr_heads.TransductionConstantMantissa           = segy_get_field_mex( header, 205 );
            tr_heads.TransductionConstantPower              = segy_get_field_mex( header, 209 );
            tr_heads.TransductionUnit                       = segy_get_field_mex( header, 211 );
            tr_heads.TraceIdentifier                        = segy_get_field_mex( header, 213 );
            tr_heads.ScalarTraceHeader                      = segy_get_field_mex( header, 215 );
            tr_heads.SourceType                             = segy_get_field_mex( header, 217 );
            tr_heads.SourceEnergyDirectionMantissa          = segy_get_field_mex( header, 219 );
            tr_heads.SourceEnergyDirectionExponent          = segy_get_field_mex( header, 223 );
            tr_heads.SourceMeasurementMantissa              = segy_get_field_mex( header, 225 );
            tr_heads.SourceMeasurementExponent              = segy_get_field_mex( header, 229 );
            tr_heads.SourceMeasurementUnit                  = segy_get_field_mex( header, 231 );
            tr_heads.UnassignedInt1                         = segy_get_field_mex( header, 233 );
            tr_heads.UnassignedInt2                         = segy_get_field_mex( header, 237 );
        end

        % put_headers(filename,headers,headword)
        %
        % Goal:
        %   Fast writing of trace header words in segy file.
        %
        % Inputs:
        %   filename    Filename of segyfile
        %   headers     Array of headervalues or single headervalue. Will be
        %               written to all trace headers. If length of array is
        %               different from number of traces in file an error will be
        %               thrown.
        %   headword    Name of header word to be written (example: 'cdp')
        %
        function put_headers(filename, headers, headword)
            ntraces = Segy.get_ntraces( filename );
            if and( ~isscalar(headers), max(size( headers )) ~= ntraces )
                error( 'Inconsistent dimensions of header values' )
            end

            % if single header value, create a traces-sized vector of that
            % header value, so that segy_put_headers_mex can always assume array
            if isscalar( headers )
                headers = ones( 1, ntraces ) * headers;
            end

            if ischar(headword)
                headword = TraceField.(headword);
            end

            segy_put_headers_mex( filename, headers, int32(headword) );
        end
    end
end
