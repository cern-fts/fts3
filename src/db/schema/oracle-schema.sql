--
-- Only one host at a time must run sanity checks
--
CREATE TABLE t_server_sanity (
  revertToSubmitted         INT DEFAULT 0,
  cancelWaitingFiles        INT DEFAULT 0,
  revertNotUsedFiles        INT DEFAULT 0,
  forceFailTransfers        INT DEFAULT 0,
  setToFailOldQueuedJobs    INT DEFAULT 0,
  checkSanityState          INT DEFAULT 0,
  cleanUpRecords            INT DEFAULT 0,
  msgcron            	    INT DEFAULT 0,
  t_revertToSubmitted       TIMESTAMP WITH TIME ZONE,
  t_cancelWaitingFiles      TIMESTAMP WITH TIME ZONE,
  t_revertNotUsedFiles      TIMESTAMP WITH TIME ZONE,
  t_forceFailTransfers      TIMESTAMP WITH TIME ZONE,
  t_setToFailOldQueuedJobs  TIMESTAMP WITH TIME ZONE,
  t_checkSanityState        TIMESTAMP WITH TIME ZONE,
  t_cleanUpRecords          TIMESTAMP WITH TIME ZONE,
  t_msgcron                 TIMESTAMP WITH TIME ZONE  
); 
INSERT INTO t_server_sanity (revertToSubmitted, cancelWaitingFiles, revertNotUsedFiles, forceFailTransfers, setToFailOldQueuedJobs, checkSanityState, cleanUpRecords,msgcron, t_revertToSubmitted, t_cancelWaitingFiles, t_revertNotUsedFiles, t_forceFailTransfers, t_setToFailOldQueuedJobs, t_checkSanityState, t_cleanUpRecords, t_msgcron) VALUES (0, 0, 0, 0, 0, 0, 0, 0,CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);

--
-- Holds various server configuration options
--
CREATE TABLE t_server_config (
  retry       INTEGER default 0,
  max_time_queue INTEGER default 0,
  global_timeout INTEGER default 0,
  sec_per_mb INTEGER default 0,
  vo_name VARCHAR2(100),
  show_user_dn VARCHAR2(3) CHECK (show_user_dn in ('on', 'off'))
);
insert into t_server_config(retry,max_time_queue,global_timeout,sec_per_mb) values(0,0,0,0);

--
-- Holds the optimizer mode
--
CREATE TABLE t_optimize_mode (
  mode_opt       INTEGER DEFAULT 1
);

--
-- Holds optimization parameters
--
CREATE TABLE t_optimize (
--
   auto_number		INTEGER
			CONSTRAINT t_optimize_a_not_null NOT NULL		     
		        CONSTRAINT t_optimize_anumber_pk PRIMARY KEY,
--
-- file id
   file_id	INTEGER NOT NULL,
--
-- source se
   source_se	VARCHAR2(150),
--
-- dest se
   dest_se	VARCHAR2(150),
--
-- number of streams
   nostreams       	NUMBER default NULL,
--
-- timeout
   timeout       	NUMBER default NULL,
--
-- active transfers
   active       	NUMBER default NULL,
--
-- throughput
   throughput       	NUMBER default NULL,
--
-- tcp buffer size
   buffer       	NUMBER default NULL,
--
-- the nominal size of the file (bytes)
  filesize         	NUMBER default NULL,
--
-- timestamp
   datetime			TIMESTAMP WITH TIME ZONE,
--
-- udt
   udt VARCHAR2(3) CHECK (udt in ('on', 'off')),
--
-- IPv6
   ipv6 VARCHAR2(3) CHECK (ipv6 in ('on', 'off'))
);

--
-- Historial optimizer evolution
--
CREATE TABLE t_optimizer_evolution (
    datetime     TIMESTAMP WITH TIME ZONE,
    source_se    VARCHAR(150),
    dest_se      VARCHAR(150),
    nostreams    NUMBER DEFAULT NULL,
    timeout      NUMBER DEFAULT NULL,
    active       NUMBER DEFAULT NULL,
    throughput   NUMBER DEFAULT NULL,
    buffer       NUMBER DEFAULT NULL,
    filesize     NUMBER DEFAULT NULL,
    agrthroughput   NUMBER DEFAULT NULL
);
CREATE INDEX t_optimizer_source_and_dest ON t_optimizer_evolution(source_se, dest_se);
CREATE INDEX t_optimizer_evolution_datetime ON t_optimizer_evolution(datetime);

