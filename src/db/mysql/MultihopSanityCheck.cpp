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
#include "sociConversions.h"


using namespace fts3::common;
using namespace db;


static void logInconsistency(const std::string &jobId, const std::string &message)
{
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Found inconsistency for " << jobId << ": " << message << commit;
}


/// Search for files in multihob jobs whose state is NOT_USED but previous hop is already FINISHED
void MySqlAPI::fixFilesInNotUsedState(soci::session &sql)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity check of multihop jobs with a stuck hop" << commit;

    sql.begin();

    soci::rowset<soci::row> multihopJobIds = (
        sql.prepare <<
            "SELECT SQL_BUFFER_RESULT job_id "
            "FROM t_job "
            "WHERE job_state IN ('SUBMITTED', 'ACTIVE') "
            "AND job_type='H'"
    );

    for (auto i = multihopJobIds.begin(); i != multihopJobIds.end(); ++i) {
        const std::string jobId = i->get<std::string>("job_id");

        int lastFinishedIndex;
        std::string fileState;
        soci::indicator nullIndex = soci::i_ok;
        soci::indicator nullState = soci::i_ok;

        sql << "SELECT MAX(file_index) "
            "FROM t_file "
            "WHERE job_id = :job_id "
            "AND file_state = 'FINISHED' ",
            soci::use(jobId),
            soci::into(lastFinishedIndex, nullIndex);

        // If there is no file in FINISHED state continue to next job
        if (nullIndex == soci::i_null) {
            continue;
        }

        sql << "SELECT file_state "
            "FROM t_file "
            "WHERE job_id = :job_id "
            "AND file_index = :file_index ",
            soci::use(jobId), soci::use(lastFinishedIndex+1),
            soci::into(fileState, nullState);

        // Continue to next job if there is no file with file_index = lastFinishedIndex + 1
        if (nullState == soci::i_null) {
            continue;
        }

        // If the file after the last file in FINISHED state is in NOT_USED state change it to SUBMITTED
        if (fileState == "NOT_USED"){
            sql << "UPDATE t_file SET "
                "    file_state = 'SUBMITTED' "
                "    WHERE file_index = :file_index "
                "    AND job_id = :jobId ",
                soci::use(lastFinishedIndex+1),
                soci::use(jobId);
            logInconsistency(jobId, "Multihop job with a file in NOT_USED state when previous hop is FINISHED");
        }
    }
    sql.commit();
}

void MySqlAPI::multihopSanitySate()
{
    if (hashSegment.start != 0) {
        return;
    }
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Multihop sanity check thread started " << commit;

    soci::session sql(*connectionPool);

    try {
        fixFilesInNotUsedState(sql);
    }
    catch (std::exception &e) {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...) {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Multihop sanity check thread ended " << commit;
}