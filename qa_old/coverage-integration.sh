#!/bin/bash
# Extract coverage data out of the functional tests

if [ "$#" -ne 3 ]; then
    echo "Wrong number of arguments"
    exit 1
fi

SOURCE_DIR=`readlink -f $1`
BUILD_DIR=`readlink -f $2`
OUTPUT_DIR=`readlink -f $3`

set -x
pushd "${BUILD_DIR}"

# Clean coverage data
lcov --directory . --zerocounters

# Start daemons with supervisor
export LD_LIBRARY_PATH=${BUILD_DIR}/src/db/mysql/
export PATH=$PATH:${BUILD_DIR}/src/url-copy/

supervisord -c /supervisord.conf &
SUPERVISORD_PID=$!
sleep 5

# Get the tests and run them against the REST API connected to the same DB
# We assume there is a valid proxy in the environment!
if [ ! -d "fts-tests" ]; then
    git clone https://gitlab.cern.ch/fts/fts-tests.git
fi
pushd fts-tests
    export FTS3_HOST=fts3cov.cern.ch
    export CLIENT_IMPL=lib.cli
    export PATH=$PATH:${BUILD_DIR}/src/cli
    export X509_USER_CERT=${HOME}/.globus/usercert.pem
    export X509_USER_KEY=${HOME}/.globus/userkey.pem
    ./run-helper.sh

    # Copy tests results
    cp results.xml ${OUTPUT_DIR}/tests.xml
popd

# Kill supervisor
kill -SIGTERM $SUPERVISORD_PID
wait $SUPERVISORD_PID

# Generate the coverage lcov data
lcov --directory . --capture --output-file="${OUTPUT_DIR}/coverage-integration.info"

# Download the converter
if [ ! -f "/tmp/lcov_cobertura.py" ]; then
    wget "https://raw.github.com/eriwen/lcov-to-cobertura-xml/master/lcov_cobertura/lcov_cobertura.py" -O "/tmp/lcov_cobertura.py"
fi

# Generate the xml
python /tmp/lcov_cobertura.py "${OUTPUT_DIR}/coverage-integration.info" -b "${SOURCE_DIR}" -e ".+usr.include." -o "${OUTPUT_DIR}/coverage-integration.xml"

# Done
popd
echo "Done. Functional test coverage in coverage-functional.xml"
