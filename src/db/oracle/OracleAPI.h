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

#include <soci/soci.h>
#include "db/generic/GenericDbIfce.h"
#include "db/generic/StoragePairState.h"
#include "db/generic/SanityFlags.h"
#include "msg-bus/consumer.h"
#include "msg-bus/producer.h"


class OracleAPI : public GenericDbIfce
{
public:
    OracleAPI();
    virtual ~OracleAPI();

    class CleanUpSanityChecks
    {
    public:
        CleanUpSanityChecks(OracleAPI* instanceLocal, soci::session& sql, struct SanityFlags &msg):instanceLocal(instanceLocal), sql(sql), msg(msg), returnValue(false)
        {
            returnValue = instanceLocal->assignSanityRuns(sql, msg);
        }

        ~CleanUpSanityChecks()
        {
            instanceLocal->resetSanityRuns(sql, msg);
        }

        bool getCleanUpSanityCheck()
        {
            return returnValue;
        }

        OracleAPI* instanceLocal;
        soci::session& sql;
        struct SanityFlags &msg;
        bool returnValue;
    };


    struct HashSegment
    {
        unsigned start;
        unsigned end;

        HashSegment(): start(0), end(0xFFFF) {}
    } hashSegment;

    std::vector<StoragePairState> filesMemStore;

    virtual void init(const std::string& username, const std::string& password,
            const std::string& connectString, int nPooledConnections);

    virtual void submitDelete(const std::string& jobId,
            const std::map<std::string, std::string>& urlsHost,
            const std::string& DN, const std::string& voName,
            const std::string& credID);

    virtual void submitPhysical(const std::string& jobId,
            std::list<SubmittedTransfer>& transfers, const std::string& DN,
            const std::string& cred, const std::string& voName,
            const std::string& myProxyServer, const std::string& delegationID,
            const std::string& sourceSe, const std::string& destinationSe,
            const fts3::common::JobParameterHandler& params);

    virtual boost::optional<Job> getJob(const std::string& jobId, bool archive);

    virtual void getTransferStatuses(const std::string& requestID,
            bool archive, unsigned offset, unsigned limit,
            std::vector<FileTransferStatus>& files);

    virtual void getDmStatuses(const std::string& requestID, bool archive,
            unsigned offset, unsigned limit,
            std::vector<FileTransferStatus>& files);

    virtual void listJobs(const std::vector<std::string>& inGivenStates,
            const std::string& forDN, const std::string& voName,
            const std::string& src, const std::string& dst,
            std::vector<JobStatus>& jobs);

    virtual void listDmJobs(const std::vector<std::string>& inGivenStates,
            const std::string& forDN, const std::string& voName,
            const std::string& src, const std::string& dst,
            std::vector<JobStatus>& jobs);

    virtual void getReadySessionReuseTransfers(
            const std::vector<QueueId>& queues,
            std::map<std::string, std::queue<std::pair<std::string, std::list<TransferFile>>>>& files);

    virtual void getReadyTransfers(
            const std::vector<QueueId>& queues,
            std::map<std::string, std::list<TransferFile>>& files);


    virtual void getMultihopJobs(std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFile> > > >& files);

    virtual boost::optional<StorageElement> getStorageElement(const std::string& seName);

    virtual void addStorageElement(const std::string& seName, const std::string& state);

    virtual void updateStorageElement(const std::string& seName, const std::string& state);

    virtual bool updateTransferStatus(const std::string& jobId, int fileId, double throughput,
            const std::string& transferState, const std::string& errorReason,
            int processId, double filesize, double duration, bool retry);

    virtual bool updateJobStatus(const std::string& jobId, const std::string& jobState, int pid);

    virtual void cancelJob(const std::vector<std::string>& requestIDs);

    virtual void cancelAllJobs(const std::string& voName, std::vector<std::string>& canceledJobs);

    virtual bool insertCredentialCache(const std::string& delegationId, const std::string& userDn,
            const std::string& certRequest, const std::string& privateKey, const std::string& vomsAttrs);

    virtual boost::optional<UserCredentialCache> findCredentialCache(const std::string& delegationId, const std::string& userDn);

    virtual void deleteCredentialCache(const std::string& delegationId, const std::string& userDn);

    virtual void insertCredential(const std::string& delegationId, const std::string& userDn,
            const std::string& proxy, const std::string& vomsAttrs, time_t terminationTime);

    virtual void updateCredential(const std::string& delegationId, const std::string& userDn,
            const std::string& proxy, const std::string& vomsAttrs, time_t terminationTime);

