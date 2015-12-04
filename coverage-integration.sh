#!/bin/bash
# Extract coverage data out of the functional tests

if [ "$#" -ne 5 ]; then
    echo "Wrong number of arguments"
    exit 1
fi

SOURCE_DIR=$1
BUILD_DIR=$2
OUTPUT_DIR=$3
FTS3CONFIG=$4
MSGCONFIG=$5


set -x
pushd "${BUILD_DIR}"

# Clean coverage data
lcov --directory . --zerocounters

# Spawn fts_server, fts_bringonline and msg-bulk
export LD_LIBRARY_PATH="src/db/mysql"
export PATH="$PATH:src/url-copy"

./src/server/fts_server -f "${FTS3CONFIG}"
./src/bringonline-daemon/fts_bringonline -f "${FTS3CONFIG}"
./src/monitoring/fts_msg_bulk -f "${FTS3CONFIG}"  "--MonitoringConfigFile=${MSGCONFIG}"

sleep 2

# Get pids
FTS_PID=`pidof fts_server`
BRINGONLINE_PID=`pidof fts_bringonline`
MSG_PID=`pidof fts_msg_bulk`

echo "Running fts_server: ${FTS_PID}"
echo "Running fts_bringonline: ${BRINGONLINE_PID}"
echo "Running fts_msg_bulk: ${MSG_PID}"

# Get the tests and run them against the REST API connected to the same DB
# We assume there is a valid proxy in the environment!
if [ ! -d "fts-tests" ]; then
    git clone https://gitlab.cern.ch/fts/fts-tests.git
fi
pushd fts-tests
    ./run-helper.sh
popd

# Stop the servers
echo "Stop services..."
kill "${FTS_PID}"
kill "${BRINGONLINE_PID}"
kill "${MSG_PID}"
sleep 5

kill -9 "${FTS_PID}"
kill -9 "${BRINGONLINE_PID}"
kill -9 "${MSG_PID}"

# Generate the coverage lcov data
lcov --directory . --capture --output-file="${OUTPUT_DIR}/coverage-integration.info"

# Download the converter
if [ ! -f "lcov_cobertura.py" ]; then
    wget "https://raw.github.com/eriwen/lcov-to-cobertura-xml/master/lcov_cobertura/lcov_cobertura.py"
fi

# Generate the xml
python lcov_cobertura.py "${OUTPUT_DIR}/coverage-integration.info" -b "${SOURCE_DIR}" -e ".+usr.include." -o "${OUTPUT_DIR}/coverage-integration.xml"

# Done
popd
echo "Done. Functional test coverage in coverage-functional.xml"
