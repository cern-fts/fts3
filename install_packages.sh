
#Add fts3_msg_cron and fts3_server and fts3_cli in the path

echo "Export required env valiables"
export LCG_GFAL_INFOSYS=certtb-bdii-top.cern.ch:2170
export GFAL_PLUGIN_LIST=libgfal_plugin_srm.so:libgfal_plugin_rfio.so:libgfal_plugin_lfc.so:libgfal_plugin_dcap.so:libgfal_plugin_dcap.so:libgfal_plugin_gridftp.so

sudo updatedb
export URLCOPY=`locate fts3_url_copy | head -1 | sed 's/\/fts3_url_copy//'`
export PATH=$PATH:$URLCOPY
export LD_LIBRARY_PATH=`locate libfts3_db_oracle.so | head -1 | sed 's/\/libfts3_db_oracle.so//'`

echo "Prepare fts3 server config file"
if [ ! -f "/etc/sysconfig/fts3config" ]; then
sudo cat > /etc/sysconfig/fts3config <<EOF
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
sudo cat > /etc/fts-msg-monitoring.conf <<EOF
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
sudo mkdir /etc/vomses
sudo cat > /etc/vomses/dteam-lxbra2309.cern.ch <<EOF
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
sudo cat > /etc/yum.repos.d/atrpms.repo <<EOF
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
sudo cat > /etc/yum.repos.d/rpmforge.repo <<EOF
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
sudo cat > /etc/yum.repos.d/slc6-extras.repo <<EOF
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
sudo cat > /etc/yum.repos.d/slc6-testing.repo <<EOF
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
sudo cat > /etc/yum.repos.d/elrepo.repo <<EOF
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
sudo cat > /etc/yum.repos.d/EMI-2-RC4-third-party.repo  <<EOF
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
sudo cat > /etc/yum.repos.d/lcg-CA.repo  <<EOF
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
sudo cat > /etc/yum.repos.d/slc6-cernonly.repo  <<EOF
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
sudo cat > /etc/yum.repos.d/slc6-os.repo  <<EOF
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
sudo cat > /etc/yum.repos.d/slc6-updates.repo  <<EOF
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
sudo yum -y install gfal2-debuginfo
sudo yum -y install libuuid-devel
sudo yum -y install activemq-cpp-devel
sudo yum -y install globus-gssapi-error
sudo yum -y install libxslt
sudo yum -y install oracle-instantclient-devel
sudo yum -y install libXau-devel
sudo yum -y install sudo
sudo yum -y install voms-clients
sudo yum -y install gfal2-plugin-srm
sudo yum -y install gsoap
sudo yum -y install globus-gass-transfer
sudo yum -y install apr-util-devel
sudo yum -y install CGSI-gSOAP
sudo yum -y install glibc-devel
sudo yum -y install boost-date-time
sudo yum -y install lfc-libs
sudo yum -y install fuse
sudo yum -y install mysql-libs
sudo yum -y install gfal2-plugin-gridftp
sudo yum -y install globus-gsi-sysconfig
sudo yum -y install globus-gass-copy
sudo yum -y install libuuid
sudo yum -y install is-interface-debuginfo
sudo yum -y install glibc
sudo yum -y install voms
sudo yum -y install boost-filesystem
sudo yum -y install boost-devel
sudo yum -y install mlocate
sudo yum -y install gfal2-plugin-lfc
sudo yum -y install globus-gsi-cert-utils
sudo yum -y install gcc
sudo yum -y install valgrind
sudo yum -y install oracle-instantclient-basic
sudo yum -y install glibc-headers
sudo yum -y install gfal2-core
sudo yum -y install gfal2-plugin-dcap
sudo yum -y install gfal2-all
sudo yum -y install globus-gsi-proxy-core
sudo yum -y install globus-ftp-control
sudo yum -y install boost-signals
sudo yum -y install is-interface
sudo yum -y install cmake
sudo yum -y install boost-doc
sudo yum -y install libgcc
sudo yum -y install gfal2-doc
sudo yum -y install globus-gsi-openssl-error
sudo yum -y install globus-xio
sudo yum -y install globus-callout
sudo yum -y install openldap
sudo yum -y install globus-gass-copy-progs
sudo yum -y install activemq-cpp
sudo yum -y install apr-util
sudo yum -y install boost-regex
sudo yum -y install gcc-c++
sudo yum -y install CGSI-gSOAP-devel
sudo yum -y install gfal2-plugin-rfio
sudo yum -y install srm-ifce
sudo yum -y install globus-gsi-callback
sudo yum -y install globus-io
sudo yum -y install gsoap-devel
sudo yum -y install boost-iostreams
sudo yum -y install gfal
sudo yum -y install boost-serialization
sudo yum -y install apr-devel
sudo yum -y install glib2
sudo yum -y install oracle-instantclient-sqlplus
sudo yum -y install boost-thread
sudo yum -y install glib2-devel
sudo yum -y install boost-openmpi
sudo yum -y install strace
sudo yum -y install gfal2-transfer
sudo yum -y install globus-common
sudo yum -y install globus-gssapi-gsi
sudo yum -y install globus-ftp-client
sudo yum -y install is-interface-devel
sudo yum -y install boost-program-options
sudo yum -y install apr
sudo yum -y install boost-system
sudo yum -y install cpp
sudo yum -y install gdb
sudo yum -y install boost
sudo yum -y install glibc-common
sudo yum -y install gfal2
sudo yum -y install globus-openssl-module
sudo yum -y install globus-gsi-credential
sudo yum -y install pkgconfig
sudo yum -y install openldap-devel
sudo yum -y install lcg-CA
sudo yum -y install boost-openmpi-devel
sudo yum -y install gfal2-devel
sudo yum -y install globus-gsi-proxy-ssl
sudo yum -y install globus-xio-popen-driver
sudo yum -y install srm-ifce-debuginfo
sudo yum -y install globus-xio-gsi-driver
sudo yum -y install boost-test
sudo yum -y install openssl


echo "Fetch-crls now"
sudo fetch-crl


echo " Now manually set X509_USER_CERT & X509_USER_KEY to point to your certificate and key"
echo " Run voms-proxy-init -voms dteam"

echo "Set username, password and connection string in /etc/sysconfig/fts3config"

echo "Set the password to connect to the broker in /etc/fts-msg-monitoring.conf"
