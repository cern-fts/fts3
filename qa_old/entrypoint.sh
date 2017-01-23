#!/usr/bin/env bash
set -x

export HOME=/
export SONAR_USER_HOME=/tmp

VOMS=${VOMS:=dteam}
PASSWD=$1

if [[ -z "$PASSWD" ]]; then
    echo "Missing password"
    exit 1
fi

# Static analysis
./static-analysis.sh /fts3 /fts3

# Build
./build.sh /fts3 /tmp/build

# Unit tests
./coverage-unit.sh /fts3 /tmp/build /fts3

# Integration tests
echo ${PASSWD} | voms-proxy-init --pwstdin --voms ${VOMS}
./coverage-integration.sh /fts3 /tmp/build /fts3
voms-proxy-destroy

# Combine
./coverage-overall.sh /fts3

# Push to sonarqube
./sonarqube.sh /fts3
