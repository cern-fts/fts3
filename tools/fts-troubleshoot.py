#!/usr/bin/env python
#
# Copyright (c) CERN 2013-2015
#
# Copyright (c) Members of the EMI Collaboration. 2010-2013
#  See  http://www.eu-emi.eu/partners for details on the copyright
#  holders.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import glob
import grp
import os
import pwd
import rpm
import sys
import subprocess
import yum
from datetime import datetime


class Reporter(object):
    """
    Facilitate the generation of a report
    """
    RED     = 91
    GREEN   = 92
    YELLOW  = 93
    BLUE    = 34
    MAGENTA = 95
    CYAN    = 96
    GRAY    = 37

    def __init__(self, stream=sys.stdout):
        self.stream = stream
        self.enable_colors = sys.stdout.isatty()

    def colored(self, text, fg, bold=False):
        if self.enable_colors:
            bold_fmt = "\033[1m" if bold else ""
            return "%s\033[%dm%s\033[0m" % (bold_fmt, fg, text)
        else:
            return text

    def _status(self, status, fg):
        filling = (11 - len(status))
        left = filling / 2
        right = filling / 2 + filling % 2
        return "[%s%s%s]" % (" " * left, self.colored(status, fg), " " * right)

    def test(self, func):
        doc_string = func.__doc__.strip() if func.__doc__ else func.__name__
        print >>self.stream, " " * 13, self.colored(doc_string.upper(), Reporter.CYAN, bold=True)

    def skip(self, msg):
        print >>self.stream, self._status("SKIPPED", Reporter.YELLOW), msg

    def cat(self, path):
        print >>self.stream, open(path).read()

    def ok(self, msg):
        print >>self.stream, self._status("OK", Reporter.GREEN), msg

    def not_found(self, msg):
        print >>self.stream, self._status("NOT FOUND", Reporter.RED), msg

    def fail(self, msg):
        print >>self.stream, self._status("FAIL", Reporter.RED), msg

    def info(self, msg):
        print >>self.stream, self._status("INFO", Reporter.GRAY), msg

    def advice(self, msg):
        print >>self.stream, self._status("ADVICE", Reporter.MAGENTA), msg

