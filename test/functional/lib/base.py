import itertools
import logging
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


	def tearDown(self):
		allFiles = set(list(itertools.chain(*self.transfers)))
		for f in allFiles:
			try:
				logging.debug("Removing %s" % f)
				self.surl.unlink(f)
			except Exception, e:
				logging.warning(e)


