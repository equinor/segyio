ARG S390X_BASE_IMAGE=s390x_base
FROM s390x/debian:stable-slim AS s390x_base
RUN apt-get update
RUN apt-get install -y git cmake g++ python3 python3-pip python3-venv python3-numpy

FROM $S390X_BASE_IMAGE AS tester
WORKDIR /
COPY . /segyio
WORKDIR /segyio

RUN python3 -m venv pyvenv --system-site-packages
RUN /segyio/pyvenv/bin/python -m pip install --upgrade pip
RUN /segyio/pyvenv/bin/python -m pip install -r python/requirements-dev.txt

WORKDIR /segyio/build
RUN cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Debug -DPython_ROOT_DIR="/segyio/pyvenv/" ..
RUN make -j4
RUN ctest --verbose
