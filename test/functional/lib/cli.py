import config
import fts3
import logging
import subprocess
import time


class Cli:
	def submit(self, source, destination, parameters = [], checksum = None):
		"""
		Spawns a transfer and returns the job ID
		Source is assumed to exist
		"""
		cmdArray = ['fts-transfer-submit',
                    '-s', config.Fts3Endpoint,
                    '--job-metadata', config.TestLabel] + parameters
		cmdArray += [source, destination]
		if checksum:
			cmdArray.append(config.Checksum + ':' + checksum)
		logging.debug("Spawning %s" % ' '.join(cmdArray))
		proc = subprocess.Popen(cmdArray, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
		rcode = proc.wait()
		if rcode != 0:
			logging.error(proc.stdout.read())
			logging.error(proc.stderr.read())
			raise Exception("fts-transfer-submit failed with exit code %d" % rcode)
		jobId = proc.stdout.read().strip()
		return jobId


	def getJobState(self, jobId):
		cmdArray = ['fts-transfer-status', '-s', config.Fts3Endpoint, jobId]
		logging.debug("Spawning %s" % ' '.join(cmdArray))
		proc = subprocess.Popen(cmdArray, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
		rcode = proc.wait()
		if rcode != 0:
			logging.error(proc.stdout.read())
			logging.error(proc.stderr.read())
			raise Exception("fts-transfer-status failed with exit code %d" % rcode)
		state = proc.stdout.read().strip()
		return state


	def poll(self, jobId):
		state = self.getJobState(jobId)
		remaining = config.Timeout
		while state not in fts3.JobTerminalStates:
			logging.debug("%s %s" % (jobId, state))
			time.sleep(config.PollInterval)
			remaining -= config.PollInterval
			state = self.getJobState(jobId)
			if remaining <= 0:
				self.error("Timeout expired, cancelling job")
				self.cancel(jobId)
				raise Exception("Timeout expired while polling")

		return state


	def cancel(self, jobId):
		cmdArray = ['fts-transfer-cancel', '-s', config.Fts3Endpoint, jobId]
		logging.debug("Spawning %s" % ' '.join(cmdArray))
		proc = subprocess.Popen(cmdArray, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
		rcode = proc.wait()
		if rcode != 0:
			logging.error(proc.stdout.read())
			logging.error(proc.stderr.read())
			raise Exception("fts-transfer-cancel failed with exit code %d" % rcode)

