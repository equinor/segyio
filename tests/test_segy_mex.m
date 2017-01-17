% test segyline

% preconditions
filename = 'test-data/small.sgy';
assert(exist(filename,'file')==2);
t0 = 1111.0;

%% Spec is created
try
    spec = Segy.interpret_segycube(filename, 'Inline3D', 'Crossline3D', t0);
    spec = Segy.interpret_segycube(filename, TraceField.Inline3D, 'Crossline3D', t0);
    spec = Segy.interpret_segycube(filename, TraceField.Inline3D, 193, t0);
    data = Segy.get_cube( spec );

    assert(all(size(data) == [50, 5, 5]));
catch
    %nothing should be caught
    assert(false);
end

% fail when file doesn't exist

try
    Segy.get_header('does-not-exist', 193);
    assert(false);
catch
    assert(true);
end

try
    Segy.get_traces('does-not-exist', 189, 193 );
    assert(false);
catch
    assert(true);
end

try
    Segy.get_ntraces('does-not-exist');
    assert(false);
catch
    assert(true);
end

try
    Segy.interpret_segycube('does-not-exist');
    assert(false);
catch
    assert(true);
end

%% Segy.readInLine 4

spec = Segy.interpret_segycube(filename, 'Inline3D', 'Crossline3D', t0);
data = Segy.get_line(spec, 'iline', 4);
sample_count = length(spec.sample_indexes);

eps = 1e-6;
% first trace along xline
% first sample
assert(abs(data(1, 1) - 4.2)<eps);
% middle sample
assert(abs(data(sample_count/2,1)-4.20024)<eps);
% last sample
assert(abs(data(sample_count,1)-4.20049)<eps);

% middle trace along xline
middle = 3;
% first sample
assert(abs(data(1, middle) - 4.22) < eps);
% middle sample
assert(abs(data(sample_count/2,middle)-4.22024)<eps);
% last sample
assert(abs(data(sample_count,middle)-4.22049)<eps);

% middle trace along xline
last = length(spec.crossline_indexes);
% first sample
assert(abs(data(1, last) - 4.24) < eps);
% middle sample
assert(abs(data(sample_count/2,last)-4.24024)<eps);
% last sample
assert(abs(data(sample_count,last)-4.24049)<eps);

%% Segy.readCrossLine 1433

spec = SegySpec(filename, TraceField.Inline3D, TraceField.Crossline3D, t0);
data = Segy.readCrossLine(spec, 20);
data = Segy.readCrossLine(spec, 21);
data = Segy.readCrossLine(spec, 22);
data = Segy.readCrossLine(spec, 23);
data = Segy.readCrossLine(spec, 24);
data = Segy.readCrossLine(spec, 22);
sample_count = length(spec.sample_indexes);

eps = 1e-4;
% first trace along iline
% first sample
assert(abs(data(1, 1) - 1.22) < eps);
% middle sample
assert(abs(data(sample_count/2,1)-1.22024)<eps);
% last sample
assert(abs(data(sample_count,1)-1.22049)<eps);

% middle trace along iline
middle = 3;
% first sample
assert(abs(data(1, middle) - 3.22) < eps);
% middle sample
assert(abs(data(sample_count/2,middle)-3.22029)<eps);
% last sample
assert(abs(data(sample_count,middle)-3.22049)<eps);

% middle trace along iline
last = length(spec.inline_indexes);
% first sample
assert(abs(data(1, last) - 5.22) < eps);
% middle sample
assert(abs(data(sample_count/2,last)-5.22029)<eps);
% last sample
assert(abs(data(sample_count,last)-5.22049)<eps);

filename_write = 'test-data/SEGY-3D_write_mex.sgy';

copyfile(filename, filename_write)

spec = SegySpec(filename_write, TraceField.Inline3D, TraceField.Crossline3D, t0);
data = Segy.get_line(spec, 'xline', 22);

assert(abs(data(1, 1) - 1.22) < eps);

data(1,1) = 100;

Segy.writeCrossLine(spec, data, 22);

data = Segy.readCrossLine(spec, 22);
assert(data(1, 1) == 100);

data = Segy.readInLine(spec, 4);

assert(abs(data(1, 1) - 4.2) < eps);

data(1,1) = 200;

Segy.writeInLine(spec, data, 4);

data = Segy.readInLine(spec, 4);
assert(data(1, 1) == 200);

[~, dt, nt] = Segy.get_traces(filename);
assert(dt == 4000);
assert(nt == 1);

[headers, notraces] = Segy.get_header(filename, 'Inline3D');
assert(isequal((1:5), unique(headers)));
assert(notraces == 25)

assert(Segy.get_ntraces(filename) == 25);


% Goal:
%   Fast writing of segy file
%
% Inputs:
%   filename         Filename of segyfile to write
%   filename_orig    Filename of segyfile to copy header
%   data:            Data
%   nt:              Number of time samples
%   nxl:             Number of Xlines
%   nil:             Number of Inlines
%
% function provided by Matteo Ravasi

Segy_struct_orig = Segy.interpret_segycube(filename,'Inline3D','Crossline3D');
data = Segy.get_traces(filename);
data = data + 1000;

