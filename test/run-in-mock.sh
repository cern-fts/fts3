#!/bin/bash
# Exit on error
set -e

# Defaults
CHROOT="epel-6-x86_64"
PUPPET_REPO="http://svn.cern.ch/guest/saket/trunk/favourite/puppet/test/"
TEST_MANIFEST="fts3-dev.pp"
CERT_PASSWD=
VOMS="dteam:/dteam/Role=lcgadmin"
TARGET="fts3-devel.cern.ch"
OUTPUT_DIR="/tmp"

# Get parameters
while [ -n "$1" ]; do
    case "$1" in
        --manifest)
            shift
            TEST_MANIFEST=$1
            ;;
        --target)
            shift
            TARGET=$1
            ;;
        --output)
            shift
            OUTPUT_DIR=$1
            ;;
        --passwd)
            shift
            CERT_PASSWD=$1
            ;;
        --voms)
            shift
            VOMS=$1
            ;;
        --chroot)
            shift
            CHROOT=$1
            ;;
    esac
    shift
done

# Mock flags
MOCK_FLAGS="-r ${CHROOT} --disable-plugin=tmpfs"

# Function to install inside the mock env
function mock_install()
{
    /usr/bin/mock ${MOCK_FLAGS} --install "$@"
}

# Function to run a command inside the mock env
function mock_run()
{
    /usr/bin/mock ${MOCK_FLAGS} --chroot -- "$@"
}

# Extract files from the mock env
function mock_extract()
{
    /usr/bin/mock ${MOCK_FLAGS} --copyout "$1" "$2"
}

# Get dependencies
mock_install subversion puppet yum
mock_run rpm --rebuilddb
mock_run rm -rf "puppet"
mock_run svn co "${PUPPET_REPO}" "puppet"

# /etc/yum/yum.conf is populated by mock, so we need to
# get rid of it prior to running puppet, or our repos will
# not be picked
mock_run "rm -vf /etc/yum/yum.conf && yum clean all"
mock_run "rm -vf /etc/yum/yum.conf && puppet apply --verbose --modulepath=puppet puppet/${TEST_MANIFEST}" 
# Now keep going even if failed
set +e

# Run the tests
mock_run /opt/favourite/wrapper.py --debug --test-prefix "fts" --test-location "/usr/share/fts3/tests/" --user-cert "/tmp/ucert.pem" --user-key "/tmp/ukey.pem" --user-pass "${CERT_PASSWD}" --voms "${VOMS}" --target "${TARGET}" --log-dir "/var/log/fav/"

# Convert to junit
mock_install "libxslt"
mock_run xsltproc -o "/var/log/fav/junit.xml" "http://svn.cern.ch/guest/saket/trunk/favourite/favourite/style/sak2junit.xsl" "/var/log/fav/tests.xml"

# Extract the generated logs
rm -rf "${OUTPUT_DIR}"/fav
mock_extract "/var/log/fav" "${OUTPUT_DIR}"/fav

