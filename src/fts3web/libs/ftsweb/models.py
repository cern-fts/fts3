# Copyright notice:
# Copyright (C) Members of the EMI Collaboration, 2010.
#
# See www.eu-emi.eu for details on the copyright holders
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from django.db import models


STATES               = ['SUBMITTED', 'READY', 'ACTIVE', 'FAILED', 'FINISHED', 'CANCELED', 'STAGING', 'NOT_USED']
ACTIVE_STATES        = ['SUBMITTED', 'READY', 'ACTIVE', 'STAGING']
FILE_TERMINAL_STATES = ['FINISHED', 'FAILED', 'CANCELED', 'NOT_USED']


class JobBase(models.Model):
    job_id          = models.CharField(max_length = 36, primary_key = True)
    job_state       = models.CharField(max_length = 32)
    source_se       = models.CharField(max_length = 255)
    dest_se         = models.CharField(max_length = 255)
    reuse_job       = models.CharField(max_length = 3)
    cancel_job      = models.CharField(max_length = 1)
    job_params      = models.CharField(max_length = 255)
    user_dn         = models.CharField(max_length = 1024)
    agent_dn        = models.CharField(max_length = 1024)
    user_cred       = models.CharField(max_length = 255)
    cred_id         = models.CharField(max_length = 100)
    voms_cred       = models.CharField()
    vo_name         = models.CharField(max_length = 50)
    reason          = models.CharField(max_length = 2048)
    submit_time     = models.DateTimeField()
    finish_time     = models.DateTimeField()
    priority        = models.IntegerField()
    submit_host     = models.CharField(max_length = 255)
    max_time_in_queue = models.IntegerField()
    space_token     = models.CharField(max_length = 255)
    storage_class   = models.CharField(max_length = 255)
    myproxy_server  = models.CharField(max_length = 255)
    internal_job_params = models.CharField(max_length = 255)
    overwrite_flag  = models.CharField(max_length = 1)
    job_finished    = models.DateTimeField()
    source_space_token = models.CharField(max_length = 255)
    source_token_description = models.CharField(max_length = 255)
    copy_pin_lifetime = models.IntegerField()
    fail_nearline   = models.CharField(max_length = 1)
    checksum_method = models.CharField(max_length = 1)
    bring_online    = models.IntegerField()
    job_metadata    = models.CharField(max_length = 1024)
    retry           = models.IntegerField()
    retry_delay     = models.IntegerField()

    class Meta:
        abstract = True

    def isFinished(self):
        return self.job_state not in ['SUBMITTED', 'READY', 'ACTIVE', 'STAGING']

    def __str__(self):
        return self.job_id


class Job(JobBase):
    class Meta:
        db_table = 't_job'


class FileBase(models.Model):
    file_id      = models.IntegerField(primary_key = True)
    hashed_id    = models.IntegerField()
    vo_name      = models.CharField(max_length = 255)
    source_se    = models.CharField(max_length = 255)
    dest_se      = models.CharField(max_length = 255)
    symbolicName = models.CharField(max_length = 255)
    file_state   = models.CharField(max_length = 32)
    transferHost = models.CharField(max_length = 255)
    source_surl  = models.CharField(max_length = 1100)
    dest_surl    = models.CharField(max_length = 1100)
    agent_dn     = models.CharField(max_length = 1024)
    error_scope  = models.CharField(max_length = 32)
    error_phase  = models.CharField(max_length = 32)
    reason_class = models.CharField(max_length = 32)
    reason       = models.CharField(max_length = 2048)
    num_failures = models.IntegerField()
    current_failures  = models.IntegerField()
    catalog_failures  = models.IntegerField()
    prestage_failures = models.IntegerField()
    filesize     = models.FloatField()
    checksum     = models.CharField(max_length = 100)
    finish_time  = models.DateTimeField()
    start_time   = models.DateTimeField()
    internal_file_params = models.CharField(max_length = 255)
    job_finished = models.DateTimeField()
    pid          = models.IntegerField()
    tx_duration  = models.FloatField()
    throughput   = models.FloatField()
    transferred  = models.FloatField()
    retry        = models.IntegerField()
    file_metadata     = models.CharField(max_length = 1024)
    user_filesize     = models.FloatField()
    staging_start     = models.DateTimeField()
    staging_finished  = models.DateTimeField()
    bringonline_token = models.CharField(max_length = 255)
    log_file        = models.CharField(max_length = 2048, db_column = 't_log_file')
    log_debug       = models.IntegerField(db_column = 't_log_file_debug')
    activity        = models.CharField(max_length = 255)

    def get_start_time(self):
        """
        Return staging_start, or start_time or None in that order
        """
        if self.staging_start:
            return self.staging_start
        elif self.start_time:
            return self.start_time
        else:
            return None

    def __str__(self):
        return str(self.file_id)

    class Meta:
        abstract = True


