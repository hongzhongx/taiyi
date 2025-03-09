FROM zuowangdaox/taiyi-base:latest

WORKDIR /var/taiyi
VOLUME [ "/var/taiyi" ]

RUN apt-get update

RUN apt-get install -y \
    libsnappy-dev \
    libreadline-dev \
    nginx \
    fcgiwrap \
    jq \
    wget \
    virtualenv \
    gdb \
    libgflags-dev \
    zlib1g-dev \
    libbz2-dev \
    liblz4-dev \
    libzstd-dev 

RUN apt-get autoremove -y && \
    rm -rf /var/lib/apt/lists

RUN \
    ( /usr/local/taiyi-full/bin/taiyin --version | grep -o '[0-9]*\.[0-9]*\.[0-9]*' \
    && echo '_' \
    && git rev-parse --short HEAD ) \
    | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n//g' \
    > /etc/taiyinversion 

RUN cat /etc/taiyinversion

# 创建 taiyi 用户
RUN useradd -s /bin/bash -m -d /var/taiyi taiyi

ENV HOME=/var/taiyi
RUN chown taiyi:taiyi -R /var/taiyi

# 暴露 RPC 服务和 P2P 服务端口
EXPOSE 8090 2025

# NGINX 模板文件
ADD contrib/taiyi.nginx.conf /etc/nginx/taiyi.nginx.conf
ADD contrib/healthcheck.conf.template /etc/nginx/healthcheck.conf.template

# 配置文件
ADD contrib/config-for-docker.ini /etc/taiyi/config.ini
ADD contrib/fullnode.config.ini /etc/taiyi/fullnode.config.ini
ADD contrib/fullnode.opswhitelist.config.ini /etc/taiyi/fullnode.opswhitelist.config.ini
ADD contrib/config-for-broadcaster.ini /etc/taiyi/config-for-broadcaster.ini
ADD contrib/config-for-ahnode.ini /etc/taiyi/config-for-ahnode.ini

# 启动脚本
ADD contrib/taiyientrypoint.sh /etc/taiyi/taiyi-entrypoint.sh
RUN chmod +x /etc/taiyi/taiyi-entrypoint.sh

ENTRYPOINT [ "/etc/taiyi/taiyi-entrypoint.sh" ]
