FROM zuowangdaox/taiyi-base:latest AS final

# https://askubuntu.com/questions/909277/avoiding-user-interaction-with-tzdata-when-installing-certbot-in-a-docker-contai
ARG DEBIAN_FRONTEND=noninteractive

#ARG TAIYI_BLOCKCHAIN=https://example.com/taiyi-blockchain.tbz2

WORKDIR /var/taiyi

VOLUME [ "/var/taiyi" ]

RUN apt-get update && \
    apt-get install -y \
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
        libzstd-dev \
        runit \
    && \
    apt-get autoremove -y && \
    rm -rf /var/lib/apt/lists

RUN \
    ( /usr/local/taiyi-full/bin/taiyin --version | grep -o '[0-9]*\.[0-9]*\.[0-9]*' \
    && echo '_' \
    && git rev-parse --short HEAD ) \
    | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n//g' \
    > /etc/taiyinversion && \
    cat /etc/taiyinversion

RUN useradd -s /bin/bash -m -d /var/taiyi taiyi

RUN mkdir /var/cache/taiyi && \
    chown taiyi:taiyi -R /var/cache/taiyi
# add blockchain cache to image
#ADD $TAIYI_BLOCKCHAIN /var/cache/taiyi/blocks.tbz2

ENV HOME=/var/taiyi
RUN chown taiyi:taiyi -R /var/taiyi

# rpc service:
EXPOSE 8090
# p2p service:
EXPOSE 2025

# add seednodes from documentation to image
ADD doc/seednodes.txt /etc/taiyi/seednodes.txt

# the following adds lots of logging info to stdout
ADD contrib/config-for-docker.ini /etc/taiyi/config.ini
ADD contrib/fullnode.config.ini /etc/taiyi/fullnode.config.ini
ADD contrib/fullnode.opswhitelist.config.ini /etc/taiyi/fullnode.opswhitelist.config.ini
ADD contrib/config-for-broadcaster.ini /etc/taiyi/config-for-broadcaster.ini
ADD contrib/config-for-ahnode.ini /etc/taiyi/config-for-ahnode.ini

# add normal startup script that starts via sv
ADD contrib/taiyin.run /usr/local/bin/taiyi-sv-run.sh
RUN chmod +x /usr/local/bin/taiyi-sv-run.sh

# add nginx templates
ADD contrib/taiyi.nginx.conf /etc/nginx/taiyi.nginx.conf
ADD contrib/healthcheck.conf.template /etc/nginx/healthcheck.conf.template

# add PaaS startup script and service script
ADD contrib/startpaastaiyi.sh /usr/local/bin/startpaastaiyi.sh
ADD contrib/paas-sv-run.sh /usr/local/bin/paas-sv-run.sh
ADD contrib/sync-sv-run.sh /usr/local/bin/sync-sv-run.sh
ADD contrib/healthcheck.sh /usr/local/bin/healthcheck.sh
RUN chmod +x /usr/local/bin/startpaastaiyi.sh
RUN chmod +x /usr/local/bin/paas-sv-run.sh
RUN chmod +x /usr/local/bin/sync-sv-run.sh
RUN chmod +x /usr/local/bin/healthcheck.sh

# new entrypoint for all instances
# this enables exitting of the container when the writer node dies
# for PaaS mode (elasticbeanstalk, etc)
# AWS EB Docker requires a non-daemonized entrypoint
ADD contrib/taiyientrypoint.sh /usr/local/bin/taiyientrypoint.sh
RUN chmod +x /usr/local/bin/taiyientrypoint.sh
CMD /usr/local/bin/taiyientrypoint.sh
