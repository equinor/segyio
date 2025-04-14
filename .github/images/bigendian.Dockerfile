FROM s390x/debian:stable-slim
RUN apt-get update
RUN apt-get install -y git cmake g++ python3 python3-pip python3-venv python3-numpy

WORKDIR /
COPY . /segyio
WORKDIR /segyio

RUN python3 -m venv pyvenv --system-site-packages
RUN /segyio/pyvenv/bin/python -m pip install --upgrade pip
RUN /segyio/pyvenv/bin/python -m pip install -r python/requirements-dev.txt

WORKDIR /segyio/build
RUN cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Debug -DPYTHON_EXECUTABLE="/segyio/pyvenv/bin/python" ..
RUN make -j4
RUN ctest --verbose
