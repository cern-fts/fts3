import config
import json
import fts3
import inspect
import logging
import os
import subprocess
import tempfile
import time
import random


class Cli:
    
    def _spawn(self, cmdArray, canFail = False):
        logging.debug("Spawning %s" % ' '.join(cmdArray))
        proc = subprocess.Popen(cmdArray, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        rcode = proc.wait()
        if rcode != 0:
            if canFail:
                logging.warning(proc.stdout.read())
                logging.warning(proc.stdout.read())
                return ''
            else:
                logging.error(proc.stdout.read())
                logging.error(proc.stderr.read())
                raise Exception("%s failed with exit code %d" % (cmdArray[0], rcode))
        return proc.stdout.read().strip()


    def submit(self, transfers, extraArgs = []):
        """
        Spawns a transfer and returns the job ID
        """
        submission = tempfile.NamedTemporaryFile(delete = False, suffix = '.submission')
        submission.write(json.dumps({'Files': transfers}))
        submission.close()

        # If retry is not explicit, set it to 0
        if '--retry' not in extraArgs:
            extraArgs += ['--retry', '-1']

        # Label the job
        caller = inspect.stack()[1][3]
        labeldict = {'label': config.TestLabel, 'test': caller}
        label = json.dumps(labeldict)
        if '--retry' not in extraArgs:
            extraArgs += ['--retry', '-1']
        # Spawn the transfer
        cmdArray = ['fts-rest-transfer-submit','-j',
                    '-s', config.Fts3Endpoint,
                    '--job-metadata', label,
                     '-f', submission.name] + extraArgs
        jobId = json.loads(self._spawn(cmdArray))
        os.unlink(submission.name)
        return jobId


    def getJobState(self, jobId):
        cmdArray = ['fts-rest-transfer-status', '-s', config.Fts3Endpoint, jobId]
        result =  self._spawn(cmdArray)
        for line in result.split("\n"):
            if "Status" in line:
                return line.split(":")[1].strip()
        return None

    def poll(self, jobId):
        state = self.getJobState(jobId)
        remaining = config.Timeout
        while state not in fts3.JobTerminalStates:
            logging.debug("[%d] %s %s" % (remaining, jobId, state))
            time.sleep(config.PollInterval)
            remaining -= config.PollInterval
            state = self.getJobState(jobId)
            if remaining <= 0:
                logging.error("Timeout expired, cancelling job")
                self.cancel(jobId)
                raise Exception("Timeout expired while polling")
        return state


    def cancel(self, jobId):
        cmdArray = ['fts-rest-transfer-cancel', '-s', config.Fts3Endpoint, jobId]
        self._spawn(cmdArray)


    def getFileInfo(self, jobId, detailed = False):
        cmdArray = ['fts-rest-transfer-status', '-s', config.Fts3Endpoint, '-j', jobId]
        out =  self._spawn(cmdArray)
        detailedState = json.loads(out)
        fileStates = detailedState['files']
        pairDict = {}
        for f in fileStates:
            src = f['source_surl']
            dst = f['dest_surl']
            pairDict[(src,dst)] = f
        return pairDict


    def redelegate(self, force = False):
        cmdArray = ['fts-rest-delegate', '-v', '-s', config.Fts3Endpoint]
        if force:
            cmdArray.append('--force')
        out = self._spawn(cmdArray)
        for line in out.split('\n'):
            logging.debug(line)  
