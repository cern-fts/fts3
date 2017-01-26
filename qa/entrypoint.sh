#!/usr/bin/env bash
set -x

# Recover unit test coverage and static analysis
cp -v /usr/src/fts3/*.xml /usr/src/fts3/*.info "/coverage"

# Set the permissions properly, so the user and cov tools can access the artifacts
chown ${FTS3_USER}.${FTS3_GROUP} -R "/coverage" "/logs" "/usr/src/fts3"

# Prepare configuration files
mkdir -p "/var/lib/fts3"
chown root.${FTS3_GROUP} "/var/lib/fts3"
chmod g+w "/var/lib/fts3"

sed "s/user=fts3 group=fts3/user=${FTS3_USER} group=${FTS3_GROUP}/g" -i /etc/httpd/conf.d/fts3rest.conf
sed "s?WSGISocketPrefix run/wsgi?WSGISocketPrefix /var/lib/fts3/wsgi?g" -i /etc/httpd/conf.d/fts3rest.conf
sed "s/User apache/User ${FTS3_USER}/g" -i /etc/httpd/conf/httpd.conf
sed "s/Group apache/Group ${FTS3_GROUP}/g" -i /etc/httpd/conf/httpd.conf
sed "s?/var/log/fts3rest/fts3rest.log?/logs/fts3rest.log?g" -i /etc/fts3/fts3rest.ini

cat > /etc/fts3/fts3config <<EOF
User=${FTS3_USER}
Group=${FTS3_GROUP}

DbType=mysql
DbUserName=${MYSQL_USER}
DbPassword=${MYSQL_PASSWORD}
DbConnectString=fts3-qa-mysql/${MYSQL_DATABASE}

Alias=fts3cov.cern.ch

Infosys=lcg-bdii.cern.ch:2170
InfoProviders=glue1

AuthorizedVO=*
SiteName=FTS-DEV

MonitoringMessaging=true
TransferLogDirectory=/logs/transfers
ServerLogDirectory=/logs/
LogLevel=DEBUG

CheckStalledTransfers = true

MinRequiredFreeRAM = 50

StagingWaitingFactor=30
StagingBulkSize=200
StagingConcurrentRequests=1000

[roles]
Public = all:*
EOF
chmod 0644 /etc/fts3/fts3config

cat > /etc/fts3/fts-msg-monitoring.conf << EOF
ACTIVE=true
BROKER=${STOMP_HOST}
COMPLETE=transfer.fts_monitoring_complete
STATE=transfer.fts_monitoring_state
START=transfer.fts_monitoring_start
LOGFILEDIR=/logs/
LOGFILENAME=msg.log
TOPIC=true
TTL=24
USE_BROKER_CREDENTIALS=true
PASSWORD=${STOMP_PASSWORD}
USERNAME=${STOMP_USER}
FQDN=fts3cov.cern.ch
EOF
chmod 0644 /etc/fts3/fts-msg-monitoring.conf

# Clean coverage information
lcov --directory "/usr/src/fts3" --zerocounters

# Run servers
_term() {
    echo "Caught SIGTERM signal!"
    child=`cat "/tmp/supervisord.pid"`
    kill -SIGTERM $child 2>/dev/null
    wait $child
}

_cov() {
    lcov --directory "/usr/src/fts3" --capture --output-file="/coverage/coverage-integration.info"
    lcov_cobertura.py "/coverage/coverage-integration.info" --base-dir "/usr/src/fts3" -e ".+usr.include." -o "/coverage/coverage-integration.xml"
    # Combine
    lcov --add-tracefile "/coverage/coverage-unit.info" -a "/coverage/coverage-integration.info" -o "/coverage/coverage-overall.info"
    lcov_cobertura.py "/coverage/coverage-overall.info" --base-dir "/usr/src/fts3" -e ".+usr.include." -o "/coverage/coverage-overall.xml"
}

trap _term SIGTERM
trap _cov EXIT

/usr/bin/supervisord --configuration "/etc/supervisord.conf" --pidfile "/tmp/supervisord.pid" &
wait $!
