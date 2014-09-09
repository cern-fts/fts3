#include <ctime>
#include "Profiled.h"
#include "profiler/Profiler.h"
#include "profiler/Macros.h"
#include <boost/tuple/tuple.hpp>


void destroy_profiled_db(void *db)
{
    ProfiledDB* profiled = static_cast<ProfiledDB*>(db);
    delete profiled;
}

ProfiledDB::ProfiledDB(GenericDbIfce* db, void (*destroy_db)(void *)):
    db(db), destroy_db(destroy_db)
{
}

ProfiledDB::~ProfiledDB()
{
    destroy_db(db);
}

void ProfiledDB::init(std::string username, std::string password, std::string connectString, int pooledConn)
{
    PROFILE_PREFIXED("DB::", db->init(username, password, connectString, pooledConn));
}

void ProfiledDB::submitPhysical(const std::string & jobId, std::list<job_element_tupple>& src_dest_pair,
                                const std::string & DN, const std::string & cred,
                                const std::string & voName, const std::string & myProxyServer, const std::string & delegationID,
                                const std::string & sourceSe, const std::string & destinationSe,
                                const fts3::common::JobParameterHandler & params)
{
    PROFILE_PREFIXED("DB::", db->submitPhysical(
                         jobId, src_dest_pair,
                         DN, cred,
                         voName, myProxyServer, delegationID,
                         sourceSe, destinationSe,
                         params));
}

void ProfiledDB::getTransferJobStatus(std::string requestID, bool archive, std::vector<JobStatus*>& jobs)
{
    PROFILE_PREFIXED("DB::", db->getTransferJobStatus(requestID, archive, jobs));
}

void ProfiledDB::getDmJobStatus(std::string requestID, bool archive, std::vector<JobStatus*>& jobs)
{
    PROFILE_PREFIXED("DB::", db->getDmJobStatus(requestID, archive, jobs));
}

void ProfiledDB::getTransferFileStatus(std::string requestID, bool archive,
                                       unsigned offset, unsigned limit, std::vector<FileTransferStatus*>& files)
{
    PROFILE_PREFIXED("DB::", db->getTransferFileStatus(requestID, archive, offset, limit, files));
}


void ProfiledDB::getDmFileStatus(std::string requestID, bool archive,
                                 unsigned offset, unsigned limit, std::vector<FileTransferStatus*>& files)
{
    PROFILE_PREFIXED("DB::", db->getDmFileStatus(requestID, archive, offset, limit, files));
}


void ProfiledDB::listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates,
                              std::string restrictToClientDN, std::string forDN, std::string VOname, std::string src, std::string dst)
{
    PROFILE_PREFIXED("DB::", db->listRequests(jobs, inGivenStates, restrictToClientDN, forDN, VOname, src, dst));
}

void ProfiledDB::listRequestsDm(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates,
                                std::string restrictToClientDN, std::string forDN, std::string VOname, std::string src, std::string dst)
{
    PROFILE_PREFIXED("DB::", db->listRequestsDm(jobs, inGivenStates, restrictToClientDN, forDN, VOname, src, dst));
}

TransferJobs* ProfiledDB::getTransferJob(std::string jobId, bool archive)
{
    PROFILE_PREFIXED("DB::", return db->getTransferJob(jobId, archive));
}


void ProfiledDB::getByJobIdReuse(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct, std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFiles> > > >& files)
{
    PROFILE_PREFIXED("DB::", db->getByJobIdReuse(distinct, files));
}

void ProfiledDB::getByJobId(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct, std::map< std::string, std::list<TransferFiles> >& files)
{
    PROFILE_PREFIXED("DB::", db->getByJobId(distinct, files));
}

void ProfiledDB::getMultihopJobs(std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFiles> > > >& files)
{
    PROFILE_PREFIXED("DB::", db->getMultihopJobs(files));
}

void ProfiledDB::getSe(Se* &se, std::string seName)
{
    PROFILE_PREFIXED("DB::", db->getSe(se, seName));
}


unsigned int ProfiledDB::updateFileStatus(TransferFiles& file, const std::string status)
{
    PROFILE_PREFIXED("DB::", return db->updateFileStatus(file, status));
}


void ProfiledDB::addSe(std::string endpoint, std::string se_type, std::string site, std::string name, std::string state, std::string version, std::string host,
                       std::string se_transfer_type, std::string se_transfer_protocol, std::string se_control_protocol, std::string gocdb_id)
{
    PROFILE_PREFIXED("DB::", db->addSe(endpoint, se_type, site, name, state, version, host,
                                       se_transfer_type, se_transfer_protocol, se_control_protocol, gocdb_id));
}


