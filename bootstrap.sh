#!/bin/bash

cat > /etc/yum.repos.d/dmc.repo <<EOF
[dmc-builds]
name=DMC Continuous Build Repository
baseurl=http://dmc-repo.web.cern.ch/dmc-repo/testing/el\$releasever/\$basearch
gpgcheck=0
enabled=1
protect=1
EOF

yum install -y vim
yum groupinstall -y 'Development Tools'
yum install -y epel-release yum-builddep git
yum-builddep -y "/vagrant/packaging/rpm/fts.spec"
yum install -y gfal2-all

# Prepare the environment
mkdir -p "/etc/fts3"
cp "/vagrant/src/config/fts3config" "/etc/fts3/fts3config"

mkdir -p "/var/lib/fts3/"
mkdir -p "/var/log/fts3"
chown vagrant.vagrant "/var/lib/fts3/"
chown vagrant.vagrant "/var/log/fts3/"
