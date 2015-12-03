#!/bin/bash
# Extract coverage data out of the unit tests
if [ "$#" -ne 3 ]; then
    echo "Wrong number of arguments"
    exit 1
fi

SOURCE_DIR=$1
BUILD_DIR=$2
OUTPUT_DIR=$3

set -x

pushd "${BUILD_DIR}"

# Clean coverage data
lcov --directory . --zerocounters

# Run tests
./test/unit/unit --log_level=all --log_format=XML --report_level=detailed > "${OUTPUT_DIR}/tests.xml"

# Generate the coverage lcov data
lcov --directory . --capture --output-file="${OUTPUT_DIR}/coverage-unit.info"

# Download the converter
if [ ! -f "lcov_cobertura.py" ]; then
    wget "https://raw.github.com/eriwen/lcov-to-cobertura-xml/master/lcov_cobertura/lcov_cobertura.py"
fi

# Generate the xml
python lcov_cobertura.py "${OUTPUT_DIR}/coverage-unit.info" -b "${SOURCE_DIR}" -e ".+usr.include." -o "${OUTPUT_DIR}/coverage-unit.xml"

# Done!
popd
echo "Done. Unit test coverage in coverage-unit.xml"
