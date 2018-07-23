#!/usr/bin/env bash

docker run --rm -d --name provider-large --cpu-period 50000 --cpu-quota 90000 -m 6G --network=benchmarker -v /tmp/logs:/root/logs agent:latest provider-large

docker logs -f provider-large