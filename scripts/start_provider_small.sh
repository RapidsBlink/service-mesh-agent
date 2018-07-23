#!/usr/bin/env bash

docker run --rm -d --name provider-small --cpu-period 50000 --cpu-quota 30000 -m 2G --network=benchmarker -v /tmp/logs:/root/logs agent:latest provider-small

docker logs -f provider-small