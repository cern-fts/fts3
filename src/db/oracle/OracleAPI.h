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


class OracleAPI : public GenericDbIfce
{
public:
    OracleAPI();
    virtual ~OracleAPI();

    class CleanUpSanityChecks
    {
    public:
        CleanUpSanityChecks(OracleAPI* instanceLocal, soci::session& sql, struct message_sanity &msg):instanceLocal(instanceLocal), sql(sql), msg(msg), returnValue(false)
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
        struct message_sanity &msg;
        bool returnValue;
    };


    /**
     * Intialize database connection  by providing information from fts3config file
     **/

    virtual void init(std::string username, std::string password, std::string connectString, int pooledConn);

    virtual void submitDelete(const std::string & jobId, const std::map<std::string,std::string>& urlsHost,
                               const std::string & DN, const std::string & voName, const std::string & credID);

    /**
     * Submit a transfer request to be stored in the database
     **/
    virtual void submitPhysical(const std::string & jobId, std::list<JobElementTuple>& src_dest_pair,
                                const std::string & DN, const std::string & cred,
                                const std::string & voName, const std::string & myProxyServer, const std::string & delegationID,
                                const std::string & sourceSe, const std::string & destinationSe,
                                const JobParameterHandler & params);

    virtual void getTransferJobStatus(const std::string& requestID, bool archive, std::vector<JobStatus>& jobs);

    virtual void getDmJobStatus(const std::string& requestID, bool archive, std::vector<JobStatus>& jobs);

    virtual void getTransferFileStatus(const std::string& requestID,
            bool archive, unsigned offset, unsigned limit,
            std::vector<FileTransferStatus>& files);

    virtual void getDmFileStatus(const std::string& requestID, bool archive,
            unsigned offset, unsigned limit,
            std::vector<FileTransferStatus>& files);

    virtual void listRequests(const std::vector<std::string>& inGivenStates,
            const std::string& restrictToClientDN, const std::string& forDN,
            const std::string& voName, const std::string& src, const std::string& dst,
            std::vector<JobStatus>& jobs);

    virtual void listRequestsDm(const std::vector<std::string>& inGivenStates,
            const std::string& restrictToClientDN, const std::string& forDN,
            const std::string& voName, const std::string& src, const std::string& dst,
            std::vector<JobStatus>& jobs);

    virtual std::unique_ptr<TransferJob> getTransferJob(const std::string & jobId, bool archive);

    virtual void getByJobIdReuse(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct, std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFile> > > >& files);

    virtual void getByJobId(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct, std::map< std::string, std::list<TransferFile> >& files);

    virtual void getMultihopJobs(std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFile> > > >& files);

    virtual std::unique_ptr<Se> getSe(const std::string& seName);

    virtual unsigned int updateFileStatus(TransferFile& file, const std::string status);

