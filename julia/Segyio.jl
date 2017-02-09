module Segyio

export trace, iline, xline

const libsegyio = @windows? :segyio : :libsegyio

# we create a type handle for the opaque segy_file handle which has to be
# passed to most segyio functions, then alias it to sgy because it's tedious
# to read in already dense C interop function calls
type sgy_struct
end
typealias sgy Ptr{sgy_struct}

# unlike most functions in segyio, the to_native! C function is used in more
# than one operation - for convenience, we create a julia-native function that
# makes sure we return native floats. Updates its buf argument in-place.
function to_native!(fmt :: Cint, buf)
    ccall( (:segy_to_native, libsegyio),
           Cint,
           (Cint, Cint, Ref{Cfloat}),
           fmt, length(buf), buf )
    return buf
end

# We group the internal meta-data in a record. Users are not really meant to
# query these, which is why they're "hidden" inside a nested record
immutable Meta
    trace0      :: Clong
    traces      :: Int
    samples     :: Int
    bsize       :: Cuint
    fmt         :: Cint
    sorting     :: Int
    ilstride    :: Int
    xlstride    :: Int
end

type SegyFile
    fd          :: sgy
    ilines      :: Array{Cuint}
    xlines      :: Array{Cuint}
    offsets     :: Array{Cuint}
    meta        :: Meta
    filename    :: AbstractString
end

# The user-facing segy object implements IO functions - mostly for experiments
# and REPL use. These are potentially rapidly changing and should ONLY be used
# for printing and inspection, not to control program flow.
function Base.show(io :: IO, f :: SegyFile)
    int(x)  = convert(Int, x)
    ilines  = map( int, f.ilines )
    xlines  = map( int, f.xlines )
    offsets = map( int, f.offsets )
    sorting = f.meta.sorting == 2 ? "Inline" : "Crossline"
    format  = f.meta.fmt == 1 ? "IBM float" : "IEEE float"
    println( io, "SegyFile:     $(f.filename)")
    println( io, "- ilines:     $(length(ilines)) $ilines")
    println( io, "- xlines:     $(length(xlines)) $xlines")
    println( io, "- offsets:    $(length(offsets)) $offsets")
    println( io, "- sorting:    $sorting")
    println( io, "- format:     $format")
end

function Base.showcompact(io :: IO, f :: SegyFile)
    println(io, "SegyFile($(split(f.filename, '/')[end]))" )
end

function binaryheader(fd :: sgy)
    headersz = ccall( (:segy_binheader_size, libsegyio), Cuint, () )
    header = Array(Cchar, headersz)
    err  = ccall( (:segy_binheader, libsegyio), Cint,
                  (sgy, Ptr{Cchar}),
                  fd, header)

    if err != 0 error("Could not read binary header") end

    return header
end

