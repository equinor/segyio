% test segyspec

% preconditions 
filename = 'test-data/small.sgy';
assert(exist(filename,'file')==2);
t0 = 1111.0;

%% no such file
no_such_filename = 'no-such-dir/no-such-file.sgy';
assert(exist(no_such_filename,'file')~=2);
try
    spec = SegySpec(no_such_filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
    %should not reach here
    assert(false);
catch
    %not actually needed...
    assert(true);
end

%% Spec is created
try
    spec = SegySpec(filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
catch
    %nothing should be caught
    assert(false);
end

%% IBM_FLOAT_4_BYTE
spec = SegySpec(filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
assert(spec.sample_format == SegySampleFormat.IBM_FLOAT_4_BYTE);

%% filename is set
spec = SegySpec(filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
assert(strcmp(spec.filename,filename));

%% trace_sorting_format
spec = SegySpec(filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
assert(spec.trace_sorting_format == TraceSortingFormat.iline);

%%offset_count
spec = SegySpec(filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
assert(length(spec.offset_count) == 1);

%% sample_indexes
spec = SegySpec(filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
sample_indexes = spec.sample_indexes;
assert(length(sample_indexes) == 50);

for i = 1:length(sample_indexes)
    t = t0 + (i-1) * 4;
    assert(sample_indexes(i) == t);
end


%% first_trace_pos
spec = SegySpec(filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
first_trace_pos = spec.first_trace_pos;
assert(first_trace_pos == 3600);

%% il_stride
spec = SegySpec(filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
il_stride = spec.il_stride;
assert(il_stride == 1);

%% xl_stride
spec = SegySpec(filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
xl_stride = spec.xl_stride;
assert(xl_stride == 5);

%% xl_stride
spec = SegySpec(filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
trace_bsize = spec.trace_bsize;
assert(trace_bsize == 50*4);

%% xline
spec = SegySpec(filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
assert(length(spec.crossline_indexes)==5)
for xl = spec.crossline_indexes'
    assert(xl >= 20 && xl <= 24);
end

%% iline
spec = SegySpec(filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
assert(length(spec.inline_indexes)==5)

for il = spec.inline_indexes'
    assert(il >= 1 && il <= 5);
end
