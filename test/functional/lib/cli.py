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
        # Build the submission file
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

        # Spawn the transfer
        cmdArray = ['fts-transfer-submit',
                    '-s', config.Fts3Endpoint,
                    '--job-metadata', label, 
                    '--json-submission', '-f', submission.name] + extraArgs
        jobId = self._spawn(cmdArray)
        os.unlink(submission.name)
        return jobId


    def getJobState(self, jobId):
        cmdArray = ['fts-transfer-status', '-s', config.Fts3Endpoint, jobId]
        return self._spawn(cmdArray)

    def setPriority(self, jobId,priority):

#        verbose = random.randint(1,5)
        cmdArray = ['fts-set-priority', '-s',config.Fts3Endpoint, jobId, str(priority)]
#        print "Prior = ", priority
        logging.info("Set priority " + str(priority) + " to job with ID " + str(jobId) + ".")
        self._spawn(cmdArray)       

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
        self._spawn(cmdArray)
    
    def delete(self, transfers):
        deletion = tempfile.NamedTemporaryFile(delete = False, suffix = '.deletion')
        deletion.write(transfers)
        print str(transfers)
        deletion.close()
        
        cmdArray = ['fts-transfer-delete',
                    '-s', config.Fts3Endpoint,
                    '-f', deletion.name]
        jobId = self._spawn(cmdArray)
        return jobId
        
    def getFileInfo(self, jobId, detailed = False):
        cmdArray = ['fts-transfer-status', '-s', config.Fts3Endpoint, 
                    '--json', '-l', jobId]
        if detailed:
            cmdArray += ['--detailed']
        out = self._spawn(cmdArray)
        detailedState = json.loads(out)
        fileStates = detailedState['job'][0]['files']
        pairDict = {}
        for f in fileStates:
            src = f['source']
            dst = f['destination']
            pairDict[(src,dst)] = f
        logging.debug(str(pairDict))
        return pairDict


    def getConfig(self, sourceSE, destSE = None):
        cmdArray = ['fts-config-get', '-s', config.Fts3Endpoint, sourceSE]
        if destSE:
            cmdArray.append(destSE)
        # May fail if there is no configuration available
        try:
            return json.loads(self._spawn(cmdArray, canFail = True))
        except:
            return {}


    def setConfig(self, cfg):
        cmdArray = ['fts-config-set', '-s', config.Fts3Endpoint, "'" + json.dumps(cfg) + "'"]
        self._spawn(cmdArray)


    def delConfig(self, cfg):
        cmdArray = ['fts-config-del', '-s', config.Fts3Endpoint, "'" + json.dumps(cfg) + "'"]
        self._spawn(cmdArray)


    def banSe(self, se, status = None):
        if status == None:
            cmdArray = ['fts-set-blacklist', '-s', config.Fts3Endpoint, 'se', se, 'ON']
        elif status == "CANCEL" or status == "WAIT":
            cmdArray = ['fts-set-blacklist', '-s', config.Fts3Endpoint, 'se', se, '--status=%s' % status, 'ON']
        else:
            raise Exception("Wrong status argument in fts-set-blacklist")
        self._spawn(cmdArray)


    def unbanSe(self, se, status = None):
        if status == None:
            cmdArray = ['fts-set-blacklist', '-s', config.Fts3Endpoint, 'se', se, 'OFF']
        elif status == "CANCEL" or status == "WAIT":
            cmdArray = ['fts-set-blacklist', '-s', config.Fts3Endpoint, 'se', se, '--status=%s' % status, 'OFF']
        else:
            raise Exception("Wrong status argument in fts-set-blacklist")
        self._spawn(cmdArray)


    def banDn(self, dn):
        cmdArray = ['fts-set-blacklist', '-s', config.Fts3Endpoint, 'dn', dn, 'ON']
        self._spawn(cmdArray)

    def unBanDn(self, dn):
        cmdArray = ['fts-set-blacklist', '-s', config.Fts3Endpoint, 'dn', dn, 'OFF']
        self._spawn(cmdArray)

    def setDebug(self, src, dst, status):
        cmdArray = ['fts-set-debug','-s', config.Fts3Endpoint, src, dst, status]
        self._spawn(cmdArray)
