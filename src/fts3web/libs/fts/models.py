from django.db import models
from settings.database import DATABASES

class Job(models.Model):
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
        db_table = 't_job'
        
    def isFinished(self):
        return self.job_state not in ['SUBMITTED', 'READY', 'ACTIVE', 'STAGING']



class File(models.Model):
    file_id      = models.IntegerField(primary_key = True)
    job          = models.ForeignKey('Job', db_column = 'job_id')
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
    
    class Meta:
        db_table = 't_file'



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
    
    class Meta:
        db_table = 't_optimize'
