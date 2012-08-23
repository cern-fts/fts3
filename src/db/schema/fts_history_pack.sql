-- G. McCance May 2006

create or replace package fts_history
as

-- PUBLIC variables
v_starttime TIMESTAMP WITH TIME ZONE;
v_endtime TIMESTAMP WITH TIME ZONE;
v_nojobs number;
v_nofiles number;
v_notransfers number;

-- Do the move
-- Selecting v_job jobs more than v_days old
procedure movedata
   (v_days binary_integer DEFAULT 7,
    v_jobs binary_integer DEFAULT 500);
  
-- Submit the DBMS job
-- v_minutes is the interval between jobs
procedure start_job
   (v_minutes binary_integer DEFAULT 10);

-- Stop the DBMS job  
procedure stop_job;

-- See if the DBMS job is running
procedure status_job;

-- Register the package for version control
procedure register;

end fts_history;
/
exit;
/