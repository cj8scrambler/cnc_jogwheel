FROM ubuntu:latest
ARG PICO_SDK_VER=2.2.0

RUN apt-get update && \
    apt-get install -y build-essential cmake git \
        python3 \
        gcc-arm-none-eabi libnewlib-arm-none-eabi \
        libstdc++-arm-none-eabi-newlib && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /buildtools
RUN git clone --recurse-submodules --branch ${PICO_SDK_VER} https://github.com/raspberrypi/pico-sdk.git
ENV PICO_SDK_PATH=/buildtools/pico-sdk

RUN git clone https://github.com/raspberrypi/picotool.git && \
    cd picotool && \
    cmake -S . -B build  && \
    cmake --build build --target install && \
    cd - && rm -rf picotool

#WORKDIR /app
CMD ["/bin/bash"]
