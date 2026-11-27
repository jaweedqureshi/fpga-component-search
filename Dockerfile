FROM ubuntu:24.04 AS toolchain

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        autoconf \
        bison \
        build-essential \
        ca-certificates \
        clang \
        cmake \
        flex \
        g++ \
        git \
        make \
        perl \
        python3 \
        verilator \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

FROM toolchain AS build

COPY Makefile ./
COPY rtl/ ./rtl/
COPY sim/ ./sim/

RUN make build

FROM build AS test

CMD ["make", "test"]
