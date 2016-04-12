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

void ProfiledDB::init(const std::string& username, const std::string& password, const std::string& connectString, int pooledConn)
{
    PROFILE_PREFIXED("DB::", db->init(username, password, connectString, pooledConn));
}

void ProfiledDB::submitPhysical(const std::string & jobId,
        std::list<SubmittedTransfer>& transfers, const std::string & DN,
        const std::string & cred, const std::string & voName,
        const std::string & myProxyServer, const std::string & delegationID,
        const std::string & sourceSe, const std::string & destinationSe,
        const fts3::common::JobParameterHandler & params)
{
    PROFILE_PREFIXED("DB::", db->submitPhysical(jobId, transfers, DN, cred, voName, myProxyServer, delegationID,
                         sourceSe, destinationSe, params));
}


void ProfiledDB::getTransferStatuses(const std::string& jobId, bool archive,
        unsigned offset, unsigned limit, std::vector<FileTransferStatus>& files)
{
    PROFILE_PREFIXED("DB::", db->getTransferStatuses(jobId, archive, offset, limit, files));
}


void ProfiledDB::getDmStatuses(const std::string& requestID, bool archive,
        unsigned offset, unsigned limit, std::vector<FileTransferStatus>& files)
{
    PROFILE_PREFIXED("DB::", db->getDmStatuses(requestID, archive, offset, limit, files));
}


void ProfiledDB::listJobs(const std::vector<std::string>& inGivenStates,
        const std::string& forDN, const std::string& voName,
        const std::string& src, const std::string& dst,
        std::vector<JobStatus>& jobs)
{
    PROFILE_PREFIXED("DB::", db->listJobs(inGivenStates, forDN, voName, src, dst, jobs));
}

void ProfiledDB::listDmJobs(const std::vector<std::string>& inGivenStates,
        const std::string& forDN, const std::string& voName,
        const std::string& src, const std::string& dst,
        std::vector<JobStatus>& jobs)
{
    PROFILE_PREFIXED("DB::", db->listDmJobs(inGivenStates, forDN, voName, src, dst, jobs));
}

boost::optional<Job> ProfiledDB::getJob(const std::string & jobId, bool archive)
{
    PROFILE_PREFIXED("DB::", return db->getJob(jobId, archive));
}


void ProfiledDB::getReadySessionReuseTransfers(const std::vector<QueueId>& queues,
        std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFile> > > >& files)
{
    PROFILE_PREFIXED("DB::", db->getReadySessionReuseTransfers(queues, files));
}


void ProfiledDB::getReadyTransfers(const std::vector<QueueId>& queues,
        std::map< std::string, std::list<TransferFile> >& files)
{
    PROFILE_PREFIXED("DB::", db->getReadyTransfers(queues, files));
}


void ProfiledDB::getMultihopJobs(std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFile> > > >& files)
{
    PROFILE_PREFIXED("DB::", db->getMultihopJobs(files));
}


boost::optional<StorageElement> ProfiledDB::getStorageElement(const std::string& seName)
{
    PROFILE_PREFIXED("DB::", return db->getStorageElement(seName));
}


void ProfiledDB::addStorageElement(const std::string& name, const std::string& state)
{
    PROFILE_PREFIXED("DB::", db->addStorageElement(name, state));
}


void ProfiledDB::updateStorageElement(const std::string& name, const std::string& state)
{
    PROFILE_PREFIXED("DB::", db->updateStorageElement(name, state));
}


bool ProfiledDB::updateTransferStatus(const std::string& jobId, int fileId, double throughput,
        const std::string& transferState, const std::string& errorReason,
        int processId, double filesize, double duration, bool retry)
{
    PROFILE_PREFIXED("DB::", return db->updateTransferStatus(jobId, fileId, throughput,
            transferState, errorReason, processId, filesize, duration, retry));
}


bool ProfiledDB::updateJobStatus(const std::string& jobId, const std::string& jobState, int pid)
{
    PROFILE_PREFIXED("DB::", return db->updateJobStatus(jobId, jobState, pid));
}


void ProfiledDB::updateFileTransferProgressVector(std::vector<fts3::events::MessageUpdater>& messages)
{
    PROFILE_PREFIXED("DB::", db->updateFileTransferProgressVector(messages));
}


void ProfiledDB::cancelJob(const std::vector<std::string>& requestIDs)
{
    PROFILE_PREFIXED("DB::", db->cancelJob(requestIDs));
}


void ProfiledDB::cancelAllJobs(const std::string& voName, std::vector<std::string>& canceledJobs)
{
    PROFILE_PREFIXED("DB::", db->cancelAllJobs(voName, canceledJobs));
}


