#pragma once

#include <boost/python.hpp>
#include <db/generic/MonitoringDbIfce.h>

class MonitoringDbWrapper {
public:
    MonitoringDbWrapper();
    ~MonitoringDbWrapper();

    static MonitoringDbWrapper& getInstance(void);

    void init(const std::string& username, const std::string& password,
              const std::string& connectString);

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

    boost::python::list getUniqueReasons(void);

    unsigned averageDurationPerSePair(const SourceAndDestSE& pair);

    boost::python::list averageThroughputPerSePair(void);

    JobVOAndSites getJobVOAndSites(const std::string& jobId);

private:
    static MonitoringDbWrapper instance;
    MonitoringDbIfce* db;
};
