/*
 * Copyright (c) CERN 2013-2015
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

#include "ArchivingContext.h"

#include "common/Logger.h"


void ArchivingContext::add(const ArchivingOperation &archiveOp)
{
    add(archiveOp.surl, archiveOp.jobId, archiveOp.fileId);
    expiryMap[archiveOp.fileId] = archiveOp.startTime + archiveOp.timeout;

    if (storageEndpoint.empty()) {
        storageEndpoint = Uri::parse(archiveOp.surl).getSeName();
    }
}


bool ArchivingContext::hasTransferTimedOut(uint64_t file_id)
{
    if (expiryMap.count(file_id)) {
        return time(0) >= expiryMap[file_id];
    }

    return false;
}


void ArchivingContext::removeTransfers(const std::list<std::tuple<std::string, std::string, uint64_t>>& transfers)
{
    for (auto it_t = transfers.begin(); it_t != transfers.end(); it_t++)
    {
        const auto& surl = std::get<0>(*it_t);
        const auto& job_id = std::get<1>(*it_t);
        const auto& file_id = std::get<2>(*it_t);

        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Removing transfer from the watch list: "
            << surl << " / " << job_id << " / " << file_id << commit;

        // Remove from map(job_id, map(sURL, file_id))
        if (jobs.count(job_id) && jobs[job_id].count(surl)) {
            auto& file_ids = jobs[job_id][surl];
            auto pos = std::find(file_ids.begin(), file_ids.end(), file_id);

            if (pos != file_ids.end()) {
                file_ids.erase(pos);
            }

            if (file_ids.empty()) {
                jobs[job_id].erase(surl);
            }

            if (jobs[job_id].empty()) {
                jobs.erase(job_id);
            }
        }

        // Remove from multi_map(sURL, pair(job_id, file_id))
        auto range = urlToIDs.equal_range(surl);
        for (auto it = range.first; it != range.second; it++) {
            if ((it->second.first == job_id) && (it->second.second == file_id)) {
                urlToIDs.erase(it);
                break;
            }
        }

        // Remove from map(file_id, expiry_timestamp)
        expiryMap.erase(file_id);
    }
}


void ArchivingContext::removeUrlWithIds(const std::string& url,
                                        const std::vector<std::pair<std::string, uint64_t>>& ids)
{
    removeUrl(url);

    for (auto it = ids.begin(); it != ids.end(); it++) {
        expiryMap.erase(it->second);
    }
}