void ProfiledDB::updateSe(std::string endpoint, std::string se_type, std::string site, std::string name, std::string state, std::string version, std::string host,
                          std::string se_transfer_type, std::string se_transfer_protocol, std::string se_control_protocol, std::string gocdb_id)
{
    PROFILE_PREFIXED("DB::", db->updateSe(endpoint, se_type, site, name, state, version, host,
                                          se_transfer_type, se_transfer_protocol, se_control_protocol, gocdb_id));
}


bool ProfiledDB::updateFileTransferStatus(double throughput, std::string job_id, int file_id, std::string transfer_status,
        std::string transfer_message, int process_id, double filesize, double duration, bool retry)
{
    PROFILE_PREFIXED("DB::", return db->updateFileTransferStatus(throughput, job_id, file_id, transfer_status,
                                    transfer_message, process_id,
                                    filesize, duration, retry));
}


bool ProfiledDB::updateJobTransferStatus(std::string job_id, const std::string status, int pid)
{
    PROFILE_PREFIXED("DB::", return db->updateJobTransferStatus(job_id, status, pid));
}


void ProfiledDB::updateFileTransferProgressVector(std::vector<struct message_updater>& messages)
{
    PROFILE_PREFIXED("DB::", db->updateFileTransferProgressVector(messages));
}


void ProfiledDB::cancelJob(std::vector<std::string>& requestIDs)
{
    PROFILE_PREFIXED("DB::", db->cancelJob(requestIDs));
}


bool ProfiledDB::insertGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs)
{
    PROFILE_PREFIXED("DB::", return db->insertGrDPStorageCacheElement(dlg_id, dn, cert_request, priv_key, voms_attrs));
}


void ProfiledDB::updateGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs)
{
    PROFILE_PREFIXED("DB::", db->updateGrDPStorageCacheElement(dlg_id, dn, cert_request, priv_key, voms_attrs));
}


CredCache* ProfiledDB::findGrDPStorageCacheElement(std::string delegationID, std::string dn)
{
    PROFILE_PREFIXED("DB::", return db->findGrDPStorageCacheElement(delegationID, dn));
}


void ProfiledDB::deleteGrDPStorageCacheElement(std::string delegationID, std::string dn)
{
    PROFILE_PREFIXED("DB::", db->deleteGrDPStorageCacheElement(delegationID, dn));
}


void ProfiledDB::insertGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time)
{
    PROFILE_PREFIXED("DB::", db->insertGrDPStorageElement(dlg_id, dn, proxy, voms_attrs, termination_time));
}


void ProfiledDB::updateGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time)
{
    PROFILE_PREFIXED("DB::", db->updateGrDPStorageElement(dlg_id, dn, proxy, voms_attrs, termination_time));
}


Cred* ProfiledDB::findGrDPStorageElement(std::string delegationID, std::string dn)
{
    PROFILE_PREFIXED("DB::", return db->findGrDPStorageElement(delegationID, dn));
}


void ProfiledDB::deleteGrDPStorageElement(std::string delegationID, std::string dn)
{
    PROFILE_PREFIXED("DB::", db->deleteGrDPStorageElement(delegationID, dn));
}


unsigned ProfiledDB::getDebugLevel(std::string source_hostname, std::string destin_hostname)
{
    PROFILE_PREFIXED("DB::", return db->getDebugLevel(source_hostname, destin_hostname));
}


void ProfiledDB::setDebugLevel(std::string source_hostname, std::string destin_hostname, unsigned level)
{
    PROFILE_PREFIXED("DB::", db->setDebugLevel(source_hostname, destin_hostname, level));
}


void ProfiledDB::auditConfiguration(const std::string & dn, const std::string & config, const std::string & action)
{
    PROFILE_PREFIXED("DB::", db->auditConfiguration(dn, config, action));
}


void ProfiledDB::fetchOptimizationConfig2(OptimizerSample* ops, const std::string & source_hostname, const std::string & destin_hostname)
{
    PROFILE_PREFIXED("DB::", db->fetchOptimizationConfig2(ops, source_hostname, destin_hostname));
}


bool ProfiledDB::isCredentialExpired(const std::string & dlg_id, const std::string & dn)
{
    PROFILE_PREFIXED("DB::", return db->isCredentialExpired(dlg_id, dn));
}