    virtual void addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
                       std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID);

    virtual void updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
                          std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID);

    virtual bool updateFileTransferStatus(double throughput, std::string job_id, int file_id, std::string transfer_status, std::string transfer_message,
                                          int process_id, double filesize, double duration, bool retry);

    virtual bool updateJobTransferStatus(std::string job_id, const std::string status, int pid);

    virtual void cancelJob(std::vector<std::string>& requestIDs);

    virtual void cancelAllJobs(const std::string& voName, std::vector<std::string>& canceledJobs);


    /*t_credential API*/
    virtual bool insertCredentialCache(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs);

    virtual std::unique_ptr<CredCache> findCredentialCache(std::string delegationID, std::string dn);

    virtual void deleteCredentialCache(std::string delegationID, std::string dn);

    virtual void insertCredential(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time);

    virtual void updateCredential(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time);

    virtual std::unique_ptr<Cred> findCredential(std::string delegationID, std::string dn);

    virtual void deleteCredential(std::string delegationID, std::string dn);

    virtual unsigned getDebugLevel(std::string source_hostname, std::string destin_hostname);

    virtual void setDebugLevel(std::string source_hostname, std::string destin_hostname, unsigned level);

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

    virtual void setPid(const std::string & jobId, int fileId, int pid);

    virtual void setPidV(int pid, std::map<int,std::string>& pids);

    virtual void revertToSubmitted();

    virtual void backup(long* nJobs, long* nFiles);

    virtual void forkFailedRevertState(const std::string & jobId, int fileId);

    virtual void forkFailedRevertStateV(std::map<int,std::string>& pids);

    virtual bool retryFromDead(std::vector<struct message_updater>& messages, bool diskFull);

    virtual void blacklistSe(std::string se, std::string vo, std::string status, int timeout, std::string msg, std::string adm_dn);

    virtual void blacklistDn(std::string dn, std::string msg, std::string adm_dn);

    virtual void unblacklistSe(std::string se);

    virtual void unblacklistDn(std::string dn);

    virtual void allowSubmit(std::string ses, std::string vo, std::list<std::string>& notAllowed);

    virtual boost::optional<int> getTimeoutForSe(std::string se);

    virtual void getTimeoutForSe(std::string ses, std::map<std::string, int>& ret);

    virtual bool isDnBlacklisted(std::string dn);

    virtual bool isFileReadyState(int fileID);

    virtual bool isFileReadyStateV(std::map<int,std::string>& fileIds);

    //t_group_members
    virtual  bool checkGroupExists(const std::string & groupName);

    virtual void getGroupMembers(const std::string & groupName, std::vector<std::string>& groupMembers);

    virtual void addMemberToGroup(const std::string & groupName, std::vector<std::string>& groupMembers);

    virtual void deleteMembersFromGroup(const std::string & groupName, std::vector<std::string>& groupMembers);

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

    virtual void updateProtocol(std::vector<struct message>& tempProtocol);

    virtual void cancelFilesInTheQueue(const std::string& se, const std::string& vo, std::set<std::string>& jobs);

    virtual void cancelJobsInTheQueue(const std::string& dn, std::vector<std::string>& jobs);

    virtual std::vector<struct message_state> getStateOfTransfer(const std::string& jobId, int file_id);

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

    bool assignSanityRuns(soci::session& sql, struct message_sanity &msg);

    void resetSanityRuns(soci::session& sql, struct message_sanity &msg);

    void updateHeartBeat(unsigned* index, unsigned* count, unsigned* start, unsigned* end, std::string service_name);

    std::map<std::string, double> getActivityShareConf(soci::session& sql, std::string vo);

    virtual std::vector<std::string> getAllActivityShareConf();

    std::map<std::string, long long> getActivitiesInQueue(soci::session& sql, std::string src, std::string dst, std::string vo);

    std::map<std::string, int> getFilesNumPerActivity(soci::session& sql, std::string src, std::string dst, std::string vo, int filesNum, std::set<std::string> & default_activities);

    virtual void updateFileTransferProgressVector(std::vector<struct message_updater>& messages);

    virtual void transferLogFileVector(std::map<int, struct message_log>& messagesLog);

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

    virtual void getTransferJobStatusDetailed(std::string job_id, std::vector<boost::tuple<std::string, std::string, int, std::string, std::string> >& files);

    virtual void getVOPairs(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct);

    virtual void getVOPairsWithReuse(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct);


    //NEW deletions and staging API
    //deletions						 //file_id / state / reason
    virtual void updateDeletionsState(std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files);

    //file_id / surl / proxy
    virtual void getFilesForDeletion(std::vector< boost::tuple<std::string, std::string, std::string, int, std::string, std::string> >& files);

    virtual void revertDeletionToStarted();

    //staging						//file_id / state / reason / token
    virtual void updateStagingState(std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files);
    virtual void updateBringOnlineToken(std::map< std::string, std::map<std::string, std::vector<int> > > const & jobs, std::string const & token);

    //file_id / surl / proxy / pinlifetime / bringonlineTimeout
    virtual void getFilesForStaging(std::vector< boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string> >& files);

    virtual void getAlreadyStartedStaging(std::vector< boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string, std::string> >& files);

    //file_id / surl / token
    virtual void getStagingFilesForCanceling(std::set< std::pair<std::string, std::string> >& files);

    virtual bool getUserDnVisible();


