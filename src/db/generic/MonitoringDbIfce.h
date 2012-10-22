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
 * @file GenericDbIfce.h
 * @brief generic database interface
 * @author Alejandro Álvarez Ayllón
 * @date 17/10/2012
 *
 **/


#pragma once

#include <vector>
#include "SeConfig.h"
#include "TransferJobs.h"
#include "TransferFiles.h"


struct SourceAndDestSE {
    std::string sourceStorageElement;
    std::string destinationStorageElement;
};

struct ConfigAudit {
    time_t      when;
    std::string userDN;
    std::string config;
    std::string action;
};

struct ReasonOccurrences {
    unsigned    count;
    std::string reason;
};

struct SePairThroughput {
    struct SourceAndDestSE storageElements;
    double                 averageThroughput;
    time_t                 duration;
};

struct JobVOAndSites {
    std::string vo;
    std::string sourceSite;
    std::string destinationSite;
};

/**
 * MonitoringDbIfce class declaration
 **/
class MonitoringDbIfce {
public:
    virtual ~MonitoringDbIfce() {};

    virtual void init(const std::string& username, const std::string& password,
                      const std::string& connectString) = 0;

    virtual void setNotBefore(time_t notBefore) = 0;

    virtual void getVONames(std::vector<std::string>& vos) = 0;

    virtual void getSourceAndDestSEForVO(const std::string& vo,
                                         std::vector<SourceAndDestSE>& pairs) = 0;

    virtual unsigned numberOfJobsInState(const SourceAndDestSE& pair,
                                           const std::string& state) = 0;

    virtual void getConfigAudit(const std::string& actionLike,
                                std::vector<ConfigAudit>& audit) = 0;

    virtual void getTransferFiles(const std::string& jobId,
                                  std::vector<TransferFiles>& files) = 0;

    virtual void getJob(const std::string& jobId, TransferJobs& job) = 0;

    virtual void filterJobs(const std::vector<std::string>& inVos,
                            const std::vector<std::string>& inStates,
                            std::vector<TransferJobs>& jobs) = 0;

    virtual unsigned numberOfTransfersInState(const std::string& vo,
                                              const std::vector<std::string>& state) = 0;

    virtual void getUniqueReasons(std::vector<ReasonOccurrences>& reasons) = 0;

    virtual unsigned averageDurationPerSePair(const SourceAndDestSE& pair) = 0;

    virtual void averageThroughputPerSePair(std::vector<SePairThroughput>& avgThroughput) = 0;

    virtual void getJobVOAndSites(const std::string& jobId, JobVOAndSites& voAndSites) = 0;
};
