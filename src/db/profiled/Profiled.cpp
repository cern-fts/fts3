#include <ctime>
#include "Profiled.h"


inline double getMilliseconds()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<double>(ts.tv_nsec) / 1000000 +
           static_cast<double>(ts.tv_sec) * 1000;
}



ProfiledDB::MethodProfile::MethodProfile():
        startTs(0), nCalled(0), nException(0), totalTime(0)
{
}



void ProfiledDB::MethodProfile::start()
{
    startTs = getMilliseconds();
}



void ProfiledDB::MethodProfile::end()
{
    double end = getMilliseconds();
    totalTime += end - startTs;
    ++nCalled;
}



void ProfiledDB::MethodProfile::exception()
{
    end();
    ++nException;
}



void ProfiledDB::MethodProfile::reset()
{
    nCalled    = 0;
    nException = 0;
    totalTime  = 0;
}



ProfiledDB::ProfiledDB(GenericDbIfce* db): db(db)
{
}



ProfiledDB::~ProfiledDB()
{
    // db is supposed to be destroyed by the caller
}


void ProfiledDB::reset()
{
    profiles.clear();
}


#define PROFILE_START \
    MethodProfile &profiler = profiles[std::string("DbAPI::") + __func__];\
    try {\
        profiler.start();

#define PROFILE_END \
        profiler.end();\
    }\
    catch (...) {\
        profiler.exception();\
        throw;\
    }


#define PROFILE_RETURN(type, func) \
type r;\
PROFILE_START;\
r = func;\
PROFILE_END;\
return r;


#define PROFILE(func) \
PROFILE_START;\
func;\
PROFILE_END;


void ProfiledDB::init(std::string username, std::string password, std::string connectString, int pooledConn)
{
    db->init(username, password, connectString, pooledConn);
}

void ProfiledDB::submitPhysical(const std::string & jobId, std::vector<job_element_tupple> src_dest_pair, const std::string & paramFTP,
                        const std::string & DN, const std::string & cred, const std::string & voName, const std::string & myProxyServer,
                        const std::string & delegationID, const std::string & spaceToken, const std::string & overwrite,
                        const std::string & sourceSpaceToken, const std::string & sourceSpaceTokenDescription, int copyPinLifeTime,
                        const std::string & failNearLine, const std::string & checksumMethod, const std::string & reuse,
                        int bringonline, std::string metadata,
                        int retry, int retryDelay, std::string sourceSe, std::string destinationSe)
{
    PROFILE(db->submitPhysical(jobId, src_dest_pair, paramFTP,
            DN, cred, voName, myProxyServer,
            delegationID, spaceToken, overwrite,
            sourceSpaceToken, sourceSpaceTokenDescription, copyPinLifeTime,
            failNearLine, checksumMethod, reuse,
            bringonline, metadata,
            retry, retryDelay, sourceSe, destinationSe));
}

void ProfiledDB::getTransferJobStatus(std::string requestID, bool archive, std::vector<JobStatus*>& jobs)
{
    PROFILE(db->getTransferJobStatus(requestID, archive, jobs));
}



void ProfiledDB::getTransferFileStatus(std::string requestID, bool archive,
                           unsigned offset, unsigned limit, std::vector<FileTransferStatus*>& files)
{
    PROFILE(db->getTransferFileStatus(requestID, archive, offset, limit, files));
}


void ProfiledDB::listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates,
                std::string restrictToClientDN, std::string forDN, std::string VOname)
{
    PROFILE(db->listRequests(jobs, inGivenStates, restrictToClientDN, forDN, VOname));
}


TransferJobs* ProfiledDB::getTransferJob(std::string jobId, bool archive)
{
    PROFILE_RETURN(TransferJobs*, db->getTransferJob(jobId, archive));
}


void ProfiledDB::getSubmittedJobs(std::vector<TransferJobs*>& jobs, const std::string & vos)
{
    PROFILE(db->getSubmittedJobs(jobs, vos));
}


void ProfiledDB::getByJobId(std::vector<TransferJobs*>& jobs, std::map< std::string, std::list<TransferFiles*> >& files, bool reuse)
{
    PROFILE(db->getByJobId(jobs, files, reuse));
}


void ProfiledDB::getSe(Se* &se, std::string seName)
{
    PROFILE(db->getSe(se, seName));
}


unsigned int ProfiledDB::updateFileStatus(TransferFiles* file, const std::string status)
{
    PROFILE_RETURN(unsigned int, db->updateFileStatus(file, status));
}


