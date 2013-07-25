--
-- Holds various server configuration options
--
CREATE TABLE t_server_config (
  retry       INTEGER default 0,
  max_time_queue INTEGER default 0
);
insert into t_server_config(retry,max_time_queue) values(0,0);




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
   source_se	VARCHAR2(255),
--
-- dest se
   dest_se	VARCHAR2(255),
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
   datetime			TIMESTAMP WITH TIME ZONE
);

--
-- Historial optimizer evolution
--
CREATE TABLE t_optimizer_evolution (
    datetime     TIMESTAMP,
    source_se    VARCHAR(255),
    dest_se      VARCHAR(255),
    nostreams    NUMBER DEFAULT NULL,
    timeout      NUMBER DEFAULT NULL,
    active       NUMBER DEFAULT NULL,
    throughput   NUMBER DEFAULT NULL,
    buffer       NUMBER DEFAULT NULL,
    filesize     NUMBER DEFAULT NULL
);
CREATE INDEX t_optimizer_source_and_dest ON t_optimizer_evolution(source_se, dest_se);

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
   source_se	VARCHAR2(255),
--
-- dest hostanme
   dest_se		VARCHAR2(255),
--
-- debug on/off
   debug		VARCHAR2(3) default 'off'
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
  ,name VARCHAR2(255) not null
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
  ,gocdb_id VARCHAR2(100)
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
   ,CONSTRAINT t_share_config_fk FOREIGN KEY (source, destination) REFERENCES t_link_config (source, destination)
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
   vo_name		VARCHAR2(50)
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
  ,source_se         VARCHAR2(255)
--
-- Dest SE host name
  ,dest_se           VARCHAR2(255)
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
  ,vo_name              VARCHAR2(50)
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
  ,job_metadata    VARCHAR2(255)
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
  ,source_se         VARCHAR2(255)
--
-- Dest SE host name
  ,dest_se           VARCHAR2(255)
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
-- How many times should the transfer be retried
  ,retry           NUMBER DEFAULT 0
  ,staging_start          TIMESTAMP WITH TIME ZONE DEFAULT NULL
  ,staging_finished       TIMESTAMP WITH TIME ZONE DEFAULT NULL
--
-- user provided size of the file (bytes)
  ,user_filesize         	INTEGER
--
-- user provided metadata
  ,file_metadata    VARCHAR2(255)
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
);



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
   vo_name           VARCHAR2(255) CONSTRAINT stagereq_vo_name_not_null NOT NULL
--
-- hostname
   ,host           VARCHAR2(255) CONSTRAINT stagereq_host_not_null NOT NULL
--
-- parallel bringonline ops
  ,concurrent_ops              INTEGER DEFAULT 0
--
-- Set primary key
  ,CONSTRAINT stagereq_pk PRIMARY KEY (vo_name, host)
);



--
--
-- Index Section
--
--

-- t_job indexes:
-- t_job(job_id) is primary key
CREATE INDEX job_job_state    ON t_job(job_state);
CREATE INDEX job_vo_name      ON t_job(vo_name);
CREATE INDEX job_cred_id      ON t_job(user_dn,cred_id);
CREATE INDEX job_jobfinished_id     ON t_job(job_finished);
CREATE INDEX job_priority     ON t_job(priority);
CREATE INDEX job_submit_time     ON t_job(submit_time);
CREATE INDEX job_priority_s_time     ON t_job(priority,submit_time);
CREATE INDEX job_list     ON t_job(job_id, job_state, reason, submit_time, user_dn,vo_name, priority, cancel_job);

-- t_file indexes:
-- t_file(file_id) is primary key
CREATE INDEX file_job_id     ON t_file(job_id);
CREATE INDEX file_file_state_job_id ON t_file(file_state,file_id);
CREATE INDEX file_jobfinished_id ON t_file(job_finished);
CREATE INDEX file_job_id_a ON t_file(job_id, FINISH_TIME);
CREATE INDEX file_finish_time ON t_file(finish_time);
CREATE INDEX file_file_index ON t_file(file_index);
CREATE INDEX file_job_id_file_index ON t_file(job_id, file_index);
CREATE INDEX file_retry_timestamp ON t_file(retry_timestamp);
CREATE INDEX file_file_throughput ON t_file(throughput);
CREATE INDEX file_file_src_dest ON t_file(source_se, dest_se);
CREATE INDEX file_file_src_dest_job_id ON t_file(source_se, dest_se, job_id);
CREATE INDEX file_file_state_job_id4 ON t_file(file_state, dest_se);
CREATE INDEX file_pid_job_id ON t_file(pid, job_id);


CREATE INDEX optimize_active         ON t_optimize(active);
CREATE INDEX optimize_source_a         ON t_optimize(source_se,dest_se);
CREATE INDEX optimize_dest_se           ON t_optimize(dest_se);
CREATE INDEX optimize_nostreams         ON t_optimize(nostreams);
CREATE INDEX optimize_timeout           ON t_optimize(timeout);
CREATE INDEX optimize_buffer            ON t_optimize(buffer);
CREATE INDEX optimize_order         ON t_optimize(nostreams,timeout,buffer);
CREATE INDEX optimize_prot         ON t_optimize(nostreams,active,throughput);
CREATE INDEX optimize_prot2         ON t_optimize(throughput, active, nostreams, timeout, buffer);



CREATE INDEX t_server_config_max_time         ON t_server_config(max_time_queue);
CREATE INDEX t_server_config_retry         ON t_server_config(retry);

CREATE index idx_report_job      ON t_job (vo_name,job_id);


-- Config index
CREATE INDEX t_group_members  ON t_group_members(groupName);

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

CREATE INDEX t_job_backup_1            ON t_job_backup(job_id);
CREATE INDEX t_file_backup_1            ON t_file_backup(file_id);



exit;
