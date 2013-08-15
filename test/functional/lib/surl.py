import config
import gfal2
import logging
import subprocess
import tempfile
import time
from urlparse import urlparse



class Surl:
	def __init__(self):
		self.gctx = gfal2.creat_context()


	def generate(self, base):
		"""
		Generates a file surl located under base
		i.e. srm://host/path => srm://host/path/file.temp
		"""
		return "%s/%s.%f" % (base, 'fts3tests', time.time())


	def _fillFile(self, fd, size):
		randfd = open('/dev/urandom', 'r')
		remaining = size
		while remaining > 0:
			nread = min(1024, remaining)
			buffer = randfd.read(nread)
			fd.write(buffer)
			remaining -= nread
		randfd.close()


	def create(self, surl, size = 1048576): 
		"""
		Creates a file with the given file size in the
		given location.
		"""
		local = tempfile.NamedTemporaryFile(delete = True)
		logging.debug("Generating a %d bytes files in %s" % (size, local.name))
		self._fillFile(local, size)
	
		logging.debug("Uploading %s => %s" % (local.name, surl))

		params = self.gctx.transfer_parameters()
		params.timeout = config.Timeout
		params.checksum_check = True
		self.gctx.filecopy(params, 'file://' + local.name, surl)

		checksum = self.gctx.checksum(surl, config.Checksum)
		logging.debug("File uploaded with checksum %s" % checksum)
		return config.Checksum + ':' + checksum


	def unlink(self, surl):
		self.gctx.unlink(surl)


	def checksum(self, surl):
		return config.Checksum + ':' + self.gctx.checksum(surl, config.Checksum)