void ProfiledDB::addSe(std::string endpoint, std::string se_type, std::string site, std::string name, std::string state, std::string version, std::string host,
           std::string se_transfer_type, std::string se_transfer_protocol, std::string se_control_protocol, std::string gocdb_id)
{
    PROFILE(db->addSe(endpoint, se_type, site, name, state, version, host,
                se_transfer_type, se_transfer_protocol, se_control_protocol, gocdb_id));
}


void ProfiledDB::updateSe(std::string endpoint, std::string se_type, std::string site, std::string name, std::string state, std::string version, std::string host,
              std::string se_transfer_type, std::string se_transfer_protocol, std::string se_control_protocol, std::string gocdb_id)
{
    PROFILE(db->updateSe(endpoint, se_type, site, name, state, version, host,
                se_transfer_type, se_transfer_protocol, se_control_protocol, gocdb_id));
}


void ProfiledDB::deleteSe(std::string NAME)
{
    PROFILE(db->deleteSe(NAME));
}


bool ProfiledDB::updateFileTransferStatus(double throughput, std::string job_id, int file_id, std::string transfer_status,
        std::string transfer_message, int process_id, double filesize, double duration)
{
    PROFILE_RETURN(bool, db->updateFileTransferStatus(throughput, job_id, file_id, transfer_status,
                                transfer_message, process_id,
                                filesize, duration));
}


bool ProfiledDB::updateJobTransferStatus(int file_id, std::string job_id, const std::string status)
{
    PROFILE_RETURN(bool, db->updateJobTransferStatus(file_id, job_id, status));
}


void ProfiledDB::updateFileTransferProgress(std::string job_id, int file_id, double throughput, double transferred)
{
    PROFILE(db->updateFileTransferProgress(job_id, file_id, throughput, transferred));
}


void ProfiledDB::cancelJob(std::vector<std::string>& requestIDs)
{
    PROFILE(db->cancelJob(requestIDs));
}


void ProfiledDB::getCancelJob(std::vector<int>& requestIDs)
{
    PROFILE(db->getCancelJob(requestIDs));
}


void ProfiledDB::insertGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs)
{
    PROFILE(db->insertGrDPStorageCacheElement(dlg_id, dn, cert_request, priv_key, voms_attrs));
}


void ProfiledDB::updateGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs)
{
    PROFILE(db->updateGrDPStorageCacheElement(dlg_id, dn, cert_request, priv_key, voms_attrs));
}


CredCache* ProfiledDB::findGrDPStorageCacheElement(std::string delegationID, std::string dn)
{
    PROFILE_RETURN(CredCache*, db->findGrDPStorageCacheElement(delegationID, dn));
}


void ProfiledDB::deleteGrDPStorageCacheElement(std::string delegationID, std::string dn)
{
    PROFILE(db->deleteGrDPStorageCacheElement(delegationID, dn));
}


void ProfiledDB::insertGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time)
{
    PROFILE(db->insertGrDPStorageElement(dlg_id, dn, proxy, voms_attrs, termination_time));
}


void ProfiledDB::updateGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time)
{
    PROFILE(db->updateGrDPStorageElement(dlg_id, dn, proxy, voms_attrs, termination_time));
}


Cred* ProfiledDB::findGrDPStorageElement(std::string delegationID, std::string dn)
{
    PROFILE_RETURN(Cred*, db->findGrDPStorageElement(delegationID, dn));
}


void ProfiledDB::deleteGrDPStorageElement(std::string delegationID, std::string dn)
{
    PROFILE(db->deleteGrDPStorageElement(delegationID, dn));
}


bool ProfiledDB::getDebugMode(std::string source_hostname, std::string destin_hostname)
{
    PROFILE_RETURN(bool, db->getDebugMode(source_hostname, destin_hostname));
}


void ProfiledDB::setDebugMode(std::string source_hostname, std::string destin_hostname, std::string mode)
{
    PROFILE(db->setDebugMode(source_hostname, destin_hostname, mode));
}


void ProfiledDB::getSubmittedJobsReuse(std::vector<TransferJobs*>& jobs, const std::string & vos)
{
    PROFILE(db->getSubmittedJobsReuse(jobs, vos));
}


void ProfiledDB::auditConfiguration(const std::string & dn, const std::string & config, const std::string & action)
{
    PROFILE(db->auditConfiguration(dn, config, action));
}


