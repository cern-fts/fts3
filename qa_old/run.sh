#!/usr/bin/env bash
set -x

FTS3_CONFIG_DIR=$1
IMAGE=$2
PASSWD=$3
SOURCE_DIR=`readlink -m $PWD/..`


if [[ -z "${FTS3_CONFIG_DIR}" ]]; then
    echo "Missing configuration directory"
    exit 1
fi

docker run --rm=true \
    -v "/etc/passwd:/etc/passwd" \
    -v "/tmp:/tmp" \
    -v "${SOURCE_DIR}:/fts3" \
    -v "${FTS3_CONFIG_DIR}:/etc/fts3" \
    -v "${HOME}/.globus:/.globus" \
    -u `id -u`:`id -g` \
    ${IMAGE} ${PASSWD}