    virtual boost::optional<UserCredential> findCredential(const std::string& delegationId, const std::string& userDn);

    virtual void deleteCredential(const std::string& delegationId, const std::string& userDn);

    virtual unsigned getDebugLevel(const std::string& sourceStorage, const std::string& destStorage);

    virtual void setDebugLevel(const std::string& sourceStorage, const std::string& destStorage, unsigned level);

    virtual void auditConfiguration(const std::string & dn, const std::string & config, const std::string & action);

    virtual OptimizerSample fetchOptimizationConfig(const std::string & source_hostname, const std::string & destin_hostname);

    virtual bool isCredentialExpired(const std::string & dlg_id, const std::string & dn);

    virtual bool updateOptimizer();

    virtual bool isTrAllowed(const std::string & source_se, const std::string & dest, int &currentActive);

    virtual int getSeOut(const std::string & source, const std::set<std::string> & destination);

    virtual int getSeIn(const std::set<std::string> & source, const std::string & destination);

    virtual int getCredits(const std::string & source_hostname, const std::string & destination_hostname);

    virtual bool terminateReuseProcess(const std::string & jobId, int pid, const std::string & message);

    virtual void forceFailTransfers(std::map<int, std::string>& collectJobs);

    virtual void setPidForJob(const std::string& jobId, int pid);

    virtual void revertToSubmitted();

    virtual void backup(long bulkSize, long* nJobs, long* nFiles);

    virtual void forkFailed(const std::string& jobId);

    virtual bool markAsStalled(const std::vector<fts3::events::MessageUpdater>& messages, bool diskFull);

    virtual void blacklistSe(const std::string& storage, const std::string& voName,
            const std::string& status, int timeout, const std::string& msg, const std::string& adminDn);

    virtual void blacklistDn(const std::string& userDn, const std::string& msg, const std::string& adminDn);

    virtual void unblacklistSe(const std::string& storage);

    virtual void unblacklistDn(const std::string& userDn);

    virtual bool allowSubmit(const std::string& storage, const std::string& voName);

    virtual boost::optional<int> getTimeoutForSe(const std::string& storage);

    virtual bool isDnBlacklisted(const std::string& userDn);

    //t_group_members
    virtual  bool checkGroupExists(const std::string & groupName);

    virtual void getGroupMembers(const std::string & groupName,
            std::vector<std::string>& groupMembers);

    virtual void addMemberToGroup(const std::string & groupName,
            const std::vector<std::string>& groupMembers);

    virtual void deleteMembersFromGroup(const std::string & groupName,
            const std::vector<std::string>& groupMembers);

    virtual std::string getGroupForSe(const std::string se);

    //t_config_symbolic
    virtual void addLinkConfig(const LinkConfig& cfg);
    virtual void updateLinkConfig(const LinkConfig& cfg);
    virtual void deleteLinkConfig(std::string source, std::string destination);
    virtual std::unique_ptr<LinkConfig> getLinkConfig(std::string source, std::string destination);
    virtual std::unique_ptr<std::pair<std::string, std::string>> getSourceAndDestination(std::string symbolic_name);
    virtual bool isGrInPair(std::string group);
    virtual bool isShareOnly(std::string se);

    virtual void addShareConfig(const ShareConfig& cfg);
    virtual void updateShareConfig(const ShareConfig& cfg);
    virtual void deleteShareConfig(std::string source, std::string destination, std::string vo);
    virtual void deleteShareConfig(std::string source, std::string destination);
    virtual std::unique_ptr<ShareConfig> getShareConfig(std::string source, std::string destination, std::string vo);
    virtual std::vector<ShareConfig> getShareConfig(std::string source, std::string destination);

    virtual void addActivityConfig(std::string vo, std::string shares, bool active);
    virtual void updateActivityConfig(std::string vo, std::string shares, bool active);
    virtual void deleteActivityConfig(std::string vo);
    virtual bool isActivityConfigActive(std::string vo);
    virtual std::map< std::string, double > getActivityConfig(std::string vo);

    virtual bool checkIfSeIsMemberOfAnotherGroup( const std::string & member);

    virtual void addFileShareConfig(int file_id, std::string source, std::string destination, std::string vo);