class FTS3Troubleshooter(object):
    """
    Run a set of checks and generate troubleshooting information
    for running a FTS3 service
    """

    def __init__(self, reporter):
        self.reporter = reporter
        try:
            self.fts3_uid = pwd.getpwnam("fts3")[2]
        except:
            self.fts3_uid = None

        try:
            self.fts3_gid = grp.getgrnam("fts3")[2]
        except:
            self.fts3_gid = None

    @staticmethod
    def execute(cmd):
        """
        Run a command and returns the output
        """
        proc = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)
        (out, err) = proc.communicate()
        if proc.returncode != 0:
            raise Exception("%s failed with %d" % (cmd, proc.returncode))
        return out.strip()

    def check_repositories(self):
        """
        List of enabled repositories
        """
        yb = yum.YumBase()
        for repo in yb.repos.listEnabled():
            self.reporter.info(repo.urls[0])

    def check_rpms(self):
        """
        List of rpms
        """
        rpm_list = [
            "fts-libs", "fts-server", "fts-msg", "fts-mysql", "fts-infosys",
            "fts-rest", "python-fts",
            "gfal2", "gfal2-plugin-gridftp", "gfal2-plugin-srm",
            "srm-ifce",
            "fetch-crl"
        ]
        ts = rpm.TransactionSet()
        not_installed = []
        for r in rpm_list:
            matches = ["%s-%s-%s" % (h["name"], h["version"], h["release"]) for h in ts.dbMatch("name", r)]
            if len(matches):
                for m in matches:
                    self.reporter.ok(m)
            else:
                not_installed.append(r)
        if len(not_installed):
            for r in not_installed:
                self.reporter.not_found(r)
            self.reporter.advice("yum install %s" % " ".join(not_installed))

    def check_ca(self):
        """
        Validating root certificates
        """
        if not os.path.exists("/etc/grid-security/certificates"):
            self.reporter.not_found("/etc/grid-security/certificates")
        else:
            self.reporter.ok("/etc/grid-security/certificates")
        certs = glob.glob("/etc/grid-security/certificates/*.0")
        crls = glob.glob("/etc/grid-security/certificates/*.r0")
        if len(certs) == 0:
            self.reporter.not_found("No root certificates found (*.0)")
            self.reporter.advice("yum install ca-policy-egi-core")
        else:
            self.reporter.ok("%d root certificates found" % len(certs))
            n_cert_ok = 0
            n_cert_expired = 0
            for cert in certs:
                try:
                    end_date = self.execute("openssl x509 -in %s -noout -enddate" % cert)
                    end_date = datetime.strptime(end_date, "notAfter=%b %d %H:%M:%S %Y %Z")
                    if end_date < datetime.utcnow():
                        self.reporter.fail("%s expired the %s" % (cert, end_date))
                        n_cert_expired += 1
                    else:
                        n_cert_ok += 1
                except Exception, e:
                    self.reporter.fail(str(e))
            if n_cert_ok > 0:
                self.reporter.ok("%d certificates are ok" % n_cert_ok)
            if n_cert_expired > 0:
                self.reporter.advice("Update the root ca / yum update ca-policy-egi-core")

        if len(crls) == 0:
            self.reporter.not_found("No CRL found")
        else:
            self.reporter.ok("%d CRL found" % len(crls))
            n_crl_ok = 0
            n_crl_expired = 0
            for crl in crls:
                try:
                    next_update = self.execute("openssl crl -in %s -noout -nextupdate" % crl)
                    next_update = datetime.strptime(next_update, "nextUpdate=%b %d %H:%M:%S %Y %Z")
                    if next_update < datetime.utcnow():
                        self.reporter.fail("%s expired (next update in the past: %s)" % (crl, next_update))
                        n_crl_expired += 1
                    else:
                        n_crl_ok += 1
                except Exception, e:
                    self.reporter.fail(str(e))
            if n_crl_ok > 0:
                self.reporter.ok("%d crl are ok" % n_crl_ok)
            if n_crl_expired > 0:
                self.reporter.advice("fetch-crl")

    def check_fts3_user(self):
        """
        Check if fts3 user and group exist
        """
        if self.fts3_uid is None:
            self.reporter.fail("fts3 user not found")
        else:
            self.reporter.ok("fts3 uid %d" % self.fts3_uid)

        if self.fts3_gid is None:
            self.reporter.fail("fts3 group not found")
        else:
            self.reporter.ok("fts3 gid %d" % self.fts3_gid)

    def check_vomses(self):
        """
        Check if voms are configured
        """
        if not os.path.exists("/etc/grid-security/vomsdir"):
            self.reporter.not_found("/etc/grid-security/vomsdir")
        else:
            vos = os.listdir("/etc/grid-security/vomsdir")
            if len(vos) == 0:
                self.reporter.fail("No VO configured")
            for vo in vos:
                vodir = "/etc/grid-security/vomsdir/" + vo
                lsc = os.listdir(vodir)
                if len(lsc) == 0:
                    self.reporter.fail("%s is missing lsc files" % vo)
                for l in lsc:
                    self.reporter.ok("%s: %s" % (vo, l))

    def check_hostcert(self):
        """
        Check host certificate and private key
        """
        hostcert = "/etc/grid-security/fts3hostcert.pem"
        hostkey = "/etc/grid-security/fts3hostcert.pem"

        if not os.path.exists(hostcert):
            self.reporter.not_found(hostcert)
        else:
            self.reporter.ok(hostcert)

        if not os.path.exists(hostkey):
            self.reporter.not_found(hostkey)
        else:
            self.reporter.ok(hostkey)

        # Check permissions
        if self.fts3_gid and self.fts3_uid:
            hcs = os.stat(hostcert)
            hks = os.stat(hostkey)
            if hcs.st_uid != self.fts3_uid:
                self.reporter.fail("Host certificate does not belong to fts3")
            else:
                self.reporter.ok("Host certificate belongs to %d" % hcs.st_uid)
            if hks.st_uid != self.fts3_uid:
                self.reporter.fail("Host key does not belong to fts3")
            else:
                self.reporter.ok("Host key belongs to %d" % hks.st_uid)

        # Check expiration time
        end_date = self.execute("openssl x509 -in %s -noout -enddate" % hostcert)
        end_date = datetime.strptime(end_date, "notAfter=%b %d %H:%M:%S %Y %Z")
        if end_date < datetime.utcnow():
            self.reporter.fail("Host certificate expired (%s)" % end_date)
        else:
            remaining = end_date - datetime.utcnow()
            self.reporter.ok("Host certificate expires in %d days and %d seconds" % (remaining.days, remaining.seconds))

    def run(self):
        """
        Self-instrospect and run every method that start with check_
        """
        for member_name in dir(self):
            if member_name.startswith("check_"):
                member = getattr(self, member_name)
                self.reporter.test(member)
                member()

    def __call__(self):
        self.run()


if __name__ == "__main__":
    FTS3Troubleshooter(Reporter(sys.stdout))()
