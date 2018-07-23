#!/bin/bash

#apt-get install libtool m4 automake

VERSION=1.20.3

if [[ -z $1 ]]
then
    DIR=$PWD/3rd-deps/libuv
else
    DIR=$1
fi

echo "target path: ${DIR}"
# download source

curl -L https://github.com/libuv/libuv/archive/v${VERSION}.tar.gz -o libuv.tar.gz

tar xvzf libuv.tar.gz -C /tmp/ >>/dev/null
rm libuv.tar.gz

cd /tmp/libuv-${VERSION}

bash autogen.sh
./configure --prefix=${DIR} >>/dev/null

make -j >>/dev/null && make install >>/dev/null

cd ${DIR}

rm -rf /tmp/libuv-${VERSION}