    virtual void getFilesForNewSeCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out);

    virtual void getFilesForNewGrCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out);

    virtual void delFileShareConfig(int file_id, std::string source, std::string destination, std::string vo);

    virtual void delFileShareConfig(std::string groupd, std::string se);

    virtual bool hasStandAloneSeCfgAssigned(int file_id, std::string vo);

    virtual bool hasPairSeCfgAssigned(int file_id, std::string vo);

    virtual bool hasPairGrCfgAssigned(int file_id, std::string vo);

    virtual int countActiveTransfers(std::string source, std::string destination, std::string vo);

    virtual int countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo);

    virtual int countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo);

    virtual int sumUpVoShares (std::string source, std::string destination, std::set<std::string> vos);

    virtual void setPriority(std::string jobId, int priority);

    virtual void setSeProtocol(std::string protocol, std::string se, std::string state);

    virtual void setRetry(int retry, const std::string & vo);

    virtual int getRetry(const std::string & jobId);

    virtual int getRetryTimes(const std::string & jobId, int fileId);

    virtual int getMaxTimeInQueue();

    virtual void setMaxTimeInQueue(int afterXHours);

    virtual void setGlobalLimits(const int* maxActivePerLink, const int* maxActivePerSe);

    virtual void authorize(bool add, const std::string& op, const std::string& dn);

    virtual void setToFailOldQueuedJobs(std::vector<std::string>& jobs);

    virtual std::vector< std::pair<std::string, std::string> > getPairsForSe(std::string se);

    virtual std::vector<std::string> getAllStandAlloneCfgs();

    virtual std::vector<std::string> getAllShareOnlyCfgs();

    virtual std::vector< std::pair<std::string, std::string> > getAllPairCfgs();

    virtual void setMaxStageOp(const std::string& se, const std::string& vo, int val, const std::string & opt);

    virtual void updateProtocol(std::vector<fts3::events::Message>& tempProtocol);

    virtual void cancelFilesInTheQueue(const std::string& se, const std::string& vo, std::set<std::string>& jobs);

    virtual void cancelJobsInTheQueue(const std::string& dn, std::vector<std::string>& jobs);

    virtual std::vector<TransferState> getStateOfTransfer(const std::string& jobId, int file_id);

    virtual void getFilesForJob(const std::string& jobId, std::vector<int>& files);

    virtual void getFilesForJobInCancelState(const std::string& jobId, std::vector<int>& files);

    virtual void setFilesToWaiting(const std::string& se, const std::string& vo, int timeout);

    virtual void setFilesToWaiting(const std::string& dn, int timeout);

    virtual void cancelWaitingFiles(std::set<std::string>& jobs);

    virtual void revertNotUsedFiles();

    virtual void checkSanityState();

    virtual void checkSchemaLoaded();

    virtual void storeProfiling(const fts3::ProfilingSubsystem* prof);

    virtual void setOptimizerMode(int mode);

    virtual void setRetryTransfer(const std::string & jobId, int fileId, int retry, const std::string& reason);

    virtual void getTransferRetries(int fileId, std::vector<FileRetry>& retries);

    bool assignSanityRuns(soci::session& sql, struct SanityFlags &msg);

    void resetSanityRuns(soci::session& sql, struct SanityFlags &msg);

    void updateHeartBeat(unsigned* index, unsigned* count, unsigned* start, unsigned* end, std::string service_name);

    std::map<std::string, double> getActivityShareConf(soci::session& sql, std::string vo);

    virtual std::vector<std::string> getAllActivityShareConf();

    std::map<std::string, long long> getActivitiesInQueue(soci::session& sql, std::string src, std::string dst, std::string vo);

    std::map<std::string, int> getFilesNumPerActivity(soci::session& sql, std::string src, std::string dst, std::string vo, int filesNum, std::set<std::string> & default_activities);

    virtual void updateFileTransferProgressVector(std::vector<fts3::events::MessageUpdater>& messages);

    virtual void transferLogFileVector(std::map<int, fts3::events::MessageLog>& messagesLog);

    unsigned int updateFileStatusReuse(TransferFile const & file, const std::string status);

    void getCancelJob(std::vector<int>& requestIDs);

    virtual void snapshot(const std::string & vo_name, const std::string & source_se_p, const std::string & dest_se_p, const std::string & endpoint, std::stringstream & result);

    virtual bool getDrain();

    virtual void setDrain(bool drain);

    virtual void setShowUserDn(bool show);

    virtual void setBandwidthLimit(const std::string & source_hostname, const std::string & destination_hostname, int bandwidthLimit);

    virtual std::string getBandwidthLimit();

    virtual bool isProtocolUDT(const std::string & source_hostname, const std::string & destination_hostname);

    virtual bool isProtocolIPv6(const std::string & source_hostname, const std::string & destination_hostname);

    virtual int getStreamsOptimization(const std::string & source_hostname, const std::string & destination_hostname);

    virtual int getGlobalTimeout();

    virtual void setGlobalTimeout(int timeout);

    virtual int getSecPerMb();

    virtual void setSecPerMb(int seconds);

    virtual void setSourceMaxActive(const std::string & source_hostname, int maxActive);

    virtual void setDestMaxActive(const std::string & destination_hostname, int maxActive);

    virtual void setFixActive(const std::string & source, const std::string & destination, int active);

    virtual int getBufferOptimization();

    virtual void getQueuesWithPending(std::vector<QueueId>& queues);

    virtual void getQueuesWithSessionReusePending(std::vector<QueueId>& queues);

    virtual void updateDeletionsState(const std::vector<MinFileStatus>& delOpsStatus);

    virtual void getFilesForDeletion(std::vector<DeleteOperation>& delOps);

    virtual void requeueStartedDeletes();

    virtual void updateStagingState(const std::vector<MinFileStatus>& stagingOpsStatus);

    virtual void updateBringOnlineToken(std::map< std::string, std::map<std::string, std::vector<int> > > const & jobs, std::string const & token);

    virtual void getFilesForStaging(std::vector<StagingOperation> &stagingOps);
    virtual void getAlreadyStartedStaging(std::vector<StagingOperation> &stagingOps);

    //file_id / surl / token
    virtual void getStagingFilesForCanceling(std::set< std::pair<std::string, std::string> >& files);

    virtual bool getUserDnVisible();


