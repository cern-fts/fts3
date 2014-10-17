#!/usr/bin/env python
#
# Creates a list of source files
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


class Creater:
  """
  Iterable. Shuffle source lists.
  """
  def __init__(self, number, sources):
    self.number       = number
    self.sources      = sources
    self.runIndex     = 0


  def __iter__(self):
    timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
    for i in xrange(self.number):
      src = self.sources
      src = str('\n'.join(src))
      transferId = "%d" % (self.runIndex)
      src = src.replace("$ID", transferId).replace("$TIMESTAMP", timestamp)
      yield str(src)
      self.runIndex += 1


if __name__ == '__main__':
  parser = OptionParser(usage = "usage: %prog [options] source-list")
  parser.add_option('-n', None, type='int', dest='number', default = 1,
                    help = 'Number of creations to generate')

  (options, args) = parser.parse_args()

  if len(args) == 0:
    print >>sys.stderr, "Need source lists"
    sys.exit(2)
  elif len(args) > 1:
    print >>sys.stderr, "Too many arguments specified"
    sys.exit(2)

  try:
    srcList = loadList(args[0])
  except Exception, e:
    print >>sys.stderr, str(e)
    sys.exit(2)

  creater = Creater(options.number, srcList)
  for source in creater:
    print source
    
  sys.exit(0)

