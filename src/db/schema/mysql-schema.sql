--
-- Holds the log files path and host
--
CREATE TABLE t_log (
  path       VARCHAR(255),
  job_id     CHAR(36),
  dn         VARCHAR(255),
  vo         VARCHAR(255),
-- Note: 'when' is a reserved word in MySQL, so a different word has to be picked
  timemark   TIMESTAMP,
  PRIMARY KEY (path, job_id, dn, vo)
);

--
-- Holds optimization parameters
--
CREATE TABLE t_optimize (
--
-- file id
   file_id              INTEGER NOT NULL,
--
-- source se
   source_se            VARCHAR(255),
--
-- dest se      
   dest_se              VARCHAR(255),
--
-- number of streams
   nostreams            INTEGER DEFAULT NULL,
--
-- timeout
   timeout              INTEGER DEFAULT NULL,
--
-- active transfers
   active               INTEGER DEFAULT NULL,
--
-- throughput
   throughput           FLOAT DEFAULT NULL,
--
-- tcp buffer size
   buffer               INTEGER DEFAULT NULL,   
--
-- the nominal size of the file (bytes)
   filesize             BIGINT DEFAULT NULL,
--
-- timestamp
   timemark             TIMESTAMP,
   
  INDEX (source_se,dest_se),
  INDEX (dest_se),
  INDEX (nostreams),
  INDEX (timeout),
  INDEX (buffer)
);


--
-- Holds certificate request information
--
CREATE TABLE t_config_audit (
--
-- timestamp
   datetime             TIMESTAMP,
--
-- dn
   dn                   VARCHAR(1024),
--
-- what has changed
   config               VARCHAR(4000), 
--
-- action (insert/update/delete)
   action               VARCHAR(100)    
);


--
-- Configures debug mode for a given pair
--
CREATE TABLE t_debug (
--
-- source hostname
   source_se    VARCHAR(255),
--
-- dest hostanme
   dest_se      VARCHAR(255),
--
-- debug on/off
   debug        VARCHAR(3) DEFAULT 'off',
   
   PRIMARY KEY (source_se, dest_se)
);


--
-- Holds certificate request information
--
CREATE TABLE t_credential_cache (
--
-- delegation identifier
   dlg_id       VARCHAR(100),
--
-- DN of delegated proxy owner
   dn           VARCHAR(255),
--
-- certificate request
   cert_request LONGTEXT,
--
-- private key of request
   priv_key     LONGTEXT,
--
-- list of voms attributes contained in delegated proxy
   voms_attrs   LONGTEXT,
--
-- set primary key
   PRIMARY KEY (dlg_id, dn)
);

--
-- Holds delegated proxies
--
CREATE TABLE t_credential (
--
-- delegation identifier
   dlg_id       VARCHAR(100),
--
-- DN of delegated proxy owner
   dn           VARCHAR(255),
--
-- delegated proxy certificate chain
   proxy        LONGTEXT,
--
-- list of voms attributes contained in delegated proxy
   voms_attrs   LONGTEXT,
--
-- termination time of the credential
   termination_time TIMESTAMP NOT NULL,
--
-- set primary key
   PRIMARY KEY (dlg_id, dn),
   INDEX (termination_time)
);

--
-- Credentials version
--
CREATE TABLE t_credential_vers (
  major INT NOT NULL,
  minor INT NOT NULL,
  patch INT NOT NULL
);
INSERT INTO t_credential_vers (major,minor,patch) VALUES (1,2,0);

--
-- SE from the information service, currently BDII
--
CREATE TABLE t_se (
-- The internal id
  se_id_info INTEGER AUTO_INCREMENT,
  endpoint   VARCHAR(1024),
  se_type    VARCHAR(30),
  site       VARCHAR(100),
  name       VARCHAR(512) NOT NULL,
  state      VARCHAR(30),
  version    VARCHAR(30),
-- This field will contain the host parse for FTS and extracted from name 
  host       VARCHAR(100),
  se_transfer_type     VARCHAR(30),
  se_transfer_protocol VARCHAR(30),
  se_control_protocol  VARCHAR(30),
  gocdb_id VARCHAR(100),

  KEY (se_id_info),
  PRIMARY KEY (name)
);