"""
    fopen(path, [mode = "r", il = 189, xl = 193])

Open a file for REPL use and understand basic geometric properties.

Uses standard unix mode for read/write options. You can specify what header
field contains the inline and crossline number with il and xl respectively.

This function is largely meant for interpreter use or specific scenarios where
you know exactly what you're doing. If you're writing a program, please use
`open`.

# Examples
```julia
julia> fd = fopen("small.sgy")
SegyFile:     small.sgy
- ilines:     5 [1,2,3,4,5]
- xlines:     5 [20,21,22,23,24]
- offsets:    1 [1]
- sorting:    Inline
- format:     IBM float
```
"""
function fopen(path :: AbstractString;
               mode :: AbstractString = "r",
               il   :: Integer = 189,
               xl   :: Integer = 193)
    modeb = mode
    if mode[end] != 'b'
        modeb = "$(mode)b"
    end

    if !in( modeb, ["rb", "wb", "ab", "r+b", "w+b", "a+b"] )
        throw( ArgumentError( "Invalid mode '$mode'" ) )
    end

    fd = ccall( (:segy_open, libsegyio), sgy,
                  (Cstring, Cstring),
                  path, mode )
    errno = Libc.errno()

    if fd == C_NULL
        throw( SystemError( "Unable to open file '$path'", errno ) )
    end

    # read values from the binary header
    header = binaryheader( fd )

    trace0 = ccall( (:segy_trace0, libsegyio), Clong, (Ptr{Cchar},), header )
    sample_count = ccall( (:segy_samples, libsegyio), Cuint,
                            (Ptr{Cchar},),
                            header )
    format = ccall( (:segy_format, libsegyio), Cint, (Ptr{Cchar},), header )
    trace_bsize = ccall( (:segy_trace_bsize, libsegyio), Cuint,
                         (Cuint,),
                         sample_count )

    # figure out sorting
    sorting = Array(Cint, 1) # julia >= 0.4 could use Ref{Cint}
    err = ccall( (:segy_sorting, libsegyio), Cint,
                 (sgy, Cint, Cint, Ptr{Cint}, Clong, Cuint),
                 fd, il, xl, sorting, trace0, trace_bsize )

    if err != 0 error( "Could not determine sorting." ) end
    sorting = sorting[1]


    trace_count = Array(Cint, 1)
    err = ccall( (:segy_traces, libsegyio), Cint,
                 (sgy, Ptr{Cint}, Clong, Cuint),
                 fd, trace_count, trace0, trace_bsize )

    if err != 0
        error( "Trace count/fd size mismatch: Segyio requires all traces to be of same length" )
    end
    trace_count = trace_count[1]


    offset_count = Array(Cuint, 1)
    err = ccall( (:segy_offsets, libsegyio), Cint,
                 (sgy, Cint, Cint, Cuint, Ptr{Cuint}, Clong, Cuint),
                 fd, il, xl, trace_count, offset_count, trace0, trace_bsize )

    if err != 0 error( "Could not determine number of offsets in fd." ) end
    offset_count = offset_count[1]


    iline_count = Array(Cint, 1)
    xline_count = Array(Cint, 1)
    err = ccall( (:segy_lines_count, libsegyio), Cint,
                 (sgy, Cint, Cint, Cint, Cint, Ptr{Cint}, Ptr{Cint}, Clong, Cuint),
                 fd, il, xl, sorting, offset_count, iline_count, xline_count, trace0, trace_bsize )

    if err != 0 error( "Could determine number of inlines and crosslines" ) end
    iline_count = iline_count[1]
    xline_count = xline_count[1]

    # figure out line lengths and strides
    iline_length = ccall( (:segy_inline_length,    libsegyio), Cuint,
                          (Cuint,),
                          xline_count )
    xline_length = ccall( (:segy_crossline_length, libsegyio), Cuint,
                          (Cuint,),
                          iline_count )

    iline_stride = Array( Cuint, 1 )
    err = ccall( (:segy_inline_stride, libsegyio), Cint,
                 (Cint, Cuint, Ptr{Cuint}),
                 sorting, iline_count, iline_stride )
    if err != 0 error( "Could not figure out iline stride" ) end
    iline_stride = iline_stride[1]

    xline_stride = Array( Cuint, 1 )
    err = ccall( (:segy_crossline_stride, libsegyio), Cint,
                 (Cint, Cuint, Ptr{Cuint}),
                 sorting, xline_count, xline_stride )
    if err != 0 error( "Could not figure out xline stride" ) end
    xline_stride = xline_stride[1]

    # finally, find the line numbers themselves
    ilines = Array(Cuint, iline_count)
    err = ccall( (:segy_inline_indices, libsegyio), Cint,
                 (sgy, Cint, Cint, Cuint, Cuint, Cuint, Ptr{Cuint}, Clong, Cuint),
                 fd, il, sorting, iline_count, xline_count, offset_count, ilines, trace0, trace_bsize )
    if err != 0 error( "Could not determine inline numbers" ) end

    xlines  = Array(Cuint, xline_count)
    err = ccall( (:segy_crossline_indices, libsegyio), Cint,
                 (sgy, Cint, Cint, Cuint, Cuint, Cuint, Ptr{Cuint}, Clong, Cuint),
                 fd, xl, sorting, iline_count, xline_count, offset_count, xlines, trace0, trace_bsize )
    if err != 0 error( "Could not determine crossline numbers" ) end

    offset_field = 37
    offsets = Array(Cint, offset_count)
    err = ccall( (:segy_offset_indices, libsegyio), Cint,
                 (sgy, Cint, Cint, Ptr{Cint}, Clong, Cuint),
                 fd, offset_field, offset_count, offsets, trace0, trace_bsize )

    if err != 0 error( "Could not determine offset numbers." ) end

    # all done, now assemble the finished fd object
    meta = Meta( trace0,
                 trace_count,
                 sample_count,
                 trace_bsize,
                 format,
                 sorting,
                 iline_stride,
                 xline_stride )

    SegyFile(fd, ilines, xlines, offsets, meta, path)
end