bool ProfiledDB::updateOptimizer()
{
    PROFILE_PREFIXED("DB::", return db->updateOptimizer());
}

bool ProfiledDB::isTrAllowed(const std::string & source_se, const std::string & dest,int &currentActive)
{
    PROFILE_PREFIXED("DB::", return db->isTrAllowed(source_se, dest, currentActive));
}


int ProfiledDB::getSeOut(const std::string & source, const std::set<std::string> & destination)
{
    PROFILE_PREFIXED("DB::", return db->getSeOut(source, destination));
}


int ProfiledDB::getSeIn(const std::set<std::string> & source, const std::string & destination)
{
    PROFILE_PREFIXED("DB::", return db->getSeIn(source, destination));
}


bool ProfiledDB::terminateReuseProcess(const std::string & jobId, int pid, const std::string & message)
{
    PROFILE_PREFIXED("DB::", return db->terminateReuseProcess(jobId, pid, message));
}


void ProfiledDB::forceFailTransfers(std::map<int, std::string>& collectJobs)
{
    PROFILE_PREFIXED("DB::", db->forceFailTransfers(collectJobs));
}


void ProfiledDB::setPid(const std::string & jobId, int fileId, int pid)
{
    PROFILE_PREFIXED("DB::", db->setPid(jobId, fileId, pid));
}


void ProfiledDB::setPidV(int pid, std::map<int,std::string>& pids)
{
    PROFILE_PREFIXED("DB::", db->setPidV(pid, pids));
}


void ProfiledDB::revertToSubmitted()
{
    PROFILE_PREFIXED("DB::", db->revertToSubmitted());
}


void ProfiledDB::backup(long* nJobs, long* nFiles)
{
    PROFILE_PREFIXED("DB::", db->backup(nJobs, nFiles));
}


void ProfiledDB::forkFailedRevertState(const std::string & jobId, int fileId)
{
    PROFILE_PREFIXED("DB::", db->forkFailedRevertState(jobId, fileId));
}


void ProfiledDB::forkFailedRevertStateV(std::map<int,std::string>& pids)
{
    PROFILE_PREFIXED("DB::", db->forkFailedRevertStateV(pids));
}


bool ProfiledDB::retryFromDead(std::vector<struct message_updater>& messages, bool diskFull)
{
    PROFILE_PREFIXED("DB::", return db->retryFromDead(messages, diskFull));
}


void ProfiledDB::blacklistSe(std::string se, std::string vo, std::string status, int timeout, std::string msg, std::string adm_dn)
{
    PROFILE_PREFIXED("DB::", db->blacklistSe(se, vo, status, timeout, msg, adm_dn));
}


void ProfiledDB::blacklistDn(std::string dn, std::string msg, std::string adm_dn)
{
    PROFILE_PREFIXED("DB::", db->blacklistDn(dn, msg, adm_dn));
}


void ProfiledDB::unblacklistSe(std::string se)
{
    PROFILE_PREFIXED("DB::", db->unblacklistSe(se));
}


void ProfiledDB::unblacklistDn(std::string dn)
{
    PROFILE_PREFIXED("DB::", db->unblacklistDn(dn));
}


bool ProfiledDB::isSeBlacklisted(std::string se, std::string vo)
{
    PROFILE_PREFIXED("DB::", return db->isSeBlacklisted(se, vo));
}


bool ProfiledDB::allowSubmitForBlacklistedSe(std::string se)
{
    PROFILE_PREFIXED("DB::", return db->allowSubmitForBlacklistedSe(se));
}

void ProfiledDB::allowSubmit(std::string ses, std::string vo, std::list<std::string>& notAllowed)
{
    PROFILE_PREFIXED("DB::", return db->allowSubmit(ses, vo, notAllowed));
}

boost::optional<int> ProfiledDB::getTimeoutForSe(std::string se)
{
    PROFILE_PREFIXED("DB::", return db->getTimeoutForSe(se));
}

void ProfiledDB::getTimeoutForSe(std::string ses, std::map<std::string, int>& ret)
{
    PROFILE_PREFIXED("DB::", return db->getTimeoutForSe(ses, ret));
}

bool ProfiledDB::isDnBlacklisted(std::string dn)
{
    PROFILE_PREFIXED("DB::", return db->isDnBlacklisted(dn));
}