-- 
-- relation of SE and VOs
--
CREATE TABLE t_se_acl (
  name VARCHAR(255),
  vo   VARCHAR(32), 
  PRIMARY KEY (name, vo)
);


--
-- se, se pair and se groups in the system
--
CREATE TABLE t_se_protocol (
   se_row_id           INTEGER NOT NULL AUTO_INCREMENT,
--
-- Name of the se_pair
   se_group_name       VARCHAR(512),
--          
-- Source site name
   se_name             VARCHAR(512),
--
-- Email contact of the se_pair responsbile
   contact             VARCHAR(255),
--
-- Maximum bandwidth, capacity, in Mbits/s
   bandwidth           FLOAT DEFAULT NULL,
--
-- The target number of concurrent streams on the network
   nostreams           INTEGER DEFAULT NULL,
--
-- The target number of concurrent files on the network
   nofiles             INTEGER DEFAULT NULL,
--
-- Default TCP Buffer Size for the transfer
   tcp_buffer_size     INTEGER DEFAULT NULL,
--
-- The target throughput for the system (Mbits/s)
   nominal_throughput  FLOAT DEFAULT NULL,
--
-- The state of the se_pair ("Active", "Inactive", "Drain", "Stopped")
   se_pair_state       VARCHAR(30),
--
-- The time the se_pair was last active
   last_active         TIMESTAMP,
-- 
-- The Message concerning Last Modification 
   message             VARCHAR(1024),
--   
-- Last Modification time
   last_modification   TIMESTAMP,
--
-- The DN of the administrator who did last modification
   admin_dn            VARCHAR(1024),
--
-- per-SE limit on se_pair
   se_limit           INTEGER DEFAULT NULL,
--
   blocksize          INTEGER DEFAULT NULL,
--
-- HTTP timeout
   http_to            INTEGER DEFAULT NULL,
--
-- Transfer log level. Allowed values are (DEBUG, INFO, WARN, ERROR)
   tx_loglevel        VARCHAR(12) DEFAULT NULL,
--
-- urlcopy put timeout
   urlcopy_put_to     INTEGER DEFAULT NULL,
--
-- urlcopy putdone timeout
   urlcopy_putdone_to INTEGER DEFAULT NULL,
--
-- urlcopy get timeout
   urlcopy_get_to     INTEGER DEFAULT NULL,
--
-- urlcopy getdone timeout
   urlcopy_getdone_to INTEGER DEFAULT NULL,
--
-- urlcopy transfer5 timeout
   urlcopy_tx_to      INTEGER DEFAULT NULL,
--
-- urlcopy transfer markers timeout
   urlcopy_txmarks_to INTEGER DEFAULT NULL,
--
-- srmcopy direction
   srmcopy_direction  VARCHAR(4) DEFAULT NULL,
--
-- srmcopy transfer timeout
   srmcopy_to         INTEGER DEFAULT NULL,
--
-- srmcopy refresh timeout (timeout since last status update)
   srmcopy_refresh_to INTEGER DEFAULT NULL,
--
-- check that target directory is accessible
--
   target_dir_check CHAR(1) DEFAULT NULL,
--
-- The parameter to set after how many seconds to mark the first transfer activity
   url_copy_first_txmark_to INTEGER DEFAULT NULL,
--
-- size-based transfer timeout, in seconds per MB
   tx_to_per_mb          FLOAT DEFAULT NULL, 
-- 
-- maximum interval with no activity detected during the transfer, in seconds
   no_tx_activity_to     INTEGER DEFAULT NULL,
--
-- maximum number of files in the preparation phase = nofiles * preparing_files_ratio
-- preparing_files_ratio DEFAULT = 2. urlcopy se_pairs only.
   preparing_files_ratio FLOAT DEFAULT NULL,
   
   PRIMARY KEY (se_row_id)
);