void ProfiledDB::fetchOptimizationConfig2(OptimizerSample* ops, const std::string & source_hostname, const std::string & destin_hostname)
{
    PROFILE(db->fetchOptimizationConfig2(ops, source_hostname, destin_hostname));
}


bool ProfiledDB::updateOptimizer(double throughput, int file_id , double filesize, double timeInSecs, int nostreams, int timeout, int buffersize,
        std::string source_hostname, std::string destin_hostname)
{
    PROFILE_RETURN(bool, db->updateOptimizer(throughput, file_id, filesize, timeInSecs, nostreams, timeout, buffersize,
                            source_hostname, destin_hostname));
}


void ProfiledDB::initOptimizer(const std::string & source_hostname, const std::string & destin_hostname, int file_id)
{
    PROFILE(db->initOptimizer(source_hostname, destin_hostname, file_id));
}


bool ProfiledDB::isCredentialExpired(const std::string & dlg_id, const std::string & dn)
{
    PROFILE_RETURN(bool, db->isCredentialExpired(dlg_id, dn));
}


bool ProfiledDB::isTrAllowed(const std::string & source_se, const std::string & dest)
{
    PROFILE_RETURN(bool, db->isTrAllowed(source_se, dest));
}


int ProfiledDB::getSeOut(const std::string & source, const std::set<std::string> & destination)
{
    PROFILE_RETURN(int, db->getSeOut(source, destination));
}


int ProfiledDB::getSeIn(const std::set<std::string> & source, const std::string & destination)
{
    PROFILE_RETURN(int, db->getSeIn(source, destination));
}


void ProfiledDB::setAllowed(const std::string & job_id, int file_id, const std::string & source_se, const std::string & dest,
        int nostreams, int timeout, int buffersize)
{
    PROFILE(db->setAllowed(job_id, file_id, source_se, dest, nostreams, timeout, buffersize));
}


void ProfiledDB::setAllowedNoOptimize(const std::string & job_id, int file_id, const std::string & params)
{
    PROFILE(db->setAllowedNoOptimize(job_id, file_id, params));
}


bool ProfiledDB::terminateReuseProcess(const std::string & jobId)
{
    PROFILE_RETURN(bool, db->terminateReuseProcess(jobId));
}


void ProfiledDB::forceFailTransfers(std::map<int, std::string>& collectJobs)
{
    PROFILE(db->forceFailTransfers(collectJobs));
}


void ProfiledDB::setPid(const std::string & jobId, int fileId, int pid)
{
    PROFILE(db->setPid(jobId, fileId, pid));
}


void ProfiledDB::setPidV(int pid, std::map<int,std::string>& pids)
{
    PROFILE(db->setPidV(pid, pids));
}


void ProfiledDB::revertToSubmitted()
{
    PROFILE(db->revertToSubmitted());
}


void ProfiledDB::backup()
{
    PROFILE(db->backup());
}


void ProfiledDB::forkFailedRevertState(const std::string & jobId, int fileId)
{
    PROFILE(db->forkFailedRevertState(jobId, fileId));
}


void ProfiledDB::forkFailedRevertStateV(std::map<int,std::string>& pids)
{
    PROFILE(db->forkFailedRevertStateV(pids));
}


bool ProfiledDB::retryFromDead(std::vector<struct message_updater>& messages)
{
    PROFILE_RETURN(bool, db->retryFromDead(messages));
}


void ProfiledDB::blacklistSe(std::string se, std::string vo, std::string status, int timeout, std::string msg, std::string adm_dn)
{
    PROFILE(db->blacklistSe(se, vo, status, timeout, msg, adm_dn));
}


void ProfiledDB::blacklistDn(std::string dn, std::string msg, std::string adm_dn)
{
    PROFILE(db->blacklistDn(dn, msg, adm_dn));
}


void ProfiledDB::unblacklistSe(std::string se)
{
    PROFILE(db->unblacklistSe(se));
}


void ProfiledDB::unblacklistDn(std::string dn)
{
    PROFILE(db->unblacklistDn(dn));
}


bool ProfiledDB::isSeBlacklisted(std::string se, std::string vo)
{
    PROFILE_RETURN(bool, db->isSeBlacklisted(se, vo));
}


bool ProfiledDB::allowSubmitForBlacklistedSe(std::string se)
{
    PROFILE_RETURN(bool, db->allowSubmitForBlacklistedSe(se));
}


boost::optional<int> ProfiledDB::getTimeoutForSe(std::string se)
{
    PROFILE_RETURN(boost::optional<int>, db->getTimeoutForSe(se));
}