bool ProfiledDB::isFileReadyState(int fileID)
{
    PROFILE_PREFIXED("DB::", return db->isFileReadyState(fileID));
}


bool ProfiledDB::isFileReadyStateV(std::map<int,std::string>& fileIds)
{
    PROFILE_PREFIXED("DB::", return db->isFileReadyStateV(fileIds));
}


bool ProfiledDB::checkGroupExists(const std::string & groupName)
{
    PROFILE_PREFIXED("DB::", return db->checkGroupExists(groupName));
}


void ProfiledDB::getGroupMembers(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    PROFILE_PREFIXED("DB::", db->getGroupMembers(groupName, groupMembers));
}


void ProfiledDB::addMemberToGroup(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    PROFILE_PREFIXED("DB::", db->addMemberToGroup(groupName, groupMembers));
}


void ProfiledDB::deleteMembersFromGroup(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    PROFILE_PREFIXED("DB::", db->deleteMembersFromGroup(groupName, groupMembers));
}


std::string ProfiledDB::getGroupForSe(const std::string se)
{
    PROFILE_PREFIXED("DB::", return db->getGroupForSe(se));
}


void ProfiledDB::addLinkConfig(LinkConfig* cfg)
{
    PROFILE_PREFIXED("DB::", db->addLinkConfig(cfg));
}

void ProfiledDB::updateLinkConfig(LinkConfig* cfg)
{
    PROFILE_PREFIXED("DB::", db->updateLinkConfig(cfg));
}

void ProfiledDB::deleteLinkConfig(std::string source, std::string destination)
{
    PROFILE_PREFIXED("DB::", db->deleteLinkConfig(source, destination));
}

LinkConfig* ProfiledDB::getLinkConfig(std::string source, std::string destination)
{
    PROFILE_PREFIXED("DB::", return db->getLinkConfig(source, destination));
}

std::pair<std::string, std::string>* ProfiledDB::getSourceAndDestination(std::string symbolic_name)
{
    PROFILE_PREFIXED("DB::", return db->getSourceAndDestination(symbolic_name));
}

bool ProfiledDB::isGrInPair(std::string group)
{
    PROFILE_PREFIXED("DB::", return db->isGrInPair(group));
}

bool ProfiledDB::isShareOnly(std::string se)
{
    PROFILE_PREFIXED("DB::", return db->isShareOnly(se));
}


void ProfiledDB::addShareConfig(ShareConfig* cfg)
{
    PROFILE_PREFIXED("DB::", db->addShareConfig(cfg));
}

void ProfiledDB::updateShareConfig(ShareConfig* cfg)
{
    PROFILE_PREFIXED("DB::", db->updateShareConfig(cfg));
}

void ProfiledDB::deleteShareConfig(std::string source, std::string destination, std::string vo)
{
    PROFILE_PREFIXED("DB::", db->deleteShareConfig(source, destination, vo));
}

void ProfiledDB::deleteShareConfig(std::string source, std::string destination)
{
    PROFILE_PREFIXED("DB::", db->deleteShareConfig(source, destination));
}

ShareConfig* ProfiledDB::getShareConfig(std::string source, std::string destination, std::string vo)
{
    PROFILE_PREFIXED("DB::", return db->getShareConfig(source, destination, vo));
}

std::vector<ShareConfig*> ProfiledDB::getShareConfig(std::string source, std::string destination)
{
    PROFILE_PREFIXED("DB::", return db->getShareConfig(source, destination));
}

void ProfiledDB::addActivityConfig(std::string vo, std::string shares, bool active)
{
    PROFILE_PREFIXED("DB::", db->addActivityConfig(vo, shares, active));
}

void ProfiledDB::updateActivityConfig(std::string vo, std::string shares, bool active)
{
    PROFILE_PREFIXED("DB::", db->updateActivityConfig(vo, shares, active));
}

void ProfiledDB::deleteActivityConfig(std::string vo)
{
    PROFILE_PREFIXED("DB::", db->deleteActivityConfig(vo));
}

bool ProfiledDB::isActivityConfigActive(std::string vo)
{
    PROFILE_PREFIXED("DB::", return db->isActivityConfigActive(vo));
}

std::map< std::string, double > ProfiledDB::getActivityConfig(std::string vo)
{
    PROFILE_PREFIXED("DB::", return db->getActivityConfig(vo));
}

bool ProfiledDB::checkIfSeIsMemberOfAnotherGroup( const std::string & member)
{
    PROFILE_PREFIXED("DB::", return db->checkIfSeIsMemberOfAnotherGroup(member));
}


