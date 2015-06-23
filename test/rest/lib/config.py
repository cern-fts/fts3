# FTS3 tests configuration
import os
import sys

# FTS3 server
Fts3Host = os.environ.get('FTS3_HOST', 'fts3-devel.cern.ch')
Fts3Port = int(os.environ.get('FTS3_REST_PORT', 8446))
Fts3Endpoint = 'https://%s:%d' % (Fts3Host, Fts3Port)

# VO
Vo = os.environ.get('VO', 'dteam')

# Tests label (passed in the metadata)
TestLabel = 'fts3-tests'

# Polling interval in seconds
PollInterval = 5

# Job timeout in seconds
Timeout = 600

# Checksum algorithm
Checksum = 'ADLER32'

# Dictionary that can be used to parametrize the storage areas
StorageParametrization = {
    'vo': Vo
}

# Storage areas
# Give the full path under which the files will be stored
# You can use string formatting to replace some parts of it,
# i.e. %(vo)s
# They will be replaced with the values of StorageParametrization
StorageAreaPairs = [
    (
        'gsiftp://se2.egi.cesga.es/dpm/egi.cesga.es/home/%(vo)s/',
        'gsiftp://sbgse1.in2p3.fr/dpm/in2p3.fr/home/%(vo)s/'
    ),
    (
        'srm://srm-dteam.gridpp.rl.ac.uk:8443/srm/mana?SFN=/castor/ads.rl.ac.uk/test/%(vo)s',
        'srm://sbgse1.in2p3.fr:8446/srm/managerv2?SFN=/dpm/in2p3.fr/home/%(vo)s/'
    ),
#    (
#        'davs://lxfsra04a04.cern.ch/dpm/cern.ch/home/%(vo)s/',
#        'davs://lxfsra10a01.cern.ch/dpm/cern.ch/home/%(vo)s/'
#    )
]

# Logging level
import logging
try:
    debug = int(os.environ.get('DEBUG', 0))
except:
    debug = 1
if debug:
    logLevel = logging.DEBUG
else:
    logLevel = logging.INFO

# Let's make it nicer
logging.basicConfig(level = logLevel, format = '%(levelname)s %(message)s')

if sys.stdout.isatty():
    logging.addLevelName(logging.DEBUG, "\033[1;2m%-8s\033[1;m" % logging.getLevelName(logging.DEBUG))
    logging.addLevelName(logging.INFO, "\033[1;34m%-8s\033[1;m" % logging.getLevelName(logging.INFO))
    logging.addLevelName(logging.ERROR, "\033[1;31m%-8s\033[1;m" % logging.getLevelName(logging.ERROR))
    logging.addLevelName(logging.WARNING, "\033[1;33m%-8s\033[1;m" % logging.getLevelName(logging.WARNING))