bool ProfiledDB::isDnBlacklisted(std::string dn)
{
    PROFILE_RETURN(bool, db->isDnBlacklisted(dn));
}


bool ProfiledDB::isFileReadyState(int fileID)
{
    PROFILE_RETURN(bool, db->isFileReadyState(fileID));
}


bool ProfiledDB::isFileReadyStateV(std::map<int,std::string>& fileIds)
{
    PROFILE_RETURN(bool, db->isFileReadyStateV(fileIds));
}


bool ProfiledDB::checkGroupExists(const std::string & groupName)
{
    PROFILE_RETURN(bool, db->checkGroupExists(groupName));
}


void ProfiledDB::getGroupMembers(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    PROFILE(db->getGroupMembers(groupName, groupMembers));
}


void ProfiledDB::addMemberToGroup(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    PROFILE(db->addMemberToGroup(groupName, groupMembers));
}


void ProfiledDB::deleteMembersFromGroup(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    PROFILE(db->deleteMembersFromGroup(groupName, groupMembers));
}


std::string ProfiledDB::getGroupForSe(const std::string se)
{
    PROFILE_RETURN(std::string, db->getGroupForSe(se));
}



void ProfiledDB::submitHost(const std::string & jobId)
{
    PROFILE(db->submitHost(jobId));
}


std::string ProfiledDB::transferHost(int fileId)
{
    PROFILE_RETURN(std::string, db->transferHost(fileId));
}


std::string ProfiledDB::transferHostV(std::map<int,std::string>& fileIds)
{
    PROFILE_RETURN(std::string, db->transferHostV(fileIds));
}


void ProfiledDB::addLinkConfig(LinkConfig* cfg)
{
    PROFILE(db->addLinkConfig(cfg));
}

void ProfiledDB::updateLinkConfig(LinkConfig* cfg)
{
    PROFILE(db->updateLinkConfig(cfg));
}

void ProfiledDB::deleteLinkConfig(std::string source, std::string destination)
{
    PROFILE(db->deleteLinkConfig(source, destination));
}

LinkConfig* ProfiledDB::getLinkConfig(std::string source, std::string destination)
{
    PROFILE_RETURN(LinkConfig*, db->getLinkConfig(source, destination));
}

bool ProfiledDB::isThereLinkConfig(std::string source, std::string destination)
{
    PROFILE_RETURN(bool, db->isThereLinkConfig(source, destination));
}

std::pair<std::string, std::string>* ProfiledDB::getSourceAndDestination(std::string symbolic_name)
{
    typedef std::pair<std::string, std::string>* Pair;
    PROFILE_RETURN(Pair, db->getSourceAndDestination(symbolic_name));
}

bool ProfiledDB::isGrInPair(std::string group)
{
    PROFILE_RETURN(bool, db->isGrInPair(group));
}

bool ProfiledDB::isShareOnly(std::string se)
{
    PROFILE_RETURN(bool, db->isShareOnly(se));
}


void ProfiledDB::addShareConfig(ShareConfig* cfg)
{
    PROFILE(db->addShareConfig(cfg));
}

void ProfiledDB::updateShareConfig(ShareConfig* cfg)
{
    PROFILE(db->updateShareConfig(cfg));
}

void ProfiledDB::deleteShareConfig(std::string source, std::string destination, std::string vo)
{
    PROFILE(db->deleteShareConfig(source, destination, vo));
}

void ProfiledDB::deleteShareConfig(std::string source, std::string destination)
{
    PROFILE(db->deleteShareConfig(source, destination));
}

ShareConfig* ProfiledDB::getShareConfig(std::string source, std::string destination, std::string vo)
{
    PROFILE_RETURN(ShareConfig*, db->getShareConfig(source, destination, vo));
}

std::vector<ShareConfig*> ProfiledDB::getShareConfig(std::string source, std::string destination)
{
    PROFILE_RETURN(std::vector<ShareConfig*>, db->getShareConfig(source, destination));
}


bool ProfiledDB::checkIfSeIsMemberOfAnotherGroup( const std::string & member)
{
    PROFILE_RETURN(bool, db->checkIfSeIsMemberOfAnotherGroup(member));
}


void ProfiledDB::addFileShareConfig(int file_id, std::string source, std::string destination, std::string vo)
{
    PROFILE(db->addFileShareConfig(file_id, source, destination, vo));
}


void ProfiledDB::getFilesForNewSeCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out)
{
    PROFILE(db->getFilesForNewSeCfg(source, destination, vo, out));
}


