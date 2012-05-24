
#Add fts3_msg_cron and fts3_server and fts3_cli in the path

echo "Export required env valiables"
export LCG_GFAL_INFOSYS=certtb-bdii-top.cern.ch:2170
export GFAL_PLUGIN_LIST=libgfal_plugin_srm.so:libgfal_plugin_rfio.so:libgfal_plugin_lfc.so:libgfal_plugin_dcap.so:libgfal_plugin_dcap.so:libgfal_plugin_gridftp.so

updatedb
export URLCOPY=`locate fts3_url_copy | head -1 | sed 's/\/fts3_url_copy//'`
export PATH=$PATH:$URLCOPY
export LD_LIBRARY_PATH=`locate libfts3_db_oracle.so | head -1 | sed 's/\/libfts3_db_oracle.so//'`

echo "Prepare fts3 server config file"
if [ ! -f "/etc/sysconfig/fts3config" ]; then
cat > /etc/sysconfig/fts3config <<EOF
TransferPort=8080
IP=0.0.0.0
DbConnectString=
DbType=oracle
DbUserName=
DbPassword=
TransferLogDirectory=/var/log/fts3
ConfigPort=8081
EOF
fi

echo "Prepare fts3 monitoring using messaging config file"
if [ ! -f "/etc/fts-msg-monitoring.conf" ]; then
cat > /etc/fts-msg-monitoring.conf <<EOF
#broker uri / port number
BROKER=gridmsg007.cern.ch:6163
#start msg topic/queue name
START=transfer.test.msg.star
#complete msg topic/queue name
COMPLETE=transfer.test.msg.complet
#cron msg topic/queue name
CRON=transfer.fts_monitoring_queue_statu
#the periodic sent of monitoring message requires a single hostname 
#of the machine that will send the messages
FQDN=fts3src.cern.ch
#use topic or queue, values: true/false
TOPIC=true
#time before a message is expired inside the broker
#the value is in hours
TTL=24
#activate/deactivate message dispaching (true/false)
#When the following is false, no message will be sent or logged
ACTIVE=true
#ERROR/WARNING LOGGING
#enable logging of warning/error messages to a log file(e.g /var/tmp/msg.log)
#By default is false
ENABLELOG=true
#if it is enabled, all the messages will be stored in json format
#in the log file below
ENABLEMSGLOG=true
#directory that the log file will be stored (inluding tailing slash)
LOGFILEDIR=/var/log/fts3/
#name of log file
LOGFILENAME=msg.log
#username to connect to the broker
USERNAME=ftspublisher
#password to connect to the broker
PASSWORD=
#set to use username/password or not for the broker
USE_BROKER_CREDENTIALS=true                                                             
EOF
fi


echo "Setup vomses"
#check if /etc/vomses exists and copy hardoced voms
if [ ! -d "/etc/vomses" ]; then
mkdir /etc/vomses
cat > /etc/vomses/dteam-lxbra2309.cern.ch <<EOF
"dteam" "lxbra2309.cern.ch" "15002" "/DC=ch/DC=cern/OU=computers/CN=lxbra2309.cern.ch" "dteam" "24"
EOF

cat > /etc/vomses/dteam-lcg-voms.cern.ch <<EOF
"dteam" "lcg-voms.cern.ch" "15004" "/DC=ch/DC=cern/OU=computers/CN=lcg-voms.cern.ch" "dteam" "24"
EOF

cat > /etc/vomses/dteam-voms.cern.ch <<EOF
"dteam" "voms.cern.ch" "15004" "/DC=ch/DC=cern/OU=computers/CN=voms.cern.ch" "dteam" "24"
EOF

cat > /etc/vomses/org.glite.voms-test-lxbra2309.cern.ch <<EOF
"org.glite.voms-test" "lxbra2309.cern.ch" "15004" "/DC=ch/DC=cern/OU=computers/CN=lxbra2309.cern.ch" "org.glite.voms-test"
EOF
fi