"""
    fclose(fd)

Manually close an open segy file.

Any operation on the file after this call is undefined and *will* cause your
program to misbehave. This function should not be used when a file is opened
with `open`.
"""
function fclose(fd :: SegyFile)
    if fd.fd == C_NULL return end
    ccall( (:segy_close, libsegyio), Void, (sgy,), fd.fd )
    fd.fd = C_NULL
end

"""
    fflush(fd [, async])

Write changes to disk.

Async is ignored if file is not mmapd, and defaults to false (synchronous
write), which makes the call block until the writing is completed.
"""
function fflush(fd :: SegyFile, async :: Integer = 0 )
    ccall( (:segy_flush, libsegyio), Void, (sgy, Cint), fd.fd, async )
end

"""
    open(f, filename, path [, mode = "r", il = 189, xl = 193])

Open a file and understand basic geometric properties.

Uses standard unix mode for read/write options. You can specify what header
field contains the inline and crossline number with il and xl respectively.
This is the recommended way to interact with Segyio in your programs.

Please prefer this function with do notation for exception safe operations on
your seismic data.

# Examples
```julia
julia> open(filename) do fd
    # fd is the now opened, exception-safe file handle

    println("Segyio - simple & easy")
    display(Segyio.iline(fd)[:])
end
```
"""
function open(f :: Function,
              path :: AbstractString;
              mode :: AbstractString = "rb",
              il   :: Integer = 189,
              xl   :: Integer = 193)
    io = Segyio.fopen(path, mode = mode)
    try
        f(io)
    finally
        Segyio.fclose(io)
    end
end

"""
    mmap(fd)

Memory map the file - returns `true` if the file was memory mapped, and false
if the traditional streaming IO fallback is used. Calling this function is
always safe.

mmap can signifcantly speed up some workloads, but will be slower under some
conditions. Please profile your programs when using mmap.

# Examples
```julia
julia> fd = fopen("small.sgy");
julia> mmap(fd)
true
```
"""
function mmap(fd :: SegyFile)
    ccall( (:segy_mmap, libsegyio), Cint, (sgy,), fd.fd) == 0
end

##############
# TRACE
##############
type Traces <: DenseVector{Cfloat}
    fd :: SegyFile
end

"""
    trace(fd)

Obtain a virtual array whose elements correspond to individual traces in the
file.

Accessing an element in this array will yield an array of the individual
samples for that trace. The traces support normal julia array operations,
ranges, and iteration.

For convenience, accessing a single sample is supported via [n,m].

# Examples
```julia
julia> fd = fopen("small.sgy");
julia> tr = trace(fd)
Traces(length = 25, samples = 50)

julia> tr[1]
50-element Array{Float32,1}:
 1.2
 1.20001
 1.20002
 1.20003
 1.20004
 ⋮
 1.20045
 1.20046
 1.20047
 1.20048
 1.20049

julia> tr[1,2]
 1.20001
```

"""
trace(fd :: SegyFile) = Traces(fd)

function Base.show(io :: IO, tr :: Traces)
    len = length(tr)
    samples = tr.fd.meta.samples
    println(io, "Traces(length = $len, samples = $samples)")
end

# override display so that doing tr = Segyio.trace(fd) won't print the entire
# file it is *VERY* unlikely that you'd want this in a repl, especially with
# larger files, and if you do then you can still do tr[:]
Base.display(tr :: Traces) = show(tr)

# the workhorse C function. does *not* allocate memory nor convert to native
# float.
function readtr!(fd:: SegyFile, i :: Int, buf)
    # subtract 1 from the i in all cases. julia uses 1-indexing, but the
    # underlying C assumes 0-indexing.
    err = ccall( (:segy_readtrace, libsegyio), Cint,
                 (sgy, Cuint, Ref{Cfloat}, Clong, Cuint),
                 fd.fd, i - 1, buf, fd.meta.trace0, fd.meta.bsize )

    if err != 0 error( "Error reading trace $i" ) end
    return buf
end

# allocate a buffer, read a trace and convert to native float
function readtr!(fd :: SegyFile, i :: Int)
    to_native!(fd.meta.fmt, readtr!(fd, i, Array(Cfloat, fd.meta.samples)))
end

##############
# TRACE.array
##############
Base.size(tr :: Traces)             = (tr.fd.meta.traces,)
Base.linearindexing(::Type{Traces}) = Base.LinearFast()
Base.endof(tr :: Traces)            = length(tr)