void ProfiledDB::addFileShareConfig(int file_id, std::string source, std::string destination, std::string vo)
{
    PROFILE_PREFIXED("DB::", db->addFileShareConfig(file_id, source, destination, vo));
}


void ProfiledDB::getFilesForNewSeCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out)
{
    PROFILE_PREFIXED("DB::", db->getFilesForNewSeCfg(source, destination, vo, out));
}


void ProfiledDB::getFilesForNewGrCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out)
{
    PROFILE_PREFIXED("DB::", db->getFilesForNewGrCfg(source, destination, vo, out));
}


void ProfiledDB::delFileShareConfig(int file_id, std::string source, std::string destination, std::string vo)
{
    PROFILE_PREFIXED("DB::", db->delFileShareConfig(file_id, source, destination, vo));
}


void ProfiledDB::delFileShareConfig(std::string group, std::string se)
{
    PROFILE_PREFIXED("DB::", db->delFileShareConfig(group, se));
}


bool ProfiledDB::hasStandAloneSeCfgAssigned(int file_id, std::string vo)
{
    PROFILE_PREFIXED("DB::", return db->hasStandAloneSeCfgAssigned(file_id, vo));
}


bool ProfiledDB::hasPairSeCfgAssigned(int file_id, std::string vo)
{
    PROFILE_PREFIXED("DB::", return db->hasPairSeCfgAssigned(file_id, vo));
}


bool ProfiledDB::hasPairGrCfgAssigned(int file_id, std::string vo)
{
    PROFILE_PREFIXED("DB::", return db->hasPairGrCfgAssigned(file_id, vo));
}


int ProfiledDB::countActiveTransfers(std::string source, std::string destination, std::string vo)
{
    PROFILE_PREFIXED("DB::", return db->countActiveTransfers(source, destination, vo));
}


int ProfiledDB::countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo)
{
    PROFILE_PREFIXED("DB::", return db->countActiveOutboundTransfersUsingDefaultCfg(se, vo));
}


int ProfiledDB::countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo)
{
    PROFILE_PREFIXED("DB::", return db->countActiveInboundTransfersUsingDefaultCfg(se, vo));
}


int ProfiledDB::sumUpVoShares (std::string source, std::string destination, std::set<std::string> vos)
{
    PROFILE_PREFIXED("DB::", return db->sumUpVoShares(source, destination, vos));
}


void ProfiledDB::setPriority(std::string jobId, int priority)
{
    PROFILE_PREFIXED("DB::", db->setPriority(jobId, priority));
}

void ProfiledDB::setSeProtocol(std::string protocol, std::string se, std::string state)
{
    PROFILE_PREFIXED("DB::", db->setSeProtocol(protocol, se, state));
}

void ProfiledDB::setRetry(int retry, const std::string & vo)
{
    PROFILE_PREFIXED("DB::", db->setRetry(retry, vo));
}


int ProfiledDB::getRetry(const std::string & jobId)
{
    PROFILE_PREFIXED("DB::", return db->getRetry(jobId));
}


int ProfiledDB::getRetryTimes(const std::string & jobId, int fileId)
{
    PROFILE_PREFIXED("DB::", return db->getRetryTimes(jobId, fileId));
}


void ProfiledDB::setMaxTimeInQueue(int afterXHours)
{
    PROFILE_PREFIXED("DB::", db->setMaxTimeInQueue(afterXHours));
}


void ProfiledDB::setToFailOldQueuedJobs(std::vector<std::string>& jobs)
{
    PROFILE_PREFIXED("DB::", db->setToFailOldQueuedJobs(jobs));
}


std::vector< std::pair<std::string, std::string> > ProfiledDB::getPairsForSe(std::string se)
{
    PROFILE_PREFIXED("DB::", return db->getPairsForSe(se));
}


std::vector<std::string> ProfiledDB::getAllActivityShareConf()
{
    PROFILE_PREFIXED("DB::", return db->getAllActivityShareConf());
}


std::vector<std::string> ProfiledDB::getAllStandAlloneCfgs()
{
    PROFILE_PREFIXED("DB::", return db->getAllStandAlloneCfgs());
}


std::vector<std::string> ProfiledDB::getAllShareOnlyCfgs()
{
    PROFILE_PREFIXED("DB::", return db->getAllShareOnlyCfgs());
}


