
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
  ,se_transfer_protocol VARCHAR2(30) not null
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
-- se_pairs in the system
--
CREATE TABLE t_se_pair (
--
-- Name of the se_pair
   se_pair_name     	VARCHAR2(32)
                    	CONSTRAINT se_pair_se_pair_name_not_null NOT NULL
                    	CONSTRAINT se_pair_se_pair_name_id_pk PRIMARY KEY
--
-- Source site name
   ,source_site        	VARCHAR2(100)
			CONSTRAINT se_pair_source_site_not_null NOT NULL
--
-- Destination site name
   ,dest_site        	VARCHAR2(100)
	                CONSTRAINT se_pair_dest_site_not_null NOT NULL
--
-- Email contact of the se_pair responsbile
   ,contact         	VARCHAR2(255)
--
-- Maximum bandwidth, capacity, in Mbits/s
   ,bandwidth       	NUMBER
--
-- The target number of concurrent streams on the network
   ,nostreams       	INTEGER
--
-- The target number of concurrent files on the network
   ,nofiles             INTEGER
--
-- Default TCP Buffer Size for the transfer
   ,tcp_buffer_size     VARCHAR(24)
--
-- The target throughput for the system (Mbits/s)
   ,nominal_throughput	NUMBER
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
-- se_pair_type (DEDICATED, NONDEDICATED)
   ,se_pair_type     VARCHAR2(20)
-------------------------------------------
-- se_pair configuration parameters.
-- Note: probably these values should be 
-- factorized in a separate table in order
-- to support different kind of transfer
-- mechanism. se_pair_type could be used
-- to select the correct table
-------------------------------------------
-- the block size to be used during the transfer
--
   ,blocksize          VARCHAR2(24)
-- HTTP timeout
--
   ,http_to            INTEGER
-- Transfer log level. Allowed values are (DEBUG, INFO, WARN, ERROR)
--
   ,tx_loglevel        VARCHAR2(12)
-- urlcopy put timeout
--
   ,urlcopy_put_to     INTEGER
-- urlcopy putdone timeout
--
   ,urlcopy_putdone_to INTEGER
-- urlcopy get timeout
--
   ,urlcopy_get_to     INTEGER
-- urlcopy getdone timeout
--
   ,urlcopy_getdone_to INTEGER
-- urlcopy transfer5 timeout
--
   ,urlcopy_tx_to      INTEGER
-- urlcopy transfer markers timeout
--
   ,urlcopy_txmarks_to INTEGER
-- srmcopy direction
--
   ,srmcopy_direction  VARCHAR2(4)
-- srmcopy transfer timeout
--
   ,srmcopy_to        INTEGER
-- srmcopy refresh timeout (timeout since last status update)
--
   ,srmcopy_refresh_to INTEGER
--
-- check that target directory is accessible
--
   ,target_dir_check CHAR(1)
--
-- The parameter to set after how many seconds to mark the first transfer activity
--
   ,url_copy_first_txmark_to INTEGER
--
--  The transfer mode of the se_pair [URLCOPY|SRMCOPY]
--
    ,transfer_type VARCHAR(15) 
--
-- *** New in 3.3.0 ***
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
   ,message             VARCHAR2(1024) DEFAULT NULL
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
-- share ID
   ,share_id         VARCHAR2(512)  NOT NULL			                    			
--
   ,share_type VARCHAR2(512)  NOT NULL
-- Set primary key
   ,CONSTRAINT se_pair_vo_share_pk PRIMARY KEY (se_name, share_id, share_type)
   ,CONSTRAINT se_pair_vo_share_fk FOREIGN KEY (se_name) REFERENCES t_se (name)
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
--
-- Set Foreign key
  ,CONSTRAINT se_pair_acl_fk FOREIGN KEY (se_pair_name) REFERENCES t_se_pair (se_pair_name)
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
-- the name of the transfer se_pair for this job
  ,se_pair_name     	VARCHAR2(32)
                    	REFERENCES t_se_pair(se_pair_name)
--
-- The reason the job is in the current state
  ,reason           	VARCHAR2(1024)
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
  ,overwrite_flag        CHAR(1)
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
  ,reason           	VARCHAR2(1024)
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
-- internal file parameters for storing information between retry attempts
  ,internal_file_params    VARCHAR2(255)
--
-- this timestamp will be set when the job enter in one of the terminal 
-- states (Finished, FinishedDirty, Failed, Canceled). Use for table
-- partitioning
  ,job_finished          TIMESTAMP WITH TIME ZONE DEFAULT NULL
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
-- t_transfer table stores the data related to a file transfer
--
CREATE TABLE t_transfer (
--
-- transfer request id
   request_id           VARCHAR2(255)
                        CONSTRAINT transfer_req_id_not_null NOT NULL
--
-- Identifier of the file to transfer
  ,file_id              INTEGER
		        CONSTRAINT transfer_file_id_not_null NOT NULL
		        REFERENCES t_file(file_id)
-- 
-- transfer identifier within the request. It could be used for ordering
  ,transfer_id          INTEGER
                        CONSTRAINT transfer_tr_id_not_null NOT NULL
--
-- Identifier of the job owning the file to transfer
  ,job_id               CHAR(36)
                        REFERENCES t_job(job_id)
--
-- The state of this file
  ,transfer_state       VARCHAR2(32)
                        CONSTRAINT transfer_tr_state_not_null NOT NULL
--
-- The Source TURL
  ,source_turl          VARCHAR2(1100)
--
-- The Destination TURL
  ,dest_turl            VARCHAR2(1100)
--
-- Source SRM request ID
  ,source_srm_token     VARCHAR2(256)
--
-- Destination SRM request ID
  ,dest_srm_token       VARCHAR2(256)
--
-- Source TURL hostname
  ,source_host          VARCHAR2(256)
--
-- Destination TURL hostname
  ,dest_host            VARCHAR2(256)  
--
-- the time at which the file was transferred
  ,transfer_time        TIMESTAMP WITH TIME ZONE
--
-- the time at which the filepreparation was started
  ,prepare_time         TIMESTAMP WITH TIME ZONE
--
-- the total transfer duration in seconds
  ,duration             NUMBER(12,2)
--
-- The source preparation duration
  ,src_prep_duration    NUMBER(12,3)
--
-- The destination preparation duration
  ,dest_prep_duration   NUMBER(12,3)
--
-- The transfer duration
  ,tx_duration          NUMBER(12,3)
--
-- The source finalization duration
  ,src_final_duration   NUMBER(12,3)
--
-- The destination finalization duration
  ,dest_final_duration  NUMBER(12,3)
--
-- the number of bytes written to the destination
  ,bytes_written        INTEGER
--
-- Average throughput
  ,throughput           NUMBER
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
  ,reason               VARCHAR2(1024)
--
-- the nominal size of the file (bytes)
  ,filesize         	INTEGER
--
-- the transfer service type used to perform the transfer
  ,transfer_type      	VARCHAR(32)
--
-- the time at which the transfer finished (successfully or not)
  ,finish_time          TIMESTAMP WITH TIME ZONE
--
-- this timestamp will be set when the job enter in one of the terminal 
-- states (Finished, FinishedDirty, Failed, Canceled). Use for table
-- partitioning
  ,job_finished         TIMESTAMP WITH TIME ZONE DEFAULT NULL
--
-- Set primary key
  ,CONSTRAINT transfer_pk PRIMARY KEY (request_id, file_id)
);



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
  ,reason               VARCHAR2(1024)
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
-- Trigger to set the job finished t_job, t_file, t_transfer, t_stage_req
--
CREATE OR REPLACE TRIGGER SET_JOB_FINISHED
BEFORE UPDATE ON T_JOB
FOR EACH ROW
WHEN ( new.job_state IN ('Finished','FinishedDirty','Failed','Canceled') )
BEGIN
  SELECT SYSTIMESTAMP INTO :new.job_finished FROM DUAL;
  UPDATE T_FILE SET job_finished=:new.job_finished WHERE T_FILE.job_id=:new.job_id;
  UPDATE T_TRANSFER SET job_finished=:new.job_finished WHERE T_TRANSFER.job_id=:new.job_id;
  UPDATE T_STAGE_REQ SET job_finished=:new.job_finished WHERE T_STAGE_REQ.job_id=:new.job_id;
END;
/

--
--
-- Index Section 
--
--

-- t_se_pair indexes:
-- t_se_pair(se_pair_name) is primary key
CREATE INDEX se_pair_se_pair_state ON t_se_pair(se_pair_state);

-- t_bad_ses indexes:
-- t_bad_ses(se_id) is primary key
CREATE INDEX bad_se_hostname ON t_bad_ses(se_hostname);

-- t_site_group indexes:
-- t_site_group(group_name,site_name) is primary key
CREATE INDEX site_group_sname_id ON t_site_group(site_name);

-- t_job indexes:
-- t_job(job_id) is primary key
CREATE INDEX job_job_state    ON t_job(job_state);
CREATE INDEX job_se_pair_name ON t_job(se_pair_name);
CREATE INDEX job_vo_name      ON t_job(vo_name);
CREATE INDEX job_cred_id      ON t_job(user_dn,cred_id);
CREATE INDEX job_jobfinished_id     ON t_job(job_finished);

-- t_file indexes:
-- t_file(file_id) is primary key
CREATE INDEX file_job_id     ON t_file(job_id);
CREATE INDEX file_file_state_job_id ON t_file(file_state,job_id);
CREATE INDEX file_jobfinished_id ON t_file(job_finished);

CREATE INDEX transfer_file_id           ON t_transfer(file_id);
CREATE INDEX transfer_job_id            ON t_transfer(job_id);
CREATE INDEX transfer_transfer_state    ON t_transfer(transfer_state);
CREATE INDEX transfer_jobfinished_id    ON t_transfer(job_finished);

--------------------------------------------------------------------------------------
-- Indices suggested by M. Anjo in order to improve performances and reduce the DB load
--------------------------------------------------------------------------------------
CREATE index idx_report_transfer ON t_transfer (transfer_state,reason_class,file_id,duration,bytes_written);
CREATE index idx_report_job      ON t_job (se_pair_name,vo_name,job_id);

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





exit;
