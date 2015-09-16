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

#pragma once

#include <iomanip>
#include <map>
#include "db/generic/GenericDbIfce.h"

/**
 * This class is intended to wrap implementations of the DB,
 * profiling their times.
 */
class ProfiledDB : public GenericDbIfce
{
private:
    GenericDbIfce *db;
    void (*destroy_db)(void *);

public:
    ProfiledDB(GenericDbIfce* db, void (*destroy_db)(void *));
    ~ProfiledDB();

    void init(const std::string& username, const std::string& password, const std::string& connectString, int pooledConn);

    void submitPhysical(const std::string& jobId,
            std::list<SubmittedTransfer>& transfers,
            const std::string& DN, const std::string& cred,
            const std::string& voName, const std::string& myProxyServer,
            const std::string& delegationID, const std::string& sourceSe,
            const std::string& destinationSe,
            const fts3::common::JobParameterHandler& params);

    boost::optional<Job> getJob(const std::string & jobId, bool archive);

    void getTransferStatuses(const std::string& jobId, bool archive,
            unsigned offset, unsigned limit,
            std::vector<FileTransferStatus>& files);

    void getDmStatuses(const std::string& requestID, bool archive,
            unsigned offset, unsigned limit,
            std::vector<FileTransferStatus>& files);

    void listJobs(const std::vector<std::string>& inGivenStates,
            const std::string& forDN, const std::string& voName,
            const std::string& src, const std::string& dst,
            std::vector<JobStatus>& jobs);

    void listDmJobs(const std::vector<std::string>& inGivenStates,
            const std::string& forDN, const std::string& voName,
            const std::string& src, const std::string& dst,
            std::vector<JobStatus>& jobs);

    void getReadySessionReuseTransfers(const std::vector<QueueId>& queues,
            std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFile> > > >& files);

    void getReadyTransfers(const std::vector<QueueId>& queues,
            std::map< std::string, std::list<TransferFile> >& files);

    void getMultihopJobs(std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFile> > > >& files);

    boost::optional<StorageElement> getStorageElement(const std::string& seName);

    void addStorageElement(const std::string& name, const std::string& state);

    void updateStorageElement(const std::string& name, const std::string& state);


    bool updateTransferStatus(const std::string& jobId, int fileId, double throughput,
                const std::string& transferState, const std::string& errorReason,
                int processId, double filesize, double duration, bool retry);

    bool updateJobStatus(const std::string& jobId, const std::string& jobState, int pid);

    void updateFileTransferProgressVector(std::vector<struct message_updater>& messages);

    void cancelJob(const std::vector<std::string>& requestIDs);

    void cancelAllJobs(const std::string& voName, std::vector<std::string>& canceledJobs);

    bool insertCredentialCache(const std::string& delegationId,
            const std::string& userDn, const std::string& certRequest,
            const std::string& privateKey, const std::string& vomsAttrs);

    boost::optional<UserCredentialCache> findCredentialCache(
            const std::string& delegationId, const std::string& userDn);

    void deleteCredentialCache(const std::string& delegationId, const std::string& userDn);

    void insertCredential(const std::string& delegationId,
            const std::string& userDn, const std::string& proxy,
            const std::string& vomsAttrs, time_t terminationTime);

    void updateCredential(const std::string& delegationId,
            const std::string& userDn, const std::string& proxy,
            const std::string& vomsAttrs, time_t terminationTime);

    boost::optional<UserCredential> findCredential(const std::string& delegationId, const std::string& userDn);

    void deleteCredential(const std::string& delegationId, const std::string& userDn);

    unsigned getDebugLevel(const std::string& sourceStorage, const std::string& destStorage);

    void setDebugLevel(const std::string& sourceStorage, const std::string& destStorage, unsigned level);

    void auditConfiguration(const std::string & dn, const std::string & config, const std::string & action);

    OptimizerSample fetchOptimizationConfig(const std::string & source_hostname, const std::string & destin_hostname);

    bool isCredentialExpired(const std::string & dlg_id, const std::string & dn);

    bool updateOptimizer();

    bool isTrAllowed(const std::string & source_se, const std::string & dest, int &currentActive);

    int getSeOut(const std::string & source, const std::set<std::string> & destination);

    int getSeIn(const std::set<std::string> & source, const std::string & destination);

    bool terminateReuseProcess(const std::string & jobId, int pid, const std::string & message);

    void forceFailTransfers(std::map<int, std::string>& collectJobs);

