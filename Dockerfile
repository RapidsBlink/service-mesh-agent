# Builder container
FROM gcc:8.1.0 AS gcc_builder

COPY . /root/workspace/agent
WORKDIR /root/workspace/agent
RUN apt-get update -y && apt-get install -y cmake libcurl4-openssl-dev && \
    ./scripts/install_libuv.sh /usr/local/libuv && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DenableLOG=OFF -Dtest=OFF .. \
    && make -j && lscpu


FROM registry.cn-hongkong.aliyuncs.com/alicontest18/demo-services AS builder

# Runner container
FROM registry.cn-hangzhou.aliyuncs.com/aliware2018/debian-jdk8

COPY --from=builder /root/workspace/services/mesh-provider/target/mesh-provider-1.0-SNAPSHOT.jar /root/dists/mesh-provider.jar
COPY --from=builder /root/workspace/services/mesh-consumer/target/mesh-consumer-1.0-SNAPSHOT.jar /root/dists/mesh-consumer.jar
COPY --from=builder /usr/local/bin/docker-entrypoint.sh /usr/local/bin


COPY --from=gcc_builder /usr/local/libuv /usr/local/libuv
COPY --from=gcc_builder /root/workspace/agent/build/agent /usr/local/bin
COPY start-agent.sh /usr/local/bin

RUN set -ex && mkdir -p /root/logs && apt-get update && apt-get install -y nano procps libcurl4-openssl-dev && apt-get clean

ENV LD_LIBRARY_PATH /usr/local/libuv/lib:${LD_LIBRARY_PATH}

ENTRYPOINT ["docker-entrypoint.sh"]