--
-- Holds certificate request information
--
CREATE TABLE t_config_audit (
--
-- timestamp
   datetime		TIMESTAMP WITH TIME ZONE,
--
-- dn
   dn			VARCHAR2(1024),
--
-- what has changed
   config		VARCHAR2(4000),
--
-- action (insert/update/delete)
   action		VARCHAR2(100)
);


--
-- Configures debug mode for a given pair
--
CREATE TABLE t_debug (
--
-- source hostname
   source_se	VARCHAR2(150),
--
-- dest hostanme
   dest_se		VARCHAR2(150),
--
-- debug on/off
   debug		VARCHAR2(3) default 'off',
--
-- debug level
  debug_level           INTEGER DEFAULT 1
);


--
-- Holds certificate request information
--
CREATE TABLE t_credential_cache (
--
-- delegation identifier
   dlg_id	VARCHAR2(100),
--
-- DN of delegated proxy owner
   dn		VARCHAR2(255),
--
-- certificate request
   cert_request CLOB default empty_clob(),
--
-- private key of request
   priv_key	CLOB default empty_clob(),
--
-- list of voms attributes contained in delegated proxy
   voms_attrs CLOB default empty_clob(),
--
-- set primary key
   CONSTRAINT cred_cache_pk PRIMARY KEY (dlg_id, dn)
);

--
-- Holds delegated proxies
--
CREATE TABLE t_credential (
--
-- delegation identifier
   dlg_id	VARCHAR2(100),
--
-- DN of delegated proxy owner
   dn		VARCHAR2(255),
--
-- delegated proxy certificate chain
   proxy CLOB default empty_clob(),
--
-- list of voms attributes contained in delegated proxy
   voms_attrs CLOB default empty_clob(),
--
-- termination time of the credential
   termination_time TIMESTAMP WITH TIME ZONE
        CONSTRAINT cred_term_time_not_null NOT NULL,
--
-- set primary key
   CONSTRAINT cred_pk PRIMARY KEY (dlg_id, dn)
);

--
-- Schema version
--
CREATE TABLE t_credential_vers (
  major NUMBER NOT NULL,
  minor NUMBER NOT NULL,
  patch NUMBER NOT NULL
);
INSERT INTO t_credential_vers (major,minor,patch) VALUES (1,2,0);

-- t_credential index:
-- t_credential(dlg,dn) is primary key
CREATE INDEX credential_term_time ON t_credential(termination_time);

--
-- SE from the information service, currently BDII
--

CREATE TABLE t_se (
-- The internal id
   se_id_info INTEGER
  ,name VARCHAR2(255) NOT NULL
  ,endpoint VARCHAR2(1024)
  ,se_type VARCHAR2(30)
  ,site VARCHAR2(100)
  ,state VARCHAR2(30)
  ,version VARCHAR2(30)
-- This field will contain the host parse for FTS and extracted from name
  ,host varchar2(100)
  ,se_transfer_type VARCHAR2(30)
  ,se_transfer_protocol VARCHAR2(30)
  ,se_control_protocol VARCHAR2(30)
  ,gocdb_id             VARCHAR2(100)
  ,CONSTRAINT se_info_pk PRIMARY KEY (name)
);

--
-- relation of SE and VOs
--
CREATE TABLE t_se_acl (
  name varchar2(255)
  ,vo varchar2(32)
  ,CONSTRAINT se_acl_pk PRIMARY KEY (name, vo)
);

--
-- autoinc sequence on se_id_info
--
CREATE SEQUENCE se_id_info_seq;

CREATE OR REPLACE TRIGGER se_id_info_auto_inc
BEFORE INSERT ON t_se
FOR EACH ROW
WHEN (new.se_id_info IS NULL)
BEGIN
  SELECT se_id_info_seq.nextval
  INTO   :new.se_id_info from dual;
END;
/

-- GROUP NAME and its members
CREATE TABLE t_group_members(
	groupName VARCHAR2(255) NOT NULL
	,member VARCHAR2(255) NOT NULL UNIQUE
	,CONSTRAINT t_group_members_pk PRIMARY KEY (groupName, member)
	,CONSTRAINT t_group_members_fk FOREIGN KEY (member) REFERENCES t_se (name)
);

