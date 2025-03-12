#!/bin/bash

echo /tmp/core | tee /proc/sys/kernel/core_pattern
ulimit -c unlimited

get_version() {
  local TAIYIN="$1"
  # 执行命令并提取版本号
  local VERSION_OUTPUT="$($TAIYIN --version 2>/dev/null)"
  # 使用grep和awk提取版本号
  echo "$VERSION_OUTPUT" | grep "taiyi_blockchain_version:" | awk '{print $2}'
}

get_exec_path() {
  local TAIYIN="/usr/local/taiyi-default/bin/taiyin"
  if [[ "$IS_TESTNET" ]]; then
    TAIYIN="/usr/local/taiyi-testnet/bin/taiyin"
  elif [[ "$USE_WAY_TOO_MUCH_RAM" ]]; then
    TAIYIN="/usr/local/taiyi-full/bin/taiyin"
  fi
  echo "$TAIYIN"
}

build_args() {
  local args=""

  # if user did pass in desired seed nodes, use
  # the ones the user specified:
  if [[ ! -z "$TAIYI_SEED_NODES" ]]; then
    for NODE in $TAIYI_SEED_NODES; do
      args+=" --p2p-seed-node=$NODE"
    done
  fi

  if [[ ! -z "$TAIYI_SIMING_NAME" ]]; then
    args+=" --siming=\"$TAIYI_SIMING_NAME\""
  fi

  if [[ ! -z "$TAIYI_PRIVATE_KEY" ]]; then
    args+=" --private-key=$TAIYI_PRIVATE_KEY"
  fi

  if [[ "$STALE_PRODUCTION" ]]; then
    args+=" --enable-stale-production"
  fi

  if [[ ! -z "$REQUIRED_PARTICIPATION" ]]; then
    args+=" --required-participation=$REQUIRED_PARTICIPATION"
  fi

  if [[ ! -z "$TRACK_ACCOUNT" ]]; then
    if [[ ! "$USE_WAY_TOO_MUCH_RAM" ]]; then
      args+=" --plugin=account_history --plugin=account_history_api"
    fi
    args+=" --account-history-track-account-range=[\"$TRACK_ACCOUNT\",\"$TRACK_ACCOUNT\"]"
  fi

  echo "$args"
}

copy_config() {
  echo "Copying taiyi config file..."
  local config_file="/etc/taiyi/config.ini"

  if [[ "$USE_FULL_WEB_NODE" ]]; then
    config_file="/etc/taiyi/fullnode.config.ini"
  elif [[ "$IS_BROADCAST_NODE" ]]; then
    config_file="/etc/taiyi/config-for-broadcaster.ini"
  elif [[ "$IS_AH_NODE" ]]; then
    config_file="/etc/taiyi/config-for-ahnode.ini"
  elif [[ "$IS_OPSWHITELIST_NODE" ]]; then
    config_file="/etc/taiyi/fullnode.opswhitelist.config.ini"
  fi

  cp "$config_file" "$HOME/config.ini"
  echo "Taiyi config file copied."
}

setup_nginx() {

  echo "Setting up nginx frontend..."

  mv /etc/nginx/nginx.conf /etc/nginx/nginx.original.conf
  cp /etc/nginx/taiyi.nginx.conf /etc/nginx/nginx.conf

  cp /etc/nginx/healthcheck.conf.template /etc/nginx/healthcheck.conf
  echo server 127.0.0.1:8091\; >>/etc/nginx/healthcheck.conf
  echo } >>/etc/nginx/healthcheck.conf

  rm /etc/nginx/sites-enabled/default
  cp /etc/nginx/healthcheck.conf /etc/nginx/sites-enabled/default
  /etc/init.d/fcgiwrap restart
  service nginx restart

  echo "Nginx frontend setup complete."
}

run_taiyin() {
  local ARGS=$(build_args)
  local TAIYIN=$(get_exec_path)
  local VERSION=$(get_version $TAIYIN)

  copy_config

  if [[ ! -d $HOME/blockchain ]]; then
    if [[ -e /var/cache/taiyi/blocks.tbz2 ]]; then
      # init with blockchain cached in image
      ARGS+=" --replay-blockchain"
      mkdir -p $HOME/blockchain
      cd $HOME/blockchain
      tar xvjpf /var/cache/taiyi/blocks.tbz2
      chown -R taiyi:taiyi $HOME/blockchain
    fi
  fi

  cd $HOME

  if [[ "$USE_PUBLIC_SHARED_MEMORY" ]]; then
    echo taiyi: Downloading and uncompressing blockchain-$VERSION-latest.tar.lz4 - this may take a while.
    wget -qO- https://s3.amazonaws.com/taiyi-dev-blockchainstate/blockchain-$VERSION-latest.tar.lz4 | lz4 -d | tar x
  fi

  if [[ "$USE_PUBLIC_BLOCKLOG" ]]; then
    if [[ ! -e $HOME/blockchain/block_log ]]; then
      if [[ ! -d $HOME/blockchain ]]; then
        mkdir -p $HOME/blockchain
      fi
      echo "taiyi: Downloading a block_log and replaying the blockchain"
      echo "This may take a little while..."
      wget -O $HOME/blockchain/block_log https://s3.amazonaws.com/taiyi-dev-blockchainstate/block_log-latest
      ARGS+=" --replay-blockchain"
    fi
  fi

  sleep 1

  chown -R taiyi:taiyi $HOME

  chown taiyi:taiyi "$HOME/config.ini"

  echo "Attempting to execute v$VERSION taiyin using additional command line arguments: $ARGS"

  if [[ "$USE_NGINX_FRONTEND" ]]; then
    setup_nginx
    exec su -c "$TAIYIN \
      --webserver-ws-endpoint=0.0.0.0:8091 \
      --webserver-http-endpoint=0.0.0.0:8091 \
      --p2p-endpoint=0.0.0.0:2025 \
      --data-dir=$HOME \
      $ARGS \
      $TAIYI_EXTRA_OPTS" taiyi
  else
    exec su -c "$TAIYIN \
      --webserver-ws-endpoint=0.0.0.0:8090 \
      --webserver-http-endpoint=0.0.0.0:8090 \
      --p2p-endpoint=0.0.0.0:2025 \
      --data-dir=$HOME \
      $ARGS \
      $TAIYI_EXTRA_OPTS" taiyi
  fi

}

run_taiyin
