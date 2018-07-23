#!/usr/bin/env bash

docker run --rm -d --name provider-medium --cpu-period 50000 --cpu-quota 60000 -m 4G --network=benchmarker -v /tmp/logs:/root/logs agent:latest provider-medium

docker logs -f provider-medium