-- SE HOSTNAME / GROUP NAME / *

CREATE TABLE t_link_config (
   source         VARCHAR2(255)   NOT NULL
   ,destination         VARCHAR2(255)   NOT NULL
   ,state VARCHAR2(30)  NOT NULL
   ,symbolicName         VARCHAR2(255)  NOT NULL UNIQUE
   ,nostreams       	INTEGER NOT NULL
   ,tcp_buffer_size     INTEGER DEFAULT 0
   ,urlcopy_tx_to      INTEGER NOT NULL
   ,no_tx_activity_to INTEGER DEFAULT 360
   ,auto_tuning VARCHAR2(3) check (auto_tuning in ('on', 'off', 'all'))
   ,placeholder1 INTEGER
   ,placeholder2 INTEGER
   ,placeholder3 VARCHAR2(255)
   ,CONSTRAINT t_link_config_pk PRIMARY KEY (source, destination)
);

CREATE TABLE t_share_config (
   source         VARCHAR2(255)   NOT NULL
   ,destination         VARCHAR2(255)   NOT NULL
   ,vo VARCHAR2(100) NOT NULL
   ,active INTEGER NOT NULL
   ,CONSTRAINT t_share_config_pk PRIMARY KEY (source, destination, vo)
   ,CONSTRAINT t_share_config_fk FOREIGN KEY (source, destination) REFERENCES t_link_config (source, destination) ON DELETE CASCADE
);

CREATE TABLE t_activity_share_config (
  vo             VARCHAR(100) NOT NULL PRIMARY KEY,
  activity_share VARCHAR(255) NOT NULL,
  active         VARCHAR(3) check (active in ('on', 'off'))
);

--
-- blacklist of bad SEs that should not be transferred to
--
CREATE TABLE t_bad_ses (
--
-- The hostname of the bad SE
   se         VARCHAR2(256)
--
-- The reason this host was added
   ,message             VARCHAR2(2048) DEFAULT NULL
--
-- The time the host was added
   ,addition_time       TIMESTAMP WITH TIME ZONE
--
-- The DN of the administrator who added it
   ,admin_dn            VARCHAR2(1024)
--
-- VO that is banned for the SE
   ,vo   				VARCHAR2(50) DEFAULT NULL
--
-- status: either CANCEL or WAIT or WAIT_AS
   ,status 				VARCHAR2(10) DEFAULT NULL
--
-- the timeout that is used when WAIT status was specified
   , wait_timeout 		NUMBER default 0
   ,CONSTRAINT bad_se_pk PRIMARY KEY (se)
);

--
-- blacklist of bad DNs that should not be transferred to
--
CREATE TABLE t_bad_dns (
--
-- The hostname of the bad SE
   dn         VARCHAR2(256)
--
-- The reason this host was added
   ,message             VARCHAR2(2048) DEFAULT NULL
--
-- The time the host was added
   ,addition_time       TIMESTAMP WITH TIME ZONE
--
-- The DN of the administrator who added it
   ,admin_dn            VARCHAR2(1024)
--
-- status: either CANCEL or WAIT
   ,status 				VARCHAR2(10) DEFAULT NULL
--
-- the timeout that is used when WAIT status was specified
   , wait_timeout 		NUMBER default 0
   ,CONSTRAINT bad_dn_pk PRIMARY KEY (dn)
);




--
-- Store se_pair ACL
--
CREATE TABLE t_se_pair_acl (
--
-- the name of the se_pair
   se_pair_name         VARCHAR2(32)
                    	CONSTRAINT se_pair_acl_ch_name_not_null NOT NULL
--
-- The principal name
  ,principal            VARCHAR2(255)
                    	CONSTRAINT se_pair_acl_pr_not_null NOT NULL
--
-- Set Primary Key
  ,CONSTRAINT se_pair_acl_pk PRIMARY KEY (se_pair_name, principal)
);

--
-- Store VO ACL
--
CREATE TABLE t_vo_acl (
--
-- the name of the VO
   vo_name		VARCHAR2(100)
			CONSTRAINT vo_acl_vo_name_not_null NOT NULL
--
-- The principal name
  ,principal            VARCHAR2(255)
                    	CONSTRAINT vo_acl_pr_not_null NOT NULL
--
-- Set Primary Key
  ,CONSTRAINT vo_acl_pk PRIMARY KEY (vo_name, principal)
);

