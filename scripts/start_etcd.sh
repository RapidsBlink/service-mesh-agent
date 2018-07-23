#!/usr/bin/env bash

docker run --rm -d --name etcd --network=benchmarker -v /tmp/logs:/root/logs registry.cn-hangzhou.aliyuncs.com/aliware2018/alpine-etcd
