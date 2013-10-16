import config
import json
import fts3
import inspect
import logging
import os
import subprocess
import tempfile
import time


class Cli:

    def submit(self, transfers, extraArgs = []):
        """
        Spawns a transfer and returns the job ID
        """
        # Build the submission file
        submission = tempfile.NamedTemporaryFile(delete = False, suffix = '.submission')
        submission.write(json.dumps({'Files': transfers}))
        submission.close()

        # Label the job
        caller = inspect.stack()[1][3]
        labeldict = {'label': config.TestLabel, 'test': caller}
        label = json.dumps(labeldict)

        # Spawn the transfer
        cmdArray = ['fts-transfer-submit',
                    '-s', config.Fts3Endpoint,
                    '--retry', '0',
                    '--job-metadata', label, 
                    '--new-bulk-format', '-f', submission.name] + extraArgs
        logging.debug("Spawning %s" % ' '.join(cmdArray))
        proc = subprocess.Popen(cmdArray, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        rcode = proc.wait()
        if rcode != 0:
            logging.error(proc.stdout.read())
            logging.error(proc.stderr.read())
            raise Exception("fts-transfer-submit failed with exit code %d" % rcode)
        jobId = proc.stdout.read().strip()

        os.unlink(submission.name)

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
                logging.error("Timeout expired, cancelling job")
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


    def getFileInfo(self, jobId):
        cmdArray = ['fts-transfer-status', '-s', config.Fts3Endpoint, 
                    '--json', '-l', jobId]
        logging.debug("Spawning %s" % ' '.join(cmdArray))
        proc = subprocess.Popen(cmdArray, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        rcode = proc.wait()
        if rcode != 0:
            logging.error(proc.stdout.read())
            logging.error(proc.stderr.read())
            raise Exception("fts-transfer-status failed with exit code %d" % rcode)
        jsonDetailedState = proc.stdout.read().strip()
        detailedState = json.loads(jsonDetailedState)
        fileStates = detailedState['job'][0]['files']
        pairDict = {}
        for f in fileStates:
            src = f['source']
            dst = f['destination']
            pairDict[(src,dst)] = f
        return pairDict


    def getConfig(self, sourceSE, destSE = None):
        cmdArray = ['fts-config-get', '-s', config.Fts3Endpoint, sourceSE]
        if destSE:
            cmdArray.append(destSE)
        logging.debug("Spawning %s" % ' '.join(cmdArray))
        proc = subprocess.Popen(cmdArray, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        rcode = proc.wait()
        if rcode != 0:
            return None
        return json.loads(proc.stdout.read().strip())


    def setConfig(self, cfg):
        cmdArray = ['fts-config-set', '-s', config.Fts3Endpoint, "'" + json.dumps(cfg) + "'"]
        logging.debug("Spawning %s" % ' '.join(cmdArray))
        proc = subprocess.Popen(cmdArray, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        rcode = proc.wait()
        if rcode != 0:
            logging.error(proc.stdout.read())
            logging.error(proc.stderr.read())
            raise Exception("fts-config-set failed with exit code %d" % rcode)


    def delConfig(self, cfg):
        cmdArray = ['fts-config-del', '-s', config.Fts3Endpoint, "'" + json.dumps(cfg) + "'"]
        logging.debug("Spawning %s" % ' '.join(cmdArray))
        proc = subprocess.Popen(cmdArray, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        rcode = proc.wait()
        if rcode != 0:
            logging.error(proc.stdout.read())
            logging.error(proc.stderr.read())
            raise Exception("fts-config-del failed with exit code %d" % rcode)
