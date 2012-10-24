#include <error.h>
#include "MySqlMonitoring.h"


MySqlMonitoring::MySqlMonitoring() {
}



MySqlMonitoring::~MySqlMonitoring()
{
}



void MySqlMonitoring::init(const std::string& username, const std::string& password, const std::string &connectString) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlMonitoring::setNotBefore(time_t nb) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlMonitoring::getVONames(std::vector<std::string>& vos) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlMonitoring::getSourceAndDestSEForVO(const std::string& vo,
                                              std::vector<SourceAndDestSE>& pairs) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



unsigned MySqlMonitoring::numberOfJobsInState(const SourceAndDestSE& pair,
                                                 const std::string& state) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlMonitoring::getConfigAudit(const std::string& actionLike,
                                      std::vector<ConfigAudit>& audit) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlMonitoring::getTransferFiles(const std::string& jobId,
                                        std::vector<TransferFiles>& files) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlMonitoring::getJob(const std::string& jobId, TransferJobs& job) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlMonitoring::filterJobs(const std::vector<std::string>& inVos,
                                  const std::vector<std::string>& inStates,
                                  std::vector<TransferJobs>& jobs) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



unsigned MySqlMonitoring::numberOfTransfersInState(const std::string& vo,
                                                    const std::vector<std::string>& state) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlMonitoring::getUniqueReasons(std::vector<ReasonOccurrences>& reasons) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



unsigned MySqlMonitoring::averageDurationPerSePair(const SourceAndDestSE& pair) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlMonitoring::averageThroughputPerSePair(std::vector<SePairThroughput>& avgThroughput) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlMonitoring::getJobVOAndSites(const std::string& jobId, JobVOAndSites& voAndSites) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}
