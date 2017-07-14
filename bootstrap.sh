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
