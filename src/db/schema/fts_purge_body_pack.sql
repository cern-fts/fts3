-- R. Garcia Rioja Feb 2009

create or replace package body fts_purge_history
as

-- PRIVATE methods
procedure clearvars
as
begin
  v_starttime := NULL;
  v_endtime := NULL;
  v_nojobs := NULL;
  v_nofiles := NULL;
  v_notransfers := NULL;  
end clearvars;

procedure cleartables
as
begin
  execute immediate('truncate table X_PURGE_JOBIDS');
  execute immediate('truncate table X_PURGE_FILEIDS');
end cleartables;

procedure get_jobids
   (v_days binary_integer,
    v_jobs binary_integer)
as
  v_major  INTEGER;
  v_minor  INTEGER;
begin
  execute immediate('insert into x_purge_jobids
	select distinct(job_id) from t_job_history where job_finished < ( systimestamp - interval '''||v_days||''' DAY ) and rownum <= '|| v_jobs);

  select count(job_id) into v_nojobs FROM X_PURGE_JOBIDS;  
	
end get_jobids;
 
procedure get_fileids
as
begin
  insert into x_purge_fileids 
    select distinct(file_id) from t_file_history 
    where job_id IN (select * from x_purge_jobids);
  select count(file_id) into v_nofiles FROM X_PURGE_FILEIDS;
  select count(transfer_id) into v_notransfers FROM T_TRANSFER_HISTORY T 
    WHERE T.file_id in (select * from x_purge_fileids); 
end get_fileids;
      
procedure logitsuccess
as
begin
  execute immediate('insert into t_history_purge_log (STARTTIME, FINISHTIME, JOBS, FILES, TRANSFERS) VALUES (:1, :2, :3, :4, :5)')
      using v_starttime, v_endtime, v_nojobs, v_nofiles, v_notransfers;  
end logitsuccess;

procedure logitfailure
   ( errcode number, errmessage varchar)
as
begin
  execute immediate('insert into t_history_purge_log (STARTTIME, FINISHTIME, JOBS, FILES, TRANSFERS, ERRCODE, ERRMSG) 
                                                VALUES (:1, :2, :3, :4, :5, :6, SUBSTR(:7, 1, 2048))')
      using v_starttime, v_endtime, v_nojobs, v_nofiles, v_notransfers, errcode, errmessage;  
end logitfailure;

-- PUBLIC methods
-- Do the move
-- Selecting entries in the history table elder than v_days
procedure movedata
   (v_days binary_integer,
    v_jobs binary_integer)
as
begin
  clearvars;
  select SYSTIMESTAMP into v_starttime from DUAL;
  cleartables;
  get_jobids(v_days,v_jobs);
  get_fileids;
  commit;
  delete from t_transfer_history where file_id in (select * from x_purge_fileids);
  delete from t_file_history where file_id in (select * from x_purge_fileids);
  delete from t_job_history where job_id in (select * from x_purge_jobids);
  delete from t_history_log 
    where finishtime < systimestamp - TO_DSINTERVAL(v_days || ' 00:00:00');
  commit;
  select SYSTIMESTAMP into v_endtime from DUAL;
  logitsuccess; 
EXCEPTION
   WHEN OTHERS THEN
       rollback;
       select SYSTIMESTAMP into v_endtime from DUAL;
       logitfailure(SQLCODE, SQLERRM);       
end movedata;
  
-- Submit the DBMS job
-- v_minutes is the interval between jobs
procedure start_job
   (v_minutes binary_integer)
as
njob USER_JOBS.JOB%TYPE;
begin
   dbms_scheduler.create_job(job_name=>'FTS_PURGE_HISTORY_JOB',
   			     job_type=>'STORED_PROCEDURE',
   			     job_action=>'fts_purge_history.movedata',
   			     start_date=>SYSDATE,
   			     repeat_interval=>'FREQ=MINUTELY; INTERVAL=10',
   			     enabled=>TRUE);
end start_job;

-- Stop the DBMS job
procedure stop_job
as
v_jobid USER_JOBS.JOB%TYPE;
begin
	dbms_scheduler.drop_job( job_name=>'FTS_PURGE_HISTORY_JOB');  
end stop_job;

-- Get status of the job
procedure status_job
as
  job_state VARCHAR (15);
  job_last_start_date  TIMESTAMP(6) WITH TIME ZONE;
  job_next_run_date  TIMESTAMP(6) WITH TIME ZONE;

begin 
  --Show the details on the job run
  SELECT state, last_start_date, next_run_date 
  INTO job_state, job_last_start_date, job_next_run_date
  FROM user_scheduler_jobs where job_name ='FTS_PURGE_HISTORY_JOB';

  IF job_state = 'SCHEDULED' or job_state = 'RUNNING' THEN 
    DBMS_OUTPUT.PUT_LINE('There is a job in status ' || job_state || ' last started time ' || job_last_start_date || ' and schedule for ' || job_next_run_date  ||'.'); 
  END IF;
end status_job;

-- It register the version of the package. 
-- Temporary procedure till the register package is available.
procedure register 
as 
  counter NUMBER;
begin
  select count (*) into counter from t_fts_plugin_schema where packagename='FTS_PURGE_PACK';

  IF counter = 0 THEN  
    INSERT INTO t_fts_plugin_schema (packagename,schemaversion) VALUES ('FTS_PURGE_PACK','1.0.0');
  ELSE
    UPDATE t_fts_plugin_schema  set schemaversion= '1.0.0' where packagename='FTS_PURGE_PACK';
  END IF;
  commit;
end register;


end fts_purge_history;
/
exec fts_purge_history.register;
/
exit;
/