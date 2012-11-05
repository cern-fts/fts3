--
-- Holds the log files path and host
--
CREATE TABLE t_log (
  path       VARCHAR2(255),
  job_id     CHAR(36),
  dn         VARCHAR2(255),
  vo         VARCHAR2(255),
  when       TIMESTAMP WITH TIME ZONE,
  CONSTRAINT t_log_pk PRIMARY KEY (path, job_id, dn, vo)
);

--
-- Holds optimization parameters
--
CREATE TABLE t_optimize (
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
   when			TIMESTAMP WITH TIME ZONE  
);


--
-- Holds certificate request information
--
CREATE TABLE t_config_audit (
--
-- timestamp
   when			TIMESTAMP WITH TIME ZONE,
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
  ,endpoint VARCHAR2(1024)
  ,se_type VARCHAR2(30)
  ,site VARCHAR2(100)
  ,name VARCHAR2(512) not null
  ,state VARCHAR2(30)
  ,version VARCHAR2(30)
-- This field will contain the host parse for FTS and extracted from name 
  ,host varchar2(100)
  ,se_transfer_type VARCHAR2(30)
  ,se_transfer_protocol VARCHAR2(30)
  ,se_control_protocol VARCHAR2(30)
  ,gocdb_id VARCHAR2(100)
--  ,CONSTRAINT constraint_name PRIMARY KEY (se_id_info, name, se_transfer_protocol)
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
-- se, se pair and se groups in the system
--
CREATE TABLE t_se_protocol (
   se_row_id		INTEGER
			CONSTRAINT se_row_id_not_null NOT NULL
		        CONSTRAINT se_row_id_pk PRIMARY KEY
--
-- Name of the se_pair
   ,se_group_name     	VARCHAR2(512)
--          
-- Source site name
   ,se_name        	VARCHAR2(512)	
--
-- Email contact of the se_pair responsbile
   ,contact         	VARCHAR2(255)
--
-- Maximum bandwidth, capacity, in Mbits/s
   ,bandwidth       	NUMBER default NULL
--
-- The target number of concurrent streams on the network
   ,nostreams       	INTEGER default NULL
--
-- The target number of concurrent files on the network
   ,nofiles             INTEGER default NULL
--
-- Default TCP Buffer Size for the transfer
   ,tcp_buffer_size     INTEGER default NULL
--
-- The target throughput for the system (Mbits/s)
   ,nominal_throughput	NUMBER default NULL
--
-- The state of the se_pair ("Active", "Inactive", "Drain", "Stopped")
   ,se_pair_state	VARCHAR2(30)
--
-- The time the se_pair was last active
   ,last_active		TIMESTAMP WITH TIME ZONE
-- 
-- The Message concerning Last Modification 
   ,message             VARCHAR2(1024)
--   
-- Last Modification time
   ,last_modification   TIMESTAMP WITH TIME ZONE
--
-- The DN of the administrator who did last modification
   ,admin_dn            VARCHAR2(1024)
--
-- per-SE limit on se_pair
   ,se_limit         INTEGER DEFAULT NULL
--
   ,blocksize         INTEGER  default NULL
-- HTTP timeout
--
   ,http_to            INTEGER default NULL
-- Transfer log level. Allowed values are (DEBUG, INFO, WARN, ERROR)
--
   ,tx_loglevel        VARCHAR2(12) default NULL
-- urlcopy put timeout
--
   ,urlcopy_put_to     INTEGER default NULL
-- urlcopy putdone timeout
--
   ,urlcopy_putdone_to INTEGER default NULL
-- urlcopy get timeout
--
   ,urlcopy_get_to     INTEGER default NULL
-- urlcopy getdone timeout
--
   ,urlcopy_getdone_to INTEGER default NULL
-- urlcopy transfer5 timeout
--
   ,urlcopy_tx_to      INTEGER default NULL
-- urlcopy transfer markers timeout
--
   ,urlcopy_txmarks_to INTEGER default NULL
-- srmcopy direction
--
   ,srmcopy_direction  VARCHAR2(4) default NULL
-- srmcopy transfer timeout
--
   ,srmcopy_to        INTEGER default NULL
-- srmcopy refresh timeout (timeout since last status update)
--
   ,srmcopy_refresh_to INTEGER default NULL
--
-- check that target directory is accessible
--
   ,target_dir_check CHAR(1) default NULL
--
-- The parameter to set after how many seconds to mark the first transfer activity
--
   ,url_copy_first_txmark_to INTEGER default NULL
--
-- size-based transfer timeout, in seconds per MB
  ,tx_to_per_mb NUMBER default NULL 
-- 
-- maximum interval with no activity detected during the transfer, in seconds
  ,no_tx_activity_to INTEGER default NULL
--
-- maximum number of files in the preparation phase = nofiles * preparing_files_ratio
-- preparing_files_ratio default = 2. urlcopy se_pairs only.
  ,preparing_files_ratio NUMBER default NULL
--
--
--   ,CONSTRAINT tx_to_per_mb_value CHECK (tx_to_per_mb >= 0)
--   ,CONSTRAINT no_tx_activity_to_value CHECK (no_tx_activity_to >= 0)
--   ,CONSTRAINT preparing_files_ratio_value CHECK (preparing_files_ratio >= 1) 
);


--
-- blacklist of bad SEs that should not be transferred to
--
CREATE TABLE t_bad_ses (
--
-- The internal id
   se_id                INTEGER
                        CONSTRAINT bad_ses_id_pk PRIMARY KEY   
--
-- The hostname of the bad SE   
   ,se_hostname         VARCHAR2(256)
--
-- The reason this host was added 
   ,message             VARCHAR2(2048) DEFAULT NULL
--
-- The time the host was added
   ,addition_time       TIMESTAMP WITH TIME ZONE
--
-- The DN of the administrator who added it
   ,admin_dn            VARCHAR2(1024)
);

--
-- autoinc sequence on se_id
--
CREATE SEQUENCE se_bad_id_seq;

CREATE OR REPLACE TRIGGER bad_se_id_auto_inc
BEFORE INSERT ON t_bad_ses
FOR EACH ROW
WHEN (new.se_id IS NULL)
BEGIN
  SELECT se_bad_id_seq.nextval
  INTO   :new.se_id from dual;
END;
/


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

--
-- Table for saving the site-group association. As convention, group names should be between "[""]"
-- 
CREATE TABLE t_site_group (
-- name of the group 
--
   group_name     VARCHAR2(100)
                  CONSTRAINT site_group_gname_not_null NOT NULL
-- name of the site
-- 
  ,site_name      VARCHAR2(100)
                  CONSTRAINT site_group_sname_not_null NOT NULL
-- priority used for ordering
--
  ,priority       INTEGER DEFAULT 3
-- define private key
--
  ,CONSTRAINT sitegroup_pk PRIMARY KEY (group_name, site_name)
);


--
-- The se_pair VO share table stores the percentage of the se_pair resources 
-- available for a VO
--
CREATE TABLE t_se_vo_share (
--
-- the name of the se_pair
   se_name         VARCHAR2(512)  NOT NULL			
   ,share_id         VARCHAR2(512)  NOT NULL			                    			
   ,share_value         VARCHAR2(512)  NOT NULL
   ,share_type VARCHAR2(512)  NOT NULL
   ,CONSTRAINT se_pair_vo_share_pk PRIMARY KEY (se_name, share_id, share_type) 
);


CREATE TABLE t_se_group(
	se_group_name varchar2(512) NOT NULL
	,se_name varchar2(512) NOT NULL
	,state VARCHAR2(30)
	,CONSTRAINT t_se_group_pk PRIMARY KEY (se_group_name,se_name)	
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
-- Source site name - the source cluster name
  ,source            VARCHAR2(255)
--
-- Dest site name - the destination cluster name
  ,dest              VARCHAR2(255)
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
-- The endpoint of Source Catalog to be used in case 
-- logical names are used in the files
  ,src_catalog          VARCHAR2(1024)
--
-- The type of the the Source Catalog to be used in case 
-- logical names are used in the files
  ,src_catalog_type     VARCHAR2(1024)
--
-- The endpoint of the Destination Catalog to be used in case 
-- logical names are used in the files
  ,dest_catalog          VARCHAR2(1024)
--
-- The type of the Destination Catalog to be used in case 
-- logical names are used in the files
  ,dest_catalog_type     VARCHAR2(1024)
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
  ,copy_pin_lifetime INTEGER default NULL
--
-- use "LAN" as ConnectionType (FTS by default uses WAN). Default value is false.
  ,lan_connection CHAR(1) default NULL
--
-- fail the transfer immediately if the file location is NEARLINE (do not even
-- start the transfer). The default is false.
  ,fail_nearline CHAR(1) default NULL
--
-- Specified is the checksum is required on the source and destination, destination or none
  ,checksum_method CHAR(1) default NULL);


--
-- t_file stores the actual file transfers - one row per source/dest pair
--
CREATE TABLE t_file (
-- file_id is a unique identifier for a (source, destination) pair with a
-- job.  It is created automatically.
--
   file_id		INTEGER
			CONSTRAINT file_file_id_not_null NOT NULL
		        -- JC next constraint is actually too strict!
		        CONSTRAINT file_file_id_pk PRIMARY KEY
--
-- job_id (used in joins with file table)
   ,job_id		CHAR(36)
                    	CONSTRAINT file_job_id_not_null NOT NULL
                    	REFERENCES t_job(job_id)
--
-- The state of this file
  ,file_state		VARCHAR2(32)
                    	CONSTRAINT file_file_state_not_null NOT NULL
--
-- The Source Logical Name
  ,logical_name      	VARCHAR2(1100)
--
-- The Source
  ,source_surl      	VARCHAR2(1100)
--
-- The Destination
  ,dest_surl		VARCHAR2(1100)
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
-- autoinc sequence on se_row_id
--
CREATE SEQUENCE se_row_id_seq;

CREATE OR REPLACE TRIGGER se_row_id_auto_inc
BEFORE INSERT ON t_se_protocol
FOR EACH ROW
WHEN (new.se_row_id IS NULL)
BEGIN
  SELECT se_row_id_seq.nextval
  INTO   :new.se_row_id from dual;
END;
/





--
-- t_stage_req table stores the data related to a file orestaging request
--
CREATE TABLE t_stage_req (
--
-- transfer request id
   request_id           VARCHAR2(255)
                        CONSTRAINT stagereq_req_id_not_null NOT NULL
--
-- Identifier of the file to transfer
  ,file_id              INTEGER
		        CONSTRAINT stagereq_file_id_not_null NOT NULL
		        REFERENCES t_file(file_id)
-- 
-- file identifier within the request. It could be used for ordering
  ,stage_id             INTEGER
                        CONSTRAINT stagereq_stage_id_not_null NOT NULL
--
-- Identifier of the job owning the file to transfer
  ,job_id               CHAR(36)
		        CONSTRAINT stagereq_job_id_not_null NOT NULL
		        REFERENCES t_job(job_id)
--
-- The state of this file
  ,stage_state          VARCHAR2(32)
                        CONSTRAINT stagereq_state_not_null NOT NULL
--
-- The Returned TURL
  ,turl                 VARCHAR2(1100)
--
-- the time at which the stage request was started
  ,request_time         TIMESTAMP WITH TIME ZONE
--
-- the total duration in seconds
  ,duration             NUMBER(12,2)
--
-- The finalization duration
  ,final_duration       NUMBER(12,3)
--
-- The error scope
  ,error_scope          VARCHAR2(32)
--
-- The FTS phase when the error happened
  ,error_phase          VARCHAR2(32)
--
-- The class for the reason field
  ,reason_class         VARCHAR2(32)
--
-- The reason the transfer is in this state
  ,reason               VARCHAR2(2048)
--
-- the nominal size of the file (bytes)
  ,filesize         	INTEGER
--
-- the time at which the transfer finished (successfully or not)
  ,finish_time          TIMESTAMP WITH TIME ZONE
--
-- this timestamp will be set when the job enter in one of the terminal 
-- states (Finished, FinishedDirty, Failed, Canceled). Use for table
-- partitioning
  ,job_finished          TIMESTAMP WITH TIME ZONE DEFAULT NULL
--
-- Set primary key
  ,CONSTRAINT stagereq_pk PRIMARY KEY (request_id, file_id)
);



--
--
-- Index Section 
--
--

-- t_bad_ses indexes:
-- t_bad_ses(se_id) is primary key
CREATE INDEX bad_se_hostname ON t_bad_ses(se_hostname);

-- t_site_group indexes:
-- t_site_group(group_name,site_name) is primary key
CREATE INDEX site_group_sname_id ON t_site_group(site_name);

-- t_job indexes:
-- t_job(job_id) is primary key
CREATE INDEX job_job_state    ON t_job(job_state);
CREATE INDEX job_vo_name      ON t_job(vo_name);
CREATE INDEX job_cred_id      ON t_job(user_dn,cred_id);
CREATE INDEX job_jobfinished_id     ON t_job(job_finished);
CREATE INDEX job_job_A     ON t_job(source_se,dest_se);
CREATE INDEX job_priority     ON t_job(priority);
CREATE INDEX job_submit_time     ON t_job(submit_time);

-- t_file indexes:
-- t_file(file_id) is primary key
CREATE INDEX file_job_id     ON t_file(job_id);
CREATE INDEX file_file_id     ON t_file(file_id);
CREATE INDEX file_file_state_job_id ON t_file(file_state,job_id);
CREATE INDEX file_jobfinished_id ON t_file(job_finished);
CREATE INDEX file_job_id_a ON t_file(job_id, FINISH_TIME);
CREATE INDEX file_finish_time ON t_file(finish_time);


CREATE INDEX optimize_source_a         ON t_optimize(source_se,dest_se);
CREATE INDEX optimize_dest_se           ON t_optimize(dest_se);
CREATE INDEX optimize_nostreams         ON t_optimize(nostreams);
CREATE INDEX optimize_timeout           ON t_optimize(timeout);
CREATE INDEX optimize_buffer            ON t_optimize(buffer);

CREATE index idx_report_job      ON t_job (vo_name,job_id);

-- t_stage_request indexes:
-- t_stage_request(t_stagereq__unique_id) is primary key
CREATE INDEX stagereq_request_id        ON t_stage_req(request_id);
CREATE INDEX stagereq_file_id           ON t_stage_req(file_id);
CREATE INDEX stagereq_job_id            ON t_stage_req(job_id);
CREATE INDEX stagereq_stage_state       ON t_stage_req(stage_state);
CREATE INDEX stagereq_jobfinished_id    ON t_stage_req(job_finished);

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



CREATE TABLE t_file_backup (
   file_id		INTEGER
   ,job_id		CHAR(36)
   ,file_state		VARCHAR2(32)
  ,logical_name      	VARCHAR2(1100)
  ,source_surl      	VARCHAR2(1100)
  ,dest_surl		VARCHAR2(1100)
  ,agent_dn		VARCHAR2(1024)
  ,error_scope          VARCHAR2(32)
  ,error_phase          VARCHAR2(32)
  ,reason_class		VARCHAR2(32)
  ,reason           	VARCHAR2(2048)
  ,num_failures         INTEGER
  ,current_failures	INTEGER
  ,catalog_failures	INTEGER
  ,prestage_failures	INTEGER
  ,filesize         	INTEGER
  ,checksum         	VARCHAR2(100)
  ,finish_time		TIMESTAMP WITH TIME ZONE
  ,start_time		TIMESTAMP WITH TIME ZONE
  ,internal_file_params    VARCHAR2(255)
  ,job_finished          TIMESTAMP WITH TIME ZONE DEFAULT NULL
  ,pid INTEGER
  ,TX_DURATION		NUMBER
  ,throughput           NUMBER
);

CREATE TABLE t_job_backup (
   job_id		CHAR(36)
  ,job_state       	VARCHAR2(32)
  ,reuse_job           	VARCHAR2(3)
  ,cancel_job           CHAR(1)
  ,job_params       	VARCHAR2(255)
  ,source            	VARCHAR2(255)
  ,dest              	VARCHAR2(255)
  ,source_se         	VARCHAR2(255)
  ,dest_se           	VARCHAR2(255)
  ,user_dn          	VARCHAR2(1024)
  ,agent_dn         	VARCHAR2(1024)
  ,user_cred        	VARCHAR2(255)
  ,cred_id              VARCHAR2(100)
  ,voms_cred            BLOB
  ,vo_name              VARCHAR2(50)
  ,reason           	VARCHAR2(2048)
  ,submit_time      	TIMESTAMP WITH TIME ZONE DEFAULT SYSTIMESTAMP AT TIME ZONE '+00:00'
  ,finish_time      	TIMESTAMP WITH TIME ZONE
  ,priority      	INTEGER DEFAULT 3
  ,submit_host		VARCHAR2(255)
  ,max_time_in_queue	INTEGER
  ,space_token          VARCHAR2(255)
  ,storage_class        VARCHAR2(255)
  ,myproxy_server       VARCHAR2(255)
  ,src_catalog          VARCHAR2(1024)
  ,src_catalog_type     VARCHAR2(1024)
  ,dest_catalog         VARCHAR2(1024)
  ,dest_catalog_type    VARCHAR2(1024)
  ,internal_job_params  VARCHAR2(255)
  ,overwrite_flag       CHAR(1) DEFAULT NULL
  ,job_finished         TIMESTAMP WITH TIME ZONE DEFAULT NULL
  ,source_space_token 	VARCHAR2(255)
  ,source_token_description VARCHAR2(255)
  ,copy_pin_lifetime INTEGER default NULL
  ,lan_connection CHAR(1) default NULL
  ,fail_nearline CHAR(1) default NULL
  ,checksum_method CHAR(1) default NULL);

exit;
