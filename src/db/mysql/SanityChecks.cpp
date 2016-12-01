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


static void logInconsistency(const std::string &jobId, const std::string &message)
{
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Found inconsistency for " << jobId << ": " << message << commit;
}


bool MySqlAPI::assignSanityRuns(soci::session &sql, struct SanityFlags &msg)
{
    long long rows = 0;

    try {
        if (msg.checkSanityState) {
            sql.begin();
            soci::statement st(
                (sql.prepare << "update t_server_sanity set checkSanityState=1, t_checkSanityState = UTC_TIMESTAMP() "
                    "where checkSanityState=0"
                    " AND (t_checkSanityState < (UTC_TIMESTAMP() - INTERVAL '30' minute)) "));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.checkSanityState = (rows > 0 ? true : false);
            sql.commit();
            return msg.checkSanityState;
        }
        else if (msg.setToFailOldQueuedJobs) {
            sql.begin();
            soci::statement st((sql.prepare
                << "update t_server_sanity set setToFailOldQueuedJobs=1, t_setToFailOldQueuedJobs = UTC_TIMESTAMP() "
                    " where setToFailOldQueuedJobs=0"
                    " AND (t_setToFailOldQueuedJobs < (UTC_TIMESTAMP() - INTERVAL '15' minute)) "
            ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.setToFailOldQueuedJobs = (rows > 0 ? true : false);
            sql.commit();
            return msg.setToFailOldQueuedJobs;
        }
        else if (msg.forceFailTransfers) {
            sql.begin();
            soci::statement st((sql.prepare
                << "update t_server_sanity set forceFailTransfers=1, t_forceFailTransfers = UTC_TIMESTAMP() "
                    " where forceFailTransfers=0"
                    " AND (t_forceFailTransfers < (UTC_TIMESTAMP() - INTERVAL '15' minute)) "
            ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.forceFailTransfers = (rows > 0 ? true : false);
            sql.commit();
            return msg.forceFailTransfers;
        }
        else if (msg.revertToSubmitted) {
            sql.begin();
            soci::statement st(
                (sql.prepare << "update t_server_sanity set revertToSubmitted=1, t_revertToSubmitted = UTC_TIMESTAMP() "
                    " where revertToSubmitted=0"
                    " AND (t_revertToSubmitted < (UTC_TIMESTAMP() - INTERVAL '15' minute)) "
                ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.revertToSubmitted = (rows > 0 ? true : false);
            sql.commit();
            return msg.revertToSubmitted;
        }
        else if (msg.cancelWaitingFiles) {
            sql.begin();
            soci::statement st((sql.prepare
                << "update t_server_sanity set cancelWaitingFiles=1, t_cancelWaitingFiles = UTC_TIMESTAMP() "
                    "  where cancelWaitingFiles=0"
                    " AND (t_cancelWaitingFiles < (UTC_TIMESTAMP() - INTERVAL '15' minute)) "
            ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.cancelWaitingFiles = (rows > 0 ? true : false);
            sql.commit();
            return msg.cancelWaitingFiles;
        }
        else if (msg.revertNotUsedFiles) {
            sql.begin();
            soci::statement st((sql.prepare
                << "update t_server_sanity set revertNotUsedFiles=1, t_revertNotUsedFiles = UTC_TIMESTAMP() "
                    " where revertNotUsedFiles=0"
                    " AND (t_revertNotUsedFiles < (UTC_TIMESTAMP() - INTERVAL '15' minute)) "
            ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.revertNotUsedFiles = (rows > 0 ? true : false);
            sql.commit();
            return msg.revertNotUsedFiles;
        }
        else if (msg.cleanUpRecords) {
            sql.begin();
            soci::statement st(
                (sql.prepare << "update t_server_sanity set cleanUpRecords=1, t_cleanUpRecords = UTC_TIMESTAMP() "
                    " where cleanUpRecords=0"
                    " AND (t_cleanUpRecords < (UTC_TIMESTAMP() - INTERVAL '3' day)) "
                ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.cleanUpRecords = (rows > 0 ? true : false);
            sql.commit();
            return msg.cleanUpRecords;
        }
        else if (msg.msgCron) {
            sql.begin();
            soci::statement st((sql.prepare << "update t_server_sanity set msgcron=1, t_msgcron = UTC_TIMESTAMP() "
                " where msgcron=0"
                " AND (t_msgcron < (UTC_TIMESTAMP() - INTERVAL '1' day)) "
            ));
            st.execute(true);
            rows = st.get_affected_rows();
            msg.msgCron = (rows > 0 ? true : false);
            sql.commit();
            return msg.msgCron;
        }
    }
    catch (std::exception &e) {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...) {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    return false;
}


void MySqlAPI::resetSanityRuns(soci::session &sql, struct SanityFlags &msg)
{
    try {
        sql.begin();
        if (msg.checkSanityState) {
            soci::statement st(
                (sql.prepare << "update t_server_sanity set checkSanityState=0 where (checkSanityState=1 "
                    " OR (t_checkSanityState < (UTC_TIMESTAMP() - INTERVAL '45' minute)))  "));
            st.execute(true);
        }
        else if (msg.setToFailOldQueuedJobs) {
            soci::statement st(
                (sql.prepare << "update t_server_sanity set setToFailOldQueuedJobs=0 where (setToFailOldQueuedJobs=1 "
                    " OR (t_setToFailOldQueuedJobs < (UTC_TIMESTAMP() - INTERVAL '45' minute)))  "));
            st.execute(true);
        }
        else if (msg.forceFailTransfers) {
            soci::statement st(
                (sql.prepare << "update t_server_sanity set forceFailTransfers=0 where (forceFailTransfers=1 "
                    " OR (t_forceFailTransfers < (UTC_TIMESTAMP() - INTERVAL '45' minute)))  "));
            st.execute(true);
        }
        else if (msg.revertToSubmitted) {
            soci::statement st(
                (sql.prepare << "update t_server_sanity set revertToSubmitted=0 where (revertToSubmitted=1  "
                    " OR (t_revertToSubmitted < (UTC_TIMESTAMP() - INTERVAL '45' minute)))  "));
            st.execute(true);
        }
        else if (msg.cancelWaitingFiles) {
            soci::statement st(
                (sql.prepare << "update t_server_sanity set cancelWaitingFiles=0 where (cancelWaitingFiles=1  "
                    " OR (t_cancelWaitingFiles < (UTC_TIMESTAMP() - INTERVAL '45' minute)))  "));
            st.execute(true);
        }
        else if (msg.revertNotUsedFiles) {
            soci::statement st(
                (sql.prepare << "update t_server_sanity set revertNotUsedFiles=0 where (revertNotUsedFiles=1  "
                    " OR (t_revertNotUsedFiles < (UTC_TIMESTAMP() - INTERVAL '45' minute)))  "));
            st.execute(true);
        }
        else if (msg.cleanUpRecords) {
            soci::statement st((sql.prepare << "update t_server_sanity set cleanUpRecords=0 where (cleanUpRecords=1  "
                " OR (t_cleanUpRecords < (UTC_TIMESTAMP() - INTERVAL '4' day)))  "));
            st.execute(true);
        }
        else if (msg.msgCron) {
            soci::statement st((sql.prepare << "update t_server_sanity set msgcron=0 where msgcron=1"));
            st.execute(true);
        }
        sql.commit();
    }
    catch (std::exception &e) {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...) {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


void MySqlAPI::fixEmptyJob(soci::session &sql, const std::string &jobId)
{
    sql << "UPDATE t_job SET "
    " job_finished = UTC_TIMESTAMP(), "
    " job_state = 'CANCELED', "
    " reason = 'The job was empty'"
    " WHERE job_id = :jobId",
    soci::use(jobId);
    logInconsistency(jobId, "The job was empty");
}


void MySqlAPI::fixNonTerminalJob(soci::session &sql, const std::string &jobId,
    uint64_t filesInJob, uint64_t cancelCount, uint64_t finishedCount, uint64_t failedCount)
{
    const std::string failed = "One or more files failed. Please have a look at the details for more information";
    const std::string canceledMessage = "Transfer canceled by the user";

    if (cancelCount > 0) {
        sql << "UPDATE t_job SET "
            "    job_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), "
            "    reason = :canceledMessage "
            "    WHERE job_id = :jobId and  job_state <> 'CANCELED' ",
            soci::use(canceledMessage), soci::use(jobId);
    }
    else if (filesInJob == finishedCount)  // All files finished
    {
        sql << "UPDATE t_job SET "
            "    job_state = 'FINISHED', job_finished = UTC_TIMESTAMP() "
            "    WHERE job_id = :jobId and  job_state <> 'FINISHED'  ",
            soci::use(jobId);
    }
    else if (filesInJob == failedCount)  // All files failed
    {
        sql << "UPDATE t_job SET "
            "    job_state = 'FAILED', job_finished = UTC_TIMESTAMP(),"
            "    reason = :failed "
            "    WHERE job_id = :jobId and  job_state <> 'FAILED' ",
            soci::use(failed), soci::use(jobId);
    }
    else   // Otherwise it is FINISHEDDIRTY
    {
        sql << "UPDATE t_job SET "
            "    job_state = 'FINISHEDDIRTY', job_finished = UTC_TIMESTAMP(), "
            "    reason = :failed "
            "    WHERE job_id = :jobId and  job_state <> 'FINISHEDDIRTY'",
            soci::use(failed), soci::use(jobId);
    }

    logInconsistency(jobId, "Non terminal job with all its files terminal");
}


/// Search for jobs in non terminal state for which all transfers are in terminal
void MySqlAPI::fixJobNonTerminallAllFilesTerminal(soci::session &sql)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity check non terminal jobs with all transfers terminal" << commit;

    sql.begin();

    soci::rowset<std::string> notFinishedJobIds = (
        sql.prepare <<
            "SELECT SQL_BUFFER_RESULT job_id FROM t_job WHERE job_finished IS NULL"
    );

    for (auto i = notFinishedJobIds.begin(); i != notFinishedJobIds.end(); ++i) {
        const std::string jobId = (*i);

        uint64_t filesInJob;
        sql << "SELECT COUNT(*) FROM t_file WHERE job_id = :jobId",
            soci::use(jobId), soci::into(filesInJob);

        if (filesInJob == 0) {
            fixEmptyJob(sql, jobId);
            continue;
        }

        std::string mreplica;
        sql << "SELECT reuse_job FROM t_job WHERE job_id = :job_id",
            soci::use(jobId), soci::into(mreplica);

        //check if the file belongs to a multiple replica job
        uint64_t replicaJob = 0;
        uint64_t replicaJobCountAll = 0;
        sql << "SELECT count(*), COUNT(distinct file_index) FROM t_file WHERE job_id=:job_id",
            soci::use(jobId), soci::into(replicaJobCountAll), soci::into(replicaJob);


        // If the job is *NOT* a multiple replica job
        if (mreplica != "R" && !(replicaJobCountAll > 1 && replicaJob == 1)) {
            uint64_t finishedCount;
            sql << "SELECT COUNT(*) FROM t_file WHERE job_id = :jobId AND file_state = 'FINISHED'",
                soci::use(jobId), soci::into(finishedCount);

            uint64_t cancelCount;
            sql << "SELECT count(*) FROM t_file f1 WHERE f1.job_id = :jobId AND f1.file_state = 'CANCELED'",
                soci::use(jobId), soci::into(cancelCount);

            uint64_t failedCount;
            sql << "SELECT count(*) FROM t_file f1 WHERE f1.job_id = :jobId AND f1.file_state = 'FAILED'",
                soci::use(jobId), soci::into(failedCount);

            uint64_t terminalCount = finishedCount + cancelCount + failedCount;

            if (filesInJob == terminalCount) {
                fixNonTerminalJob(sql, jobId, filesInJob, cancelCount, finishedCount, failedCount);
            }
        // If the job IS a multiple replica job
        } else {
            // Fix inconsistencies on the flag
            if (mreplica != "R") {
                sql << "UPDATE t_job set reuse_job='R' where job_id = :job_id", soci::use(jobId);
                logInconsistency(jobId, "Multireplica job with an incorrect flag: " + mreplica);
            }

            std::string jobState;
            sql << "SELECT job_state FROM t_job WHERE job_id = :job_id", soci::use(jobId), soci::into(jobState);

            soci::rowset<soci::row> replicas = (sql.prepare <<
                "SELECT file_state, COUNT(file_state) "
                "FROM t_file "
                "WHERE job_id = :job_id "
                "GROUP BY file_state "
                "ORDER BY NULL",
                soci::use(jobId)
            );

            for (auto iRep = replicas.begin(); iRep != replicas.end(); ++iRep) {
                const std::string fileState = iRep->get<std::string>("file_state");

                // TODO: I am concerned about this check. jobState is supposed to be terminal, so... why would
                // TODO: job_finished be NULL?
                if (jobState == "CANCELED") {
                    sql << "UPDATE t_file SET "
                        "    file_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                        "    reason = 'Job canceled by the user' "
                        "    WHERE file_state in ('ACTIVE','SUBMITTED') and job_id = :jobId", soci::use(jobId);

                    logInconsistency(jobId, "Multireplica job canceled, but the files weren't");
                    break;
                }

                // If at least one is finished, reset the rest
                if (fileState == "FINISHED")
                {
                    sql << "UPDATE t_file SET "
                        "    file_state = 'NOT_USED', job_finished = NULL, finish_time = NULL, "
                        "    reason = '' "
                        "    WHERE file_state in ('ACTIVE','SUBMITTED') AND job_id = :jobId",
                        soci::use(jobId);

                    if (jobState != "FINISHED") {
                        sql << "UPDATE t_job SET "
                            "    job_state = 'FINISHED', job_finished = UTC_TIMESTAMP()"
                            "    WHERE job_id = :jobId",
                            soci::use(jobId);

                        logInconsistency(jobId, "Multireplica job with a finished replica not marked as FINISHED");

                    }

                    logInconsistency(jobId, "Multireplica job with a finished replica not marked as terminal");
                    break;
                }
            }

            //do some more sanity checks for m-replica jobs to avoid state inconsistencies
            if (jobState == "ACTIVE" || jobState == "READY") {
                uint64_t countSubmittedActiveReady = 0;
                sql << " SELECT count(*) "
                    "FROM t_file "
                    "WHERE file_state IN ('ACTIVE','SUBMITTED') "
                    "   AND job_id = :job_id",
                    soci::use(jobId), soci::into(countSubmittedActiveReady);

                if (countSubmittedActiveReady == 0) {
                    uint64_t countNotUsed = 0;
                    sql << " SELECT count(*) FROM t_file WHERE file_state = 'NOT_USED' AND job_id = :job_id",
                        soci::use(jobId), soci::into(countNotUsed);
                    if (countNotUsed > 0) {
                        sql << "UPDATE t_file SET "
                            "    file_state = 'SUBMITTED', job_finished = NULL, finish_time = NULL, "
                            "    reason = '' "
                            "    WHERE file_state = 'NOT_USED' and job_id = :jobId LIMIT 1", soci::use(jobId);
                    }
                }
            }
        }
    }
    sql.commit();
}

/// Search for jobs in terminal state with files still in non terminal
void MySqlAPI::fixJobTerminalFileNonTerminal(soci::session &sql)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity check job terminal with transfers not terminal" << commit;

    soci::rowset<soci::row> rs = (
        sql.prepare <<
            "SELECT j.job_id "
            "FROM t_job j INNER JOIN t_file f ON (j.job_id = f.job_id) "
            "WHERE j.job_finished IS NOT NULL "
            "AND f.file_state IN ('SUBMITTED','ACTIVE','STAGING','STARTED') "
    );

    sql.begin();
    for (auto i = rs.begin(); i != rs.end(); ++i) {
        const std::string jobId = i->get<std::string>("job_id");

        sql << "UPDATE t_file SET "
            "    file_state = 'FAILED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
            "    reason = 'Force failure due to file state inconsistency' "
            "    WHERE file_state in ('ACTIVE','SUBMITTED','STAGING','STARTED') and job_id = :jobId ",
            soci::use(jobId);

        logInconsistency(jobId, "The job is in terminal state, but there are transfers still in non terminal state");
    }
    sql.commit();
}

/// Search for DELETE tasks that are still running, but belong to a job marked as terminal
void MySqlAPI::fixDeleteInconsistencies(soci::session &sql)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity check delete jobs" << commit;

    soci::rowset<std::string> rs = (
        sql.prepare <<
            "SELECT j.job_id FROM t_job j INNER JOIN t_dm f ON (j.job_id = f.job_id) "
            "WHERE j.job_finished >= (UTC_TIMESTAMP() - INTERVAL '24' HOUR ) "
            "   AND f.file_state IN ('STARTED','DELETE')"
    );

    sql.begin();
    for (auto i = rs.begin(); i != rs.end(); ++i) {
        std::string jobId = (*i);
        sql << "UPDATE t_dm SET "
            "    file_state = 'FAILED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
            "    reason = 'Force failure due to file state inconsistency' "
            "    WHERE file_state in ('DELETE','STARTED') and job_id = :jobId",
            soci::use(jobId);

        logInconsistency(jobId,
            "The job is in terminal state, but there are still deletion tasks in non terminal state");
    }
    sql.commit();
}


/// Search for hosts that haven't updated their status for more than two hours.
/// For those matches, mark assigned transfers as CANCELED
void MySqlAPI::recoverFromDeadHosts(soci::session &sql)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity check from dead hosts" << commit;
    Producer producer(ServerConfig::instance().get<std::string>("MessagingDirectory"));

    soci::rowset<std::string> deadHosts = (
        sql.prepare <<
            " SELECT hostname "
            " FROM t_hosts "
            " WHERE beat < DATE_SUB(UTC_TIMESTAMP(), INTERVAL 120 MINUTE) AND service_name = 'fts_server' "
    );

    for (auto i = deadHosts.begin(); i != deadHosts.end(); ++i) {
        const std::string deadHost = (*i);

        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Found host offline for too long: " << deadHost << commit;

        //now check and collect if there are any active/ready in these hosts
        soci::rowset<soci::row> transfersActiveInHost = (
            sql.prepare <<
                "SELECT file_id, job_id FROM t_file "
                "WHERE file_state  = 'ACTIVE' "
                "   AND transferHost = :transferHost ",
                soci::use(deadHost)
        );
        for (auto active = transfersActiveInHost.begin(); active != transfersActiveInHost.end(); ++active) {
            int fileId = active->get<int>("file_id");
            const std::string jobId = active->get<std::string>("job_id");
            const std::string errorMessage = "Transfer has been forced-canceled because host " + deadHost +
                                             " is offline and the transfer is still assigned to it";

            updateFileTransferStatusInternal(sql, 0.0, jobId, fileId, "CANCELED", errorMessage, 0, 0, 0, false);
            updateJobTransferStatusInternal(sql, jobId, "CANCELED", 0);

            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Canceling assigned transfer " << jobId << " / " << fileId
               << commit;

            //send state monitoring message for the state transition
            const std::vector<TransferState> files = getStateOfTransferInternal(sql, jobId, fileId);
            for (auto it = files.begin(); it != files.end(); ++it) {
                TransferState tmp = (*it);
                MsgIfce::getInstance()->SendTransferStatusChange(producer, tmp);
            }
        }
    }
}

/// Search for staging operations in STARTED, for which their bring online timeout
/// has expired.
void MySqlAPI::recoverStalledStaging(soci::session &sql)
{
    std::string errorMessage = "Transfer has been forced-canceled because is has been in staging state beyond its bringonline timeout ";

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity check stalled staging" << commit;

    soci::rowset<soci::row> rsStagingStarted = (
        sql.prepare <<
            "SELECT f.file_id, f.staging_start, j.bring_online, j.job_id "
            "FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
            "WHERE file_state = 'STARTED'"
    );

    sql.begin();
    for (auto iStaging = rsStagingStarted.begin(); iStaging != rsStagingStarted.end(); ++iStaging) {
        int fileId = iStaging->get<int>("file_id");
        const std::string jobId = iStaging->get<std::string>("job_id");
        int bringOnline = iStaging->get<int>("bring_online", 0);

        time_t startTimeT = 0;
        if (iStaging->get_indicator("staging_start") == soci::i_ok) {
            struct tm startTime = iStaging->get<struct tm>("staging_start");
            startTimeT = timegm(&startTime);
        }

        time_t now = getUTC(0);
        double diff = difftime(now, startTimeT);
        int diffInt = boost::lexical_cast<int>(diff);

        if (diffInt > (bringOnline + 800)) {
            updateFileTransferStatusInternal(sql, 0.0, jobId, fileId, "FAILED", errorMessage, 0, 0, 0, false);
            updateJobTransferStatusInternal(sql, jobId, "FAILED", 0);

            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Canceling staging operation " << jobId << " / " << fileId << commit;

            sql << " UPDATE t_file set staging_finished=UTC_TIMESTAMP() where file_id=:file_id", soci::use(fileId);
        }
    }
    sql.commit();
}


void MySqlAPI::checkSanityState()
{
    if (hashSegment.start != 0) {
        return;
    }
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity states check thread started " << commit;

    soci::session sql(*connectionPool);

    try {
        fixJobNonTerminallAllFilesTerminal(sql);
        fixJobTerminalFileNonTerminal(sql);
        fixDeleteInconsistencies(sql);
        recoverFromDeadHosts(sql);
        recoverStalledStaging(sql);
    }
    catch (std::exception &e) {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...) {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity states check thread ended " << commit;
}