# tr[i, j] -> returns float
Base.getindex(tr :: Traces, j :: Int, i :: Int) = readtr!(tr.fd, i)[j]
# tr[i] -> returns [float]
Base.getindex(tr :: Traces, i :: Int)           = readtr!(tr.fd, i)
Base.getindex(tr :: Traces, :: Colon)           = tr[1:end]
function Base.getindex(tr :: Traces, rng :: AbstractArray)
    samples = tr.fd.meta.samples
    buf = Array(Cfloat, samples * length(rng))
    (fst, lst) = (1, samples)
    for n in rng
        readtr!(tr.fd, n, sub(buf, fst : lst))
        (fst, lst) = (fst + samples, lst + samples)
    end

    reshape(to_native!(tr.fd.meta.fmt, buf), samples, length(rng))
end

# Traces subtyping DenseArray gives iterator operations, but these seem to not
# reuse buffers, which can be pretty bad for larger files.
type TracesItr
    cur :: Int
    lst :: Int
    buf :: Array{Cfloat}
end

function Base.start(tr :: Traces)
    TracesItr(1, length(tr), Array(Cfloat, tr.fd.meta.samples))
end

Base.done(:: Traces, itr :: TracesItr) = itr.cur > itr.lst

function Base.next(tr :: Traces, itr :: TracesItr)
    buf = to_native!(tr.fd.fmt, readtr!(tr.fd, itr.cur, itr.buf))
    itr.cur += 1
    (buf, itr)
end

Base.length(itr :: TracesItr)       = itr.lst
Base.eltype(:: Type{TracesItr})     = Array{Cfloat}

##############
# LINE
##############
# Inlines and crosslines use the same representation, but obviously the values
# inside the record differ.
type Line <: AbstractArray{Cfloat, 4}
    fd      :: SegyFile
    len     :: Int
    stride  :: Int
    linenos :: Array{Cuint}
    other   :: Array{Cuint}
    name    :: AbstractString
end

function Base.show(io :: IO, ln :: Line)
    println(io, "$(ln.name)(length = $(ln.len))")
end

Base.display(ln :: Line) = show(ln)

function readln!(ln :: Line,
                 line_trace0 :: Integer,
                 buf)

    fd      = ln.fd
    offsets = length(fd.offsets)
    tr0     = fd.meta.trace0
    bsize   = fd.meta.bsize
    len     = ln.len
    stride  = ln.stride
    err = ccall( (:segy_read_line, libsegyio), Cint,
                 (sgy, Cuint, Cuint, Cuint, Cint, Ref{Cfloat}, Clong, Cuint),
                 fd.fd, line_trace0, len, stride, offsets, buf, tr0, bsize )

    if err != 0 error( "Unable to read line." ) end
    return buf
end

"""
    iline(fd)

Obtain a virtual array whose elements correspond to individual inlines in the
file.

The inline array is accessed as if it was a (line x offsets) array. Accessing
an element or a range of elements yields a new array of floats with dimensions
(samples x traces x lines x offsets). If only one offset is requested, the
fourth dimension is dropped. If only a particular line at a particular offset
is requested, the dimensions is (samples x traces)

Lines can be collected in both dimensions:
    - `line[n, m]`
    - `line[n, i:j:k]`
    - `line[n, :]`
    - `line[i:j:k, m]`
    - `line[:, m]`
    - `line[:, :]`
    - `line[:]`

# Examples
```julia
julia> il = Segyio.iline(fd)
Inline(length = 3)

julia> il[:]
5x3x4x2 Array{Float32,4}:
[:, :, 1, 1]
[:, :, 2, 1]
[:, :, 3, 1]
[:, :, 4, 1]
[:, :, 1, 2]
[:, :, 2, 2]
[:, :, 3, 2]
[:, :, 4, 2]

julia> il[2,1]
5x3 Array{Float32,2}:
 102.01  102.02  102.03
 102.01  102.02  102.03
 102.01  102.02  102.03
 102.01  102.02  102.03
 102.01  102.02  102.03

julia> il[2,:]
50x3x1x2 Array{Float32,4}:
[:, :, 1, 1] =
 102.01  102.02  102.03
 102.01  102.02  102.03
 102.01  102.02  102.03
 102.01  102.02  102.03
 102.01  102.02  102.03

[:, :, 1, 2] =
 202.01  202.02  202.03
 202.01  202.02  202.03
 202.01  202.02  202.03
 202.01  202.02  202.03
 202.01  202.02  202.03
"""
function iline(fd :: SegyFile)
    Line(fd,
         length(fd.xlines),
         fd.meta.ilstride,
         fd.ilines,
         fd.xlines,
         "Inline")
