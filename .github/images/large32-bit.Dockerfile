FROM i386/python:3-slim
RUN apt-get update
RUN apt-get install -y git cmake g++ libopenblas-dev

WORKDIR /
COPY . /segyio
WORKDIR /segyio

RUN pip install --upgrade pip
RUN pip install -r python/requirements-dev.txt

WORKDIR /segyio/build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_BIN=OFF -DBUILD_TESTING=OFF -DBUILD_PYTHON=ON ..
RUN make -j4

WORKDIR /segyio/python
RUN pytest test/large.py -rPV
