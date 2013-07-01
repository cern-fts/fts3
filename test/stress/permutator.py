#!/usr/bin/env python
#
# Permutates a list of source and destination files
#
import random
import sys
from datetime import datetime
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


class Permutator:
  """
  Iterable. Shuffle source and destination lists.
  """
  def __init__(self, number, sources, destinations):
    self.number       = number
    self.sources      = sources
    self.destinations = destinations
    self.runIndex     = 0


  def __iter__(self):
    random.seed()
    timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
    for i in xrange(self.number):
      src = self.sources[random.randrange(len(self.sources))]
      dst = self.destinations[random.randrange(len(self.destinations))]

      transferId = "%d.%d" % (self.runIndex, i)

      src = src.replace("$ID", transferId).replace("$TIMESTAMP", timestamp)
      dst = dst.replace("$ID", transferId).replace("$TIMESTAMP", timestamp)

      yield (src, dst)
    self.runIndex += 1


if __name__ == '__main__':
  parser = OptionParser(usage = "usage: %prog [options] source-list destination-list")
  parser.add_option('-n', None, type='int', dest='number', default = 1,
                    help = 'Number of permutations to generate')

  (options, args) = parser.parse_args()

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

  permutator = Permutator(options.number, srcList, dstList)
  for pair in permutator:
    print ' '.join(pair)
    
  sys.exit(0)