--
-- t_job contains the list of jobs currently in the transfer database.
--
CREATE TABLE t_job (
--
-- the job_id, a IETF UUID in string form.
   job_id		CHAR(36)
                    	CONSTRAINT job_job_id_not_null NOT NULL
                    	CONSTRAINT job_job_id_pk PRIMARY KEY
--
-- The state the job is currently in
  ,job_state       	VARCHAR2(32)
                    	CONSTRAINT job_job_state_not_null NOT NULL
--
-- Session reuse for this job. Allowed values are Y, (N), NULL
  ,reuse_job           VARCHAR2(3)
--
-- Canceling flag. Allowed values are Y, (N), NULL
  ,cancel_job           CHAR(1)
--
-- Transport specific parameters
  ,job_params       	VARCHAR2(255)
--
-- Source SE host name
  ,source_se         VARCHAR2(150)
--
-- Dest SE host name
  ,dest_se           VARCHAR2(150)
--
-- the DN of the user starting the job - they are the only one
-- who can sumbit/cancel
  ,user_dn          	VARCHAR2(1024)
                    	CONSTRAINT job_user_dn_not_null NOT NULL
--
-- the DN of the agent currently serving the job
  ,agent_dn         	VARCHAR2(1024)
--
-- the user credentials passphrase. This is passed to the movement service in
-- order to retrieve the appropriate user proxy to do the transfers
  ,user_cred        	VARCHAR2(255)
--
-- The user's credential delegation id
  ,cred_id              VARCHAR2(100)
--
-- Blob to store user capabilites and groups
  ,voms_cred            BLOB
--
-- The VO that owns this job
  ,vo_name              VARCHAR2(100)
--
-- The reason the job is in the current state
  ,reason           	VARCHAR2(2048)
--
-- The time that the job was submitted
  ,submit_time      	TIMESTAMP WITH TIME ZONE DEFAULT SYSTIMESTAMP AT TIME ZONE '+00:00'
--
-- The time that the job was in a terminal state
  ,finish_time      	TIMESTAMP WITH TIME ZONE
--
-- Priority for Intra-VO Scheduling
  ,priority      	INTEGER DEFAULT 3
--
-- Submitting FTS hostname
  ,submit_host		VARCHAR2(255)
--
-- Maximum time in queue before start of transfer (in seconds)
  ,max_time_in_queue	INTEGER
--
-- The Space token to be used for the destination files
  ,space_token          VARCHAR2(255)
--
-- The Storage Service Class to be used for the destination files
  ,storage_class        VARCHAR2(255)
--
-- The endpoint of the MyProxy server that should be used if the
-- legacy cert retrieval is used
  ,myproxy_server       VARCHAR2(255)
--
-- Internal job parameters,used to pass job specific data from the
-- WS to the agent
  ,internal_job_params   VARCHAR2(255)
--
-- Overwrite flag for job
  ,overwrite_flag        CHAR(1) DEFAULT NULL
--
-- this timestamp will be set when the job enter in one of the terminal
-- states (Finished, FinishedDirty, Failed, Canceled). Use for table
-- partitioning
  ,job_finished          TIMESTAMP WITH TIME ZONE DEFAULT NULL
--
--	Space token of the source files
--
  ,source_space_token VARCHAR2(255)
--
-- description used by the agents to eventually get the source token.
--
  ,source_token_description VARCHAR2(255)
-- *** New in 3.3.0 ***
--
-- pin lifetime of the copy of the file created after a successful srmPutDone
-- or srmCopy operations, in seconds
  ,copy_pin_lifetime NUMBER default NULL
  ,bring_online NUMBER default NULL
-- fail the transfer immediately if the file location is NEARLINE (do not even
-- start the transfer). The default is false.
  ,fail_nearline CHAR(1) default NULL
--
-- Specified is the checksum is required on the source and destination, destination or none
  ,checksum_method CHAR(1) default NULL
 --
 -- Specifies how many configurations were assigned to the transfer-job
  ,configuration_count INTEGER default NULL
--
-- retry
  ,retry INTEGER default 0
--
-- retry delay
  ,retry_delay INTEGER default 0
--
-- user provided metadata
  ,job_metadata    VARCHAR2(1024)
  );

