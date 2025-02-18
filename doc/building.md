# 构建太乙系统

构建编译太乙系统需要8GB内存机器。

## 编译选项 (cmake)

### CMAKE_BUILD_TYPE=[Release/Debug]

选择是否在编译时执行符号优化或者加入调试信息。除了在运行单元测试项目时要编译调试（Debug）模式，一般建议编译成发行版本（Release）。

### LOW_MEMORY_NODE=[OFF/ON]

这个开关打开来编译的太阴程序，可用于支撑共识机制的最小内存开销节点，这种节点的对象数据库中不会存储共识不需要的数据和字段。一般对于司命节点或者种子节点，建议用这个开关编译。

### BUILD_TAIYI_TESTNET=[OFF/ON]

为测试网络编译太乙系统，这也用于构建单元测试。

## Docker中编译

我们提供了系列Docker文件（Dockerfile），能构建各种常用节点的二进制文件。

1. 首先是构建包含适配测试网络、低内存节点和高内存节点二进制文件的基础镜像（zuowangdaox/taiyi-base）

    git clone https://github.com/hongzhongx/taiyi
    cd taiyi
    docker build -t zuowangdaox/taiyi-base -f Dockerfile_base .  

注意，如果你是最新的Apple Silicon芯片的Macos，可以运行模拟的amd64编译环境：

    docker buildx build --platform linux/amd64 -t zuowangdaox/taiyi-base -f Dockerfile_base .  

基础镜像中仅仅包含了常用节点各种工具的二进制程序文件，实验人员也可以从镜像中提取二进制文件到对应的Ubuntu环境中直接使用。

2. 构建能以不同配置工作的节点镜像（zuowangdaox/taiyi）

工作镜像可以用于直接在容器中启动节点和各种工具。

    git clone https://github.com/hongzhongx/taiyi
    cd taiyi
    docker build -t zuowangdaox/taiyi .  

注意，如果你是最新的Apple Silicon芯片的Macos，可以运行模拟的amd64编译环境：

    docker buildx build --platform linux/amd64 -t zuowangdaox/taiyi .  

实际上，这个镜像是基于基础镜像的，仅仅是写入各种工作环境的配置和脚本，因此镜像的构建速度会比较快。

## 在 Ubuntu 22.04 操作系统上编译

注意，首先要通过`apt-get`管理器安装各种环境工具和库。

    # Required packages
    sudo apt-get update
    sudo apt-get install -y \
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

    # Actual build
    git clone https://github.com/hongzhongx/taiyi
    cd taiyi
    git submodule update --init --recursive
    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc) taiyin xuanpin
    # optional
    make install  # defaults to /usr/local

## 在 macOS X 操作系统上编译

参考这个链接安装Xcode和它的命令行工具：https://guide.macports.org/#installing.xcode.  在OS X 10.11 (El Capitan)之后的系统中，当你在命令行下启动开发者工具时，你会收到一个安装开发者工具包的提示（需要接受Xcode的许可证（license））：

    sudo xcodebuild -license accept

接下来安装Homebrew，相关安装命令参考：http://brew.sh/

### 初始化 Homebrew：

    brew doctor
    brew update

### 安装太乙系统相关库和开发工具：

    brew install \
        autoconf \
        automake \
        cmake \
        git \
        boost@1.76 \
        libtool \
        openssl \
        snappy \
        zlib \
        bzip2 \
        python3
        
    pip3 install --user jinja2
    
*Optional.* To use TCMalloc in LevelDB:

    brew install google-perftools

### 克隆太乙代码库

    git clone https://github.com/hongzhongx/taiyi
    cd taiyi

### 开始编译

    export BOOST_ROOT=$(brew --prefix)/Cellar/boost@1.76/1.76.0_5/
    export OPENSSL_ROOT_DIR=$(brew --prefix)/Cellar/openssl/1.0.2n/
    export SNAPPY_ROOT_DIR=$(brew --prefix)/Cellar/snappy/1.1.7_1
    export ZLIB_ROOT_DIR=$(brew --prefix)/Cellar/zlib/1.2.11
    export BZIP2_ROOT_DIR=$(brew --prefix)/Cellar/bzip2/1.0.8
    git checkout main
    git submodule update --init --recursive
    mkdir build && cd build
    cmake -DBOOST_ROOT="$BOOST_ROOT" -DCMAKE_BUILD_TYPE=Release ..
    make -j$(sysctl -n hw.logicalcpu)

注意，通常用`make`编译出这几个要用的编译目标：

    taiyin
    chain_test
    xuanpin

例如：

    make -j$(sysctl -n hw.logicalcpu) taiyin

执行这个命令后只会编译出`taiyin`来。

## 其他操作系统

- 暂时未能提供Windows的编译方式
- 开发者通常要使用gcc和clang来编译，操作系统对这些编译器一般都有很好的支持。
- 看到社区里面有些人偶尔要用mingw、英特尔或者微软的编译器来编译代码，虽然这些编译可能成功，但是开发者通常不使用它们。
- 当然，大家可以把针对其他编译器下遇到错误或者警告的修改代码推送过来。