class File(FileBase):
    job = models.ForeignKey('Job', db_column = 'job_id', related_name = '+', null = True)
    class Meta:
        db_table = 't_file'


class DmFile(models.Model):
    file_id       = models.IntegerField(primary_key=True)
    job           = models.ForeignKey('Job', db_column = 'job_id', related_name = '+', null = True)
    file_state    = models.CharField(max_length=32)
    transferHost  = models.CharField(max_length=150, db_column='dmHost')
    source_surl   = models.CharField(max_length=900)
    dest_surl     = models.CharField(max_length=900)
    source_se     = models.CharField(max_length=150)
    dest_se       = models.CharField(max_length=150)
    reason        = models.CharField(max_length=2048)
    checksum      = models.CharField(max_length=100)
    finish_time   = models.DateTimeField()
    start_time    = models.DateTimeField()
    job_finished  = models.DateTimeField()
    tx_duration   = models.FloatField()
    retry         = models.IntegerField()
    user_filesize = models.FloatField()
    file_metadata = models.CharField(max_length=1024)
    activity      = models.CharField(max_length=255)
    dm_token      = models.CharField(max_length=255)
    retry_timestamp = models.DateTimeField()
    wait_timestamp	= models.DateTimeField()
    wait_timeout    = models.IntegerField()
    hashed_id       = models.IntegerField()
    vo_name         = models.CharField(max_length=100)

    # Compatibility fields, so it can be used as regular file
    filesize = 0
    transferred = 0
    throughput = 0

    def get_start_time(self):
        """
        Return staging_start, or start_time or None in that order
        """
        if self.start_time:
            return self.start_time
        else:
            return None

    def __str__(self):
        return str(self.file_id)

    class Meta:
        db_table = 't_dm'


class RetryError(models.Model):
    attempt  = models.IntegerField()
    datetime = models.DateTimeField()
    reason   = models.CharField(max_length = 2048)

    file_id = models.ForeignKey('File', db_column = 'file_id', related_name = '+', primary_key = True)

    class Meta:
        db_table = 't_file_retry_errors'

    def __eq__(self, b):
        return isinstance(b, self.__class__) and \
            self.file_id == b.file_id and \
            self.attempt == b.attempt


class  ConfigAudit(models.Model):
    # This field is definitely NOT the primary key, but since we are not modifying
    # this from Django, we can live with this workaround until Django supports fully
    # composite primary keys
    datetime = models.DateTimeField(primary_key = True)
    dn       = models.CharField(max_length = 1024)
    config   = models.CharField(max_length = 4000)
    action   = models.CharField(max_length = 100)

    class Meta:
        db_table = 't_config_audit'

    def simple_action(self):
        return self.action.split(' ')[0]

    def __eq__(self, b):
        return isinstance(b, self.__class__) and \
               self.datetime == b.datetime and self.dn == b.dn and \
               self.config == b.config and self.action == b.action


