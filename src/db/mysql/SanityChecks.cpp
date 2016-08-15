/*
 * Copyright (c) CERN 2013-2016
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MySqlAPI.h"
#include "common/Exceptions.h"
#include "common/Logger.h"
#include "db/generic/DbUtils.h"
#include "monitoring/msg-ifce.h"

using namespace fts3::common;
using namespace db;


bool MySqlAPI::assignSanityRuns(soci::session& sql, struct SanityFlags &msg)
{
    long long rows = 0;

    try
    {
        if(msg.checkSanityState)
        {
            sql.begin();
            soci::statement st((sql.prepare << "update t_server_sanity set checkSanityState=1, t_checkSanityState = UTC_TIMESTAMP() "
                "where checkSanityState=0"
                " AND (t_checkSanityState < (UTC_TIMESTAMP() - INTERVAL '30' minute)) "));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.checkSanityState = (rows > 0? true: false);
            sql.commit();
            return msg.checkSanityState;
        }
        else if(msg.setToFailOldQueuedJobs)
        {
            sql.begin();
            soci::statement st((sql.prepare << "update t_server_sanity set setToFailOldQueuedJobs=1, t_setToFailOldQueuedJobs = UTC_TIMESTAMP() "
                " where setToFailOldQueuedJobs=0"
                " AND (t_setToFailOldQueuedJobs < (UTC_TIMESTAMP() - INTERVAL '15' minute)) "
            ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.setToFailOldQueuedJobs = (rows > 0? true: false);
            sql.commit();
            return msg.setToFailOldQueuedJobs;
        }
        else if(msg.forceFailTransfers)
        {
            sql.begin();
            soci::statement st((sql.prepare << "update t_server_sanity set forceFailTransfers=1, t_forceFailTransfers = UTC_TIMESTAMP() "
                " where forceFailTransfers=0"
                " AND (t_forceFailTransfers < (UTC_TIMESTAMP() - INTERVAL '15' minute)) "
            ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.forceFailTransfers = (rows > 0? true: false);
            sql.commit();
            return msg.forceFailTransfers;
        }
        else if(msg.revertToSubmitted)
        {
            sql.begin();
            soci::statement st((sql.prepare << "update t_server_sanity set revertToSubmitted=1, t_revertToSubmitted = UTC_TIMESTAMP() "
                " where revertToSubmitted=0"
                " AND (t_revertToSubmitted < (UTC_TIMESTAMP() - INTERVAL '15' minute)) "
            ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.revertToSubmitted = (rows > 0? true: false);
            sql.commit();
            return msg.revertToSubmitted;
        }
        else if(msg.cancelWaitingFiles)
        {
            sql.begin();
            soci::statement st((sql.prepare << "update t_server_sanity set cancelWaitingFiles=1, t_cancelWaitingFiles = UTC_TIMESTAMP() "
                "  where cancelWaitingFiles=0"
                " AND (t_cancelWaitingFiles < (UTC_TIMESTAMP() - INTERVAL '15' minute)) "
            ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.cancelWaitingFiles = (rows > 0? true: false);
            sql.commit();
            return msg.cancelWaitingFiles;
        }
        else if(msg.revertNotUsedFiles)
        {
            sql.begin();
            soci::statement st((sql.prepare << "update t_server_sanity set revertNotUsedFiles=1, t_revertNotUsedFiles = UTC_TIMESTAMP() "
                " where revertNotUsedFiles=0"
                " AND (t_revertNotUsedFiles < (UTC_TIMESTAMP() - INTERVAL '15' minute)) "
            ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.revertNotUsedFiles = (rows > 0? true: false);
            sql.commit();
            return msg.revertNotUsedFiles;
        }
        else if(msg.cleanUpRecords)
        {
            sql.begin();
            soci::statement st((sql.prepare << "update t_server_sanity set cleanUpRecords=1, t_cleanUpRecords = UTC_TIMESTAMP() "
                " where cleanUpRecords=0"
                " AND (t_cleanUpRecords < (UTC_TIMESTAMP() - INTERVAL '3' day)) "
            ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.cleanUpRecords = (rows > 0? true: false);
            sql.commit();
            return msg.cleanUpRecords;
        }
        else if(msg.msgCron)
        {
            sql.begin();
            soci::statement st((sql.prepare << "update t_server_sanity set msgcron=1, t_msgcron = UTC_TIMESTAMP() "
                " where msgcron=0"
                " AND (t_msgcron < (UTC_TIMESTAMP() - INTERVAL '1' day)) "
            ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.msgCron = (rows > 0? true: false);
            sql.commit();
            return msg.msgCron;
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    return false;
}


void MySqlAPI::resetSanityRuns(soci::session& sql, struct SanityFlags &msg)
{
    try
    {
        sql.begin();
        if(msg.checkSanityState)
        {
            soci::statement st((sql.prepare << "update t_server_sanity set checkSanityState=0 where (checkSanityState=1 "
                " OR (t_checkSanityState < (UTC_TIMESTAMP() - INTERVAL '45' minute)))  "));
            st.execute(true);
        }
        else if(msg.setToFailOldQueuedJobs)
        {
            soci::statement st((sql.prepare << "update t_server_sanity set setToFailOldQueuedJobs=0 where (setToFailOldQueuedJobs=1 "
                " OR (t_setToFailOldQueuedJobs < (UTC_TIMESTAMP() - INTERVAL '45' minute)))  "));
            st.execute(true);
        }
        else if(msg.forceFailTransfers)
        {
            soci::statement st((sql.prepare << "update t_server_sanity set forceFailTransfers=0 where (forceFailTransfers=1 "
                " OR (t_forceFailTransfers < (UTC_TIMESTAMP() - INTERVAL '45' minute)))  "));
            st.execute(true);
        }
        else if(msg.revertToSubmitted)
        {
            soci::statement st((sql.prepare << "update t_server_sanity set revertToSubmitted=0 where (revertToSubmitted=1  "
                " OR (t_revertToSubmitted < (UTC_TIMESTAMP() - INTERVAL '45' minute)))  "));
            st.execute(true);
        }
        else if(msg.cancelWaitingFiles)
        {
            soci::statement st((sql.prepare << "update t_server_sanity set cancelWaitingFiles=0 where (cancelWaitingFiles=1  "
                " OR (t_cancelWaitingFiles < (UTC_TIMESTAMP() - INTERVAL '45' minute)))  "));
            st.execute(true);
        }
        else if(msg.revertNotUsedFiles)
        {
            soci::statement st((sql.prepare << "update t_server_sanity set revertNotUsedFiles=0 where (revertNotUsedFiles=1  "
                " OR (t_revertNotUsedFiles < (UTC_TIMESTAMP() - INTERVAL '45' minute)))  "));
            st.execute(true);
        }
        else if(msg.cleanUpRecords)
        {
            soci::statement st((sql.prepare << "update t_server_sanity set cleanUpRecords=0 where (cleanUpRecords=1  "
                " OR (t_cleanUpRecords < (UTC_TIMESTAMP() - INTERVAL '4' day)))  "));
            st.execute(true);
        }
        else if(msg.msgCron)
        {
            soci::statement st((sql.prepare << "update t_server_sanity set msgcron=0 where msgcron=1"));
            st.execute(true);
        }
        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


void MySqlAPI::checkSanityState()
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity states check thread started " << commit;

    soci::session sql(*connectionPool);

    unsigned int numberOfFiles = 0;
    unsigned int terminalState = 0;
    unsigned int allFinished = 0;
    unsigned int allFailed = 0;
    unsigned int allCanceled = 0;
    unsigned int numberOfFilesRevert = 0;
    unsigned int numberOfFilesDelete = 0;

    std::string mreplica;

    std::string canceledMessage = "Transfer canceled by the user";
    std::string failed = "One or more files failed. Please have a look at the details for more information";
    std::string job_id;

    try
    {
        if(hashSegment.start == 0)
        {
            soci::rowset<std::string> rs = (
                sql.prepare <<
                            " select SQL_BUFFER_RESULT job_id from t_job  where job_finished is null "
            );

            soci::statement stmt1 = (sql.prepare << "SELECT COUNT(*) FROM t_file where job_id=:jobId ", soci::use(job_id), soci::into(numberOfFiles));

            soci::statement stmt2 = (sql.prepare << "UPDATE t_job SET "
                "    job_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                "    reason = :canceledMessage "
                "    WHERE job_id = :jobId and  job_state <> 'CANCELED' ", soci::use(canceledMessage), soci::use(job_id));

            soci::statement stmt3 = (sql.prepare << "UPDATE t_job SET "
                "    job_state = 'FINISHED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP() "
                "    WHERE job_id = :jobId and  job_state <> 'FINISHED'  ", soci::use(job_id));

            soci::statement stmt4 = (sql.prepare << "UPDATE t_job SET "
                "    job_state = 'FAILED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                "    reason = :failed "
                "    WHERE job_id = :jobId and  job_state <> 'FAILED' ", soci::use(failed), soci::use(job_id));


            soci::statement stmt5 = (sql.prepare << "UPDATE t_job SET "
                "    job_state = 'FINISHEDDIRTY', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                "    reason = :failed "
                "    WHERE job_id = :jobId and  job_state <> 'FINISHEDDIRTY'", soci::use(failed), soci::use(job_id));

            soci::statement stmt6 = (sql.prepare << "SELECT COUNT(*) FROM t_file where job_id=:jobId AND file_state in ('ACTIVE','SUBMITTED','STAGING','STARTED') ", soci::use(job_id), soci::into(numberOfFilesRevert));

            soci::statement stmt7 = (sql.prepare << "UPDATE t_file SET "
                "    file_state = 'FAILED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                "    reason = 'Force failure due to file state inconsistency' "
                "    WHERE file_state in ('ACTIVE','SUBMITTED','STAGING','STARTED') and job_id = :jobId ", soci::use(job_id));

            soci::statement stmt8 = (sql.prepare << " select count(*)  "
                " from t_file "
                " where job_id = :jobId "
                "  and  file_state = 'FINISHED' ",
                soci::use(job_id),
                soci::into(allFinished));

            soci::statement stmt9 = (sql.prepare << " select count(*) "
                " from t_file f1 "
                " where f1.job_id = :jobId "
                "  and f1.file_state = 'CANCELED' ",
                soci::use(job_id),
                soci::into(allCanceled));

            soci::statement stmt10 = (sql.prepare << " select count(*) "
                " from t_file f1 "
                " where f1.job_id = :jobId "
                " and f1.file_state = 'FAILED' ",
                soci::use(job_id),
                soci::into(allFailed));


            soci::statement stmt_m_replica = (sql.prepare << " select reuse_job from t_job where job_id=:job_id  ",
                soci::use(job_id),
                soci::into(mreplica));

            //this section is for deletion jobs
            soci::statement stmtDel1 = (sql.prepare << "SELECT COUNT(*) FROM t_dm where job_id=:jobId AND file_state in ('DELETE','STARTED') ", soci::use(job_id), soci::into(numberOfFilesDelete));

            soci::statement stmtDel2 = (sql.prepare << "UPDATE t_dm SET "
                "    file_state = 'FAILED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                "    reason = 'Force failure due to file state inconsistency' "
                "    WHERE file_state in ('DELETE','STARTED') and job_id = :jobId", soci::use(job_id));


            sql.begin();
            for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
            {
                try
                {
                    job_id = (*i);
                    numberOfFiles = 0;
                    allFinished = 0;
                    allCanceled = 0;
                    allFailed = 0;
                    terminalState = 0;
                    mreplica = std::string("");

                    stmt1.execute(true);

                    //check for m-replicas job
                    stmt_m_replica.execute(true);

                    //check if the file belongs to a multiple replica job
                    long long replicaJob = 0;
                    long long replicaJobCountAll = 0;
                    sql << "select count(*), count(distinct file_index) from t_file where job_id=:job_id",
                        soci::use(job_id), soci::into(replicaJobCountAll), soci::into(replicaJob);



                    if(numberOfFiles > 0 && (mreplica == "N" || mreplica == "Y" || mreplica == "H") &&  !(replicaJobCountAll > 1 && replicaJob == 1))
                    {
                        stmt8.execute(true);
                        stmt9.execute(true);
                        stmt10.execute(true);

                        terminalState = allFinished + allCanceled + allFailed;

                        if(numberOfFiles == terminalState)  /* all files terminal state but job in ('ACTIVE','READY','SUBMITTED','STAGING') */
                        {
                            if(allCanceled > 0)
                            {
                                stmt2.execute(true);
                            }
                            else   //non canceled, check other states: "FINISHED" and FAILED"
                            {
                                if(numberOfFiles == allFinished)  /*all files finished*/
                                {
                                    stmt3.execute(true);
                                }
                                else
                                {
                                    if(numberOfFiles == allFailed)  /*all files failed*/
                                    {
                                        stmt4.execute(true);
                                    }
                                    else   // otherwise it is FINISHEDDIRTY
                                    {
                                        stmt5.execute(true);
                                    }
                                }
                            }
                        }
                    }

                    if(mreplica == "R" ||  (replicaJobCountAll > 1 && replicaJob == 1))
                    {
                        if(mreplica != "R")
                        {
                            sql << "UPDATE t_job set reuse_job='R' where job_id=:job_id", soci::use(job_id);
                        }
                        std::string job_state;
                        soci::rowset<soci::row> rsReplica = (
                            sql.prepare <<
                                        " select file_state, COUNT(file_state) from t_file where job_id=:job_id group by file_state order by null ",
                                soci::use(job_id)
                        );

                        sql << "SELECT job_state from t_job where job_id=:job_id", soci::use(job_id), soci::into(job_state);

                        soci::rowset<soci::row>::const_iterator iRep;
                        for (iRep = rsReplica.begin(); iRep != rsReplica.end(); ++iRep)
                        {
                            std::string file_state = iRep->get<std::string>("file_state");
                            //long long countStates = iRep->get<long long>("COUNT(file_state)",0);
                            if(job_state == "CANCELED")
                            {
                                sql << "UPDATE t_file SET "
                                    "    file_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                                    "    reason = 'Job canceled by the user' "
                                    "    WHERE file_state in ('ACTIVE','SUBMITTED') and job_id = :jobId", soci::use(job_id);
                                break;
                            }

                            if(file_state == "FINISHED") //if at least one is finished, reset the rest
                            {
                                sql << "UPDATE t_file SET "
                                    "    file_state = 'NOT_USED', job_finished = NULL, finish_time = NULL, "
                                    "    reason = '' "
                                    "    WHERE file_state in ('ACTIVE','SUBMITTED') and job_id = :jobId", soci::use(job_id);

                                if(job_state != "FINISHED")
                                {
                                    stmt3.execute(true); //set the job_state to finished if at least one finished
                                }
                                break;
                            }
                        }

                        //do some more sanity checks for m-replica jobs to avoid state incosistencies
                        if(job_state == "ACTIVE" || job_state == "READY")
                        {
                            long long countSubmittedActiveReady = 0;
                            sql << " SELECT count(*) from t_file where file_state in ('ACTIVE','SUBMITTED') and job_id = :job_id",
                                soci::use(job_id), soci::into(countSubmittedActiveReady);

                            if(countSubmittedActiveReady == 0)
                            {
                                long long countNotUsed = 0;
                                sql << " SELECT count(*) from t_file where file_state = 'NOT_USED' and job_id = :job_id",
                                    soci::use(job_id), soci::into(countNotUsed);
                                if(countNotUsed > 0)
                                {
                                    sql << "UPDATE t_file SET "
                                        "    file_state = 'SUBMITTED', job_finished = NULL, finish_time = NULL, "
                                        "    reason = '' "
                                        "    WHERE file_state = 'NOT_USED' and job_id = :jobId LIMIT 1", soci::use(job_id);
                                }
                            }
                        }
                    }

                }
                catch(...)
                {

                }
            }
            sql.commit();


            //now check reverse sanity checks, JOB can't be FINISH,  FINISHEDDIRTY, FAILED is at least one tr is in SUBMITTED, READY, ACTIVE
            //special case for canceled
            soci::rowset<soci::row> rs2 = (
                sql.prepare <<
                            " select  j.job_id, j.job_state, j.reuse_job from t_job j inner join t_file f on (j.job_id = f.job_id) where j.job_finished is not NULL  and f.file_state in ('SUBMITTED','ACTIVE','STAGING','STARTED') "
            );



            sql.begin();
            for (soci::rowset<soci::row>::const_iterator i2 = rs2.begin(); i2 != rs2.end(); ++i2)
            {
                try
                {
                    job_id = i2->get<std::string>("job_id");
                    std::string job_state = i2->get<std::string>("job_state");
                    std::string reuse_job = i2->get<std::string>("reuse_job");
                    mreplica = std::string("");

                    //check for m-replicas sanity
                    stmt_m_replica.execute(true);

                    long long replicaJob = 0;
                    long long replicaJobCountAll = 0;
                    sql << "select count(*), count(distinct file_index) from t_file where job_id=:job_id",
                        soci::use(job_id), soci::into(replicaJobCountAll), soci::into(replicaJob);

                    if(mreplica == "R" ||  (replicaJobCountAll > 1 && replicaJob == 1))
                    {
                        int checkFinished = 0;
                        sql << "SELECT COUNT(*) from t_file where file_state='FINISHED' and job_id=:job_id", soci::use(job_id), soci::into(checkFinished);
                        if(checkFinished >= 1)
                        {
                            sql << "UPDATE t_file SET "
                                "    file_state = 'FAILED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                                "    reason = 'File state inconsistencies, better force-fail' "
                                "    WHERE file_state in ('ACTIVE','SUBMITTED') and job_id = :jobId", soci::use(job_id);
                        }
                    }
                    else
                    {
                        stmt7.execute(true);
                    }
                }
                catch(...)
                {
                }
            }
            sql.commit();

            //now check reverse sanity checks, JOB can't be FINISH,  FINISHEDDIRTY, FAILED is at least one tr is in STARTED/DELETE
            soci::rowset<std::string> rs3 = (
                sql.prepare <<
                            " select  j.job_id from t_job j inner join t_dm f on (j.job_id = f.job_id) where j.job_finished >= (UTC_TIMESTAMP() - interval '24' HOUR ) and f.file_state in ('STARTED','DELETE')  "
            );

            sql.begin();
            for (soci::rowset<std::string>::const_iterator i3 = rs3.begin(); i3 != rs3.end(); ++i3)
            {
                job_id = (*i3);
                stmtDel2.execute(true);
            }
            sql.commit();


            //now check if a host has been offline for more than 120 min and set its transfers to failed
            soci::rowset<std::string> rsCheckHosts = (
                sql.prepare <<
                            " SELECT hostname "
                                " FROM t_hosts "
                                " WHERE beat < DATE_SUB(UTC_TIMESTAMP(), interval 120 minute) and service_name = 'fts_server' "
            );

            std::vector<TransferState> files;

            for (soci::rowset<std::string>::const_iterator irsCheckHosts = rsCheckHosts.begin(); irsCheckHosts != rsCheckHosts.end(); ++irsCheckHosts)
            {
                std::string deadHost = (*irsCheckHosts);

                //now check and collect if there are any active/ready in these hosts
                soci::rowset<soci::row> rsCheckHostsActive = (
                    sql.prepare <<
                                " SELECT file_id, job_id from t_file where file_state  = 'ACTIVE' and transferHost = :transferHost ", soci::use(deadHost)
                );
                for (soci::rowset<soci::row>::const_iterator iCheckHostsActive = rsCheckHostsActive.begin(); iCheckHostsActive != rsCheckHostsActive.end(); ++iCheckHostsActive)
                {
                    int file_id = iCheckHostsActive->get<int>("file_id");
                    std::string job_id = iCheckHostsActive->get<std::string>("job_id");
                    std::string errorMessage = "Transfer has been forced-canceled because host " + deadHost + " is offline and transfers still assigned to it";

                    updateFileTransferStatusInternal(sql, 0.0, job_id, file_id, "CANCELED", errorMessage, 0, 0, 0, false);
                    updateJobTransferStatusInternal(sql, job_id, "CANCELED",0);

                    //send state monitoring message for the state transition
                    files = getStateOfTransferInternal(sql, job_id, file_id);
                    if(!files.empty())
                    {
                        std::vector<TransferState>::iterator it;
                        for (it = files.begin(); it != files.end(); ++it)
                        {
                            TransferState tmp = (*it);
                            MsgIfce::getInstance()->SendTransferStatusChange(producer, tmp);
                        }
                        files.clear();
                    }
                }
            }

            //now check for stalled bringonline files
            soci::rowset<soci::row> rsStagingStarted = (
                sql.prepare <<
                            " select  f.file_id, f.staging_start, j.bring_online, j.job_id from t_file f inner join t_job j on (f.job_id = j.job_id) where file_state = 'STARTED' "
            );
            for (soci::rowset<soci::row>::const_iterator iStaging = rsStagingStarted.begin(); iStaging != rsStagingStarted.end(); ++iStaging)
            {
                int file_id = iStaging->get<int>("file_id");
                std::string job_id = iStaging->get<std::string>("job_id");
                int bring_online  = iStaging->get<int>("bring_online");
                struct tm start_time = iStaging->get<struct tm>("staging_start");
                time_t start_time_t = timegm(&start_time);
                std::string errorMessage = "Transfer has been forced-canceled because is has been in staging state beyond its bringonline timeout ";

                time_t now = getUTC(0);
                double diff = difftime(now, start_time_t);
                int diffInt =  boost::lexical_cast<int>(diff);

                if (diffInt > (bring_online + 800))
                {
                    updateFileTransferStatusInternal(sql, 0.0, job_id, file_id, "FAILED", errorMessage, 0, 0, 0, false);
                    updateJobTransferStatusInternal(sql, job_id, "FAILED",0);

                    sql.begin();
                    sql << " UPDATE t_file set staging_finished=UTC_TIMESTAMP() where file_id=:file_id", soci::use(file_id);
                    sql.commit();
                }
            }
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity states check thread ended " << commit;
}
