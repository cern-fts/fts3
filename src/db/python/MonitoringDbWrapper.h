/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 */

#pragma once

#include <boost/python.hpp>
#include <db/generic/MonitoringDbIfce.h>

class MonitoringDbWrapper
{
public:
    MonitoringDbWrapper();
    ~MonitoringDbWrapper();

    static MonitoringDbWrapper& getInstance(void);

    void init(const std::string& username, const std::string& password,
              const std::string& connectString, int pooledConn);

    void setNotBefore(time_t notBefore);

    boost::python::list getVONames(void);

    boost::python::list getSourceAndDestSEForVO(const std::string& vo);

    unsigned numberOfJobsInState(const SourceAndDestSE& pair,
                                 const std::string& state);

    boost::python::list getConfigAudit(const std::string& actionLike);

    boost::python::list getTransferFiles(const std::string& jobId);

    TransferJobs getJob(const std::string& jobId);

    boost::python::list filterJobs(const boost::python::list& inVos,
                                   const boost::python::list& inStates);

    unsigned numberOfTransfersInState(const std::string& vo,
                                      const boost::python::list& states);

    unsigned numberOfTransfersInState(const std::string& vo,
                                      const SourceAndDestSE& pair,
                                      const boost::python::list& states);

    boost::python::list getUniqueReasons(void);

    unsigned averageDurationPerSePair(const SourceAndDestSE& pair);

    boost::python::list averageThroughputPerSePair(void);

    JobVOAndSites getJobVOAndSites(const std::string& jobId);

private:
    static MonitoringDbWrapper instance;
    MonitoringDbIfce* db;
};
