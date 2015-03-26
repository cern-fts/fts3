#!/bin/bash

set -x
set -e

# Build RATS
function build_rats() {
    if [ ! -e "rats-2.4" ]; then
	    wget "https://rough-auditing-tool-for-security.googlecode.com/files/rats-2.4.tgz" -O "rats-2.4.tgz"
	    tar xzf "rats-2.4.tgz"
	fi
	
	pushd "rats-2.4"
	if [ ! -e "rats" ]; then
	    ./configure
	    make
	fi
	popd
} 

# Build vera++
# Dependencies: tcl-devel lua-devel 
function build_vera() {
	if [ ! -e "vera++-1.3.0" ]; then
	    wget "https://bitbucket.org/verateam/vera/downloads/vera%2B%2B-1.3.0.tar.gz" -O "vera++-1.3.0.tar.gz"
	    tar xzf "vera++-1.3.0.tar.gz"
	fi
	pushd "vera++-1.3.0"
	cmake .
	make
	popd	
}
   
###############
# Build tools #
###############

mkdir -p /tmp/fts3-qa

pushd /tmp/fts3-qa
    build_rats
    build_vera
popd
