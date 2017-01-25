#!/usr/bin/env bash
set -ex

export HOME=/
export X509_USER_CERT=${HOME}/.globus/usercert.pem
export X509_USER_KEY=${HOME}/.globus/userkey.pem
export X509_USER_KEY_PASSWORD=${PROXY_PASSWD}

echo ${PROXY_PASSWD} | voms-proxy-init -voms dteam -rfc -pwstdin

mkdir -p /tmp/tests
pushd /tmp/tests
if [ ! -d fts-tests ]; then
    git clone https://gitlab.cern.ch/fts/fts-tests.git
fi
pushd fts-tests
    export CLIENT_IMPL=lib.cli
    export PATH=$PATH:/usr/src/fts3/src/cli
    export FTS3_HOST=${FTS3_HOST:-`hostname -f`}
    ./run-helper.sh
    cp -v results.xml /coverage
popd
popd
