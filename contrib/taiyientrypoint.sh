#!/bin/bash

echo /tmp/core | tee /proc/sys/kernel/core_pattern
ulimit -c unlimited

ARGS=""

# if user did pass in desired seed nodes, use
# the ones the user specified:
if [[ ! -z "$TAIYI_SEED_NODES" ]]; then
  for NODE in $TAIYI_SEED_NODES; do
    ARGS+=" --p2p-seed-node=$NODE"
  done
fi

if [[ ! -z "$TAIYI_SIMING_NAME" ]]; then
  ARGS+=" --siming=\"$TAIYI_SIMING_NAME\""
fi

if [[ ! -z "$TAIYI_PRIVATE_KEY" ]]; then
  ARGS+=" --private-key=$TAIYI_PRIVATE_KEY"
fi

if [[ "$STALE_PRODUCTION" ]]; then
  ARGS+=" --enable-stale-production"
fi

if [[ ! -z "$REQUIRED_PARTICIPATION" ]]; then
  ARGS+=" --required-participation=$REQUIRED_PARTICIPATION"
fi

if [[ ! -z "$TRACK_ACCOUNT" ]]; then
  if [[ ! "$USE_WAY_TOO_MUCH_RAM" ]]; then
    ARGS+=" --plugin=account_history --plugin=account_history_api"
  fi
  ARGS+=" --account-history-track-account-range=[\"$TRACK_ACCOUNT\",\"$TRACK_ACCOUNT\"]"
fi

# overwrite local config with image one
if [[ "$USE_FULL_WEB_NODE" ]]; then
  cp /etc/taiyi/fullnode.config.ini $HOME/config.ini
elif [[ "$IS_BROADCAST_NODE" ]]; then
  cp /etc/taiyi/config-for-broadcaster.ini $HOME/config.ini
elif [[ "$IS_AH_NODE" ]]; then
  cp /etc/taiyi/config-for-ahnode.ini $HOME/config.ini
elif [[ "$IS_OPSWHITELIST_NODE" ]]; then
  cp /etc/taiyi/fullnode.opswhitelist.config.ini $HOME/config.ini
else
  cp /etc/taiyi/config.ini $HOME/config.ini
fi

chown taiyi:taiyi $HOME/config.ini

cd $HOME

sleep 1

mv /etc/nginx/nginx.conf /etc/nginx/nginx.original.conf
cp /etc/nginx/taiyi.nginx.conf /etc/nginx/nginx.conf

echo "Attempting to execute taiyin using additional command line arguments: $ARGS"

cp /etc/nginx/healthcheck.conf.template /etc/nginx/healthcheck.conf
echo server 127.0.0.1:8091\; >>/etc/nginx/healthcheck.conf
echo } >>/etc/nginx/healthcheck.conf
rm /etc/nginx/sites-enabled/default
cp /etc/nginx/healthcheck.conf /etc/nginx/sites-enabled/default
/etc/init.d/fcgiwrap restart
service nginx restart
exec /usr/local/taiyi-testnet/bin/taiyin \
  --webserver-ws-endpoint=0.0.0.0:8091 \
  --webserver-http-endpoint=0.0.0.0:8091 \
  --p2p-endpoint=0.0.0.0:2025 \
  --data-dir=$HOME \
  $ARGS \
  $TAIYI_EXTRA_OPTS
