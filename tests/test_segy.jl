using Segyio

if VERSION >= v"0.5.0-dev+7720"
    using Base.Test
else
    using BaseTestNext
    const Test = BaseTestNext
end

filename = "test-data/small.sgy"
prestack = "test-data/small-ps.sgy"

@testset "geometry" begin
    Segyio.open(filename) do fd
        @test fd.ilines  == [1,2,3,4,5]
        @test fd.xlines  == [20,21,22,23,24]
        @test fd.offsets == [1]

    end

    Segyio.open(prestack) do fd
        @test fd.offsets == [1,2]
    end
end

@testset "offset traces" begin
    Segyio.open(prestack) do fd
        tr = Segyio.trace(fd)
        @test isapprox(101.01, tr[1,1], atol = 1e-4)
        @test isapprox(201.01, tr[1,2], atol = 1e-4)
        @test isapprox(101.02, tr[1,3], atol = 1e-4)
        @test isapprox(201.02, tr[1,4], atol = 1e-4)
        @test isapprox(102.01, tr[1,7], atol = 1e-4)
    end
end

@testset "trace slicing" begin
    Segyio.open(filename) do fd
        tr = Segyio.trace(fd)
        trace = tr[:]
        slice = tr[1:2:6]
        @test slice[end, 1] == trace[end, 1]
        @test slice[end, 2] == trace[end, 3]
        @test slice[end, 3] == trace[end, 5]

        rslice = tr[5:-2:1]
        @test rslice[end, 1] == trace[end, 5]
        @test rslice[end, 2] == trace[end, 3]
        @test rslice[end, 3] == trace[end, 1]
    end
end

@testset "inline 4" begin
    Segyio.open(filename) do fd
        line = Segyio.iline(fd)[4]

        fst = 1
        mid = fd.meta.samples ÷ 2
        @test isapprox(4.2,     line[fst, 1], atol = 1e-5)
        @test isapprox(4.20024, line[mid, 1], atol = 1e-5)
        @test isapprox(4.20049, line[end, 1], atol = 1e-5)

        @test isapprox(4.22,    line[fst, 3], atol = 1e-5)
        @test isapprox(4.22024, line[mid, 3], atol = 1e-5)
        @test isapprox(4.22049, line[end, 3], atol = 1e-5)

        @test isapprox(4.24,    line[fst, 5], atol = 1e-5)
        @test isapprox(4.24024, line[mid, 5], atol = 1e-5)
        @test isapprox(4.24049, line[end, 5], atol = 1e-5)
    end
end

@testset "crossline 22" begin
    Segyio.open(filename) do fd
        line = Segyio.xline(fd)[22]

        fst = 1
        mid = fd.meta.samples ÷ 2
        @test isapprox(1.22,    line[fst, 1], atol = 1e-5)
        @test isapprox(1.22024, line[mid, 1], atol = 1e-5)
        @test isapprox(1.22049, line[end, 1], atol = 1e-5)

        @test isapprox(3.22,    line[fst, 3], atol = 1e-5)
        @test isapprox(3.22024, line[mid, 3], atol = 1e-5)
        @test isapprox(3.22049, line[end, 3], atol = 1e-5)

        @test isapprox(5.22,    line[fst, 5], atol = 1e-5)
        @test isapprox(5.22024, line[mid, 5], atol = 1e-5)
        @test isapprox(5.22049, line[end, 5], atol = 1e-5)
    end
end

@testset "inline slice" begin
    Segyio.open(filename) do fd
        il = Segyio.iline(fd)
        @test length(fd.ilines) == size(il)[1]
        @test length(fd.ilines) == length(il[1:5][1,1,:])
        @test length(fd.ilines) == length(il[:][1,1,:])
        @test length(fd.ilines) == length(il[1:end][1,1,:])
        @test 3                 == length(il[1:2:end][1,1,:])
        @test 3                 == length(il[1:3][1,1,:])
        @test 2                 == length(il[3:2:end][1,1,:])
    end
