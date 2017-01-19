FROM centos:7
MAINTAINER CERN

# Add cern repos
ADD http://linux.web.cern.ch/linux/centos7/CentOS-CERN.repo /etc/yum.repos.d/CentOS-CERN.repo
ADD http://linuxsoft.cern.ch/cern/centos/7/os/x86_64/RPM-GPG-KEY-CentOS-7 /tmp/RPM-GPG-KEY-cern
RUN /usr/bin/rpm --import /tmp/RPM-GPG-KEY-cern \
        && /usr/bin/yum --enablerepo=*-testing clean all \
        && rm /tmp/RPM-GPG-KEY-cern \
        && rm -rf /var/cache/yum

# Add EPEL repos
ADD  http://mirror.switch.ch/ftp/mirror/epel/epel-release-latest-7.noarch.rpm  /tmp/epel-release-7.noarch.rpm
RUN  yum localinstall /tmp/epel-release-7.noarch.rpm -y \
        && /usr/bin/yum --enablerepo=*-testing clean all \
        && rm /tmp/epel-release-7.noarch.rpm \
        && rm -rf /var/cache/yum

# add FTS repo
ARG ftsrepo=http://fts-repo.web.cern.ch/fts-repo/fts3-continuous-el7.repo
ADD $ftsrepo /etc/yum.repos.d/fts.repo

#add DMC repo
ARG dmcrepo=http://dmc-repo.web.cern.ch/dmc-repo/dmc-ci-el7.repo
ADD $dmcrepo /etc/yum.repos.d/dmc.repo


#install fts packages
RUN /usr/bin/yum install -y fts-monitoring fts-monitoring-selinux mod_ssl

RUN /usr/bin/yum install -y voms-config-wlcg  voms-config-vo-dteam

#add user
RUN useradd fts3
RUN usermod -a -G fts3 apache

#expose ports

EXPOSE 8449