--
-- t_file stores the actual file transfers - one row per source/dest pair
--
CREATE TABLE t_file (
-- file_id is a unique identifier for a (source, destination) pair with a
-- job.  It is created automatically.
--
   file_id		INTEGER
			CONSTRAINT file_file_id_not_null NOT NULL		     
		        CONSTRAINT file_file_id_pk PRIMARY KEY
--
-- file index
   ,file_index       INTEGER
--
-- job_id (used in joins with file table)
   ,job_id		CHAR(36)
                    	CONSTRAINT file_job_id_not_null NOT NULL
			REFERENCES t_job(job_id)
--
-- The state of this file
  ,file_state		VARCHAR2(32)
                    	CONSTRAINT file_file_state_not_null NOT NULL
-- The Source Logical Name
  ,symbolicName      	VARCHAR2(255)
--
-- Hostname which this file was transfered
  ,transferHost       	VARCHAR2(255)
--
-- The Source
  ,source_surl      	VARCHAR2(1100)
--
-- The Destination
  ,dest_surl		VARCHAR2(1100)
--
-- Source SE host name
  ,source_se         VARCHAR2(150)
--
-- Dest SE host name
  ,dest_se           VARCHAR2(150)
--
-- The agent who is transferring the file. This is only valid when the file
-- is in 'Active' state
  ,agent_dn		VARCHAR2(1024)
--
-- The error scope
  ,error_scope          VARCHAR2(32)
--
-- The FTS phase when the error happened
  ,error_phase          VARCHAR2(32)
--
-- The class for the reason field
  ,reason_class		VARCHAR2(32)
--
-- The reason the file is in this state
  ,reason           	VARCHAR2(2048)
--
-- Total number of failures (including transfer,catalog and prestaging errors)
  ,num_failures         INTEGER
--
-- Number of transfer failures in last attemp cycle (reset at the Hold->Pending transition)
  ,current_failures	INTEGER
--
-- Number of catalog failures (not reset at the Hold->Pending transition)
  ,catalog_failures	INTEGER
--
-- Number of prestaging failures (reset at the Hold->Pending transition)
  ,prestage_failures	INTEGER
--
-- the nominal size of the file (bytes)
  ,filesize         	INTEGER
--
-- the user-defined checksum of the file "checksum_type:checksum"
  ,checksum         	VARCHAR2(100)
--
-- the timestamp when the file is in a terminal state
  ,finish_time		TIMESTAMP WITH TIME ZONE
--
-- the timestamp when the file is in a terminal state
  ,start_time		TIMESTAMP WITH TIME ZONE
--
-- internal file parameters for storing information between retry attempts
  ,internal_file_params    VARCHAR2(255)
--
-- this timestamp will be set when the job enter in one of the terminal
-- states (Finished, FinishedDirty, Failed, Canceled). Use for table
-- partitioning
  ,job_finished          TIMESTAMP WITH TIME ZONE DEFAULT NULL
--
-- the pid of the process which is executing the file transfer
  ,pid INTEGER
--
-- transfer duration
  ,TX_DURATION		NUMBER
--
-- Average throughput
  ,throughput           NUMBER
--
-- Transferred bytes
  ,transferred          NUMBER DEFAULT 0
--
-- How many times should the transfer be retried
  ,retry           NUMBER DEFAULT 0
  ,staging_start          TIMESTAMP WITH TIME ZONE DEFAULT NULL
  ,staging_finished       TIMESTAMP WITH TIME ZONE DEFAULT NULL
--
-- user provided size of the file (bytes)
  ,user_filesize         	INTEGER
--
-- user provided metadata
  ,file_metadata    VARCHAR2(1024)
--
-- activity name
  ,activity   VARCHAR(255) DEFAULT 'default'
--
-- selection strategy used in case when multiple protocols were provided
  ,selection_strategy VARCHAR(255)
--
-- bringonline token
  ,bringonline_token VARCHAR2(255)
--
-- the timestamp that the file will be retried
  ,retry_timestamp          TIMESTAMP WITH TIME ZONE DEFAULT NULL
--
--
  ,wait_timestamp		TIMESTAMP WITH TIME ZONE DEFAULT NULL
--
--
  ,wait_timeout			NUMBER
  ,t_log_file        VARCHAR2(2048)
  ,t_log_file_debug  INTEGER
--
  ,hashed_id       INTEGER DEFAULT 0
--
-- The VO that owns this job
  ,vo_name              VARCHAR(100)  
);




