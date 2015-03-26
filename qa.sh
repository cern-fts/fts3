#!/bin/bash
SOURCE_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# CPPCheck
cppcheck --version &> /dev/null
if [ $? -ne 0 ]; then
    echo "cppcheck not installed"
else
    echo "Running cppcheck"
    cppcheck -v --enable=all --xml "${SOURCE_DIR}/src" 2> cppcheck.xml
fi

# Vera++
vera++ --version &> /dev/null
if [ $? -ne 0 ]; then
    echo "vera++ not installed"
else
    if [ ! -e "/tmp/vera++Report2checkstyleReport.perl" ]; then
        wget "https://raw.githubusercontent.com/wenns/sonar-cxx/master/sonar-cxx-plugin/src/tools/vera%2B%2BReport2checkstyleReport.perl" -O "/tmp/vera++Report2checkstyleReport.perl"
        chmod a+x "/tmp/vera++Report2checkstyleReport.perl"
    fi
    echo "Runnign vera++"
    find "${SOURCE_DIR}/src" -regex ".*\.c\|.*\.h\|.*\.cpp" | vera++ - -showrules -nodup |& /tmp/vera++Report2checkstyleReport.perl > vera.xml
fi

# Rats
rats &> /dev/null
if [ $? -ne 0 ]; then
    echo "rats not installed"
else
    echo "Running rats"
    rats -w 3 --xml "${SOURCE_DIR}/src" > rats.xml
fi

# Done
echo "Ready to run sonar-runner"