void ProfiledDB::getFilesForNewGrCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out)
{
    PROFILE(db->getFilesForNewGrCfg(source, destination, vo, out));
}


void ProfiledDB::delFileShareConfig(int file_id, std::string source, std::string destination, std::string vo)
{
    PROFILE(db->delFileShareConfig(file_id, source, destination, vo));
}


void ProfiledDB::delFileShareConfig(std::string group, std::string se)
{
    PROFILE(db->delFileShareConfig(group, se));
}


bool ProfiledDB::hasStandAloneSeCfgAssigned(int file_id, std::string vo)
{
    PROFILE_RETURN(bool, db->hasStandAloneSeCfgAssigned(file_id, vo));
}


bool ProfiledDB::hasPairSeCfgAssigned(int file_id, std::string vo)
{
    PROFILE_RETURN(bool, db->hasPairSeCfgAssigned(file_id, vo));
}


bool ProfiledDB::hasStandAloneGrCfgAssigned(int file_id, std::string vo)
{
    PROFILE_RETURN(bool, db->hasStandAloneGrCfgAssigned(file_id, vo));
}


bool ProfiledDB::hasPairGrCfgAssigned(int file_id, std::string vo)
{
    PROFILE_RETURN(bool, db->hasPairGrCfgAssigned(file_id, vo));
}


int ProfiledDB::countActiveTransfers(std::string source, std::string destination, std::string vo)
{
    PROFILE_RETURN(int, db->countActiveTransfers(source, destination, vo));
}


int ProfiledDB::countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo)
{
    PROFILE_RETURN(int, db->countActiveOutboundTransfersUsingDefaultCfg(se, vo));
}


int ProfiledDB::countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo)
{
    PROFILE_RETURN(int, db->countActiveInboundTransfersUsingDefaultCfg(se, vo));
}


int ProfiledDB::sumUpVoShares (std::string source, std::string destination, std::set<std::string> vos)
{
    PROFILE_RETURN(int, db->sumUpVoShares(source, destination, vos));
}


bool ProfiledDB::checkConnectionStatus()
{
    PROFILE_RETURN(bool, db->checkConnectionStatus());
}


void ProfiledDB::setPriority(std::string jobId, int priority)
{
    PROFILE(db->setPriority(jobId, priority));
}


void ProfiledDB::setRetry(int retry)
{
    PROFILE(db->setRetry(retry));
}


int ProfiledDB::getRetry(const std::string & jobId)
{
    PROFILE_RETURN(int, db->getRetry(jobId));
}


void ProfiledDB::setRetryTimes(int retry, const std::string & jobId, int fileId)
{
    PROFILE(db->setRetryTimes(retry, jobId, fileId));
}


int ProfiledDB::getRetryTimes(const std::string & jobId, int fileId)
{
    PROFILE_RETURN(int, db->getRetryTimes(jobId, fileId));
}


void ProfiledDB::setRetryTransfer(const std::string & jobId, int fileId)
{
    PROFILE(db->setRetryTransfer(jobId, fileId));
}


int ProfiledDB::getMaxTimeInQueue()
{
    PROFILE_RETURN(int, db->getMaxTimeInQueue());
}


void ProfiledDB::setMaxTimeInQueue(int afterXHours)
{
    PROFILE(db->setMaxTimeInQueue(afterXHours));
}


void ProfiledDB::setToFailOldQueuedJobs(std::vector<std::string>& jobs)
{
    PROFILE(db->setToFailOldQueuedJobs(jobs));
}


std::vector< std::pair<std::string, std::string> > ProfiledDB::getPairsForSe(std::string se)
{
    typedef std::vector< std::pair<std::string, std::string> > PairVector;
    PROFILE_RETURN(PairVector, db->getPairsForSe(se));
}


std::vector<std::string> ProfiledDB::getAllStandAlloneCfgs()
{
    PROFILE_RETURN(std::vector<std::string>, db->getAllStandAlloneCfgs());
}


std::vector<std::string> ProfiledDB::getAllShareOnlyCfgs()
{
    PROFILE_RETURN(std::vector<std::string>, db->getAllShareOnlyCfgs());
}


int ProfiledDB::activeProcessesForThisHost()
{
    PROFILE_RETURN(int, db->activeProcessesForThisHost());
}


std::vector< std::pair<std::string, std::string> > ProfiledDB::getAllPairCfgs()
{
    typedef std::vector< std::pair<std::string, std::string> > PairVector;
    PROFILE_RETURN(PairVector, db->getAllPairCfgs());
}