end

"""
    xline(fd)

Obtain a virtual array whose elements correspond to individual crosslines in
the file.

The inline array is accessed as if it was a (line x offsets) array. Accessing
an element or a range of elements yields a new array of floats with dimensions
(samples x traces x lines x offsets). If only one offset is requested, the
fourth dimension is dropped. If only a particular line at a particular offset
is requested, the dimensions is (samples x traces)

# Examples
Please refer to the documentation for inlines for examples.
"""
function xline(fd :: SegyFile)
    Line(fd,
         length(fd.ilines),
         fd.meta.xlstride,
         fd.xlines,
         fd.ilines,
         "Crossline")
end

function lntrace0(ln :: Line, i :: Integer, offset :: Integer)
    lnoff = length(ln.fd.offsets)
    if !(1 <= offset <= lnoff)
        error("Illegal offset $offset. Valid range is [1,$lnoff]")
    end

    offset = offset - 1
    line_trace0 = Array(Cuint, 1)
    err = ccall( (:segy_line_trace0, libsegyio), Cint,
                 (Cuint, Cuint, Cuint, Cint, Ref{Cuint}, Cuint, Ptr{Cuint}),
                 i,
                 length(ln.other),
                 ln.stride,
                 length(ln.fd.offsets),
                 ln.linenos,
                 length(ln.linenos),
                 line_trace0)

    if err != 0 error( "Unable to read $(ln.name) $i" ) end
    line_trace0[1] + offset
end

##############
# LINE.array
##############
Base.eltype(::Type{Line})           = Cfloat
Base.size(ln :: Line)               = (ln.len, Int(length(ln.fd.offsets)))
Base.linearindexing(::Type{Line})   = Base.LinearSlow()
Base.endof(ln :: Line)              = Int(ln.linenos[end])

# line[:]
Base.getindex(ln :: Line, :: Colon) = getindex(ln, ln.linenos, ln.fd.offsets)

# line[:,:]
Base.getindex(ln :: Line, :: Colon, :: Colon) = ln[:]

# line[:,1]
function Base.getindex(ln :: Line, :: Colon, offset :: Integer)
    samples = ln.fd.meta.samples
    reshape(getindex(ln, ln.linenos, [1]), samples, ln.len, length(ln.linenos))
end

# without some hacky overriding of trailingsize for lines, line[:, 1:2:end]
# won't work because it'll be unavalable to compute end - it's computed by
# trailingsize # instead of passed through the getindex machinery.
Base.trailingsize(ln :: Line, n :: Integer) = Int(ln.fd.offsets[end])

# line[:,1:2:end]
function Base.getindex(ln :: Line, :: Colon, offsets :: AbstractArray)
    getindex(ln, ln.linenos, offsets)
end

# line[1:2:end,:]
function Base.getindex(ln :: Line, rng :: AbstractArray, :: Colon)
    getindex(ln, rng, ln.fd.offsets)
end

# line[1,:]
function Base.getindex(ln :: Line, n :: Integer, :: Colon)
    getindex(ln, n:n, ln.fd.offsets)
end

# line[1:2:end,1]
function Base.getindex(ln :: Line, rng :: AbstractArray, offset = 1 :: Integer)
    arr = getindex(ln, rng, offset:offset)
    reshape(arr, ln.fd.meta.samples, ln.len, length(rng))
end

# line[1, 1]
function Base.getindex(ln :: Line, n = 1 :: Integer, offset = 1 :: Integer)
    arr = getindex(ln, n:n, offset:offset)
    reshape(arr, ln.fd.meta.samples, ln.len)
end

# line[1:2:end, 1:2:end] or any other range
function Base.getindex(ln :: Line, rng :: AbstractArray, offsets :: AbstractArray)
    size = ln.len * ln.fd.meta.samples
    buf = Array(Cfloat, size * length(rng) * length(offsets))

    (fst, lst) = (1, size)
    for o in offsets
        for n in rng
            ln0 = lntrace0(ln, n, o)
            readln!(ln, ln0, sub(buf, fst : lst))
            (fst, lst) = (fst + size, lst + size)
        end
    end

    buf = to_native!(ln.fd.meta.fmt, buf)
    reshape(buf, ln.fd.meta.samples, ln.len, length(rng), length(offsets))
end

end
