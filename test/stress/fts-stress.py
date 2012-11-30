#!/usr/bin/env python
#
# Favourite metadata
# meta: manual=true
#
# Launch a bunch of transfers in parallel
#
import random
import subprocess
import sys
from optparse import OptionParser

def loadList(path):
  f = open(path)
  list = []
  for l in f.readlines():
    l = l.strip()
    if len(l) > 0:
      list.append(l)
  f.close() 
  return list

def log(msg):
  print >>sys.stderr, "[LOG]", msg


class Bully:
  """
  Spawn transfers without waiting for the result
  """
  def __init__(self, endpoint, parallel, iterations, sources, destinations):
    self.endpoint     = endpoint
    self.parallel     = parallel
    self.iterations   = iterations
    self.logger       = None
    self.sources      = sources
    self.destinations = destinations

  def setLogger(self, f):
    self.logger = f

  def log(self, msg):
    if (self.logger):
      self.logger(msg)

  def spawn(self, src, dst):
    return subprocess.Popen(['fts-transfer-submit',
                             '-s', self.endpoint,
                             src, dst])

  def __call__(self):
    self.log("Transfers per attack: %d"   % self.parallel)
    self.log("Repeat:               %d"   % self.iterations)
    self.log("Number of sources:    %d"   % len(self.sources))
    self.log("Number of destinations: %d" % len(self.destinations))

    n = 0
    for i in xrange(self.iterations):
      self.log("Iteration %d" % i)
      procs = []
      for p in xrange(self.parallel):
        src = self.sources[random.randrange(len(self.sources))]
        dst = self.destinations[random.randrange(len(self.destinations))]

        transferId = "%d.%d" % (i, p)
        src = src.replace("$ID", transferId)
        dst = dst.replace("$ID", transferId)

        self.log("Spawning transfer %04d '%s' => '%s'" % (p, src, dst))
        procs.append(self.spawn(src, dst))
        n += 1
      for proc in procs:
        self.log("Waiting for pid %d" % proc.pid)
        status = proc.wait() 
        self.log("Pid %d exited with %d" % (proc.pid, status))

    return n 


if __name__ == '__main__':
  parser = OptionParser(usage = "usage: %prog [options] source-list destination-list")
  parser.add_option('-s', '--endpoint', type='string', dest='endpoint',
                    help = 'Endpoint. MANDATORY.')
  parser.add_option('-p', '--parallel', type='int', dest='parallel', default = 100,
                    help = 'Number of parallel requests')
  parser.add_option('-i', '--iterate', type='int', dest='iterations', default = 1,
                    help = 'Repeat the test this number of times')

  (options, args) = parser.parse_args()

  if options.endpoint == None or options.endpoint == '':
    print >>sys.stderr, "Need to specify -s <endpoint>"
    sys.exit(2)

  if len(args) == 0:
    print >>sys.stderr, "Need source and destination lists"
    sys.exit(2)
  elif len(args) == 1:
    print >>sys.stderr, "Missing destination list"
    sys.exit(2)
  elif len(args) > 2:
    print >>sys.stderr, "Too many arguments specified"
    sys.exit(2)

  try:
    srcList = loadList(args[0])
    dstList = loadList(args[1])
  except Exception, e:
    print >>sys.stderr, str(e)
    sys.exit(2)

  bully = Bully(options.endpoint, options.parallel, options.iterations, srcList, dstList)
  bully.setLogger(log)
  log("Starting stress test")
  log("Sent %d pushes" % bully())

  sys.exit(0)

