import gfal2
import subprocess
import tempfile
import time
import uuid
from urlparse import urlparse
from optparse import OptionParser

class Surl:
	def __init__(self):
		self.gctx = gfal2.creat_context()
		self.gctx.set_opt_string('SRM PLUGIN', 'TURL_PROTOCOLS', 'gsiftp')

	def generate(self, base):
		"""
		Generates a file surl located under base
		i.e. srm://host/path => srm://host/path/file.temp
		"""
		return "%s/%s.%s" % (base, 'fts3tests', str(uuid.uuid4()))


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
		print("Generating a %d bytes files in %s" % (size, local.name))
		self._fillFile(local, size)
		print("Uploading %s => %s" % (local.name, surl))

		params = self.gctx.transfer_parameters()
		params.timeout = 600
		params.overwrite = True
		params.checksum_check = False
		self.gctx.filecopy(params, 'file://' + local.name, surl)

	def unlink(self, surl):
		self.gctx.unlink(surl)