echo "Setup repositories"
if [ ! -f "/etc/yum.repos.d/atrpms.repo" ]; then
cat > /etc/yum.repos.d/atrpms.repo <<EOF
[atrpms-stable]
name=UNSUPPORTED: ATrpms (http://atrpms.net) stable add-ons, no formal support from CERN
baseurl=http://linuxsoft.cern.ch/atrpms/sl6-$basearch/atrpms/stable
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-atrpms
gpgcheck=1
enabled=0
protect=0
 
[atrpms-testing]
name=UNSUPPORTED: ATrpms (http://atrpms.net) testing add-ons, no formal support from CERN
baseurl=http://linuxsoft.cern.ch/atrpms/sl6-$basearch/atrpms/testing
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-atrpms
gpgcheck=1
enabled=0
protect=0
 
[atrpms-bleeding]
name=UNSUPPORTED: ATrpms (http://atrpms.net) bleeding edge, no formal support from CERN
baseurl=http://linuxsoft.cern.ch/atrpms/sl6-$basearch/atrpms/bleeding
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-atrpms
gpgcheck=1
enabled=0
protect=0
 
[atrpms-stable-source]
name=UNSUPPORTED: ATrpms (http://atrpms.net) stable add-ons, no formal support from CERN - source RPMs
baseurl=http://linuxsoft.cern.ch/atrpms/src/sl6-$basearch/atrpms/stable
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-atrpms
gpgcheck=1
enabled=0
protect=0
 
[atrpms-testing-source]
name=UNSUPPORTED: ATrpms (http://atrpms.net) testing add-ons, no formal support from CERN - source RPMs
baseurl=http://linuxsoft.cern.ch/atrpms/src/sl6-$basearch/atrpms/testing
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-atrpms
gpgcheck=1
enabled=0
protect=0
 
[atrpms-bleeding-source]
name=UNSUPPORTED: ATrpms (http://atrpms.net) bleeding edge, no formal support from CERN - source RPMs
baseurl=http://linuxsoft.cern.ch/atrpms/src/sl6-$basearch/atrpms/bleeding
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-atrpms
gpgcheck=1
enabled=0
protect=0
EOF
fi

if [ ! -f "/etc/yum.repos.d/EMI-2-RC4-base.repo" ]; then
cat > /etc/yum.repos.d/EMI-2-RC4-base.repo <<EOF
[EMI-2-RC4-base]
name=EMI 2 RC4 Base Repository
baseurl=http://emisoft.web.cern.ch/emisoft/dist/EMI/2/sl6/$basearch/base
gpgkey=http://emisoft.web.cern.ch/emisoft/dist/EMI/2/RPM-GPG-KEY-emi
protect=1
enabled=1
priority=2
gpgcheck=0
EOF
fi

if [ ! -f "/etc/yum.repos.d/epel.repo" ]; then
cat > /etc/yum.repos.d/epel.repo <<EOF
[epel]
name=UNSUPPORTED: Extra Packages for Enterprise Linux add-ons, no formal support from CERN
baseurl=http://linuxsoft.cern.ch/epel/6/$basearch
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-6
gpgcheck=1
enabled=1
protect=0
 
[epel-source]
name=UNSUPPORTED: Extra Packages for Enterprise Linux add-ons, no formal support from CERN - source RPMs
baseurl=http://linuxsoft.cern.ch/epel/6/SRPMS/
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-6
gpgcheck=1
enabled=1
protect=0
 
[epel-testing]
name=UNSUPPORTED: Extra Packages for Enterprise Linux add-ons, no formal support from CERN - testing
baseurl=http://linuxsoft.cern.ch/epel/testing/6/$basearch
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-6
gpgcheck=1
enabled=1
protect=0
 
[epel-testing-debuginfo]
name=UNSUPPORTED: Extra Packages for Enterprise Linux add-ons, no formal support from CERN - testing debuginfo
baseurl=http://linuxsoft.cern.ch/epel/testing/6/$basearch/debug
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-6
gpgcheck=1
enabled=1
protect=0
 
[epel-testing-source]
name=UNSUPPORTED: Extra Packages for Enterprise Linux add-ons, no formal support from CERN - testing source RPMs
baseurl=http://linuxsoft.cern.ch/epel/testing/6/SRPMS/
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-6
gpgcheck=1
enabled=1
protect=0
 
[epel-testing-debuginfo]
name=Extra Packages for Enterprise Linux 6 - Testing - $basearch - Debug
#baseurl=http://download.fedoraproject.org/pub/epel/testing/6/$basearch/debug
mirrorlist=https://mirrors.fedoraproject.org/metalink?repo=testing-debug-epel6&arch=$basearch
failovermethod=priority
enabled=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-6
gpgcheck=1
 
EOF
fi

if [ ! -f "/etc/yum.repos.d/rpmforge.repo" ]; then
cat > /etc/yum.repos.d/rpmforge.repo <<EOF
# note : rpmforge packages with .rf disttag: these DO NOT replace system packages
[rpmforge]
name=UNSUPPORTED: RPMforge (http://rpmgforge.net), no formal support from CERN
baseurl=http://linuxsoft/rpmforge/redhat/el6/en/$basearch/rpmforge
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-rpmforge-dag
gpgcheck=1
enabled=0
protect=0
 

# warning: packages from this repository MAY replace your system ones.
[rpmforge-testing]
name=UNSUPPORTED: RPMforge Testting (http://rpmgforge.net), no formal support from CERN
baseurl=http://linuxsoft/rpmforge/redhat/el6/en/$basearch/testing
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-rpmforge-dag
gpgcheck=1
enabled=0
protect=0
 
# warning: packages from this repository WILL replace your system ones.
[rpmforge-extras]
name=UNSUPPORTED: RPMforge Extras (http://rpmgforge.net), no formal support from CERN
baseurl=http://linuxsoft/rpmforge/redhat/el6/en/$basearch/extras
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-rpmforge-dag
gpgcheck=1
enabled=0
protect=0
EOF
fi

if [ ! -f "/etc/yum.repos.d/slc6-extras.repo" ]; then
cat > /etc/yum.repos.d/slc6-extras.repo <<EOF
[slc6-extras]
name=Scientific Linux CERN 6 (SLC6) add-on packages, no formal support
baseurl=http://linuxsoft.cern.ch/cern/slc6X/$basearch/yum/extras/
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-cern
gpgcheck=1
enabled=1
protect=1
 
[slc6-extras-source]
name=Scientific Linux CERN 6 (SLC6) add-on packages, no formal support - source RPMS
baseurl=http://linuxsoft.cern.ch/cern/slc6X/$basearch/yum/extras-source/
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-cern
gpgcheck=1
enabled=0
protect=1
EOF
fi

if [ ! -f "/etc/yum.repos.d/slc6-testing.repo" ]; then
cat > /etc/yum.repos.d/slc6-testing.repo <<EOF
[slc6-testing]
name=Scientific Linux CERN 6 (SLC6) packages in testing phase
baseurl=http://linuxsoft.cern.ch/cern/slc6X/$basearch/yum/testing/
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-cern
gpgcheck=1
enabled=0
protect=1
 
[slc6-testing-source]
name=Scientific Linux CERN 6 (SLC6) packages in testing phase - source RPMs
baseurl=http://linuxsoft.cern.ch/cern/slc6X/$basearch/yum/testing-source/
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-cern
gpgcheck=1
enabled=0
protect=1
EOF
fi

if [ ! -f "/etc/yum.repos.d/elrepo.repo" ]; then
cat > /etc/yum.repos.d/elrepo.repo <<EOF
[elrepo]
name=UNSUPPORTED: El Repo, the Community Enterprise Linux Repository, no formal support from CERN
baseurl=http://linuxsoft.cern.ch/elrepo/elrepo/el6/$basearch
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-elrepo.org
gpgcheck=1
enabled=0
protect=0
 
[elrepo-testing]
name=UNSUPPORTED: El Repo the Community Enterprise Linux Repository, no formal support from CERN
baseurl=http://linuxsoft.cern.ch/elrepo/testing/el6/$basearch
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-elrepo.org
gpgcheck=1
enabled=0
protect=0
EOF
fi

if [ ! -f "/etc/yum.repos.d/EMI-2-RC4-third-party.repo" ]; then
cat > /etc/yum.repos.d/EMI-2-RC4-third-party.repo  <<EOF
[EMI-2-RC4-third-party]
name=EMI 2 RC4 Third-Party Repository
baseurl=http://emisoft.web.cern.ch/emisoft/dist/EMI/2/sl6/$basearch/third-party
gpgkey=http://emisoft.web.cern.ch/emisoft/dist/EMI/2/RPM-GPG-KEY-emi
protect=1
enabled=0
priority=2
gpgcheck=0
EOF
fi


if [ ! -f "/etc/yum.repos.d/lcg-CA.repo" ]; then
cat > /etc/yum.repos.d/lcg-CA.repo  <<EOF
# This is the official YUM repository string for the lcg-CA repository
# Fetched from: http://grid-deployment.web.cern.ch/grid-deployment/glite/repos/3.1/lcg-CA.repo
# Place it to /etc/yum.repos/3.1.d/ and run 'yum update'
 
[CA]
name=CAs
baseurl=http://linuxsoft.cern.ch/LCG-CAs/current
gpgkey=https://dist.eugridpma.info/distribution/igtf/current/GPG-KEY-EUGridPMA-RPM-3
       http://glite.web.cern.ch/glite/glite_key_gd.asc
gpgcheck=1
enabled=1
EOF
fi



if [ ! -f "/etc/yum.repos.d/slc6-cernonly.repo" ]; then
cat > /etc/yum.repos.d/slc6-cernonly.repo  <<EOF
[slc6-cernonly]
name=Scientific Linux CERN 6 (SLC6) CERN-only packages
baseurl=http://linuxsoft.cern.ch/onlycern/slc6X/$basearch/yum/cernonly/
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-cern
       file:///etc/pki/rpm-gpg/RPM-GPG-KEY-jpolok
gpgcheck=1
enabled=1
protect=1
 

[slc6-cernonly-srpms]
name=Scientific Linux CERN 6 (SLC6) CERN-only packages (sources)
baseurl=http://linuxsoft.cern.ch/onlycern/slc6X/$basearch/yum/cernonly-srpms/
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-cern
       file:///etc/pki/rpm-gpg/RPM-GPG-KEY-jpolok
gpgcheck=1
enabled=0
protect=1
EOF
fi

if [ ! -f "/etc/yum.repos.d/slc6-os.repo" ]; then
cat > /etc/yum.repos.d/slc6-os.repo  <<EOF
[slc6-os]
name=Scientific Linux CERN 6 (SLC6) base system packages
baseurl=http://linuxsoft.cern.ch/cern/slc6X/$basearch/yum/os/
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-cern
gpgcheck=1
enabled=1
protect=1
 
[slc6-os-source]
name=Scientific Linux CERN 6 (SLC6) base system packages - source RPMs
baseurl=http://linuxsoft.cern.ch/cern/slc6X/$basearch/yum/os-source/
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-cern
gpgcheck=1
enabled=0
protect=1
EOF
fi

if [ ! -f "/etc/yum.repos.d/slc6-updates.repo" ]; then
cat > /etc/yum.repos.d/slc6-updates.repo  <<EOF
[slc6-updates]
name=Scientific Linux CERN 6 (SLC6) bugfix and security updates
baseurl=http://linuxsoft.cern.ch/cern/slc6X/$basearch/yum/updates/
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-cern
gpgcheck=1
enabled=1
protect=1
 
[slc6-updates-source]
name=Scientific Linux CERN 6 (SLC6) bugfix and security updates - source RPMs
baseurl=http://linuxsoft.cern.ch/cern/slc6X/$basearch/yum/updates-source/
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-cern
gpgcheck=1
enabled=0
protect=1
EOF
fi

echo "Install required packages"
yum -y install gfal2-debuginfo
yum -y install libuuid-devel
yum -y install activemq-cpp-devel
yum -y install globus-gssapi-error
yum -y install libxslt
yum -y install oracle-instantclient-devel
yum -y install libXau-devel
yum -y install sudo
yum -y install voms-clients
yum -y install gfal2-plugin-srm
yum -y install gsoap
yum -y install globus-gass-transfer
yum -y install apr-util-devel
yum -y install CGSI-gSOAP
yum -y install glibc-devel
yum -y install boost-date-time
yum -y install lfc-libs
yum -y install fuse
yum -y install mysql-libs
yum -y install gfal2-plugin-gridftp
yum -y install globus-gsi-sysconfig
yum -y install globus-gass-copy
yum -y install libuuid
yum -y install is-interface-debuginfo
yum -y install glibc
yum -y install voms
yum -y install boost-filesystem
yum -y install boost-devel
yum -y install mlocate
yum -y install gfal2-plugin-lfc
yum -y install globus-gsi-cert-utils
yum -y install gcc
yum -y install valgrind
yum -y install oracle-instantclient-basic
yum -y install glibc-headers
yum -y install gfal2-core
yum -y install gfal2-plugin-dcap
yum -y install gfal2-all
yum -y install globus-gsi-proxy-core
yum -y install globus-ftp-control
yum -y install boost-signals
yum -y install is-interface
yum -y install cmake
yum -y install boost-doc
yum -y install libgcc
yum -y install gfal2-doc
yum -y install globus-gsi-openssl-error
yum -y install globus-xio
yum -y install globus-callout
yum -y install openldap
yum -y install globus-gass-copy-progs
yum -y install activemq-cpp
yum -y install apr-util
yum -y install boost-regex
yum -y install gcc-c++
yum -y install CGSI-gSOAP-devel
yum -y install gfal2-plugin-rfio
yum -y install srm-ifce
yum -y install globus-gsi-callback
yum -y install globus-io
yum -y install gsoap-devel
yum -y install boost-iostreams
yum -y install gfal
yum -y install boost-serialization
yum -y install apr-devel
yum -y install glib2
yum -y install oracle-instantclient-sqlplus
yum -y install boost-thread
yum -y install glib2-devel
yum -y install boost-openmpi
yum -y install strace
yum -y install gfal2-transfer
yum -y install globus-common
yum -y install globus-gssapi-gsi
yum -y install globus-ftp-client
yum -y install is-interface-devel
yum -y install boost-program-options
yum -y install apr
yum -y install boost-system
yum -y install cpp
yum -y install gdb
yum -y install boost
yum -y install glibc-common
yum -y install gfal2
yum -y install globus-openssl-module
yum -y install globus-gsi-credential
yum -y install pkgconfig
yum -y install openldap-devel
yum -y install lcg-CA
yum -y install boost-openmpi-devel
yum -y install gfal2-devel
yum -y install globus-gsi-proxy-ssl
yum -y install globus-xio-popen-driver
yum -y install srm-ifce-debuginfo
yum -y install globus-xio-gsi-driver
yum -y install boost-test
yum -y install openssl


echo "Fetch-crls now"
fetch-crl


echo " Now manually set X509_USER_CERT & X509_USER_KEY to point to your certificate and key"
echo " Run voms-proxy-init -voms dteam"

echo "Set username, password and connection string in /etc/sysconfig/fts3config"

echo "Set the password to connect to the broker in /etc/fts-msg-monitoring.conf"