--
-- blacklist of bad SEs that should not be transferred to
--
CREATE TABLE t_bad_ses (
--
-- The internal id
   se_id                INTEGER AUTO_INCREMENT,
--
-- The hostname of the bad SE   
   se_hostname         VARCHAR(256),
--
-- The reason this host was added 
   message             VARCHAR(2048) DEFAULT NULL,
--
-- The time the host was added
   addition_time       TIMESTAMP,
--
-- The DN of the administrator who added it
   admin_dn            VARCHAR(1024),
   
   PRIMARY KEY(se_id),
   INDEX (se_hostname)
);


--
-- Table for saving the site-group association. As convention, group names should be between "[""]"
-- 
CREATE TABLE t_site_group (
--
-- name of the group 
   group_name     VARCHAR(100) NOT NULL,
--
-- name of the site 
  site_name       VARCHAR(100) NOT NULL,
--
-- priority used for ordering
  priority       INTEGER DEFAULT 3,
--
-- define private key
  PRIMARY KEY (group_name, site_name),
  INDEX (site_name)
);


--
-- The se_pair VO share table stores the percentage of the se_pair resources 
-- available for a VO
--
CREATE TABLE t_se_vo_share (
--
-- the name of the se_pair
   se_name     VARCHAR(512) NOT NULL,                    
   share_id    VARCHAR(512) NOT NULL,                                                           
   share_value VARCHAR(512) NOT NULL,
   share_type  VARCHAR(512) NOT NULL,
   PRIMARY KEY (share_id),
   INDEX (se_name(256), share_id(256), share_type(256))   
);

-- Max. index size is 1000, so we need to keep the group names
-- in a separated table with a smaller primary key.
CREATE TABLE t_se_group(
   se_group_id   INTEGER NOT NULL AUTO_INCREMENT,
   se_group_name VARCHAR(512) NOT NULL,
   PRIMARY KEY (se_group_id),
   KEY(se_group_name)
);

CREATE TABLE t_se_group_contains(
   se_group_id   INTEGER NOT NULL,
   se_name       VARCHAR(512) NOT NULL,
   state         VARCHAR(30),
   PRIMARY KEY (se_group_id, se_name),
   FOREIGN KEY (se_group_id) REFERENCES t_se_group(se_group_id)
); 
--
-- Store se_pair ACL
--
CREATE TABLE t_se_pair_acl (
--
-- the name of the se_pair
   se_pair_name         VARCHAR(32) NOT NULL,
--
-- The principal name
   principal            VARCHAR(255) NOT NULL,
--
-- Set Primary Key
   PRIMARY KEY (se_pair_name, principal)
);

--
-- Store VO ACL
--
CREATE TABLE t_vo_acl (
--
-- the name of the VO
  vo_name             VARCHAR(50) NOT NULL,
--
-- The principal name
  principal            VARCHAR(255) NOT NULL,
--
-- Set Primary Key
  PRIMARY KEY (vo_name, principal)
);

