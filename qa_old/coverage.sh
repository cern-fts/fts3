#!/bin/bash

if [ "$#" -eq 1 ]; then
    BUILD_DIR="$1"
else
    BUILD_DIR="build"
fi

SOURCE_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
RUNNING_DIR=`pwd`

echo "* Running dir: ${RUNNING_DIR}"
echo "* Source dir:  ${SOURCE_DIR}"
echo "* Build dir:   ${BUILD_DIR}"


echo "Build sources with coverage information"
${SOURCE_DIR}/coverage-build.sh "${SOURCE_DIR}" "${BUILD_DIR}"


echo "Running unit test coverage"
set -x
${SOURCE_DIR}/coverage-unit.sh "${SOURCE_DIR}" "${BUILD_DIR}" "${RUNNING_DIR}"
set +x


echo "Running integration coverage"
set -x
${SOURCE_DIR}/coverage-integration.sh "${SOURCE_DIR}" "${BUILD_DIR}" "${RUNNING_DIR}" \
    "${SOURCE_DIR}/fts3config.conf" "${SOURCE_DIR}/ftsmsg.conf"
set +x


echo "Merging results"
set -x
${SOURCE_DIR}/coverage-overall.sh "${RUNNING_DIR}"
set +x


echo "Done. Results:"
set -x
ls -l "${RUNNING_DIR}/"*.xml
set +x