    void setPidForJob(const std::string& jobId, int pid);

    void revertToSubmitted();

    void backup(long* nJobs, long* nFiles);

    void forkFailedRevertStateV(std::map<int,std::string>& pids);

    bool retryFromDead(std::vector<struct message_updater>& messages, bool diskFull);

    void blacklistSe(const std::string& storage, const std::string& voName,
            const std::string& status, int timeout,
            const std::string& msg, const std::string& adminDn);

    void blacklistDn(const std::string& userDn,
            const std::string& msg, const std::string& adminDn);

    void unblacklistSe(const std::string& storage);

    void unblacklistDn(const std::string& userDn);

    bool allowSubmit(const std::string& storage, const std::string& voName);

    boost::optional<int> getTimeoutForSe(const std::string& storage);

    bool isDnBlacklisted(const std::string& userDn);

    bool isFileReadyStateV(std::map<int,std::string>& fileIds);

    bool checkGroupExists(const std::string & groupName);

    void getGroupMembers(const std::string & groupName, std::vector<std::string>& groupMembers);

    void addMemberToGroup(const std::string & groupName, std::vector<std::string>& groupMembers);

    void deleteMembersFromGroup(const std::string & groupName, std::vector<std::string>& groupMembers);

    std::string getGroupForSe(const std::string se);

    void addLinkConfig(const LinkConfig& cfg);
    void updateLinkConfig(const LinkConfig& cfg);
    void deleteLinkConfig(std::string source, std::string destination);
    std::unique_ptr<LinkConfig> getLinkConfig(std::string source, std::string destination);
    std::unique_ptr<std::pair<std::string, std::string>> getSourceAndDestination(std::string symbolic_name);
    bool isGrInPair(std::string group);
    bool isShareOnly(std::string se);

    void addShareConfig(const ShareConfig& cfg);
    void updateShareConfig(const ShareConfig& cfg);
    void deleteShareConfig(std::string source, std::string destination, std::string vo);
    void deleteShareConfig(std::string source, std::string destination);
    std::unique_ptr<ShareConfig> getShareConfig(std::string source, std::string destination, std::string vo);
    std::vector<ShareConfig> getShareConfig(std::string source, std::string destination);

    virtual void addActivityConfig(std::string vo, std::string shares, bool active);
    virtual void updateActivityConfig(std::string vo, std::string shares, bool active);
    virtual void deleteActivityConfig(std::string vo);
    virtual bool isActivityConfigActive(std::string vo);
    virtual std::map< std::string, double > getActivityConfig(std::string vo);

    bool checkIfSeIsMemberOfAnotherGroup( const std::string & member);

    void addFileShareConfig(int file_id, std::string source, std::string destination, std::string vo);

