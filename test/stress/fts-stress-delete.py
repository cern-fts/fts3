#!/usr/bin/env python
#
# Favourite metadata
# meta: manual=true
#
# Launch and times bulk deletions
#

import errno
import gfal2
import logging
import os
import subprocess
import sys
import time, surl
import tempfile
from optparse import OptionParser


log = logging.getLogger(__name__)
#gfal2.set_verbose(gfal2.verbose_level.debug)


class DeletionBully:
    """
    Spawn transfers without waiting for the result
    """

    def __init__(self, endpoint, bulk_size, iterations, base_surl, create_files):
        self.endpoint = endpoint
        self.bulk_size = bulk_size
        self.iterations = iterations
        self.base_surl = base_surl
        self.create_files = create_files
        self.gfal2 = gfal2.creat_context()

        if self.base_surl[-1] != '/':
            self.base_surl += '/'

    def _spawn(self, cmd_array, can_fail=False):
        """
        Utility wrapping subprocess
        """
        log.debug("Spawning %s" % ' '.join(cmd_array))
        proc = subprocess.Popen(cmd_array, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        rcode = proc.wait()
        if rcode != 0:
            if can_fail:
                log.warning(proc.stdout.read())
                log.warning(proc.stdout.read())
                return ''
            else:
                log.error(proc.stdout.read())
                log.error(proc.stderr.read())
                raise Exception("%s failed with exit code %d" % (cmd_array[0], rcode))
        return proc.stdout.read().strip()

    def delete(self, file_name):
        """
        Trigger fts-transfer-felete
        """
        cmdArray = ['fts-transfer-delete', '-s', self.endpoint, '-f', file_name]
        jobId = self._spawn(cmdArray)
        return str(jobId)

    def get_job_state(self, job_id):
        """
        Get the state of the job_id
        """
        cmd_array = ['fts-transfer-status', '-s', self.endpoint, job_id]
        return self._spawn(cmd_array)

    def poll(self, job_id):
        """
        Poll the job_id until it finishes
        """
        state = self.get_job_state(job_id)
        remaining = 10000
        JobTerminalStates = ['FINISHED', 'FAILED', 'CANCELED', 'FINISHEDDIRTY']
        while state not in JobTerminalStates:
            log.debug("%s %s" % (job_id, state))
            time.sleep(5)
            remaining -= 5
            state = self.get_job_state(job_id)
            if remaining <= 0:
                log.error("Timeout expired, cancelling job")
                self.cancel(job_id)
                raise Exception("Timeout expired while polling")
        return state

    def cancel(self, job_id):
        """
        Cancel the job with the given job ID
        """
        cmd_array = ['fts-transfer-cancel', '-s', self.endpoint, job_id]
        self._spawn(cmd_array)

    def populate_base_surl(self, file_list):
        """
        Create self.bulk_size files in the destination directory
        """
        file_list.seek(0)
        log.info("Populating directory")
        for surl in file_list:
            log.debug("Creating %s" % surl.strip())
            try:
                self.gfal2.filecopy("file://etc/hosts", surl.strip())
            except gfal2.GError, e:
                log.warn(str(e))

    def run_test(self):
        """
        Run the tests, measure the time
        """
        log.info("Bulk size: %d" % self.bulk_size)
        log.info("Repeat: %d" % self.iterations)
        log.info("Create the files first: %s" % self.create_files)
        if self.create_files:
            log.warning("The creation of the files may take a long time!")

        # Generate file list
        file_list = tempfile.NamedTemporaryFile(mode="w+", prefix="fts3_bulk_delete_", delete=True)
        for i in xrange(self.bulk_size):
            print >>file_list, "%s%s_%05d" % (self.base_surl, "file", i)
        file_list.flush()

        # Run iterations
        acc_time = 0
        for round in xrange(self.iterations):
            log.info("Round %d/%d" % (round + 1, self.iterations))

            self.gfal2 = gfal2.creat_context()

            # Create first
            if self.create_files:
                self.populate_base_surl(file_list)

            # Trigger the bulk deletion
            job_id = self.delete(file_list.name)
            log.info("Got job ID %s" % job_id)

            # Measure the time until we get a terminal state
            start = time.time()
            state = self.poll(job_id)
            log.info("Finished with state %s" % state)
            end = time.time()
            acc_time += (end - start)

        total_files = self.iterations * self.bulk_size
        log.info("Deleted a total number of %d files" % total_files)
        log.info("Each bulk was of %d files" % self.bulk_size)
        log.info("Total time: %.2f" % acc_time)
        log.info("Avg. time: %.2f" % (acc_time / self.iterations))
        return total_files

    def set_up(self):
        """
        Prepare the environment
        """
        log.info("Preparing destination base SURL")
        try:
            stat = self.gfal2.stat(self.base_surl)
        except gfal2.GError, e:
            if e.code == errno.ENOENT:
                log.debug("Creating %s" % base_surl)
                self.gfal2.mkdir(self.base_surl, 0775)
            else:
                raise
        log.info("Done preparing destination base SURL")

    def tear_down(self):
        """
        Clean up the destination
        """
        log.info("Cleaning up destination base SURL")
        remainings = self.gfal2.listdir(self.base_surl)
        if len(remainings):
            log.warning("Files left on the destination storage!")
            log.warning("Cleaning up")
            for name in remainings:
                surl = self.base_surl + name
                log.debug("Unlinking %s" % surl)
                try:
                    self.gfal2.unlink(surl)
                except gfal2.GError, e:
                    log.warn(str(e))
        else:
            log.debug("Destination base SURL empty")
        log.debug("Rmdir %s" % self.base_surl)
        self.gfal2.rmdir(self.base_surl)
        log.info("Done cleaning up destination base SURL")

    def __call__(self, *args, **kwargs):
        """
        Object as a function
        """
        self.set_up()
        try:
            return self.run_test(*args, **kwargs)
        except Exception, e:
            log.error(str(e))
            raise
        finally:
            self.tear_down()


def setup_logging():
    """
    Setup nice logging
    """
    log_handler = logging.StreamHandler(sys.stderr)
    log_handler.setLevel(logging.DEBUG)
    log_handler.setFormatter(logging.Formatter("[%(levelname)s] %(message)s"))
    log.addHandler(log_handler)
    log.setLevel(logging.INFO)
    # Coloured output on ttys
    if sys.stdout.isatty():
        logging.addLevelName(logging.DEBUG, "\033[1;2m%-8s\033[1;m" % logging.getLevelName(logging.DEBUG))
        logging.addLevelName(logging.INFO, "\033[1;34m%-8s\033[1;m" % logging.getLevelName(logging.INFO))
        logging.addLevelName(logging.ERROR, "\033[1;31m%-8s\033[1;m" % logging.getLevelName(logging.ERROR))
        logging.addLevelName(logging.WARNING, "\033[1;33m%-8s\033[1;m" % logging.getLevelName(logging.WARNING))


if __name__ == '__main__':
    """
    Entry point
    """
    setup_logging()

    parser = OptionParser(usage="usage: %prog [options] source-list")
    parser.add_option('-s', '--endpoint', type='string', dest='endpoint',
                      help='Endpoint. MANDATORY.')
    parser.add_option('-n', '--number', type='int', dest='bulk_size', default=100,
                      help='Number of files in the bulk request')
    parser.add_option('-i', '--iterate', type='int', dest='iterations', default=1,
                      help='Repeat the test this number of times')
    parser.add_option('-c', '--create', dest='create_file', default=False,
                      action='store_true', help='Create the files before triggering the unlink')
    parser.add_option('-v', '--verbose', dest='verbose', default=False,
                      action='store_true', help='Verbose mode')

    (options, args) = parser.parse_args()

    if options.endpoint == None or options.endpoint == '':
        log.error("Need to specify -s <endpoint>")
        sys.exit(2)
    if options.iterations < 1:
        log.error("Iterations must be greater or equal than 1")
        sys.exit(2)
    if options.bulk_size < 1:
        log.error("Bulk size must be greater or equal than 1")
        sys.exit(2)

    if len(args) == 0:
        log.error("Need base surl")
        sys.exit(2)
    elif len(args) > 1:
        log.error("Too many arguments specified")
        sys.exit(2)

    if options.verbose:
        log.setLevel(logging.DEBUG)

    base_surl = args[0]
    bully = DeletionBully(options.endpoint, options.bulk_size, options.iterations, base_surl, options.create_file)
    log.info("Starting stress test")
    log.info("Sent %d pushes" % bully())

    sys.exit(0)
