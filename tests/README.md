# Automated Testing Documentation

    git clone https://github.com/hongzhongx/taiyi.git \
        /usr/local/src/taiyi
    cd /usr/local/src/taiyi
    git checkout <branch> # e.g. 123-feature
    git submodule update --init --recursive
    mkdir build
    cd build
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DBUILD_TAIYI_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        ..
    make -j$(nproc) chain_test
    ./tests/chain_test
    cd /usr/local/src/taiyi
    doxygen
    programs/build_helpers/check_reflect.py
