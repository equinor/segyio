# Test data

Test files names are already self-telling, but this overview aims to provide more context.

Due to changes in the scripts after the files were originally created, reproducing might not give 100% same result, but similar enough.

## General test files

| File                           | Purpose                                                                     | Recreation                                            | Comment                                    |
|--------------------------------|-----------------------------------------------------------------------------|-------------------------------------------------------|--------------------------------------------|
| 1x1.sgy                        | File with 1 inline, 1 xline and 4 offsets - 4 traces of 10 samples total    | make-ps-file.py 1x1.sgy 10 0 1 0 1 0 4                |                                            |
| 1xN.sgy                        | File with 1 inline, 6 xlines and 4 offsets - 24 traces of 10 samples total  | make-ps-file.py 1xN.sgy 10 0 1 0 6 0 4                |                                            |
| Mx1.sgy                        | File with 6 inlines, 1 xline and 4 offsets - 24 traces of 10 samples total  | make-ps-file.py Mx1.sgy 10 0 1 0 1 0 4                |                                            |
| delay-scalar.sgy               | Recording time delay is 10000 and corresponding scalar is -10.              | likely real file, cropped                             | ASCII text header                          |
| f3.sgy                         | File in format number 3 (2-byte, two's complement integers).                | cropped real file                                     | see text header                            |
| f3-lsb.sgy                     | Same as f3.sgy, but little-endian.                                          | ./flip-endianness --samples 75 -F 2 f3.sgy f3-lsb.sgy |                                            |
| interval-neg-bin-neg-trace.sgy | Sample interval in binary header is -2000 and same in trace is -25000.      |                                                       | 1 sample, 1 trace                          |
| interval-neg-bin-pos-trace.sgy | Sample interval in binary header is -2 and same in trace is 4000.           |                                                       | 1 sample, 1 trace                          |
| interval-pos-bin-neg-trace.sgy | Sample interval in binary header is 2000 and same in trace is -25000.       |                                                       | 1 sample, 1 trace                          |
| long.sgy                       | Declares 60000 samples (value is written in 16-bit field in binary header). |                                                       | broken as proper trace headers are missing |
| multi-text.sgy                 | Contains 4 extended text headers                                            | python make-multiple-text.py multi-text.sgy           | 1 sample, 1 trace                          |
| shot-gather.sgy                | Some traces have common field record [2 3 5 8]/geophone group [1 2].        | make-shot-gather.py shot-gather.sgy                   | First trace value is field record value.   |
| small-lsb.sgy                  | small.sgy converted to little endian                                        | ./flip-endianness --samples 50 small.sgy small-ps.sgy |                                            |
| small-ps.sgy                   | Pre-stack file with 2 offsets.                                              | python make-ps-file.py small-ps.sgy  10 1 5 1 4 1 3   |                                            |
| small.sgy                      | Inline-sorted file with 50 samples, 5 inlines-xlines test file.             | make-file.py small.sgy 50 1 6 20 25                   | Basic file used in testing                 |
| text-embed-null.sgy            | small.sgy but with one byte in text header exchanged with 00                |                                                       |                                            |
| text.sgy                       | File with unique textual file header.                                       |                                                       | no traces                                  |
| 小文件.sgy                      | small.sgy but with utf-8 characters in the filename                         | rename small.sgy                                      | bug test for Windows OS                    |


## Rotation test files

Shows how survey coordinate system relates to cdp. Described angles are relative to starting position, clock-wise.

To recreate:
`python python/examples/make-rotated-copies.py test-data/small.sgy small.sgy test-data`


| File                | Purpose                                                               |
|---------------------|-----------------------------------------------------------------------|
| normal-small.sgy    | 0° Survey coordinate system is aligned to cdpx/cdpy coordinate system |
| acute-small.sgy     | 45°                                                                   |
| right-small.sgy     | 90°                                                                   |
| obtuse-small.sgy    | 135°                                                                  |
| straight-small.sgy  | 180°                                                                  |
| reflex-small.sgy    | 225°                                                                  |
| left-small.sgy      | 270°                                                                  |
| inv-acute-small.sgy | 315°                                                                  |


## Dimensions sorting test files

Could be reproduced with
`python python/examples/sorting-permutation.py test-data/small-ps.sgy`

Dimensions are mentioned in the order from slowest to fastest changing.

| File                           | Purpose                                                     |
|--------------------------------|-------------------------------------------------------------|
| small-ps-dec-il-inc-xl-off.sgy | Inlines: decreasing. Xlines: increasing. Offset: increasing |
| small-ps-dec-il-off-inc-xl.sgy | Inlines: decreasing. Xlines: increasing. Offset: decreasing |
| small-ps-dec-il-xl-inc-off.sgy | Inlines: decreasing. Xlines: decreasing. Offset: increasing |
| small-ps-dec-il-xl-off.sgy     | Inlines: decreasing. Xlines: decreasing. Offset: decreasing |
| small-ps-dec-off-inc-il-xl.sgy | Inlines: increasing. Xlines: increasing. Offset: decreasing |
| small-ps-dec-xl-inc-il-off.sgy | Inlines: increasing. Xlines: decreasing. Offset: increasing |
| small-ps-dec-xl-off-inc-il.sgy | Inlines: increasing. Xlines: decreasing. Offset: decreasing |


## Seismic Un*x format

| File         | Purpose                                                                 | Recreation                                                                              |
|--------------|-------------------------------------------------------------------------|-----------------------------------------------------------------------------------------|
| small-lsb.su | small.sgy converted to Seismic Un*x format and swapped to little endian | suswapbytes < small.su > small-lsb.su                                                   |
| small.su     | small.sgy converted to Seismic Un*x format                              | segyread tape=small.sgy ns=50 remap=tracr,cdp byte=189l,193l conv=1 format=1 > small.su |


## Multiformat

Files in the `multiformat` directory.

`f3.sgy` converted to multiple formats, both big/little endian. Creations methods vary/unknown.