bool ProfiledDB::insertCredentialCache(const std::string& delegationId,
        const std::string& userDn, const std::string& certRequest,
        const std::string& privateKey, const std::string& vomsAttrs)
{
    PROFILE_PREFIXED("DB::", return db->insertCredentialCache(delegationId, userDn, certRequest, privateKey, vomsAttrs));
}


boost::optional<UserCredentialCache> ProfiledDB::findCredentialCache(const std::string& delegationId, const std::string& userDn)
{
    PROFILE_PREFIXED("DB::", return db->findCredentialCache(delegationId, userDn));
}


void ProfiledDB::deleteCredentialCache(const std::string& delegationId, const std::string& userDn)
{
    PROFILE_PREFIXED("DB::", db->deleteCredentialCache(delegationId, userDn));
}


void ProfiledDB::insertCredential(const std::string& delegationId,
        const std::string& userDn, const std::string& proxy, const std::string& vomsAttrs,
        time_t terminationTime)
{
    PROFILE_PREFIXED("DB::", db->insertCredential(delegationId, userDn, proxy, vomsAttrs, terminationTime));
}


void ProfiledDB::updateCredential(const std::string& delegationId,
        const std::string& userDn, const std::string& proxy,
        const std::string& vomsAttrs, time_t terminationTime)
{
    PROFILE_PREFIXED("DB::", db->updateCredential(delegationId, userDn, proxy, vomsAttrs, terminationTime));
}


boost::optional<UserCredential> ProfiledDB::findCredential(const std::string& delegationId, const std::string& userDn)
{
    PROFILE_PREFIXED("DB::", return db->findCredential(delegationId, userDn));
}


void ProfiledDB::deleteCredential(const std::string& delegationId, const std::string& userDn)
{
    PROFILE_PREFIXED("DB::", db->deleteCredential(delegationId, userDn));
}


unsigned ProfiledDB::getDebugLevel(const std::string& sourceStorage, const std::string& destStorage)
{
    PROFILE_PREFIXED("DB::", return db->getDebugLevel(sourceStorage, destStorage));
}


void ProfiledDB::setDebugLevel(const std::string& sourceStorage, const std::string& destStorage, unsigned level)
{
    PROFILE_PREFIXED("DB::", db->setDebugLevel(sourceStorage, destStorage, level));
}


void ProfiledDB::auditConfiguration(const std::string & dn, const std::string & config, const std::string & action)
{
    PROFILE_PREFIXED("DB::", db->auditConfiguration(dn, config, action));
}


OptimizerSample ProfiledDB::fetchOptimizationConfig(const std::string & source_hostname, const std::string & destin_hostname)
{
    PROFILE_PREFIXED("DB::", return db->fetchOptimizationConfig(source_hostname, destin_hostname));
}


bool ProfiledDB::isCredentialExpired(const std::string & dlg_id, const std::string & dn)
{
    PROFILE_PREFIXED("DB::", return db->isCredentialExpired(dlg_id, dn));
}


