#!/usr/bin/env bash
set -e

# Ensure "epel-release" package is installed
if ! rpm -q --quiet epel-release ; then
  dnf install -y epel-release
fi

dnf install -y dnf-plugins-core git rpm-build tree which \
               cmake cmake3 make gcc gcc-c++