--
-- t_dm stores the actual file transfers - one row per source/dest pair
--
CREATE TABLE t_dm (
-- file_id is a unique identifier for a (source, destination) pair with a
-- job.  It is created automatically.
--
   file_id		INTEGER
			CONSTRAINT dm_file_id_not_null NOT NULL		     
		        CONSTRAINT dm_file_id_pk PRIMARY KEY
--
-- file index
   ,file_index       INTEGER
--
-- job_id (used in joins with file table)
   ,job_id		CHAR(36)
                    	CONSTRAINT dm_job_id_not_null NOT NULL
			REFERENCES t_job(job_id)
--
-- The state of this file
  ,file_state		VARCHAR2(32)
                    	CONSTRAINT dm_file_state_not_null NOT NULL
--
-- Hostname which this file was transfered
  ,dmHost       	VARCHAR2(150)
--
-- The Source
  ,source_surl      	VARCHAR2(900)
--
-- The Destination
  ,dest_surl		VARCHAR2(900)
--
-- Source SE host name
  ,source_se         VARCHAR2(150)
--
-- Dest SE host name
  ,dest_se           VARCHAR2(150)
--
-- The reason the file is in this state
  ,reason           	VARCHAR2(2048)
--
-- the user-defined checksum of the file "checksum_type:checksum"
  ,checksum         	VARCHAR2(100)
--
-- the timestamp when the file is in a terminal state
  ,finish_time		TIMESTAMP WITH TIME ZONE DEFAULT NULL
--
-- the timestamp when the file is in a terminal state
  ,start_time		TIMESTAMP WITH TIME ZONE DEFAULT NULL
--
-- this timestamp will be set when the job enter in one of the terminal
-- states (Finished, FinishedDirty, Failed, Canceled). Use for table
-- partitioning
  ,job_finished          TIMESTAMP WITH TIME ZONE DEFAULT NULL
--
-- transfer duration
  ,TX_DURATION		NUMBER
--
-- How many times should the transfer be retried
  ,retry           NUMBER DEFAULT 0
--
-- user provided size of the file (bytes)
  ,user_filesize         	INTEGER
--
-- user provided metadata
  ,file_metadata    VARCHAR2(1024)
--
-- the timestamp that the file will be retried
  ,retry_timestamp          TIMESTAMP WITH TIME ZONE DEFAULT NULL
--
--
  ,wait_timestamp		TIMESTAMP WITH TIME ZONE DEFAULT NULL
--
--
  ,wait_timeout			NUMBER
--
  ,hashed_id       INTEGER DEFAULT 0
--
-- The VO that owns this job
  ,vo_name              VARCHAR(100)  
);










--
-- Keep error reason that drove to retries
--
CREATE TABLE t_file_retry_errors (
    file_id   INTEGER NOT NULL,
    attempt   INTEGER NOT NULL,
    datetime  TIMESTAMP WITH TIME ZONE,
    reason    VARCHAR2(2048),
    CONSTRAINT t_file_retry_errors_pk PRIMARY KEY(file_id, attempt),
    CONSTRAINT t_file_retry_fk FOREIGN KEY (file_id) REFERENCES t_file(file_id) ON DELETE CASCADE
);
CREATE INDEX t_file_retry_fid ON t_file_retry_errors (file_id);

--
-- t_file_share_config the se configuration to be used by the job
--
CREATE TABLE t_file_share_config (
   file_id			INTEGER 		NOT NULL
   ,source    		VARCHAR2(255)   NOT NULL
   ,destination     VARCHAR2(255)   NOT NULL
   ,vo 				VARCHAR2(100) 	NOT NULL
   ,CONSTRAINT t_file_share_config_pk PRIMARY KEY (file_id, source, destination, vo)
   ,CONSTRAINT t_share_config_fk1 FOREIGN KEY (source, destination, vo) REFERENCES t_share_config (source, destination, vo) ON DELETE CASCADE
   ,CONSTRAINT t_share_config_fk2 FOREIGN KEY (file_id) REFERENCES t_file (file_id) ON DELETE CASCADE
);