private:
    size_t                poolSize;
    soci::connection_pool* connectionPool;
    std::string           hostname;
    std::map<std::string, int> queuedStagingFiles;

    void updateHeartBeatInternal(soci::session& sql, unsigned* index, unsigned* count, unsigned* start, unsigned* end, std::string service_name);

    bool resetForRetryStaging(soci::session& sql, int file_id, const std::string & job_id, bool retry, int& times);

    bool resetForRetryDelete(soci::session& sql, int file_id, const std::string & job_id, bool retry);

    void updateDeletionsStateInternal(soci::session& sql, std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files);

    void updateStagingStateInternal(soci::session& sql, std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files);

    bool getDrainInternal(soci::session& sql);

    std::string getBandwidthLimitInternal(soci::session& sql, const std::string & source_hostname, const std::string & destination_hostname);

    bool bandwidthChecker(soci::session& sql, const std::string & source_hostname, const std::string & destination_hostname, int& bandwidth);

    int getOptimizerMode(soci::session& sql);

    int getOptimizerDefaultMode(soci::session& sql);

    void countFileInTerminalStates(soci::session& sql, std::string jobId,
                                   unsigned int& finished, unsigned int& cancelled, unsigned int& failed);


    bool getChangedFile (std::string source, std::string dest, double rate, double& rateStored, double thr, double& thrStored, double retry, double& retryStored, int active, int& activeStored, int& throughputSamples, int& throughputSamplesStored);

    struct HashSegment
    {
        unsigned start;
        unsigned end;

        HashSegment(): start(0), end(0xFFFF) {}
    } hashSegment;

    std::vector< boost::tuple<std::string, std::string, double, double, double, int, int, int> > filesMemStore;

    bool updateFileTransferStatusInternal(soci::session& sql, double throughput, std::string job_id, int file_id, std::string transfer_status,
                                          std::string transfer_message, int process_id, double filesize, double duration, bool retry);

    bool updateJobTransferStatusInternal(soci::session& sql, std::string job_id, const std::string status, int pid);

    void useFileReplica(soci::session& sql, std::string jobId, int fileId);

    void updateOptimizerEvolution(soci::session& sql,
            const std::string & source_hostname,
            const std::string & destination_hostname, int active,
            double throughput, double successRate, int pathFollowed, int bandwidth);

    void getMaxActive(soci::session& sql, int& source, int& destination, const std::string & source_hostname, const std::string & destination_hostname);

    std::vector<struct message_state> getStateOfTransferInternal(soci::session& sql, const std::string& jobId, int fileId);

    void bringOnlineReportStatusInternal(soci::session& sql, const std::string & state, const std::string & message,
                                         const struct message_bringonline& msg);

    int getBestNextReplica(soci::session& sql, const std::string & job_id, const std::string & vo_name);

    std::vector<struct message_state> getStateOfDeleteInternal(soci::session& sql, const std::string& jobId, int fileId);

    bool getOauthCredentials(const std::string& user_dn, const std::string& vo,
                             const std::string& cloud_name, OAuth& oauth);

    void setCloudStorageCredential(std::string const & dn, std::string const & vo, std::string const & storage, std::string const & accessKey, std::string const & secretKey);

    void setCloudStorage(std::string const & storage, std::string const & appKey, std::string const & appSecret, std::string const & apiUrl);

    bool isDmJob(std::string const & job);

    void cancelDmJobs(std::vector<std::string> const & jobs);

    bool getUserDnVisibleInternal(soci::session& sql);
};