--
-- t_job contains the list of jobs currently in the transfer database.
--
CREATE TABLE t_job (
--
-- the job_id, a IETF UUID in string form.
  job_id               CHAR(36) NOT NULL,
--
-- The state the job is currently in
  job_state            VARCHAR(32) NOT NULL,
--
-- Session reuse for this job. Allowed values are Y, (N), NULL
  reuse_job            VARCHAR(3),
--
-- Canceling flag. Allowed values are Y, (N), NULL
  cancel_job           CHAR(1),
--
-- Transport specific parameters
  job_params           VARCHAR(255),
--
-- Source site name - the source cluster name
  source               VARCHAR(255),
--
-- Dest site name - the destination cluster name
  dest                 VARCHAR(255),
--
-- Source SE host name
  source_se            VARCHAR(255),
--
-- Dest SE host name
  dest_se              VARCHAR(255),
--
-- the DN of the user starting the job - they are the only one
-- who can sumbit/cancel
  user_dn              VARCHAR(1024) NOT NULL,
--
-- the DN of the agent currently serving the job
  agent_dn             VARCHAR(1024),
--
-- the user credentials passphrase. This is passed to the movement service in
-- order to retrieve the appropriate user proxy to do the transfers
  user_cred            VARCHAR(255),
--
-- The user's credential delegation id
  cred_id              VARCHAR(100),
--
-- Blob to store user capabilites and groups
  voms_cred            LONGTEXT,
--
-- The VO that owns this job
  vo_name              VARCHAR(50),
--
-- The reason the job is in the current state
  reason               VARCHAR(2048),
--
-- The time that the job was submitted
  submit_time          TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
--
-- The time that the job was in a terminal state
  finish_time          TIMESTAMP NULL DEFAULT NULL,
--
-- Priority for Intra-VO Scheduling
  priority             INTEGER DEFAULT 3,
--
-- Submitting FTS hostname
  submit_host          VARCHAR(255),
--
-- Maximum time in queue before start of transfer (in seconds)
  max_time_in_queue    INTEGER,
--
-- The Space token to be used for the destination files
  space_token          VARCHAR(255),
--
-- The Storage Service Class to be used for the destination files
  storage_class        VARCHAR(255),
--
-- The endpoint of the MyProxy server that should be used if the
-- legacy cert retrieval is used
  myproxy_server       VARCHAR(255),
--
-- The endpoint of Source Catalog to be used in case 
-- logical names are used in the files
  src_catalog          VARCHAR(1024),
--
-- The type of the the Source Catalog to be used in case 
-- logical names are used in the files
  src_catalog_type     VARCHAR(1024),
--
-- The endpoint of the Destination Catalog to be used in case 
-- logical names are used in the files
  dest_catalog          VARCHAR(1024),
--
-- The type of the Destination Catalog to be used in case 
-- logical names are used in the files
  dest_catalog_type     VARCHAR(1024),
--
-- Internal job parameters,used to pass job specific data from the
-- WS to the agent
  internal_job_params   VARCHAR(255),
--
-- Overwrite flag for job
  overwrite_flag        CHAR(1) DEFAULT NULL,
--
-- this timestamp will be set when the job enter in one of the terminal 
-- states (Finished, FinishedDirty, Failed, Canceled). Use for table
-- partitioning
  job_finished          TIMESTAMP NULL DEFAULT NULL,
--
--      Space token of the source files
--
  source_space_token       VARCHAR(255),
--
-- description used by the agents to eventually get the source token. 
--
  source_token_description VARCHAR(255), 
-- *** New in 3.3.0 ***
--
-- pin lifetime of the copy of the file created after a successful srmPutDone
-- or srmCopy operations, in seconds
  copy_pin_lifetime INTEGER DEFAULT NULL,
--
-- use "LAN" as ConnectionType (FTS by DEFAULT uses WAN). Default value is false.
  lan_connection  CHAR(1) DEFAULT NULL,
--
-- fail the transfer immediately if the file location is NEARLINE (do not even
-- start the transfer). The DEFAULT is false.
  fail_nearline   CHAR(1) DEFAULT NULL,
--
-- Specified is the checksum is required on the source and destination, destination or none
  checksum_method CHAR(1) DEFAULT NULL,
  
  PRIMARY KEY (job_id),
  INDEX (job_state),
  INDEX (vo_name),
  INDEX (user_dn(1000)),
  INDEX (job_finished),
  INDEX (source_se,dest_se),
  INDEX (vo_name, job_id)
);


