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


/// Search for files in multihob jobs whose state is NOT_USED but first hop has already finished
void MySqlAPI::fixFilesInNotUsedState(soci::session &sql){
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity check multihop files with second hop stuck" << commit;

    sql.begin();

    soci::rowset<soci::row> multihopJobIds = (
        sql.prepare <<
            "SELECT SQL_BUFFER_RESULT job_id, job_state FROM t_job WHERE job_type='H'"
    );

    for (auto i = multihopJobIds.begin(); i != multihopJobIds.end(); ++i) {
        const std::string jobId = i->get<std::string>("job_id");

        std::map<std::string, long long> stateCount;

        soci::rowset<soci::row> fileStates = (sql.prepare <<
            "SELECT file_state, COUNT(file_state) AS cnt "
            "FROM t_file "
            "WHERE job_id = :job_id "
            "GROUP BY file_state "
            "ORDER BY NULL",
            soci::use(jobId)
        );

        for (auto i = fileStates.begin(); i != fileStates.end(); ++i) {
            const std::string fileState = i->get<std::string>("file_state");
            long long count = i->get<long long>("cnt");
            stateCount[fileState] = count;
        }

        if ((stateCount["FINISHED"] == 1) && (stateCount["NOT_USED"] == 1)){
                sql << "UPDATE t_file SET "
                    "    file_state = 'SUBMITTED' "
                    "    WHERE file_state = 'NOT_USED' AND job_id = :jobId",
                    soci::use(jobId);
                logInconsistency(jobId, "Multihop job with first hop finished and second hop not used");
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