-- 
-- t_turl store the turls used for a given surl
--
CREATE TABLE t_turl (
  source_surl     VARCHAR2(150)   NOT NULL,
  destin_surl     VARCHAR2(150)   NOT NULL,
  source_turl     VARCHAR2(150)   NOT NULL,
  destin_turl     VARCHAR2(150)   NOT NULL,
  datetime        TIMESTAMP WITH TIME ZONE,
  throughput      NUMBER DEFAULT NULL,
  finish          NUMBER DEFAULT 0,
  fail     	  NUMBER DEFAULT 0,
  CONSTRAINT t_turl_pk PRIMARY KEY (source_surl, destin_surl, source_turl, destin_turl)
);


--
-- autoinc sequence on file_id
--
CREATE SEQUENCE file_file_id_seq;

CREATE OR REPLACE TRIGGER file_file_id_auto_inc
BEFORE INSERT ON t_file
FOR EACH ROW
WHEN (new.file_id IS NULL)
BEGIN
  SELECT file_file_id_seq.nextval
  INTO   :new.file_id from dual;
END;
/


--
-- autoinc sequence on file_id (t_dm)
--
CREATE SEQUENCE dm_file_id_seq;

CREATE OR REPLACE TRIGGER dm_file_id_auto_inc
BEFORE INSERT ON t_dm
FOR EACH ROW
WHEN (new.file_id IS NULL)
BEGIN
  SELECT dm_file_id_seq.nextval
  INTO   :new.file_id from dual;
END;
/


--
-- autoinc sequence on auto_number
--
CREATE SEQUENCE t_optimize_auto_number_seq;

CREATE OR REPLACE TRIGGER t_optimize_auto_number_inc
BEFORE INSERT ON t_optimize
FOR EACH ROW
WHEN (new.auto_number IS NULL)
BEGIN
  SELECT t_optimize_auto_number_seq.nextval
  INTO   :new.auto_number from dual;
END;
/


--
-- t_stage_req table stores the data related to a file orestaging request
--
CREATE TABLE t_stage_req (
--
-- vo name
   vo_name           VARCHAR2(100) CONSTRAINT stagereq_vo_name_not_null NOT NULL
--
-- hostname
   ,host           VARCHAR2(255) CONSTRAINT stagereq_host_not_null NOT NULL
--
-- operation
   ,operation           VARCHAR2(100) CONSTRAINT stagereq_operation_not_null NOT NULL
-- parallel bringonline ops
  ,concurrent_ops              INTEGER DEFAULT 0
--
-- Set primary key
  ,CONSTRAINT stagereq_pk PRIMARY KEY (vo_name, host, operation)
);

--
-- Host hearbeats
--
CREATE TABLE t_hosts (
    hostname    VARCHAR2(64) NOT NULL,
    service_name    VARCHAR2(64) NOT NULL,    
    beat        TIMESTAMP WITH TIME ZONE DEFAULT NULL,
    drain 	INTEGER DEFAULT 0,
    CONSTRAINT t_hosts_pk PRIMARY KEY (hostname, service_name)
);


CREATE TABLE t_optimize_active (
  source_se    VARCHAR2(150) NOT NULL,
  dest_se      VARCHAR2(150) NOT NULL,
  active       INTEGER DEFAULT 2,
  message      VARCHAR2(512),
  datetime  TIMESTAMP WITH TIME ZONE,
  ema     	  NUMBER DEFAULT 0,
  fixed       VARCHAR2(3) CHECK (fixed in ('on', 'off')),
  CONSTRAINT t_optimize_active_pk PRIMARY KEY (source_se, dest_se)
);

CREATE TABLE t_optimize_streams (
  source_se    VARCHAR2(150) NOT NULL,
  dest_se      VARCHAR2(150) NOT NULL,  
  datetime     TIMESTAMP WITH TIME ZONE DEFAULT NULL,
  nostreams    INTEGER NOT NULL, 
  throughput   FLOAT DEFAULT NULL,
  CONSTRAINT t_optimize_streams_pk PRIMARY KEY (source_se, dest_se, nostreams),
  CONSTRAINT t_optimize_streams_fk FOREIGN KEY (source_se, dest_se) REFERENCES t_optimize_active (source_se, dest_se) ON DELETE CASCADE
);


