FROM ubuntu:16.04

# Install
RUN apt-get -qq update
RUN apt-get install -y cmake g++
RUN apt-get install -y libboost-all-dev libopenblas-dev opencl-headers ocl-icd-libopencl1 ocl-icd-opencl-dev zlib1g-dev
RUN apt-get install -y qt5-default qt5-qmake

RUN mkdir -p /src/
WORKDIR /src/

COPY . /src/
RUN CXX=g++ CC=gcc cmake CMakeLists.txt
RUN make -j2
RUN ./tests

WORKDIR /src/autogtp/
RUN qmake
RUN make -j2