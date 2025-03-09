#!/bin/bash

echo "执行清理操作"

PID=$(pgrep -f /etc/service/taiyi)

if [ -n "$PID" ]; then
    kill -s SIGTERM "$PID"
fi

echo "清理操作完成"

