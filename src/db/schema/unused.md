# Unused database columns/tables

Update this document when they become used, or are dropped/renamed.

## t_optimize
  * file_id
  * timeout
  * buffer
  * filesize
  
## t_credential_vers
Not used at all.

## t_se
The name is a foreign key of t_group_member, but not a single field
of the t_se table seems to be used at all.

## t_se_acl
Not used.

## t_link_config
placeholder1, 2 and 3, obviously.

NO_TX_ACTIVITY_TO is not really used anywhere either
(actually there are comments saying it is not used)

I don't even know what does it mean.

## t_se_pair_acl
Unused

## t_vo_acl
Seems used, but would have to be modified directly on the DB.
Seems to be some sort of way of granting access to a whole VO to a
specific user.

## t_job
  * job_params is inserted, and put back into job.INTERNAL_JOB_PARAMS, but that value is never used
  * agent_dn unused
  * storage_class unused
  * myproxy_server unused
  * internal_job_params seems to be used for protocol setup
  * source_token_description unused
  * fail_nearline unused
  * configuration_count unused
  
## t_file
  * logical_name unused
  * symbolicName unused
  * agent_dn refers only to the staging agent
  * error_scope
  * error_phase
  * reason_class
  * num_failures
  * catalog_failures
  * prestage_failures

## t_turl
Mentioned in one query that is never executed, because we have the
typical problem of a table growing too much.