int ProfiledDB::activeProcessesForThisHost()
{
    PROFILE_PREFIXED("DB::", return db->activeProcessesForThisHost());
}


std::vector< std::pair<std::string, std::string> > ProfiledDB::getAllPairCfgs()
{
    PROFILE_PREFIXED("DB::", return db->getAllPairCfgs());
}


void ProfiledDB::getCredentials(std::string & vo_name, const std::string & job_id, int file_id, std::string & dn, std::string & dlg_id)
{
    PROFILE_PREFIXED("DB::", db->getCredentials(vo_name, job_id, file_id, dn, dlg_id));
}


void ProfiledDB::setMaxStageOp(const std::string& se, const std::string& vo, int val)
{
    PROFILE_PREFIXED("DB::", db->setMaxStageOp(se, vo, val));
}


double ProfiledDB::getSuccessRate(std::string source, std::string destination)
{
    PROFILE_PREFIXED("DB::", return db->getSuccessRate(source, destination));
}


double ProfiledDB::getAvgThroughput(std::string source, std::string destination)
{
    PROFILE_PREFIXED("DB::", return db->getAvgThroughput(source, destination));
}


void ProfiledDB::updateProtocol(const std::string& jobId, int fileId, int nostreams, int timeout, int buffersize, double filesize)
{
    PROFILE_PREFIXED("DB::", db->updateProtocol(jobId, fileId, nostreams, timeout, buffersize, filesize));
}


void ProfiledDB::cancelFilesInTheQueue(const std::string& se, const std::string& vo, std::set<std::string>& jobs)
{
    PROFILE_PREFIXED("DB::", db->cancelFilesInTheQueue(se, vo, jobs));
}


void ProfiledDB::cancelJobsInTheQueue(const std::string& dn, std::vector<std::string>& jobs)
{
    PROFILE_PREFIXED("DB::", db->cancelJobsInTheQueue(dn, jobs));
}


void ProfiledDB::transferLogFileVector(std::map<int, struct message_log>& messagesLog)
{
    PROFILE_PREFIXED("DB::", db->transferLogFileVector(messagesLog));
}


std::vector<struct message_state> ProfiledDB::getStateOfTransfer(const std::string& jobId, int file_id)
{
    PROFILE_PREFIXED("DB::", return db->getStateOfTransfer(jobId, file_id));
}


void ProfiledDB::getFilesForJob(const std::string& jobId, std::vector<int>& files)
{
    PROFILE_PREFIXED("DB::", db->getFilesForJob(jobId, files));
}


void ProfiledDB::getFilesForJobInCancelState(const std::string& jobId, std::vector<int>& files)
{
    PROFILE_PREFIXED("DB::", db->getFilesForJobInCancelState(jobId, files));
}


void ProfiledDB::setFilesToWaiting(const std::string& se, const std::string& vo, int timeout)
{
    PROFILE_PREFIXED("DB::", db->setFilesToWaiting(se, vo, timeout));
}


void ProfiledDB::setFilesToWaiting(const std::string& dn, int timeout)
{
    PROFILE_PREFIXED("DB::", db->setFilesToWaiting(dn, timeout));
}


void ProfiledDB::cancelWaitingFiles(std::set<std::string>& jobs)
{
    PROFILE_PREFIXED("DB::", db->cancelWaitingFiles(jobs));
}


void ProfiledDB::revertNotUsedFiles()
{
    PROFILE_PREFIXED("DB::", db->revertNotUsedFiles());
}


void ProfiledDB::checkSanityState()
{
    PROFILE_PREFIXED("DB::", db->checkSanityState());
}


void ProfiledDB::checkSchemaLoaded()
{
    PROFILE_PREFIXED("DB::", db->checkSchemaLoaded());
}


void ProfiledDB::storeProfiling(const fts3::ProfilingSubsystem* prof)
{
    PROFILE_PREFIXED("DB::", db->storeProfiling(prof));
}

void ProfiledDB::setOptimizerMode(int mode)
{
    PROFILE_PREFIXED("DB::", db->setOptimizerMode(mode));
}

void ProfiledDB::setRetryTransfer(const std::string & jobId, int fileId,
                                  int retry, const std::string& reason)
{
    PROFILE_PREFIXED("DB::", db->setRetryTransfer(jobId, fileId, retry, reason));
}

void ProfiledDB::getTransferRetries(int fileId, std::vector<FileRetry*>& retries)
{
    PROFILE_PREFIXED("DB::", db->getTransferRetries(fileId, retries));
}