--
-- t_file stores the actual file transfers - one row per source/dest pair
--
CREATE TABLE t_file (
-- file_id is a unique identifier for a (source, destination) pair with a
-- job.  It is created automatically.
--
  file_id              INTEGER NOT NULL AUTO_INCREMENT,
--
-- job_id (used in joins with file table)
  job_id               CHAR(36) NOT NULL,
--
-- The state of this file
  file_state           VARCHAR(32) NOT NULL,
--
-- The Source Logical Name
  logical_name         VARCHAR(1100),
--
-- The Source
  source_surl          VARCHAR(1100),
--
-- The Destination
  dest_surl            VARCHAR(1100),
--
-- The agent who is transferring the file. This is only valid when the file
-- is in 'Active' state
  agent_dn             VARCHAR(1024),
--
-- The error scope
  error_scope          VARCHAR(32),
--
-- The FTS phase when the error happened
  error_phase          VARCHAR(32),
--
-- The class for the reason field
  reason_class         VARCHAR(32),
--
-- The reason the file is in this state
  reason               VARCHAR(2048),
--
-- Total number of failures (including transfer,catalog and prestaging errors)
  num_failures         INTEGER,
--
-- Number of transfer failures in last attemp cycle (reset at the Hold->Pending transition)
  current_failures     INTEGER,
--
-- Number of catalog failures (not reset at the Hold->Pending transition)
  catalog_failures     INTEGER,
--
-- Number of prestaging failures (reset at the Hold->Pending transition)
  prestage_failures    INTEGER,
--
-- the nominal size of the file (bytes)
  filesize             INTEGER,
--
-- the user-defined checksum of the file "checksum_type:checksum"
  checksum             VARCHAR(100),
--
-- the timestamp when the file is in a terminal state
  finish_time          TIMESTAMP NULL DEFAULT NULL,
--
-- the timestamp when the file is in a terminal state
  start_time           TIMESTAMP NULL DEFAULT NULL,
--
-- internal file parameters for storing information between retry attempts
  internal_file_params  VARCHAR(255),
--
-- this timestamp will be set when the job enter in one of the terminal 
-- states (Finished, FinishedDirty, Failed, Canceled). Use for table
-- partitioning
  job_finished          TIMESTAMP NULL DEFAULT NULL,
--
-- the pid of the process which is executing the file transfer
  pid INTEGER,
--
-- transfer duration
  tx_duration          INTEGER,
--
-- Average throughput
  throughput           FLOAT,
  
  PRIMARY KEY (file_id),
  FOREIGN KEY (job_id) REFERENCES t_job(job_id),
  INDEX (job_id),
  INDEX (file_state,job_id),
  INDEX (job_finished),
  INDEX (job_id, finish_time)
);


--
-- t_stage_req table stores the data related to a file orestaging request
--
CREATE TABLE t_stage_req (
--
-- transfer request id
  request_id           VARCHAR(255) NOT NULL,
--
-- Identifier of the file to transfer
  file_id              INTEGER NOT NULL,
-- 
-- file identifier within the request. It could be used for ordering
  stage_id             INTEGER NOT NULL,
--
-- Identifier of the job owning the file to transfer
  job_id               CHAR(36) NOT NULL,
--
-- The state of this file
  stage_state          VARCHAR(32) NOT NULL,
--
-- The Returned TURL
  turl                 VARCHAR(1100),
--
-- the time at which the stage request was started
  request_time         TIMESTAMP,
--
-- the total duration in seconds
  duration             INTEGER,
--
-- The finalization duration
  final_duration       INTEGER,
--
-- The error scope
  error_scope          VARCHAR(32),
--
-- The FTS phase when the error happened
  error_phase          VARCHAR(32),
--
-- The class for the reason field
  reason_class         VARCHAR(32),
--
-- The reason the transfer is in this state
  reason               VARCHAR(2048),
--
-- the nominal size of the file (bytes)
  filesize             BIGINT,
--
-- the time at which the transfer finished (successfully or not)
  finish_time          TIMESTAMP,
--
-- this timestamp will be set when the job enter in one of the terminal 
-- states (Finished, FinishedDirty, Failed, Canceled). Use for table
-- partitioning
  job_finished         TIMESTAMP NULL DEFAULT NULL,
--
-- Set primary key
  PRIMARY KEY (request_id, file_id),
  FOREIGN KEY (file_id) REFERENCES t_file(file_id),
  FOREIGN KEY (job_id) REFERENCES t_job(job_id),
  INDEX (request_id),
  INDEX (file_id),
  INDEX (job_id),
  INDEX (stage_state),
  INDEX (job_finished)
);


-- 
--
-- Schema version
--
CREATE TABLE t_schema_vers (
  major INT(2) NOT NULL,
  minor INT(2) NOT NULL,
  patch INT(2) NOT NULL,
  --
  -- save a state when upgrading the schema
  state VARCHAR(24)
);
INSERT INTO t_schema_vers (major,minor,patch) VALUES (1,0,0);
