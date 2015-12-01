#!/bin/bash
# Extract coverage data out of the unit tests
set -x

if [ "$#" -eq 1 ]; then
    BUILD_DIR=$1
else
    BUILD_DIR="build"
fi

SOURCE_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
RUNNING_DIR=`pwd`

mkdir -p "${BUILD_DIR}"
pushd "${BUILD_DIR}"

# Build sources
${SOURCE_DIR}/coverage-build.sh "${SOURCE_DIR}"

# Clean coverage data
lcov --directory . --zerocounters

# Run tests
./test/unit/unit --log_level=all --log_format=XML --report_level=detailed > "${RUNNING_DIR}/tests.xml"

# Generate the coverage lcov data
lcov --directory . --capture --output-file="${RUNNING_DIR}/coverage-unit.info"

# Download the converter
if [ ! -f "lcov_cobertura.py" ]; then
    wget "https://raw.github.com/eriwen/lcov-to-cobertura-xml/master/lcov_cobertura/lcov_cobertura.py"
fi

# Generate the xml
python lcov_cobertura.py "${RUNNING_DIR}/coverage-unit.info" -b "${SOURCE_DIR}" -e ".+usr.include." -o "${RUNNING_DIR}/coverage-unit.xml"

# Done!
popd
echo "Done. Unit test coverage in coverage-unit.xml"
