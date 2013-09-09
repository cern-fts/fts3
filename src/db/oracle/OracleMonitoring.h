/********************************************//**
 * Copyright @ Members of the EMI Collaboration, 2012.
 * See www.eu-emi.eu for details on the copyright holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***********************************************/

/**
 * @file OracleMonitoring.h
 * @brief Oracle implementation of the monitoring API
 * @author Alejandro Alvarez Ayll
 * @date 18/10/2012
 *
 **/


#pragma once

#include "MonitoringDbIfce.h"
#include "OracleConnection.h"
#include "OracleTypeConversions.h"
#include "threadtraits.h"
#include "definitions.h"

using namespace FTS3_COMMON_NAMESPACE;

/**
 * Oracle concrete implementation of the Monitoring API
 **/
class OracleMonitoring: public MonitoringDbIfce
{
public:
    OracleMonitoring();
    ~OracleMonitoring();

    class CleanUpSanityChecks
    {
    public:
        CleanUpSanityChecks(OracleMonitoring* instanceLocal,SafeConnection& pooled, struct message_sanity &msg):
            instanceLocal(instanceLocal), pooled(pooled), msg(msg), returnValue(false)
        {
            returnValue = instanceLocal->assignSanityRuns(pooled, msg);
        }

        ~CleanUpSanityChecks()
        {
            instanceLocal->resetSanityRuns(pooled, msg);
        }

        bool getCleanUpSanityCheck()
        {
            return returnValue;
        }

        OracleMonitoring* instanceLocal;
        SafeConnection& pooled;
        struct message_sanity &msg;
        bool returnValue;
    };

    void init(const std::string& username, const std::string& password, const std::string& connectString, int pooledConn);

    void setNotBefore(time_t notBefore);

    void getVONames(std::vector<std::string>& vos);

    void getSourceAndDestSEForVO(const std::string& vo,
                                 std::vector<SourceAndDestSE>& pairs);

    unsigned numberOfJobsInState(const SourceAndDestSE& pair,
                                 const std::string& state);

    void getConfigAudit(const std::string& actionLike,
                        std::vector<ConfigAudit>& audit);

    void getTransferFiles(const std::string& jobId,
                          std::vector<TransferFiles>& files);

    void getJob(const std::string& jobId, TransferJobs& job);

    void filterJobs(const std::vector<std::string>& inVos,
                    const std::vector<std::string>& inStates,
                    std::vector<TransferJobs>& jobs);

    unsigned numberOfTransfersInState(const std::string& vo,
                                      const std::vector<std::string>& state);

    unsigned numberOfTransfersInState(const std::string& vo,
                                      const SourceAndDestSE& pair,
                                      const std::vector<std::string>& state);

    void getUniqueReasons(std::vector<ReasonOccurrences>& reasons);

    unsigned averageDurationPerSePair(const SourceAndDestSE& pair);

    void averageThroughputPerSePair(std::vector<SePairThroughput>& avgThroughput);

    void getJobVOAndSites(const std::string& jobId, JobVOAndSites& voAndSites);

    // These are not exposed by the db interface
    bool assignSanityRuns(SafeConnection& conn, struct message_sanity &msg);

    void resetSanityRuns(SafeConnection& conn, struct message_sanity &msg);

private:
    OracleConnection *conn;
    OracleTypeConversions *conv;
    oracle::occi::Timestamp notBefore;
};
