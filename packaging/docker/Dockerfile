FROM centos:7
MAINTAINER CERN

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

RUN yum clean all

#install fts packages
RUN /usr/bin/yum install -y fts-server fts-client fts-mysql fts-server-selinux fts-msg fts-infosys cronie crontabs

#install gfal2
RUN /usr/bin/yum install -y gfal2-all gfal2-plugin-mock

#install the voms stuff

RUN /usr/bin/yum install -y voms-config-wlcg  voms-config-vo-dteam

#install supervisor
RUN yum install -y supervisor

COPY "packaging/docker/supervisord" "/etc/supervisor/conf.d/supervisord.conf"

#log folder
RUN mkdir -p /var/log/fts3 && chown fts3:fts3 /var/log/fts3

#expose ports

EXPOSE 2170

# Start supervisor
CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/conf.d/supervisord.conf"]

