#!/usr/bin/env bash

docker stop consumer
docker stop provider-small
docker stop provider-medium
docker stop provider-large
docker stop etcd