void ProfiledDB::updateHeartBeat(unsigned* index, unsigned* count, unsigned* start, unsigned* end, std::string service_name)
{
    PROFILE_PREFIXED("DB::", db->updateHeartBeat(index, count, start, end, service_name));
}

unsigned int ProfiledDB::updateFileStatusReuse(TransferFiles const & file, const std::string status)
{
    PROFILE_PREFIXED("DB::", return db->updateFileStatusReuse(file, status));
}

void ProfiledDB::getCancelJob(std::vector<int>& requestIDs)
{
    PROFILE_PREFIXED("DB::", db->getCancelJob(requestIDs));
}

void ProfiledDB::snapshot(const std::string & vo_name, const std::string & source_se, const std::string & dest_se, const std::string & endpoint,  std::stringstream & result)
{
    PROFILE_PREFIXED("DB::", db->snapshot(vo_name, source_se, dest_se, endpoint, result));
}

bool ProfiledDB::getDrain()
{
    PROFILE_PREFIXED("DB::", return db->getDrain());
}

void ProfiledDB::setDrain(bool drain)
{
    PROFILE_PREFIXED("DB::", db->setDrain(drain));
}

void ProfiledDB::setShowUserDn(bool show)
{
    PROFILE_PREFIXED("DB::", db->setShowUserDn(show));
}

bool ProfiledDB::getShowUserDn()
{
    PROFILE_PREFIXED("DB::", return db->getShowUserDn());
}

void ProfiledDB::setBandwidthLimit(const std::string & source_hostname, const std::string & destination_hostname, int bandwidthLimit)
{
    PROFILE_PREFIXED("DB::", db->setBandwidthLimit(source_hostname, destination_hostname, bandwidthLimit));
}

std::string ProfiledDB::getBandwidthLimit()
{
    PROFILE_PREFIXED("DB::", return db->getBandwidthLimit());
}

bool ProfiledDB::isProtocolUDT(const std::string & source_hostname, const std::string & destination_hostname)
{
    PROFILE_PREFIXED("DB::", return db->isProtocolUDT(source_hostname, destination_hostname));
}

bool ProfiledDB::isProtocolIPv6(const std::string & source_hostname, const std::string & destination_hostname)
{
    PROFILE_PREFIXED("DB::", return db->isProtocolIPv6(source_hostname, destination_hostname));
}

int ProfiledDB::getStreamsOptimization(const std::string & source_hostname, const std::string & destination_hostname)
{
    PROFILE_PREFIXED("DB::", return db->getStreamsOptimization(source_hostname, destination_hostname));
}

int ProfiledDB::getGlobalTimeout()
{
    PROFILE_PREFIXED("DB::", return db->getGlobalTimeout());
}

void ProfiledDB::setGlobalTimeout(int timeout)
{
    PROFILE_PREFIXED("DB::", db->setGlobalTimeout(timeout));
}

int ProfiledDB::getSecPerMb()
{
    PROFILE_PREFIXED("DB::", return db->getSecPerMb());
}

void ProfiledDB::setSecPerMb(int seconds)
{
    PROFILE_PREFIXED("DB::", db->setSecPerMb(seconds));
}

void ProfiledDB::setSourceMaxActive(const std::string & source_hostname, int maxActive)
{
    PROFILE_PREFIXED("DB::", db->setSourceMaxActive( source_hostname, maxActive));
}

void ProfiledDB::setDestMaxActive(const std::string & destination_hostname, int maxActive)
{
    PROFILE_PREFIXED("DB::", db->setDestMaxActive(destination_hostname,  maxActive));
}

void ProfiledDB::setFixActive(const std::string & source, const std::string & destination, int active)
{
    PROFILE_PREFIXED("DB::", db->setFixActive(source, destination, active));
}

int ProfiledDB::getBufferOptimization()
{
    PROFILE_PREFIXED("DB::", return db->getBufferOptimization());
}

void ProfiledDB::getTransferJobStatusDetailed(std::string job_id, std::vector<boost::tuple<std::string, std::string, int, std::string, std::string> >& files)
{
    PROFILE_PREFIXED("DB::", db->getTransferJobStatusDetailed(job_id, files));
}

void ProfiledDB::getVOPairs(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct)
{
    PROFILE_PREFIXED("DB::", db->getVOPairs(distinct));
}