--
--
-- Index Section 
--
--
-- t_job indexes:
-- t_job(job_id) is primary key
CREATE INDEX job_job_state    ON t_job(job_state, vo_name, job_finished, submit_time);
CREATE INDEX job_vo_name      ON t_job(vo_name);
CREATE INDEX job_cred_id      ON t_job(user_dn,cred_id);
CREATE INDEX job_jobfinished_id     ON t_job(job_finished);
CREATE INDEX job_priority     ON t_job(priority, submit_time);
CREATE INDEX t_job_submit_host ON t_job(submit_host);

-- t_file indexes:
-- t_file(file_id) is primary key
CREATE INDEX file_job_id     ON t_file(job_id);
CREATE INDEX file_jobfinished_id ON t_file(job_finished);
CREATE INDEX job_reuse  ON t_job(reuse_job);
CREATE INDEX file_source_dest ON t_file(source_se, dest_se, file_state); 
CREATE INDEX t_waittimeout ON t_file(wait_timeout);
CREATE INDEX file_id_hashed ON t_file(hashed_id, file_state);
CREATE INDEX t_retry_timestamp ON t_file(retry_timestamp);
CREATE INDEX t_file_select ON t_file(dest_se, source_se, job_finished, file_state );
CREATE INDEX file_vo_name_state ON t_file(file_state, vo_name, source_se, dest_se);
CREATE INDEX file_vo_name ON t_file( vo_name, source_se, dest_se, file_state);
CREATE INDEX file_tr_host  ON t_file(TRANSFERHOST);
CREATE INDEX t_file_activity ON t_file(activity);

CREATE INDEX optimize_source_a         ON t_optimize(source_se,dest_se);

CREATE INDEX t_url_datetime ON t_turl(datetime);
CREATE INDEX t_url_finish ON t_turl(finish);
CREATE INDEX t_url_fail ON t_turl(fail);


CREATE INDEX t_dm_job_id  ON t_dm(job_id);
CREATE INDEX t_dm_all  ON t_dm(vo_name, source_se, file_state);
CREATE INDEX t_dm_source  ON t_dm(source_se, file_state);
CREATE INDEX t_dm_state  ON t_dm(file_state, hashed_id);

CREATE INDEX t_optimize_active_datetime  ON t_optimize_active(datetime);

--
--
-- Schema version
--
CREATE TABLE t_schema_vers (
  major NUMBER(2) NOT NULL,
  minor NUMBER(2) NOT NULL,
  patch NUMBER(2) NOT NULL,
  --
  -- save a state when upgrading the schema
  state VARCHAR2(24)
);
INSERT INTO t_schema_vers (major,minor,patch) VALUES (1,0,0);

CREATE TABLE t_file_backup AS (SELECT * FROM t_file);
CREATE TABLE t_job_backup  AS (SELECT * FROM t_job);
CREATE TABLE t_dm_backup AS (SELECT * FROM t_dm);

CREATE INDEX t_job_backup_1            ON t_job_backup(job_id);
CREATE INDEX t_file_backup_1            ON t_file_backup(job_id);


-- Profiling information
CREATE TABLE t_profiling_info (
    period  INT NOT NULL,
    updated TIMESTAMP NOT NULL
);

CREATE TABLE t_profiling_snapshot (
    scope      VARCHAR(255) NOT NULL PRIMARY KEY,
    cnt        INT NOT NULL,
    exceptions INT NOT NULL,
    total      NUMBER NOT NULL,
    average    NUMBER NOT NULL
);

CREATE INDEX t_prof_snapshot_total ON t_profiling_snapshot(total);

--
-- Tables for cloud support
--
CREATE TABLE t_cloudStorage (
    cloudStorage_name VARCHAR2(50) NOT NULL PRIMARY KEY,
    app_key           VARCHAR2(255),
    app_secret        VARCHAR2(255),
    service_api_url   VARCHAR2(1024)
);

CREATE TABLE t_cloudStorageUser (
    user_dn              VARCHAR2(700) NULL,
    vo_name              VARCHAR2(100) NULL,
    cloudStorage_name    VARCHAR2(36) NOT NULL,
    access_token         VARCHAR2(255),
    access_token_secret  VARCHAR2(255),
    request_token        VARCHAR2(255),
    request_token_secret VARCHAR2(255),
    FOREIGN KEY (cloudStorage_name) REFERENCES t_cloudStorage(cloudStorage_name),
    PRIMARY KEY (user_dn, vo_name, cloudStorage_name)
);

exit;