nt = 50;
nxl = 5;
nil = 5;

if( ( nt  == numel( Segy_struct_orig.t ) ) &&...
    ( nxl == numel( Segy_struct_orig.xline ) ) &&...
    ( nil == numel( Segy_struct_orig.iline ) ) )

    data = reshape( data, [nt, nxl*nil] );

    filename_copy = 'test-data/SEGY-3D_copy.sgy';
    copyfile( filename, filename_copy );

    Segy.put_traces( filename_copy, data, 1, nxl*nil );
    spec = Segy.interpret_segycube( filename_copy );
    data = Segy.get_line(spec, 'iline', 4);

    assert(abs(data(1, 1) - 1004.2) < eps);
    assert(abs(data(sample_count/2,1) - 1004.20024) < eps);
    assert(abs(data(sample_count,1) - 1004.20049) < eps);

    middle = 3;
    assert(abs(data(1, middle) - 1004.22) < eps);
    assert(abs(data(sample_count/2, middle) - 1004.22024) < eps);
    assert(abs(data(sample_count, middle) - 1004.22049) < eps);

    last = length(spec.crossline_indexes);
    assert(abs(data(1, last) - 1004.24) < eps);
    assert(abs(data(sample_count/2, last) - 1004.24024) < eps);
    assert(abs(data(sample_count, last) - 1004.24049) < eps);
else
    assert(false);
end

% test put_line
data = Segy.get_line(spec, 'iline', 4);
data = data + 100;
p1 = data(sample_count/2, 1);
p2 = data(sample_count, 1);
Segy.put_line(spec, data, 'iline', 4);
data = Segy.get_line( spec, 'iline', 4);
assert(all([p1, p2] == [data(sample_count/2, 1), data(sample_count, 1)]));

% read trace headers and file headers
dummy = Segy.get_segy_header( filename );
dummy = Segy.get_trace_header( filename, 0 );
dummy = Segy.get_trace_header( filename, 10 );

Segy.put_headers( filename, 10, 'cdp' );
Segy.get_header( filename, 'cdp' );

increasing = linspace( 1, notraces, notraces );
Segy.put_headers( filename_copy, increasing, 'offset' );
assert( all(increasing == Segy.get_header( filename_copy, 'offset' )) );

%% test_traces_offset
prestack_filename = 'test-data/small-ps.sgy';
cube_with_offsets = Segy.parse_ps_segycube(prestack_filename);
assert(isequal(cube_with_offsets.offset, [1 2]));

try
    ps_line1 = Segy.get_ps_line(cube_with_offsets, 'iline', 22);
    % reading a line that doesn't exist should throw
    assert(false);
end
ps_line1 = Segy.get_ps_line(cube_with_offsets, 'iline', 1);
ps_line2 = Segy.get_ps_line(cube_with_offsets, 'xline', 1);

% offset 1
assert(abs(ps_line1(1,1,1) - 101.01)    < eps);
assert(abs(ps_line1(1,1,2) - 101.02)    < eps);
assert(abs(ps_line1(1,1,3) - 101.03)    < eps);
assert(abs(ps_line1(2,1,1) - 101.01001) < eps);
assert(abs(ps_line1(3,1,1) - 101.01002) < eps);
assert(abs(ps_line1(2,1,2) - 101.02001) < eps);

% offset 2
assert(abs(ps_line1(1,2,1) - 201.01)    < eps);
assert(abs(ps_line1(1,2,2) - 201.02)    < eps);
assert(abs(ps_line1(1,2,3) - 201.03)    < eps);
assert(abs(ps_line1(2,2,1) - 201.01001) < eps);
assert(abs(ps_line1(3,2,1) - 201.01002) < eps);
assert(abs(ps_line1(2,2,2) - 201.02001) < eps);

%%%%% test writing of prestack lines
prestack_dest = 'test-data/mex-tmp-small-ps.sgy';
assert(copyfile(prestack_filename, prestack_dest));
ps_cube_w = Segy.parse_ps_segycube(prestack_dest);
Segy.put_ps_line( ps_cube_w, ps_line1 - 100.00, 'iline', 1 );
% offset 1
wr_line1 = Segy.get_ps_line( ps_cube_w, 'iline', 1 );
assert(abs(wr_line1(1,1,1) - 001.01)    < eps);
assert(abs(wr_line1(1,1,2) - 001.02)    < eps);
assert(abs(wr_line1(1,1,3) - 001.03)    < eps);
assert(abs(wr_line1(2,1,1) - 001.01001) < eps);
assert(abs(wr_line1(3,1,1) - 001.01002) < eps);
assert(abs(wr_line1(2,1,2) - 001.02001) < eps);

% offset 2
assert(abs(wr_line1(1,2,1) - 101.01)    < eps);
assert(abs(wr_line1(1,2,2) - 101.02)    < eps);
assert(abs(wr_line1(1,2,3) - 101.03)    < eps);
assert(abs(wr_line1(2,2,1) - 101.01001) < eps);
assert(abs(wr_line1(3,2,1) - 101.01002) < eps);
assert(abs(wr_line1(2,2,2) - 101.02001) < eps);
