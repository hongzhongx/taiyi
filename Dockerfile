FROM ubuntu:22.04 AS builder

# https://askubuntu.com/questions/909277/avoiding-user-interaction-with-tzdata-when-installing-certbot-in-a-docker-contai
ARG DEBIAN_FRONTEND=noninteractive

# The default build params are for low memory version.
# This usually are used as a siming node.
ARG CMAKE_BUILD_TYPE=Release
ARG BUILD_TAG=main
ARG TAIYI_STATIC_BUILD=ON
ARG LOW_MEMORY_MODE=ON
ARG BUILD_TAIYI_TESTNET=OFF
ARG ENABLE_COVERAGE_TESTING=OFF
ARG CHAINBASE_CHECK_LOCKING=OFF
ARG UNIT_TEST=OFF
ARG DOXYGEN=OFF

RUN apt-get update && \
    apt-get install -y \
        git \
        build-essential \
        libboost-all-dev \
        cmake \
        libssl-dev \
        libsnappy-dev \
        python3-jinja2 \
        doxygen \
        autoconf \
        automake \
        autotools-dev \
        bsdmainutils \
        libyajl-dev \
        libreadline-dev \
        libssl-dev \
        libtool \
        liblz4-tool \
        ncurses-dev \
        libgflags-dev \
        zlib1g-dev \
        libbz2-dev \
        liblz4-dev \
        libzstd-dev

WORKDIR /usr/local/src
ADD . /usr/local/src

RUN pwd && \
    git fetch origin ${BUILD_TAG} && \
    git checkout ${BUILD_TAG} && \
    git submodule update --init --recursive

RUN mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/taiyin \
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
        -DLOW_MEMORY_NODE=${LOW_MEMORY_MODE} \
        -DBUILD_TAIYI_TESTNET=${BUILD_TAIYI_TESTNET} \
        -DENABLE_COVERAGE_TESTING=${ENABLE_COVERAGE_TESTING} \
        -DCHAINBASE_CHECK_LOCKING=${CHAINBASE_CHECK_LOCKING} \
        -DTAIYI_STATIC_BUILD=${TAIYI_STATIC_BUILD} \
        .. && \
    make -j$(nproc) && \
    make install

RUN if [ "${UNIT_TEST}" = "ON" ] ; then \
        cd tests && \
        ctest -j$(nproc) --output-on-failure && \
        ./chain_test -t basic_tests/adjust_balance_test && \
        cd .. && \
        ./programs/util/test_fixed_string; \
    fi

RUN if [ "${DOXYGEN}" = "ON" ] ; then \
        doxygen && \
        PYTHONPATH=programs/build_helpers \
        python3 -m taiyi_build_helpers.check_reflect && \
        programs/build_helpers/get_config_check.sh; \
    fi

FROM ubuntu:22.04 AS final

ARG CMAKE_BUILD_TYPE=Release
ARG BUILD_TAG=main
ARG TAIYI_STATIC_BUILD=ON
ARG LOW_MEMORY_MODE=ON
ARG BUILD_TAIYI_TESTNET=OFF
ARG ENABLE_COVERAGE_TESTING=OFF
ARG CHAINBASE_CHECK_LOCKING=OFF
ARG DOXYGEN=OFF

RUN echo "BUILD_TAG: ${BUILD_TAG}" >> /etc/build_info&& \
    echo "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}" >> /etc/build_info && \
    echo "TAIYI_STATIC_BUILD: ${TAIYI_STATIC_BUILD}" >> /etc/build_info && \
    echo "LOW_MEMORY_MODE: ${LOW_MEMORY_MODE}" >> /etc/build_info && \
    echo "BUILD_TAIYI_TESTNET: ${BUILD_TAIYI_TESTNET}" >> /etc/build_info && \
    echo "ENABLE_COVERAGE_TESTING: ${ENABLE_COVERAGE_TESTING}" >> /etc/build_info && \
    echo "DOXYGEN: ${DOXYGEN}" >> /etc/build_info

COPY --from=builder /usr/local/taiyin /usr/local/taiyin
WORKDIR /var/taiyi
VOLUME [ "/var/taiyi" ]
RUN apt-get update && \
    apt-get install -y libsnappy-dev libreadline-dev && \
    apt-get autoremove -y && \
    rm -rf /var/lib/apt/lists

CMD ["cat", "/etc/build_info"]
