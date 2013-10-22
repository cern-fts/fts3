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
from settings.database import DATABASES


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
    job_metadata    = models.CharField(max_length = 255)
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


class JobArchive(JobBase):
    class Meta:
        db_table = 't_job_backup'


class FileBase(models.Model):
    file_id      = models.IntegerField(primary_key = True)
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
    retry        = models.IntegerField()
    file_metadata     = models.CharField(max_length = 255)
    user_filesize     = models.FloatField()
    staging_start     = models.DateTimeField()
    staging_finished  = models.DateTimeField()
    bringonline_token = models.CharField(max_length = 255)
    log_file        = models.CharField(max_length = 2048, db_column = 't_log_file')
    log_debug       = models.IntegerField(db_column = 't_log_file_debug')
    
    def __str__(self):
        return str(self.file_id)
    
    class Meta:
        abstract = True


class File(FileBase):
    job = models.ForeignKey('Job', db_column = 'job_id', related_name = '+')
    class Meta:
        db_table = 't_file' 


class FileArchive(FileBase):
    job = models.ForeignKey('JobArchive', db_column = 'job_id', related_name = '+')        
    class Meta:
        db_table = 't_file_backup'


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


class Optimize(models.Model):
    file_id    = models.IntegerField(primary_key = True)
    source_se  = models.CharField(max_length = 255)
    dest_se    = models.CharField(max_length = 255)
    nostreams  = models.IntegerField()
    timeout    = models.IntegerField()
    active     = models.IntegerField()
    throughput = models.FloatField()
    buffer     = models.IntegerField()
    filesize   = models.FloatField()
    datetime   = models.DateTimeField()
    
    def __eq__(self, other):
        if type(self) != type(other):
            return False
        
        fields = ['file_id', 'source_se', 'dest_se', 'nostreams', 'timeout',
                  'active', 'throughput', 'buffer', 'filesize', 'datetime']
        
        return reduce(lambda a,b: a and b,
                      map(lambda attr: getattr(self, attr) == getattr(other, attr),
                         fields)) 
    
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
    buffer       = models.IntegerField()
    filesize     = models.FloatField()
    
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
    
    class Meta:
        db_table = 't_hosts'
