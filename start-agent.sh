#!/bin/bash

ETCD_HOST=etcd
ETCD_PORT=2379
ETCD_URL=http://$ETCD_HOST:$ETCD_PORT

echo ETCD_URL = $ETCD_URL

lscpu

if [[ "$1" == "consumer" ]]; then
  echo "Starting consumer agent..."
  agent consumer 20000 1234 $ETCD_URL /root/logs/consumer.log
elif [[ "$1" == "provider-small" ]]; then
  echo "Starting small provider agent..."
  agent provider-small 30000 20880 $ETCD_URL /root/logs/provider-s.log
elif [[ "$1" == "provider-medium" ]]; then
  echo "Starting medium provider agent..."
  agent provider-medium 30000 20880 $ETCD_URL /root/logs/provider-m.log
elif [[ "$1" == "provider-large" ]]; then
  echo "Starting large provider agent..."
  agent provider-large 30000 20880 $ETCD_URL /root/logs/provider-l.log
else
  echo "Unrecognized arguments, exit."
  exit 1
fi
