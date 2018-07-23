#!/usr/bin/env bash

docker run --rm -d --name consumer --cpu-period 50000 --cpu-quota 200000 -m 4G --network=benchmarker -v /tmp/logs:/root/logs -p 8087:8087 agent:latest consumer

docker logs -f consumer