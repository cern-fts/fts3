import collections
import logging
import types
import cli, storage, surl 


class TestBase():

	def __init__(self, *args, **kwargs): 
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



	def run(self):
		testMethods = [getattr(self, method) for method in dir(self) if method.startswith('test_') and callable(getattr(self, method))]
		nErrors = 0
		for test in testMethods:
			try:
				self.setUp()
				test()
			except Exception, e:
				logging.error(str(e))
				nErrors += 1
			self.tearDown()
		logging.info("%d test executed, %d failed" % (len(testMethods), nErrors))
		return nErrors


	def assertEqual(self, expected, value):
		if expected != value:
			raise AssertionError("Expected '%s', got '%s'" % (str(expected), str(value)))
		logging.info("Assertion '%s' == '%s' OK" % (str(expected), str(value)))


	def assertRaises(self, excType, method, *args, **kwargs):
		try:
			method(*args, **kwargs)
			raise AssestionError("Expected exception '%s', none raised" % (excType.__name__))
		except Exception, e:
			if not isinstance(e, excType):
				raise AssertionError("Expected exception '%s', got '%s'" % (excType.__name__, type(e).__name__))
			logging.info("Exception '%s' raised as expected" % excType.__name__)


