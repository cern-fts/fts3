#!/usr/bin/env bash

OUTPUT_DIR=$1

pushd ${OUTPUT_DIR}

export SONAR_RUNNER_OPTS="-Duser.timezone=+01:00 -Djava.security.egd=file:///dev/urandom"
/tmp/sonar-runner-2.4/bin/sonar-runner -X

popd