end

@testset "inline offset" begin
    Segyio.open(prestack) do fd
        il  = Segyio.iline(fd)
        ln1 = il[1,1]
        @test isapprox(101.01000, ln1[1,1], atol = 1e-5)
        @test isapprox(101.02000, ln1[1,2], atol = 1e-5)
        @test isapprox(101.03000, ln1[1,3], atol = 1e-5)

        @test isapprox(101.01001, ln1[2,1], atol = 1e-5)
        @test isapprox(101.01002, ln1[3,1], atol = 1e-5)
        @test isapprox(101.02001, ln1[2,2], atol = 1e-5)

        ln2 = il[1,2]
        @test isapprox(201.01000, ln2[1,1], atol = 1e-5)
        @test isapprox(201.02000, ln2[1,2], atol = 1e-5)
        @test isapprox(201.03000, ln2[1,3], atol = 1e-5)

        @test isapprox(201.01001, ln2[2,1], atol = 1e-5)
        @test isapprox(201.01002, ln2[3,1], atol = 1e-5)
        @test isapprox(201.02001, ln2[2,2], atol = 1e-5)

        @test_throws ErrorException il[1, 0]
        @test_throws ErrorException il[1, 3]
        @test_throws ErrorException il[100, 1]
        @test_throws MethodError    il[100, "foo"]
    end
end

@testset "fixed offset" begin
    Segyio.open(prestack) do fd
        ln = Segyio.iline(fd)[:,1]
        for i in fd.ilines
            @test isapprox(i + 100.01000, ln[1,1,i], atol = 1e-5)
            @test isapprox(i + 100.02000, ln[1,2,i], atol = 1e-5)
            @test isapprox(i + 100.03000, ln[1,3,i], atol = 1e-5)

            @test isapprox(i + 100.01001, ln[2,1,i], atol = 1e-5)
            @test isapprox(i + 100.01002, ln[3,1,i], atol = 1e-5)
            @test isapprox(i + 100.02001, ln[2,2,i], atol = 1e-5)
        end
    end
end

@testset "fixed line" begin
    Segyio.open(prestack) do fd
        ln = Segyio.iline(fd)[1,:]
        for i in fd.offsets
            off = i * 100
            @test isapprox(off + 1.01000, ln[1,1,1,i], atol = 1e-5)
            @test isapprox(off + 1.02000, ln[1,2,1,i], atol = 1e-5)
            @test isapprox(off + 1.03000, ln[1,3,1,i], atol = 1e-5)

            @test isapprox(off + 1.01001, ln[2,1,1,i], atol = 1e-5)
            @test isapprox(off + 1.01002, ln[3,1,1,i], atol = 1e-5)
            @test isapprox(off + 1.02001, ln[2,2,1,i], atol = 1e-5)
        end
    end
end

@testset "slice offsets" begin
    Segyio.open(prestack) do fd
        offs = length(fd.offsets)
        ils  = length(fd.ilines)
        il = Segyio.iline(fd)

        # iline[:,:] essentially gathers the entire file
        @test offs * ils        == sum([1 for _ in il[:][1,1,:,:]])
        @test offs * ils        == sum([1 for _ in il[:,:][1,1,:,:]])
        @test 240               == length(il[:,:])
        @test (offs * (ils÷2))  == sum([1 for _ in il[1:2:end,:][1,1,:,:]])
        @test ((offs÷2) * ils)  == sum([1 for _ in il[:,1:2:end][1,1,:,:]])
        @test (offs÷2)*(ils÷2)  == sum([1 for _ in il[1:2:end,1:2:end][1,1,:]])
   end
end

@testset "fopen error" begin
    @test_throws SystemError    Segyio.fopen("nodir/nofile")
    @test_throws SystemError    Segyio.open(_ -> 1, "nodir/nofile")
    @test_throws ArgumentError  Segyio.open(_ -> 1, filename, mode = "foo")
end