fts3::optimizer::OptimizerDataSource* ProfiledDB::getOptimizerDataSource()
{
    PROFILE_PREFIXED("DB::", return db->getOptimizerDataSource());
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


void ProfiledDB::setPidForJob(const std::string& jobId, int pid)
{
    PROFILE_PREFIXED("DB::", db->setPidForJob(jobId, pid));
}


void ProfiledDB::revertToSubmitted()
{
    PROFILE_PREFIXED("DB::", db->revertToSubmitted());
}


void ProfiledDB::backup(long bulkSize, long* nJobs, long* nFiles)
{
    PROFILE_PREFIXED("DB::", db->backup(bulkSize, nJobs, nFiles));
}


void ProfiledDB::forkFailed(const std::string& jobId)
{
    PROFILE_PREFIXED("DB::", db->forkFailed(jobId));
}


bool ProfiledDB::markAsStalled(const std::vector<fts3::events::MessageUpdater>& messages, bool diskFull)
{
    PROFILE_PREFIXED("DB::", return db->markAsStalled(messages, diskFull));
}


void ProfiledDB::blacklistSe(const std::string& storage,
        const std::string& voName, const std::string& status, int timeout,
        const std::string& msg, const std::string& adminDn)
{
    PROFILE_PREFIXED("DB::", db->blacklistSe(storage, voName, status, timeout, msg, adminDn));
}


void ProfiledDB::blacklistDn(const std::string& userDn, const std::string& msg,
        const std::string& adminDn)
{
    PROFILE_PREFIXED("DB::", db->blacklistDn(userDn, msg, adminDn));
}


void ProfiledDB::unblacklistSe(const std::string& storage)
{
    PROFILE_PREFIXED("DB::", db->unblacklistSe(storage));
}


void ProfiledDB::unblacklistDn(const std::string& userDn)
{
    PROFILE_PREFIXED("DB::", db->unblacklistDn(userDn));
}


bool ProfiledDB::allowSubmit(const std::string& storage, const std::string& vo)
{
    PROFILE_PREFIXED("DB::", return db->allowSubmit(storage, vo));
}


boost::optional<int> ProfiledDB::getTimeoutForSe(const std::string& storage)
{
    PROFILE_PREFIXED("DB::", return db->getTimeoutForSe(storage));
}


bool ProfiledDB::isDnBlacklisted(const std::string& userDn)
{
    PROFILE_PREFIXED("DB::", return db->isDnBlacklisted(userDn));
}


bool ProfiledDB::checkGroupExists(const std::string & groupName)
{
    PROFILE_PREFIXED("DB::", return db->checkGroupExists(groupName));
}


void ProfiledDB::getGroupMembers(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    PROFILE_PREFIXED("DB::", db->getGroupMembers(groupName, groupMembers));
}


void ProfiledDB::addMemberToGroup(const std::string & groupName,
    const std::vector<std::string>& groupMembers)
{
    PROFILE_PREFIXED("DB::", db->addMemberToGroup(groupName, groupMembers));
}


void ProfiledDB::deleteMembersFromGroup(const std::string & groupName,
    const std::vector<std::string>& groupMembers)
{
    PROFILE_PREFIXED("DB::", db->deleteMembersFromGroup(groupName, groupMembers));
}


std::string ProfiledDB::getGroupForSe(const std::string se)
{
    PROFILE_PREFIXED("DB::", return db->getGroupForSe(se));
}


void ProfiledDB::addLinkConfig(const LinkConfig& cfg)
{
    PROFILE_PREFIXED("DB::", db->addLinkConfig(cfg));
}

void ProfiledDB::updateLinkConfig(const LinkConfig& cfg)
{
    PROFILE_PREFIXED("DB::", db->updateLinkConfig(cfg));
}

void ProfiledDB::deleteLinkConfig(std::string source, std::string destination)
{
    PROFILE_PREFIXED("DB::", db->deleteLinkConfig(source, destination));
}

std::unique_ptr<LinkConfig> ProfiledDB::getLinkConfig(std::string source, std::string destination)
{
    PROFILE_PREFIXED("DB::", return db->getLinkConfig(source, destination));
}

std::unique_ptr<std::pair<std::string, std::string>> ProfiledDB::getSourceAndDestination(std::string symbolic_name)
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


void ProfiledDB::addShareConfig(const ShareConfig& cfg)
{
    PROFILE_PREFIXED("DB::", db->addShareConfig(cfg));
}

void ProfiledDB::updateShareConfig(const ShareConfig& cfg)
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

std::unique_ptr<ShareConfig> ProfiledDB::getShareConfig(std::string source, std::string destination, std::string vo)
{
    PROFILE_PREFIXED("DB::", return db->getShareConfig(source, destination, vo));
}

std::vector<ShareConfig> ProfiledDB::getShareConfig(std::string source, std::string destination)
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

void ProfiledDB::setGlobalLimits(const int* maxActivePerLink, const int* maxActivePerSe)
{
    PROFILE_PREFIXED("DB::", db->setGlobalLimits(maxActivePerLink, maxActivePerSe));
}


void ProfiledDB::authorize(bool add, const std::string& op, const std::string& dn)
{
    PROFILE_PREFIXED("DB::", db->authorize(add, op, dn));
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


std::vector< std::pair<std::string, std::string> > ProfiledDB::getAllPairCfgs()
{
    PROFILE_PREFIXED("DB::", return db->getAllPairCfgs());
}


void ProfiledDB::setMaxStageOp(const std::string& se, const std::string& vo, int val, const std::string & opt)
{
    PROFILE_PREFIXED("DB::", db->setMaxStageOp(se, vo, val, opt));
}


void ProfiledDB::updateProtocol(std::vector<fts3::events::Message>& tempProtocol)
{
    PROFILE_PREFIXED("DB::", db->updateProtocol(tempProtocol));
}


void ProfiledDB::cancelFilesInTheQueue(const std::string& se, const std::string& vo, std::set<std::string>& jobs)
{
    PROFILE_PREFIXED("DB::", db->cancelFilesInTheQueue(se, vo, jobs));
}


void ProfiledDB::cancelJobsInTheQueue(const std::string& dn, std::vector<std::string>& jobs)
{
    PROFILE_PREFIXED("DB::", db->cancelJobsInTheQueue(dn, jobs));
}


void ProfiledDB::transferLogFileVector(std::map<int, fts3::events::MessageLog>& messagesLog)
{
    PROFILE_PREFIXED("DB::", db->transferLogFileVector(messagesLog));
}


std::vector<TransferState> ProfiledDB::getStateOfTransfer(const std::string& jobId, int file_id)
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

void ProfiledDB::getTransferRetries(int fileId, std::vector<FileRetry>& retries)
{
    PROFILE_PREFIXED("DB::", db->getTransferRetries(fileId, retries));
}

void ProfiledDB::updateHeartBeat(unsigned* index, unsigned* count, unsigned* start, unsigned* end, std::string service_name)
{
    PROFILE_PREFIXED("DB::", db->updateHeartBeat(index, count, start, end, service_name));
}

unsigned int ProfiledDB::updateFileStatusReuse(TransferFile const & file, const std::string status)
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

void ProfiledDB::getQueuesWithPending(std::vector<QueueId>& queues)
{
    PROFILE_PREFIXED("DB::", db->getQueuesWithPending(queues));
}

void ProfiledDB::getQueuesWithSessionReusePending(std::vector<QueueId>& queues)
{
    PROFILE_PREFIXED("DB::", db->getQueuesWithSessionReusePending(queues));
}

void ProfiledDB::updateDeletionsState(const std::vector<MinFileStatus>& delOpsStatus)
{
    PROFILE_PREFIXED("DB::", db->updateDeletionsState(delOpsStatus));
}

void ProfiledDB::getFilesForDeletion(std::vector<DeleteOperation>& delOps)
{
    PROFILE_PREFIXED("DB::", db->getFilesForDeletion(delOps));
}

void ProfiledDB::requeueStartedDeletes()
{
    PROFILE_PREFIXED("DB::", return db->requeueStartedDeletes());
}

void ProfiledDB::updateStagingState(const std::vector<MinFileStatus>& stagingOpsStatus)
{
    PROFILE_PREFIXED("DB::", db->updateStagingState(stagingOpsStatus));
}

void ProfiledDB::updateBringOnlineToken(std::map< std::string, std::map<std::string, std::vector<int> > > const & jobs, std::string const & token)
{
    PROFILE_PREFIXED("DB::", db->updateBringOnlineToken(jobs, token));
}

//file_id / surl / proxy / pinlifetime / bringonlineTimeout
void ProfiledDB::getFilesForStaging(std::vector<StagingOperation>& stagingOps)
{
    PROFILE_PREFIXED("DB::", db->getFilesForStaging(stagingOps));
}

void ProfiledDB::getAlreadyStartedStaging(std::vector<StagingOperation> &stagingOps)
{
    PROFILE_PREFIXED("DB::", db->getAlreadyStartedStaging(stagingOps));
}

//file_id / surl / token
void ProfiledDB::getStagingFilesForCanceling(std::set< std::pair<std::string, std::string> >& files)
{
    PROFILE_PREFIXED("DB::", db->getStagingFilesForCanceling(files));
}


void ProfiledDB::submitDelete(const std::string & jobId, const std::map<std::string,std::string>& rulsHost,
                              const std::string & DN, const std::string & voName, const std::string & credID)
{
    PROFILE_PREFIXED("DB::", db->submitDelete(jobId, rulsHost, DN, voName, credID));
}


bool ProfiledDB::getCloudStorageCredentials(const std::string& user_dn,
                                     const std::string& vo, const std::string& cloud_name, CloudStorageAuth& auth)
{
    PROFILE_PREFIXED("DB::", return db->getCloudStorageCredentials(user_dn, vo, cloud_name, auth));
}

void ProfiledDB::setCloudStorageCredentials(std::string const &dn, std::string const &vo, std::string const &storage,
    std::string const &accessKey, std::string const &secretKey)
{
    PROFILE_PREFIXED("DB::", return db->setCloudStorageCredentials(dn, vo, storage, accessKey, secretKey));
}

void ProfiledDB::setCloudStorage(std::string const & storage, std::string const & appKey, std::string const & appSecret, std::string const & apiUrl)
{
    PROFILE_PREFIXED("DB::", return db->setCloudStorage(storage, appKey, appSecret, apiUrl));
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