private:
    size_t                poolSize;
    soci::connection_pool* connectionPool;
    std::string           hostname;
    std::map<std::string, int> queuedStagingFiles;
    Producer           producer;
    Consumer           consumer;

    void updateHeartBeatInternal(soci::session& sql, unsigned* index, unsigned* count, unsigned* start, unsigned* end, std::string service_name);

    bool resetForRetryStaging(soci::session& sql, int file_id, const std::string & job_id, bool retry, int& times);

    bool resetForRetryDelete(soci::session& sql, int file_id, const std::string & job_id, bool retry);

    void updateDeletionsStateInternal(soci::session& sql, const std::vector<MinFileStatus>& delOpsStatus);

    void updateStagingStateInternal(soci::session& sql, const std::vector<MinFileStatus>& stagingOpsStatus);

    bool getDrainInternal(soci::session& sql);

    std::string getBandwidthLimitInternal(soci::session& sql, const std::string & source_hostname, const std::string & destination_hostname);

    bool bandwidthChecker(soci::session& sql, const std::string & source_hostname, const std::string & destination_hostname, int& bandwidth);

    int getOptimizerMode(soci::session& sql);

    int getOptimizerDefaultMode(soci::session& sql);

    void countFileInTerminalStates(soci::session& sql, std::string jobId,
                                   unsigned int& finished, unsigned int& cancelled, unsigned int& failed);


    bool getChangedFile (std::string source, std::string dest, double rate, double& rateStored, double thr, double& thrStored, double retry, double& retryStored, int active, int& activeStored, int& throughputSamples, int& throughputSamplesStored);

    bool updateFileTransferStatusInternal(soci::session& sql, double throughput, std::string job_id, int file_id, std::string transfer_status,
                                          std::string transfer_message, int process_id, double filesize, double duration, bool retry);

    bool updateJobTransferStatusInternal(soci::session& sql, std::string job_id, const std::string status, int pid);

    void useFileReplica(soci::session& sql, std::string jobId, int fileId);

    void updateOptimizerEvolution(soci::session& sql,
            const std::string & source_hostname,
            const std::string & destination_hostname, int active,
            double throughput, double successRate, int pathFollowed, int bandwidth);

    void getMaxActive(soci::session& sql, int& source, int& destination, const std::string & source_hostname, const std::string & destination_hostname);

    std::vector<TransferState> getStateOfTransferInternal(soci::session& sql, const std::string& jobId, int fileId);

    int getBestNextReplica(soci::session& sql, const std::string & job_id, const std::string & vo_name);

    std::vector<TransferState> getStateOfDeleteInternal(soci::session& sql, const std::string& jobId, int fileId);

    bool getCloudStorageCredentials(const std::string& user_dn, const std::string& vo,
                             const std::string& cloud_name, CloudStorageAuth& auth);

    void setCloudStorageCredentials(std::string const &dn, std::string const &vo, std::string const &storage,
        std::string const &accessKey, std::string const &secretKey);

    void setCloudStorage(std::string const & storage, std::string const & appKey, std::string const & appSecret, std::string const & apiUrl);

    bool isDmJob(std::string const & job);

    void cancelDmJobs(std::vector<std::string> const & jobs);

    bool getUserDnVisibleInternal(soci::session& sql);
};