    void getFilesForNewSeCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out);

    void getFilesForNewGrCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out);

    void delFileShareConfig(int file_id, std::string source, std::string destination, std::string vo);

    void delFileShareConfig(std::string groupd, std::string se);

    bool hasStandAloneSeCfgAssigned(int file_id, std::string vo);

    bool hasPairSeCfgAssigned(int file_id, std::string vo);

    bool hasPairGrCfgAssigned(int file_id, std::string vo);

    int countActiveTransfers(std::string source, std::string destination, std::string vo);

    int countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo);

    int countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo);

    int sumUpVoShares (std::string source, std::string destination, std::set<std::string> vos);

    void setPriority(std::string jobId, int priority);

    void setSeProtocol(std::string protocol, std::string se, std::string state);

    void setRetry(int retry, const std::string & vo);

    int getRetry(const std::string & jobId);

    int getRetryTimes(const std::string & jobId, int fileId);

    void setRetryTransfer(const std::string & jobId, int fileId);

    void setMaxTimeInQueue(int afterXHours);

    void setGlobalLimits(const int* maxActivePerLink, const int* maxActivePerSe);

    void authorize(bool add, const std::string& op, const std::string& dn);

    void setToFailOldQueuedJobs(std::vector<std::string>& jobs);

    std::vector< std::pair<std::string, std::string> > getPairsForSe(std::string se);

    virtual std::vector<std::string> getAllActivityShareConf();

    std::vector<std::string> getAllStandAlloneCfgs();

    std::vector<std::string> getAllShareOnlyCfgs();

    std::vector< std::pair<std::string, std::string> > getAllPairCfgs();

    void setMaxStageOp(const std::string& se, const std::string& vo, int val, const std::string & opt);

    void updateProtocol(std::vector<struct message>& tempProtocol);

    void cancelFilesInTheQueue(const std::string& se, const std::string& vo, std::set<std::string>& jobs);

    void cancelJobsInTheQueue(const std::string& dn, std::vector<std::string>& jobs);

    void transferLogFileVector(std::map<int, struct message_log>& messagesLog);

    std::vector<struct message_state> getStateOfTransfer(const std::string& jobId, int file_id);

    void getFilesForJob(const std::string& jobId, std::vector<int>& files);

    void getFilesForJobInCancelState(const std::string& jobId, std::vector<int>& files);

    void setFilesToWaiting(const std::string& se, const std::string& vo, int timeout);

    void setFilesToWaiting(const std::string& dn, int timeout);

    void cancelWaitingFiles(std::set<std::string>& jobs);

    void revertNotUsedFiles();

    void checkSanityState();

    void checkSchemaLoaded();

    void storeProfiling(const fts3::ProfilingSubsystem* prof);

    void setOptimizerMode(int mode);

    void setRetryTransfer(const std::string & jobId, int fileId, int retry, const std::string& reason);

    void getTransferRetries(int fileId, std::vector<FileRetry>& retries);

    void updateHeartBeat(unsigned* index, unsigned* count, unsigned* start, unsigned* end, std::string service_name);

    unsigned int updateFileStatusReuse(TransferFile const & file, const std::string status);

    void getCancelJob(std::vector<int>& requestIDs);

    void snapshot(const std::string & vo_name, const std::string & source_se, const std::string & dest_se, const std::string & endpoint, std::stringstream & result);

    bool getDrain();

    void setDrain(bool drain);

    void setShowUserDn(bool show);

    void setBandwidthLimit(const std::string & source_hostname, const std::string & destination_hostname, int bandwidthLimit);

    std::string getBandwidthLimit();

    bool isProtocolUDT(const std::string & source_hostname, const std::string & destination_hostname);

    bool isProtocolIPv6(const std::string & source_hostname, const std::string & destination_hostname);

    int getStreamsOptimization(const std::string & source_hostname, const std::string & destination_hostname);

    int getGlobalTimeout();

    void setGlobalTimeout(int timeout);

    int getSecPerMb();

    void setSecPerMb(int seconds);

    void setSourceMaxActive(const std::string & source_hostname, int maxActive);

    void setDestMaxActive(const std::string & destination_hostname, int maxActive);

    void setFixActive(const std::string & source, const std::string & destination, int active);

    int getBufferOptimization();

    void getTransferJobStatusDetailed(std::string job_id, std::vector<boost::tuple<std::string, std::string, int, std::string, std::string> >& files);

    void getQueuesWithPending(std::vector<QueueId>& queues);

    void getQueuesWithSessionReusePending(std::vector<QueueId>& queues);


    //NEW deletions and staging API
    //deletions						 //file_id / state / reason
    void updateDeletionsState(std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files);

    //vo_name, source_url, job_id, file_id, user_dn, cred_id
    void getFilesForDeletion(std::vector< boost::tuple<std::string, std::string, std::string, int, std::string, std::string> >& files);

    void revertDeletionToStarted();

    //staging						//file_id / state / reason / token
    void updateStagingState(std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files);

    void updateBringOnlineToken(std::map< std::string, std::map<std::string, std::vector<int> > > const & jobs, std::string const & token);

    //file_id / surl / proxy / pinlifetime / bringonlineTimeout
    void getFilesForStaging(std::vector< boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string> >& files);

    void getAlreadyStartedStaging(std::vector< boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string, std::string> >& files);

    //file_id / surl / token
    void getStagingFilesForCanceling(std::set< std::pair<std::string, std::string> >& files);

    void submitDelete(const std::string & jobId, const std::map<std::string,std::string>& rulsHost,
                      const std::string & DN, const std::string & voName, const std::string & credID);

    bool getOauthCredentials(const std::string& user_dn, const std::string& vo,
                             const std::string& cloud_name, OAuth& oauth);

    void setCloudStorageCredential(std::string const & dn, std::string const & vo, std::string const & storage, std::string const & accessKey, std::string const & secretKey);

    void setCloudStorage(std::string const & storage, std::string const & appKey, std::string const & appSecret, std::string const & apiUrl);

    bool isDmJob(std::string const & job);

    void cancelDmJobs(std::vector<std::string> const & jobs);

    bool getUserDnVisible();
};


void destroy_profiled_db(void *db);
