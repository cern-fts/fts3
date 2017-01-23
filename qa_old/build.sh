#!/bin/bash
# Build source for coverage tests
if [ "$#" -ne 2 ]; then
    echo "Wrong number of arguments"
    exit 1;
fi

SOURCE_DIR=$1
BUILD_DIR=$2

set -x

mkdir -p "${BUILD_DIR}"
pushd "${BUILD_DIR}"

CFLAGS=--coverage CXXFLAGS=--coverage cmake "${SOURCE_DIR}" \
    -DMAINBUILD=ON -DSERVERBUILD=ON -DCLIENTBUILD=ON -DMYSQLBUILD=ON \
    -DTESTBUILD=ON
make -j3

popd
