#include <config/serverconfig.h>
#include <db/generic/SingleDbInstance.h>
#include "MonitoringDbWrapper.h"

MonitoringDbWrapper MonitoringDbWrapper::instance;

MonitoringDbWrapper::MonitoringDbWrapper() {
    fts3::config::theServerConfig().read(0, NULL);
    db = db::DBSingleton::instance().getMonitoringDBInstance();
}



MonitoringDbWrapper::~MonitoringDbWrapper() {
    // Nothing
}


MonitoringDbWrapper& MonitoringDbWrapper::getInstance(void) {
    return instance;
}



void MonitoringDbWrapper::init(const std::string& username,
                               const std::string& password,
                               const std::string& connectString) {
    db->init(username, password, connectString);
}



void MonitoringDbWrapper::setNotBefore(time_t notBefore) {
    db->setNotBefore(notBefore);
}



boost::python::list MonitoringDbWrapper::getVONames(void) {
    std::vector<std::string> vos;
    db->getVONames(vos);

    boost::python::list voList;
    for (size_t i = 0; i < vos.size(); ++i)
        voList.append(vos[i]);

    return voList;
}



boost::python::list MonitoringDbWrapper::getSourceAndDestSEForVO(const std::string& vo) {
    std::vector<SourceAndDestSE> srcAndDest;
    db->getSourceAndDestSEForVO(vo, srcAndDest);

    boost::python::list srcAndDestList;
    for (size_t i = 0; i < srcAndDest.size(); ++i)
        srcAndDestList.append(srcAndDest[i]);

    return srcAndDestList;
}



unsigned MonitoringDbWrapper::numberOfJobsInState(const SourceAndDestSE& pair,
                                                  const std::string& state) {
    return db->numberOfJobsInState(pair, state);
}



boost::python::list MonitoringDbWrapper::getConfigAudit(const std::string& actionLike) {
    std::vector<ConfigAudit> audit;
    db->getConfigAudit(actionLike, audit);

    boost::python::list auditList;
    for (size_t i = 0; i < audit.size(); ++i)
        auditList.append(audit[i]);

    return auditList;
}



boost::python::list MonitoringDbWrapper::getTransferFiles(const std::string& jobId)
{
    std::vector<TransferFiles> tfiles;
    db->getTransferFiles(jobId, tfiles);

    boost::python::list tfilesList;
    for (size_t i = 0; i < tfiles.size(); ++i)
        tfilesList.append(tfiles[i]);

    return tfilesList;
}



TransferJobs MonitoringDbWrapper::getJob(const std::string& jobId) {
    TransferJobs tjob;
    db->getJob(jobId, tjob);
    return tjob;
}



boost::python::list MonitoringDbWrapper::filterJobs(const boost::python::list& inVos,
                                                    const boost::python::list& inStates) {
    std::vector<TransferJobs> tjobs;
    std::vector<std::string>  vosStr;
    std::vector<std::string>  statesStr;

    vosStr.reserve(static_cast<size_t>(boost::python::len(inVos)));
    statesStr.reserve(static_cast<size_t>(boost::python::len(inStates)));

    for (ssize_t i = 0; i < boost::python::len(inVos); ++i)
        vosStr.push_back(boost::python::extract<std::string>(inVos[i]));
    for (ssize_t i = 0; i < boost::python::len(inStates); ++i)
        statesStr.push_back(boost::python::extract<std::string>(inStates[i]));

    db->filterJobs(vosStr, statesStr, tjobs);

    boost::python::list tjobsList;
    for (size_t i = 0; i < tjobs.size(); ++i)
        tjobsList.append(tjobs[i]);

    return tjobsList;
}



unsigned MonitoringDbWrapper::numberOfTransfersInState(const std::string& vo,
                                                       const boost::python::list& states) {
    std::vector<std::string> strStates;
    strStates.reserve(static_cast<size_t>(boost::python::len(states)));

    for (ssize_t i = 0; i < boost::python::len(states); ++i)
        strStates.push_back(boost::python::extract<std::string>(states[i]));

    return db->numberOfTransfersInState(vo, strStates);
}



unsigned MonitoringDbWrapper::numberOfTransfersInState(const std::string& vo,
                                                       const SourceAndDestSE& pair,
                                                       const boost::python::list& states) {
    std::vector<std::string> strStates;
    strStates.reserve(static_cast<size_t>(boost::python::len(states)));

    for (ssize_t i = 0; i < boost::python::len(states); ++i)
        strStates.push_back(boost::python::extract<std::string>(states[i]));

    return db->numberOfTransfersInState(vo, pair, strStates);
}



boost::python::list MonitoringDbWrapper::getUniqueReasons(void) {
    std::vector<ReasonOccurrences> reasons;
    db->getUniqueReasons(reasons);

    boost::python::list reasonsList;
    for (size_t i = 0; i < reasons.size(); ++i)
        reasonsList.append(reasons[i]);

    return reasonsList;
}



unsigned MonitoringDbWrapper::averageDurationPerSePair(const SourceAndDestSE& pair) {
    return db->averageDurationPerSePair(pair);
}



boost::python::list MonitoringDbWrapper::averageThroughputPerSePair(void) {
    std::vector<SePairThroughput> pairThroughput;
    db->averageThroughputPerSePair(pairThroughput);

    boost::python::list pairList;
    for (size_t i = 0; i < pairThroughput.size(); ++i)
        pairList.append(pairThroughput[i]);

    return pairList;
}



JobVOAndSites MonitoringDbWrapper::getJobVOAndSites(const std::string& jobId) {
    JobVOAndSites voandsites;
    db->getJobVOAndSites(jobId, voandsites);
    return voandsites;
}
