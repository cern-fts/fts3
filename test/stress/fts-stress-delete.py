#!/usr/bin/env python
#
# Favourite metadata
# meta: manual=true
#
# Launch a bunch of transfers in parallel
#
import subprocess
import sys
import time
from optparse import OptionParser
from creater import Creater, loadList


def log(msg):
  print >>sys.stderr, "[LOG]", msg


class Bully:
  """
  Spawn transfers without waiting for the result
  """
  def __init__(self, endpoint, parallel, iterations, sources):
    self.endpoint     = endpoint
    self.parallel     = parallel
    self.iterations   = iterations
    self.logger       = None
    self.sources      = sources

  def setLogger(self, f):
    self.logger = f

  def log(self, msg):
    if (self.logger):
      self.logger(msg)

  def _spawn(self, cmdArray, canFail = False):
    log("Spawning %s" % ' '.join(cmdArray))
    proc = subprocess.Popen(cmdArray, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
    rcode = proc.wait()
    if rcode != 0:
      if canFail:
        log.warning(proc.stdout.read())
        log.warning(proc.stdout.read())
        return ''
      else:
        log.error(proc.stdout.read())
        log.error(proc.stderr.read())
        raise Exception("%s failed with exit code %d" % (cmdArray[0], rcode))
    return proc.stdout.read().strip()


  def delete(self, src):
    cmdArray = ['fts-transfer-delete', '-s', self.endpoint, src]
    jobId = []
    jobId = self._spawn(cmdArray)
    return str(jobId)

  def getJobState(self, jobId):
    cmdArray = ['fts-transfer-status', '-s', self.endpoint, jobId]
    return self._spawn(cmdArray)

  def poll(self, jobId):
    state = self.getJobState(jobId)
    remaining = 600
    JobTerminalStates = ['FINISHED', 'FAILED', 'CANCELED', 'FINISHEDDIRTY']
    while state not in JobTerminalStates:
      log("%s %s" % (jobId, state))
      time.sleep(5)
      remaining -= 5
      state = self.getJobState(jobId)
      if remaining <= 0:
        log("Timeout expired, cancelling job")
        self.cancel(jobId)
        raise Exception("Timeout expired while polling")
    return state

  def cancel(self, jobId):
    cmdArray = ['fts-transfer-cancel', '-s', self.endpoint, jobId]
    self.delete(cmdArray)


  def __call__(self):
    self.log("Transfers per attack: %d"   % self.parallel)
    self.log("Repeat:               %d"   % self.iterations)
    self.log("Number of sources:    %d"   % len(self.sources))

    n = 0
    creater = Creater(self.parallel, self.sources)

    for i in xrange(self.iterations):
      self.log("Iteration %d" % i)
      procs = []

      for src in creater:
        self.log("Spawning transfer '%s'" % src)
        jobId = self.delete(src)
        log("Got job id %s" % jobId)
        state = self.poll(jobId)
	log("Finished with %s" % state)
        n += 1

    return n 


if __name__ == '__main__':
  parser = OptionParser(usage = "usage: %prog [options] source-list")
  parser.add_option('-s', '--endpoint', type='string', dest='endpoint',
                    help = 'Endpoint. MANDATORY.')
  parser.add_option('-p', '--parallel', type='int', dest='parallel', default = 1,
                    help = 'Number of parallel requests')
  parser.add_option('-i', '--iterate', type='int', dest='iterations', default = 3,
                    help = 'Repeat the test this number of times')

  (options, args) = parser.parse_args()

  if options.endpoint == None or options.endpoint == '':
    print >>sys.stderr, "Need to specify -s <endpoint>"
    sys.exit(2)

  if len(args) == 0:
    print >>sys.stderr, "Need source"
    sys.exit(2)
  elif len(args) > 1:
    print >>sys.stderr, "Too many arguments specified"
    sys.exit(2)

  try:
    srcList = loadList(args[0])
  except Exception, e:
    print >>sys.stderr, str(e)
    sys.exit(2)

  bully = Bully(options.endpoint, options.parallel, options.iterations, srcList)
  bully.setLogger(log)
  log("Starting stress test")
  log("Sent %d pushes" % bully())

  sys.exit(0)

