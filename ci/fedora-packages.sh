#!/usr/bin/env bash
set -e

if [[ -f /usr/bin/dnf ]]; then
  dnf install -y dnf-plugins-core git rpm-build tree which python2 \
                 cmake cmake3 make gcc gcc-c++
else
  yum install -y yum-utils git rpm-build make tree which python2 \
                 cmake cmake3 make gcc gcc-c++
fi