class LinkConfig(models.Model):
    source            = models.CharField(max_length = 255, primary_key = True)
    destination       = models.CharField(max_length = 255)
    state             = models.CharField(max_length = 30)
    symbolicName      = models.CharField(max_length = 255)
    nostreams         = models.IntegerField()
    tcp_buffer_size   = models.IntegerField()
    urlcopy_tx_to     = models.IntegerField()
    auto_tuning       = models.CharField(max_length = 3)

    def __eq__(self, b):
        return isinstance(b, self.__class__) and \
               self.source == b.source and \
               self.destination == b.destination

    class Meta:
        db_table = 't_link_config'


class ShareConfig(models.Model):
    source      = models.CharField(max_length = 255, primary_key = True)
    destination = models.CharField(max_length = 255)
    vo          = models.CharField(max_length = 100)
    active      = models.IntegerField()

    def __eq__(self, b):
        return isinstance(b, self.__class__) and \
               self.source == b.source and \
               self.destination == b.destination and \
               self.vo == b.vo

    class Meta:
        db_table = 't_share_config'


class Optimize(models.Model):
    source_se = models.CharField(max_length = 255, primary_key = True)
    dest_se   = models.CharField(max_length = 255, primary_key = True)
    active    = models.IntegerField()
    bandwidth = models.FloatField(db_column = 'throughput')

    def __eq__(self, other):
        if type(self) != type(other):
            return False

        return self.source_se == other.source_se and \
               self.dest_se == other.dest_se and \
               self.active == other.active

    class Meta:
        db_table = 't_optimize'


class OptimizeActive(models.Model):
    source_se  = models.CharField(max_length = 255)
    dest_se    = models.CharField(max_length = 255)
    active     = models.IntegerField(primary_key = True)

    def __eq__(self, other):
        if type(self) != type(other):
            return False

        return self.source_se == other.source_se and \
               self.dest_se   == other.dest_se

    class Meta:
        db_table = 't_optimize_active'


class OptimizerEvolution(models.Model):
    datetime     = models.DateTimeField(primary_key = True)
    source_se    = models.CharField(max_length = 255)
    dest_se      = models.CharField(max_length = 255)
    nostreams    = models.IntegerField()
    timeout      = models.IntegerField()
    active       = models.IntegerField()
    throughput   = models.FloatField()
    branch       = models.IntegerField(db_column = 'buffer')
    success      = models.FloatField(db_column = 'filesize')
    agrthroughput = models.FloatField(db_column = 'filesize')

    class Meta:
        db_table = 't_optimizer_evolution'


class ProfilingSnapshot(models.Model):
    scope      = models.CharField(primary_key = True, max_length = 255)
    cnt        = models.IntegerField()
    exceptions = models.IntegerField()
    total      = models.FloatField()
    average    = models.FloatField()

    class Meta:
        db_table = 't_profiling_snapshot'


class ProfilingInfo(models.Model):
    updated = models.DateTimeField(primary_key = True)
    period  = models.IntegerField()

    class Meta:
        db_table = 't_profiling_info'


class Host(models.Model):
    hostname = models.CharField(primary_key = True, max_length = 64)
    beat     = models.DateTimeField()
    service_name = models.CharField()
    drain    = models.IntegerField()

    class Meta:
        db_table = 't_hosts'


class DebugConfig(models.Model):
    source_se = models.CharField(primary_key = True)
    dest_se   = models.CharField()
    debug     = models.CharField()

    def __eq__(self, other):
        # Consider all entries different
        return False

    class Meta:
        db_table = 't_debug'


class Turl(models.Model):
    source_surl = models.CharField(primary_key=True)
    destin_surl = models.CharField()
    source_turl = models.CharField()
    destin_turl = models.CharField()
    datetime    = models.DateField()
    throughput  = models.FloatField()
    finish      = models.FloatField()
    fail        = models.FloatField()

    def __eq__(self, other):
        if type(other) != type(self):
            return False
        return self.source_surl == other.source_surl and \
            self.destin_surl == other.destin_surl and \
            self.source_turl == other.source_turl and \
            self.destin_turl == other.destin_turl

    class Meta:
        db_table = 't_turl'
