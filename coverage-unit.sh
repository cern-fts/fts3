#!/bin/bash
# Extract coverage data out of the unit tests
set -x

SOURCE_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# Need to build, the source, of course
mkdir -p build
pushd build
CFLAGS=--coverage CXXFLAGS=--coverage cmake "${SOURCE_DIR}" \
    -DMAINBUILD=ON -DSERVERBUILD=ON -DCLIENTBUILD=ON \
    -DTESTBUILD=ON
make -j2

# Clean coverage data
lcov --directory . --zerocounters

# Run tests
./test/unit/unit --log_level=all --log_format=XML --report_level=no > "../tests.xml"

# Generate the coverage lcov data
lcov --directory . --capture --output-file="coverage.info"

# Download the converter
rm -fv lcov_cobertura.py
wget "https://raw.github.com/eriwen/lcov-to-cobertura-xml/master/lcov_cobertura/lcov_cobertura.py"

# Generate the xml
python lcov_cobertura.py "coverage.info" -b "${SOURCE_DIR}" -e ".+usr.include." -o "../coverage.xml"

# Done!
popd
echo "Done. Coverage info in coverage.xml"
