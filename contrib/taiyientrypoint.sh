#!/bin/bash

echo /tmp/core | tee /proc/sys/kernel/core_pattern
ulimit -c unlimited

# if we're not using PaaS mode then start taiyin traditionally with sv to control it
if [[ ! "$USE_PAAS" ]]; then
  mkdir -p /etc/service/taiyi
  cp /usr/local/bin/taiyi-sv-run.sh /etc/service/taiyi/run
  chmod +x /etc/service/taiyi/run
  runsv /etc/service/taiyi
else
  /usr/local/bin/startpaastaiyi.sh
fi
