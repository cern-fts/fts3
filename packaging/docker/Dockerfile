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
ADD  http://mirror.switch.ch/ftp/mirror/epel/7/x86_64/e/epel-release-7-8.noarch.rpm  /tmp/epel-release-7-8.noarch.rpm
RUN  yum localinstall /tmp/epel-release-7-8.noarch.rpm -y \
        && /usr/bin/yum --enablerepo=*-testing clean all \
        && rm /tmp/epel-release-7-8.noarch.rpm \
        && rm -rf /var/cache/yum

# add FTS repo
ADD http://grid-deployment.web.cern.ch/grid-deployment/dms/fts3/repos/fts3-continuous-el7.repo /etc/yum.repos.d/fts.repo

#add DMC repo

ADD http://grid-deployment.web.cern.ch/grid-deployment/dms/lcgutil/repos/dmc-ci-el7.repo /etc/yum.repos.d/dmc.repo

# add trustanchors
ADD http://repository.egi.eu/sw/production/cas/1/current/repo-files/EGI-trustanchors.repo /etc/yum.repos.d/EGI-trustanchors.repo

#add install certificates ( they will go then in a separate volume)
RUN /usr/bin/yum install -y ca-policy-egi-core fetch-crl

#install fts packages
RUN /usr/bin/yum install -y fts-server fts-client fts-mysql fts-server-selinux fts-msg fts-infosys

#install gfal2
RUN /usr/bin/yum install -y gfal2-all gfal2-plugin-mock

#install the voms stuff

RUN /usr/bin/yum install -y voms-config-wlcg  voms-config-vo-dteam

#install supervisor
RUN yum install -y supervisor

COPY "supervisord" "/etc/supervisor/conf.d/supervisord.conf"

#expose ports

EXPOSE 8443
EXPOSE 2170

# Start supervisor
CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/conf.d/supervisord.conf"]