void ProfiledDB::setFilesToNotUsed(std::string jobId, int fileIndex, std::vector<int>& files)
{
    PROFILE(db->setFilesToNotUsed(jobId, fileIndex, files));
}


std::vector< boost::tuple<std::string, std::string, int> > ProfiledDB::getVOBringonlimeMax()
{
    typedef std::vector< boost::tuple<std::string, std::string, int> > VOBringOnlineMaxV;
    PROFILE_RETURN(VOBringOnlineMaxV, db->getVOBringonlimeMax());
}


std::vector<struct message_bringonline> ProfiledDB::getBringOnlineFiles(std::string voName, std::string hostName, int maxValue)
{
    PROFILE_RETURN(std::vector<struct message_bringonline>,
                   db->getBringOnlineFiles(voName, hostName, maxValue));
}


void ProfiledDB::bringOnlineReportStatus(const std::string & state, const std::string & message, struct message_bringonline msg)
{
    PROFILE(db->bringOnlineReportStatus(state, message, msg));
}


void ProfiledDB::addToken(const std::string & job_id, int file_id, const std::string & token)
{
    PROFILE(db->addToken(job_id, file_id, token));
}


void ProfiledDB::getCredentials(std::string & vo_name, const std::string & job_id, int file_id, std::string & dn, std::string & dlg_id)
{
    PROFILE(db->getCredentials(vo_name, job_id, file_id, dn, dlg_id));
}


void ProfiledDB::setMaxStageOp(const std::string& se, const std::string& vo, int val)
{
    PROFILE(db->setMaxStageOp(se, vo, val));
}


void ProfiledDB::useFileReplica(std::string jobId, int fileId)
{
    PROFILE(db->useFileReplica(jobId, fileId));
}


void ProfiledDB::setRetryTimestamp(const std::string& jobId, int fileId)
{
    PROFILE(db->setRetryTimestamp(jobId, fileId));
}


double ProfiledDB::getSuccessRate(std::string source, std::string destination)
{
    PROFILE_RETURN(double, db->getSuccessRate(source, destination));
}


double ProfiledDB::getAvgThroughput(std::string source, std::string destination)
{
    PROFILE_RETURN(double, db->getAvgThroughput(source, destination));
}


void ProfiledDB::updateProtocol(const std::string& jobId, int fileId, int nostreams, int timeout, int buffersize, double filesize)
{
    PROFILE(db->updateProtocol(jobId, fileId, nostreams, timeout, buffersize, filesize));
}


void ProfiledDB::cancelFilesInTheQueue(const std::string& se, const std::string& vo, std::set<std::string>& jobs)
{
    PROFILE(db->cancelFilesInTheQueue(se, vo, jobs));
}


void ProfiledDB::cancelJobsInTheQueue(const std::string& dn, std::vector<std::string>& jobs)
{
    PROFILE(db->cancelJobsInTheQueue(dn, jobs));
}


void ProfiledDB::transferLogFile(const std::string& filePath, const std::string& jobId, int fileId, bool debug)
{
    PROFILE(db->transferLogFile(filePath, jobId, fileId, debug));
}


struct message_state ProfiledDB::getStateOfTransfer(const std::string& jobId, int fileId)
{
    PROFILE_RETURN(message_state, db->getStateOfTransfer(jobId, fileId));
}


void ProfiledDB::getFilesForJob(const std::string& jobId, std::vector<int>& files)
{
    PROFILE(db->getFilesForJob(jobId, files));
}


void ProfiledDB::getFilesForJobInCancelState(const std::string& jobId, std::vector<int>& files)
{
    PROFILE(db->getFilesForJobInCancelState(jobId, files));
}


void ProfiledDB::setFilesToWaiting(const std::string& se, const std::string& vo, int timeout)
{
    PROFILE(db->setFilesToWaiting(se, vo, timeout));
}


void ProfiledDB::setFilesToWaiting(const std::string& dn, int timeout)
{
    PROFILE(db->setFilesToWaiting(dn, timeout));
}


void ProfiledDB::cancelWaitingFiles(std::set<std::string>& jobs)
{
    PROFILE(db->cancelWaitingFiles(jobs));
}


void ProfiledDB::revertNotUsedFiles()
{
    PROFILE(db->revertNotUsedFiles());
}


void ProfiledDB::checkSanityState()
{
    PROFILE(db->checkSanityState());
}


void ProfiledDB::checkSchemaLoaded()
{
    PROFILE(db->checkSchemaLoaded());
}
