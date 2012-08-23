-- G. McCance May 2006


create or replace package body fts_history
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
  execute immediate('truncate table X_JOBIDS');
  execute immediate('truncate table X_FILEIDS');
end cleartables;


procedure get_jobids
   (v_days binary_integer,
    v_jobs binary_integer)
as
begin
  execute immediate('insert into x_jobids
                     select distinct(job_id) from t_job where
                       t_job.job_state IN (''Finished'', ''Failed'', ''Canceled'', ''FinishedDirty'')
                       and t_job.job_finished < ( systimestamp - interval '''||v_days||''' DAY )
                       and rownum <= '||v_jobs);

  select count(job_id) into v_nojobs FROM X_JOBIDS;  
end get_jobids;
 

procedure get_fileids
as
begin
  execute immediate('insert into x_fileids 
                       select distinct(file_id) from t_file where job_id IN (select * from x_jobids)');
  select count(file_id) into v_nofiles FROM X_FILEIDS;
  select count(transfer_id) into v_notransfers FROM T_TRANSFER T WHERE T.file_id in (select * from x_fileids); 
end get_fileids;


procedure copy_transfers
as
  v_first INTEGER;
  v_query VARCHAR2(2048);
begin
  v_first := NULL;
  v_query := 'insert into t_transfer_history select ';
  FOR items IN
  (
    select column_name from USER_TAB_COLUMNS WHERE TABLE_NAME = 'T_TRANSFER_HISTORY' ORDER BY column_id ASC
  ) LOOP 
    if v_first is NULL THEN
        v_query := v_query || ' ' || items.column_name; 
        v_first := 1;
    else
        v_query := v_query || ' , ' || items.column_name;
    end if;
  END LOOP;
  v_query := v_query || ' from t_transfer where file_id in (select * from x_fileids)';
  execute immediate(v_query); 
  --execute immediate('insert into t_transfer_history select * from t_transfer where file_id in (select * from x_fileids)'); 
end copy_transfers;
  
procedure copy_files
as
  v_first INTEGER;
  v_query VARCHAR2(2048);
begin
  v_first := NULL;
  v_query := 'insert into t_file_history select ';
  FOR items IN
  (
    select column_name from USER_TAB_COLUMNS WHERE TABLE_NAME = 'T_FILE_HISTORY' ORDER BY column_id ASC
  ) LOOP 
    if v_first is NULL THEN
        v_query := v_query || ' ' || items.column_name; 
        v_first := 1;
    else
        v_query := v_query || ' , ' || items.column_name;
    end if;
  END LOOP;
  v_query := v_query || ' from t_file where file_id in (select * from x_fileids)';
  execute immediate(v_query); 
--  execute immediate('insert into t_file_history select * from t_file where file_id in (select * from x_fileids)'); 
end copy_files;
          
procedure copy_jobs
as
  v_first INTEGER;
  v_query VARCHAR2(2048);
begin
  v_first := NULL;
  v_query := 'insert into t_job_history select ';
  FOR items IN
  (
    select column_name from USER_TAB_COLUMNS WHERE TABLE_NAME = 'T_JOB_HISTORY' ORDER BY column_id ASC
  ) LOOP 
    if v_first is NULL THEN
        v_query := v_query || ' ' || items.column_name; 
        v_first := 1;
    else
        v_query := v_query || ' , ' || items.column_name;
    end if;
  END LOOP;
  v_query := v_query || ' from t_job where job_id in (select * from x_jobids)';
  execute immediate(v_query); 
--  execute immediate('insert into t_job_history select * from t_job where job_id in (select * from x_jobids)'); 
end copy_jobs;
      
      
procedure delete_transfers
as
begin
  execute immediate('delete from t_transfer where file_id in (select * from x_fileids)'); 
end delete_transfers;  

procedure delete_files
as
begin
  execute immediate('delete from t_file where file_id in (select * from x_fileids)'); 
end delete_files;

procedure delete_jobs
as
begin
  execute immediate('delete from t_job where job_id in (select * from x_jobids)'); 
end delete_jobs;

procedure logitsuccess
as
begin
  execute immediate('insert into t_history_log (STARTTIME, FINISHTIME, JOBS, FILES, TRANSFERS) VALUES (:1, :2, :3, :4, :5)')
      using v_starttime, v_endtime, v_nojobs, v_nofiles, v_notransfers;  
end logitsuccess;

procedure logitfailure
   ( errcode number, errmessage varchar)
as
begin
  execute immediate('insert into t_history_log (STARTTIME, FINISHTIME, JOBS, FILES, TRANSFERS, ERRCODE, ERRMSG) 
                                                VALUES (:1, :2, :3, :4, :5, :6, SUBSTR(:7, 1, 2048))')
      using v_starttime, v_endtime, v_nojobs, v_nofiles, v_notransfers, errcode, errmessage;  
end logitfailure;

-- PUBLIC methods
procedure movedata
   (v_days binary_integer,
    v_jobs binary_integer)
as
begin
  clearvars;
  select SYSTIMESTAMP into v_starttime from DUAL;
  cleartables;
  get_jobids(v_days, v_jobs);
  get_fileids;
  commit;
  copy_transfers;
  copy_files;
  copy_jobs;  
  delete_transfers;
  delete_files;  
  delete_jobs;
  commit;
  select SYSTIMESTAMP into v_endtime from DUAL;
  logitsuccess;
EXCEPTION
   WHEN OTHERS THEN
       rollback;
       select SYSTIMESTAMP into v_endtime from DUAL;
       logitfailure(SQLCODE, SQLERRM);       
end movedata;
  
procedure start_job
   (v_minutes binary_integer)
as
njob USER_JOBS.JOB%TYPE;
begin
   dbms_scheduler.create_job(job_name=>'FTS_HISTORY_JOB',
   			     job_type=>'STORED_PROCEDURE',
   			     job_action=>'fts_history.movedata',
   			     start_date=>SYSDATE,
   			     repeat_interval=>'FREQ=MINUTELY; INTERVAL=10',
   			     enabled=>TRUE);
end start_job;

procedure stop_job
as
v_jobid USER_JOBS.JOB%TYPE;
begin
	dbms_scheduler.drop_job( job_name=>'FTS_HISTORY_JOB');  
end stop_job;

procedure status_job
as 
  job_state VARCHAR (15);
  job_last_start_date  TIMESTAMP(6) WITH TIME ZONE;
  job_next_run_date  TIMESTAMP(6) WITH TIME ZONE;

begin 
  --Show the details on the job run
  SELECT state, last_start_date, next_run_date 
  INTO job_state, job_last_start_date, job_next_run_date
  FROM user_scheduler_jobs where job_name ='FTS_HISTORY_JOB';

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
  select count (*) into counter from t_fts_plugin_schema where packagename='FTS_HISTORY_PACK';

  IF counter = 0 THEN  
    INSERT INTO t_fts_plugin_schema (packagename,schemaversion) VALUES ('FTS_HISTORY_PACK','1.0.0');
  ELSE
  UPDATE t_fts_plugin_schema  set schemaversion= '1.0.0' where packagename='FTS_HISTORY_PACK';
  END IF;
  commit;
end register;

end fts_history;
/
exec fts_history.register;
/
exit;
/