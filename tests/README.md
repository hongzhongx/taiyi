# 关于生产关系的测试说明

单元测试中很多测试同时也在验证太乙网络下的生产关系，即互不相识的人在去中心化开发中怎么组织管理，怎么协作开发，怎么获得奖励。

在生产关系背景下，代码中一些测试账号沿用如下风格：
- 爱丽丝（Alice），喜欢设计
- 鲍勃（Bob），程序员，擅长程序开发，喜欢开发智能游戏脚本（SGS）
- 查理（Charlie），玩家/商家，喜欢创建

# 文档的自动化测试

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
    programs/build_helpers/taiyi_build_helpers/check_reflect.py