void ProfiledDB::getVOPairsWithReuse(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct)
{
    PROFILE_PREFIXED("DB::", db->getVOPairsWithReuse(distinct));
}

//deletions
void ProfiledDB::updateDeletionsState(std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files)
{
    PROFILE_PREFIXED("DB::", db->updateDeletionsState(files));
}

//file_id / surl / proxy
void ProfiledDB::getFilesForDeletion(std::vector< boost::tuple<std::string, std::string, std::string, int, std::string, std::string> >& files)
{
    PROFILE_PREFIXED("DB::", db->getFilesForDeletion(files));
}

//job_id
void ProfiledDB::cancelDeletion(std::vector<std::string>& files)
{
    PROFILE_PREFIXED("DB::", db->cancelDeletion(files));
}

//file_id / surl
void ProfiledDB::getDeletionFilesForCanceling(std::vector< boost::tuple<int, std::string, std::string> >& files)
{
    PROFILE_PREFIXED("DB::", db->getDeletionFilesForCanceling(files));
}

void ProfiledDB::setMaxDeletionsPerEndpoint(int maxDeletions, const std::string & endpoint, const std::string & vo)
{
    PROFILE_PREFIXED("DB::", db->setMaxDeletionsPerEndpoint(maxDeletions, endpoint, vo));
}

int ProfiledDB::getMaxDeletionsPerEndpoint(const std::string & endpoint, const std::string & vo)
{
    PROFILE_PREFIXED("DB::", return db->getMaxDeletionsPerEndpoint(endpoint, vo));
}



//staging						//file_id / state / reason / token
void ProfiledDB::updateStagingState(std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files)
{
    PROFILE_PREFIXED("DB::", db->updateStagingState(files));
}

void ProfiledDB::updateBringOnlineToken(std::map< std::string, std::vector<int> > const & jobs, std::string const & token)
{
    PROFILE_PREFIXED("DB::", db->updateBringOnlineToken(jobs, token));
}

//file_id / surl / proxy / pinlifetime / bringonlineTimeout
void ProfiledDB::getFilesForStaging(std::vector< boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string> >& files)
{
    PROFILE_PREFIXED("DB::", db->getFilesForStaging(files));
}

void ProfiledDB::getAlreadyStartedStaging(std::vector< boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string, std::string> >& files)
{
    PROFILE_PREFIXED("DB::", db->getAlreadyStartedStaging(files));
}

//file_id / surl / token
void ProfiledDB::getStagingFilesForCanceling(std::vector< boost::tuple<int, std::string, std::string> >& files)
{
    PROFILE_PREFIXED("DB::", db->getStagingFilesForCanceling(files));
}

void ProfiledDB::setMaxStagingPerEndpoint(int maxStaging, const std::string & endpoint, const std::string & vo)
{
    PROFILE_PREFIXED("DB::", db->setMaxStagingPerEndpoint(maxStaging, endpoint, vo));
}

int ProfiledDB::getMaxStatingsPerEndpoint(const std::string & endpoint, const std::string & vo)
{
    PROFILE_PREFIXED("DB::", return db->getMaxStatingsPerEndpoint(endpoint, vo));
}


void ProfiledDB::submitdelete(const std::string & jobId, const std::multimap<std::string,std::string>& rulsHost,
                              const std::string & DN, const std::string & voName, const std::string & credID)
{
    PROFILE_PREFIXED("DB::", db->submitdelete(jobId, rulsHost, DN, voName, credID));
}


void ProfiledDB::checkJobOperation(std::vector<std::string>& jobs, std::vector< boost::tuple<std::string, std::string> >& ops)
{
    PROFILE_PREFIXED("DB::", db->checkJobOperation(jobs, ops));
}


bool ProfiledDB::getOauthCredentials(const std::string& user_dn,
                                     const std::string& vo, const std::string& cloud_name, OAuth& oauth)
{
    PROFILE_PREFIXED("DB::", return db->getOauthCredentials(user_dn, vo, cloud_name, oauth));
}

bool ProfiledDB::isDmJob(std::string const & job)
{
    PROFILE_PREFIXED("DB::", return db->isDmJob(job));
}

void ProfiledDB::cancelDmJobs(std::vector<std::string> const & jobs)
{
    PROFILE_PREFIXED("DB::", db->cancelDmJobs(jobs));
}

bool ProfiledDB::getUserDnVisible()
{
    PROFILE_PREFIXED("DB::", return db->getUserDnVisible());
}
