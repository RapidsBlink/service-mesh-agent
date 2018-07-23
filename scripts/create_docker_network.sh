#!/usr/bin/env bash

docker network create --driver=bridge --subnet=10.10.10.0/24 --gateway=10.10.10.1 -o "com.docker.network.bridge.name"="benchmarker" -o "com.docker.network.bridge.enable_icc"="true" benchmarker