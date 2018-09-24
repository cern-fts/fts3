#
# Copyright (c) CERN 2017
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

FROM centos:7

# Required repos
ADD "http://dmc-repo.web.cern.ch/dmc-repo/dmc-ci-el7.repo" "/etc/yum.repos.d/"
ADD "http://dmc-repo.web.cern.ch/dmc-repo/dmc-externals-el7.repo" "/etc/yum.repos.d/"
ADD "http://fts-repo.web.cern.ch/fts-repo/fts3-continuous-el7.repo" "/etc/yum.repos.d/"
ADD "http://repository.egi.eu/sw/production/cas/1/current/repo-files/EGI-trustanchors.repo" "/etc/yum.repos.d/"

RUN yum install -y epel-release yum-utils

# Build requirements and static analysis tools
RUN yum install -y gcc gcc-c++ vera++ rats cppcheck
RUN yum-builddep -y fts-server

# Run dependencies
RUN yum install -y gfal2-plugin-mock git lcov \
    ca-policy-egi-core supervisor voms-config-vo-dteam voms-clients \
    python-virtualenv python2-pip

# We also need rest, with a patched config file that allows values to be passed via environment variables
RUN yum install -y fts-rest MySQL-python
EXPOSE 8446

# And monitoring, makes life easier
RUN yum install -y fts-monitoring
EXPOSE 8449
RUN chmod a+r /etc/fts3web/fts3web.ini

# Copy sources
COPY cmake  "/usr/src/fts3/cmake"
COPY test   "/usr/src/fts3/test"
COPY doc    "/usr/src/fts3/doc"
COPY src    "/usr/src/fts3/src"
COPY CMakeLists.txt "/usr/src/fts3/"
WORKDIR "/usr/src/fts3"

# Static analysis
COPY qa/static-analysis.sh /
RUN /static-analysis.sh "/usr/src/fts3" "/usr/src/fts3"

# Build fts3 with coverage
RUN CFLAGS=--coverage CXXFLAGS=--coverage cmake . \
        -DMAINBUILD=ON -DSERVERBUILD=ON -DCLIENTBUILD=ON -DMYSQLBUILD=ON \
        -DTESTBUILD=ON -DCMAKE_INSTALL_PREFIX=""
RUN make -j2
RUN make install

# Run unit tests and extract their coverage
# (this also validates the build)
ADD "https://raw.github.com/eriwen/lcov-to-cobertura-xml/master/lcov_cobertura/lcov_cobertura.py" "/usr/bin/lcov_cobertura.py"
RUN chmod a+x "/usr/bin/lcov_cobertura.py"
RUN ./test/unit/unit --log_level=all
RUN lcov --directory . --capture --output-file="coverage-unit.info"
RUN lcov_cobertura.py "coverage-unit.info" -b "." -e ".+usr.include." -o "coverage-unit.xml"

# Tests helper
RUN yum install -y sudo
ADD "qa/tests-helper.sh" "/"

# Configure supervisord
ADD "qa/watchdog.py" "/usr/bin"
ADD "qa/supervisord.conf" "/etc/supervisord.conf"

# Volumes
VOLUME ["/logs", "/coverage", "/etc/grid-security"]

# Copy unit test coverage and run supervisor when the container starts
ADD "qa/entrypoint.sh" "/entrypoint.sh"
ENTRYPOINT ["/entrypoint.sh"]
