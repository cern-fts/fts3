import collections
import logging
import types
import unittest
import cli, storage, surl 


class TestBase(unittest.TestCase):

	def __init__(self, *args, **kwargs): 
		super(TestBase, self).__init__(*args, **kwargs)
		self.surl   = surl.Surl()
		self.client = cli.Cli()


	def setUp(self):
		self.transfers = []
		for (srcSa, dstSa) in storage.getStoragePairs():
			srcSurl = self.surl.generate(srcSa)
			dstSurl = self.surl.generate(dstSa)
			self.transfers.append((srcSurl, dstSurl))


	def _flatten(self, l):
		if type(l) is types.StringType:
			yield l
		else:
			for i in l:
				if isinstance(i, collections.Iterable):
					for sub in self._flatten(i):
						yield sub
				else:
					yield i


	def tearDown(self):
		allFiles = self._flatten(self.transfers)
		for f in allFiles:
			try:
				logging.debug("Removing %s" % f)
				self.surl.unlink(f)
			except Exception, e:
				logging.warning(e)


