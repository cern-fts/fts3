#!/bin/bash
# Merge coverage information from unit and functional

if [ "$#" -ne 1 ]; then
    echo "Wrong number of arguments"
    exit 1
fi

OUTPUT_DIR=$1

set -x

lcov --add-tracefile "${OUTPUT_DIR}/coverage-unit.info" -a "${OUTPUT_DIR}/coverage-integration.info" -o "${OUTPUT_DIR}/coverage-overall.info"
python /tmp/lcov_cobertura.py "${OUTPUT_DIR}/coverage-overall.info" -b "${OUTPUT_DIR}" -e ".+usr.include." -o "${OUTPUT_DIR}/coverage-overall.xml"

echo "Done. Merged test coverage in coverage-overall.xml"
