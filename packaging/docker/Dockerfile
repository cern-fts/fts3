#
# FTS Docker file
#

FROM centos:7
LABEL maintainer="FTS Team, fts-support@cern.ch, CERN 2021"

# Add EPEL repos
ADD https://linuxsoft.cern.ch/epel/epel-release-latest-7.noarch.rpm /tmp/epel-release-7.noarch.rpm
RUN yum localinstall /tmp/epel-release-7.noarch.rpm -y \
        && yum --enablerepo=*-testing clean all \
        && rm /tmp/epel-release-7.noarch.rpm \
        && rm -rf /var/cache/yum

# Add FTS repo
ARG ftsrepo=https://fts-repo.web.cern.ch/fts-repo/fts3-continuous-el7.repo
ADD $ftsrepo /etc/yum.repos.d/fts.repo

# Add DMC repo
ARG dmcrepo=https://dmc-repo.web.cern.ch/dmc-repo/dmc-ci-el7.repo
ADD $dmcrepo /etc/yum.repos.d/dmc.repo
RUN yum clean all

# Install FTS packages
RUN yum install -y fts-server fts-client fts-mysql fts-server-selinux fts-msg fts-infosys cronie crontabs

# Install Gfal2 packages
RUN yum install -y gfal2-all gfal2-plugin-mock

# Install VOMS packages
RUN yum install -y voms-config-wlcg  voms-config-vo-dteam

# Install supervisor
RUN yum install -y supervisor
COPY "packaging/docker/supervisord" "/etc/supervisor/conf.d/supervisord.conf"

# Log folder
RUN mkdir -p /var/log/fts3 && chown fts3:fts3 /var/log/fts3

# Expose ports
EXPOSE 2170

# Start supervisor
CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/conf.d/supervisord.conf"]
