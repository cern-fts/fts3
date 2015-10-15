/********************************************//**
 * Copyright @ Members of the EMI Collaboration, 2010.
 * See www.eu-emi.eu for details on the copyright holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***********************************************/

/**
 * @file GenericDbIfce.h
 * @brief generic database interface
 * @author Michail Salichos
 * @date 09/02/2012
 *
 **/

#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <queue>

#include "JobStatus.h"
#include "JobParameterHandler.h"
#include "FileTransferStatus.h"
#include "SePair.h"
#include "TransferJobSummary.h"
#include "Se.h"
#include "SeConfig.h"
#include "SeGroup.h"
#include "SeAndConfig.h"
#include "TransferJobs.h"
#include "TransferFiles.h"
#include "SeProtocolConfig.h"
#include "CredCache.h"
#include "Cred.h"
#include "definitions.h"
#include "OptimizerSample.h"

#include "LinkConfig.h"
#include "ShareConfig.h"

#include "OAuth.h"

#include <utility>

#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>

#include "profiler/Profiler.h"

/**
 * GenericDbIfce class declaration
 **/


/**
 * Map source/destination with the checksum provided
 **/
struct job_element_tupple
{
    job_element_tupple(): filesize(0), fileIndex(0), hashedId(0) {}
    std::string source;
    std::string destination;
    std::string source_se;
    std::string dest_se;
    std::string checksum;
    double filesize;
    std::string metadata;
    std::string selectionStrategy;
    int fileIndex;
    boost::optional<int> wait_timeout;
    std::string activity;
    std::string state;
    unsigned hashedId;
};

class GenericDbIfce
{
public:

    virtual ~GenericDbIfce() {};

    /**
     * Intialize database connection  by providing information from fts3config file
     **/

    virtual void init(std::string username, std::string password, std::string connectString, int pooledConn) = 0;

    virtual  void submitdelete(const std::string & jobId, const std::map<std::string,std::string>& rulsHost,
                               const std::string & DN, const std::string & voName, const std::string & credID) = 0;

    /**
     * Submit a transfer request to be stored in the database
     **/
    virtual void submitPhysical(const std::string & jobId, std::list<job_element_tupple>& src_dest_pair,
                                const std::string & DN, const std::string & cred,
                                const std::string & voName, const std::string & myProxyServer, const std::string & delegationID,
                                const std::string & sourceSe, const std::string & destinationSe,
                                const JobParameterHandler & params) = 0;

    virtual void getTransferJobStatus(std::string requestID, bool archive, std::vector<JobStatus*>& jobs) = 0;

    virtual void getDmJobStatus(std::string requestID, bool archive, std::vector<JobStatus*>& jobs) = 0;

    // If limit == 0, then all results
    virtual void getTransferFileStatus(std::string requestID, bool archive,
                                       unsigned offset, unsigned limit, std::vector<FileTransferStatus*>& files) = 0;

    // If limit == 0, then all results
    virtual void getDmFileStatus(std::string requestID, bool archive,
                                 unsigned offset, unsigned limit, std::vector<FileTransferStatus*>& files) = 0;

    virtual void listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates,
                              std::string restrictToClientDN, std::string forDN, std::string VOname, std::string src, std::string dst) = 0;

    virtual void listRequestsDm(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates,
                                std::string restrictToClientDN, std::string forDN, std::string VOname, std::string src, std::string dst) = 0;

    virtual TransferJobs* getTransferJob(std::string jobId, bool archive) = 0;

    virtual void getByJobIdReuse(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct, std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFiles> > > >& files) = 0;

    virtual void getByJobId(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct, std::map< std::string, std::list<TransferFiles> >& files) = 0;

    virtual void getMultihopJobs(std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFiles> > > >& files) = 0;

    virtual void getSe(Se* &se, std::string seName) = 0;

    virtual unsigned int updateFileStatus(TransferFiles& file, const std::string status) = 0;

    virtual void addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
                       std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID) = 0;

    virtual void updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
                          std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID) = 0;

    virtual bool updateFileTransferStatus(double throughput, std::string job_id, int file_id, std::string transfer_status, std::string transfer_message,
                                          int process_id, double filesize, double duration, bool retry) = 0;

    virtual bool updateJobTransferStatus(std::string job_id, const std::string status, int pid) = 0;

    virtual void cancelJob(std::vector<std::string>& requestIDs) = 0;

    // The canceled job ids will be put in canceledJobs
    virtual void cancelAllJobs(const std::string& voName, std::vector<std::string>& canceledJobs) = 0;

    /*t_credential API*/
    virtual bool insertCredentialCache(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs) = 0;

    virtual CredCache* findCredentialCache(std::string delegationID, std::string dn) = 0;

    virtual void deleteCredentialCache(std::string delegationID, std::string dn) = 0;

    virtual void insertCredential(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time) = 0;

    virtual void updateCredential(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time) = 0;

    virtual  Cred* findCredential(std::string delegationID, std::string dn) = 0;

    virtual void deleteCredential(std::string delegationID, std::string dn) = 0;

    virtual unsigned getDebugLevel(std::string source_hostname, std::string destin_hostname) = 0;

    virtual void setDebugLevel(std::string source_hostname, std::string destin_hostname, unsigned level) = 0;

    virtual void auditConfiguration(const std::string & dn, const std::string & config, const std::string & action) = 0;

    virtual void fetchOptimizationConfig2(OptimizerSample* ops, const std::string & source_hostname, const std::string & destin_hostname) = 0;

    virtual bool isCredentialExpired(const std::string & dlg_id, const std::string & dn) = 0;

    virtual bool updateOptimizer() = 0;

    virtual bool isTrAllowed(const std::string & source_se, const std::string & dest, int &currentActive) = 0;

    virtual int getSeOut(const std::string & source, const std::set<std::string> & destination) = 0;

    virtual int getSeIn(const std::set<std::string> & source, const std::string & destination) = 0;

    virtual bool terminateReuseProcess(const std::string & jobId, int pid, const std::string & message) = 0;

    virtual void forceFailTransfers(std::map<int, std::string>& collectJobs) = 0;

    virtual void setPid(const std::string & jobId, int fileId, int pid) = 0;

    virtual void setPidV(int pid, std::map<int,std::string>& pids) = 0;

    virtual void revertToSubmitted() = 0;

    virtual void backup(long bulkSize, long* nJobs, long* nFiles) = 0;

    virtual void forkFailedRevertState(const std::string & jobId, int fileId) = 0;

    virtual void forkFailedRevertStateV(std::map<int,std::string>& pids) = 0;

    virtual bool retryFromDead(std::vector<struct message_updater>& messages, bool diskFull) = 0;

    virtual void blacklistSe(std::string se, std::string vo, std::string status, int timeout, std::string msg, std::string adm_dn) = 0;

    virtual void blacklistDn(std::string dn, std::string msg, std::string adm_dn) = 0;

    virtual void unblacklistSe(std::string se) = 0;

    virtual void unblacklistDn(std::string dn) = 0;

    virtual void allowSubmit(std::string ses, std::string vo, std::list<std::string>& notAllowed) = 0;

    virtual boost::optional<int> getTimeoutForSe(std::string se) = 0;

    virtual void getTimeoutForSe(std::string ses, std::map<std::string, int>& ret) = 0;

    virtual bool isDnBlacklisted(std::string dn) = 0;

    virtual bool isFileReadyState(int fileID) = 0;

    virtual bool isFileReadyStateV(std::map<int,std::string>& fileIds) = 0;

    //t_group_members
    virtual  bool checkGroupExists(const std::string & groupName) = 0;

    virtual void getGroupMembers(const std::string & groupName, std::vector<std::string>& groupMembers) = 0;

    virtual void addMemberToGroup(const std::string & groupName, std::vector<std::string>& groupMembers) = 0;

    virtual void deleteMembersFromGroup(const std::string & groupName, std::vector<std::string>& groupMembers) = 0;

    virtual std::string getGroupForSe(const std::string se) = 0;

    //t_config_symbolic
    virtual void addLinkConfig(LinkConfig* cfg) = 0;
    virtual void updateLinkConfig(LinkConfig* cfg) = 0;
    virtual void deleteLinkConfig(std::string source, std::string destination) = 0;
    virtual LinkConfig* getLinkConfig(std::string source, std::string destination) = 0;
    virtual std::pair<std::string, std::string>* getSourceAndDestination(std::string symbolic_name) = 0;
    virtual bool isGrInPair(std::string group) = 0;
    virtual bool isShareOnly(std::string se) = 0;

    virtual void addShareConfig(ShareConfig* cfg) = 0;
    virtual void updateShareConfig(ShareConfig* cfg) = 0;
    virtual void deleteShareConfig(std::string source, std::string destination, std::string vo) = 0;
    virtual void deleteShareConfig(std::string source, std::string destination) = 0;
    virtual ShareConfig* getShareConfig(std::string source, std::string destination, std::string vo) = 0;
    virtual std::vector<ShareConfig*> getShareConfig(std::string source, std::string destination) = 0;

    virtual void addActivityConfig(std::string vo, std::string shares, bool active) = 0;
    virtual void updateActivityConfig(std::string vo, std::string shares, bool active) = 0;
    virtual void deleteActivityConfig(std::string vo) = 0;
    virtual bool isActivityConfigActive(std::string vo) = 0;
    virtual std::map< std::string, double > getActivityConfig(std::string vo) = 0;

    virtual bool checkIfSeIsMemberOfAnotherGroup( const std::string & member) = 0;

    virtual void addFileShareConfig(int file_id, std::string source, std::string destination, std::string vo) = 0;

    virtual void getFilesForNewSeCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out) = 0;

    virtual void getFilesForNewGrCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out) = 0;

    virtual void delFileShareConfig(int file_id, std::string source, std::string destination, std::string vo) = 0;

    virtual void delFileShareConfig(std::string groupd, std::string se) = 0;

    virtual bool hasStandAloneSeCfgAssigned(int file_id, std::string vo) = 0;

    virtual bool hasPairSeCfgAssigned(int file_id, std::string vo) = 0;

    virtual bool hasPairGrCfgAssigned(int file_id, std::string vo) = 0;

    virtual int countActiveTransfers(std::string source, std::string destination, std::string vo) = 0;

    virtual int countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo) = 0;

    virtual int countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo) = 0;

    virtual int sumUpVoShares (std::string source, std::string destination, std::set<std::string> vos) = 0;

    virtual void setPriority(std::string jobId, int priority) = 0;

    virtual void setSeProtocol(std::string protocol, std::string se, std::string state) = 0;

    virtual void setRetry(int retry, const std::string & vo) = 0;

    virtual int getRetry(const std::string & jobId) = 0;

    virtual int getRetryTimes(const std::string & jobId, int fileId) = 0;

    virtual void setMaxTimeInQueue(int afterXHours) = 0;

    // If NULL, it will not be modified
    virtual void setGlobalLimits(const int* maxActivePerLink, const int* maxActivePerSe) = 0;

    // If add is false, the entry will be removed
    virtual void authorize(bool add, const std::string& op, const std::string& dn) = 0;

    virtual void setToFailOldQueuedJobs(std::vector<std::string>& jobs) = 0;

    virtual std::vector< std::pair<std::string, std::string> > getPairsForSe(std::string se) = 0;

    virtual std::vector<std::string> getAllActivityShareConf() = 0;

    virtual std::vector<std::string> getAllStandAlloneCfgs() = 0;

    virtual std::vector<std::string> getAllShareOnlyCfgs() = 0;

    virtual std::vector< std::pair<std::string, std::string> > getAllPairCfgs() = 0;

    virtual void setMaxStageOp(const std::string& se, const std::string& vo, int val, const std::string & opt) = 0;

    virtual void updateProtocol(std::vector<struct message>& tempProtocol) = 0;

    virtual void cancelFilesInTheQueue(const std::string& se, const std::string& vo, std::set<std::string>& jobs) = 0;

    virtual void cancelJobsInTheQueue(const std::string& dn, std::vector<std::string>& jobs) = 0;

    virtual std::vector<struct message_state> getStateOfTransfer(const std::string& jobId, int file_id) = 0;

    virtual void getFilesForJob(const std::string& jobId, std::vector<int>& files) = 0;

    virtual void getFilesForJobInCancelState(const std::string& jobId, std::vector<int>& files) = 0;

    virtual void setFilesToWaiting(const std::string& se, const std::string& vo, int timeout) = 0;

    virtual void setFilesToWaiting(const std::string& dn, int timeout) = 0;

    virtual void cancelWaitingFiles(std::set<std::string>& jobs) = 0;

    virtual void revertNotUsedFiles() = 0;

    virtual void checkSanityState() = 0;

    virtual void checkSchemaLoaded() = 0;

    virtual void storeProfiling(const fts3::ProfilingSubsystem* prof) = 0;

    virtual void setOptimizerMode(int mode) = 0;

    virtual void setRetryTransfer(const std::string & jobId, int fileId, int retry, const std::string& reason) = 0;

    virtual void getTransferRetries(int fileId, std::vector<FileRetry*>& retries) = 0;

    virtual void updateFileTransferProgressVector(std::vector<struct message_updater>& messages) = 0;

    virtual void transferLogFileVector(std::map<int, struct message_log>& messagesLog) = 0;

    /**
     * Signals that the server is alive
     * The total number of running (alive) servers is put in count
     * The index of this specific machine is put in index
     * A default implementation is provided, as this is used for optimization,
     * so it is not mandatory.
     * start and end are set to the interval of hash values this host will process
     */
    virtual void updateHeartBeat(unsigned* index, unsigned* count, unsigned* start, unsigned* end, std::string service_name)
    {
        *index = 0;
        *count = 1;
        *start = 0x0000;
        *end   = 0xFFFF;
        service_name = std::string("");
    }

    virtual unsigned int updateFileStatusReuse(TransferFiles const & file, const std::string status) = 0;

    virtual void getCancelJob(std::vector<int>& requestIDs) = 0;

    virtual void snapshot(const std::string & vo_name, const std::string & source_se, const std::string & dest_se, const std::string & endpoint, std::stringstream & result) = 0;

    virtual bool getDrain() = 0;

    virtual void setDrain(bool drain) = 0;

    virtual void setShowUserDn(bool show) = 0;

    virtual void setBandwidthLimit(const std::string & source_hostname, const std::string & destination_hostname, int bandwidthLimit) = 0;

    virtual std::string getBandwidthLimit() = 0;

    virtual bool isProtocolUDT(const std::string & source_hostname, const std::string & destination_hostname) = 0;

    virtual bool isProtocolIPv6(const std::string & source_hostname, const std::string & destination_hostname) = 0;

    virtual int getStreamsOptimization(const std::string & source_hostname, const std::string & destination_hostname) = 0;

    virtual int getGlobalTimeout() = 0;

    virtual void setGlobalTimeout(int timeout) = 0;

    virtual int getSecPerMb() = 0;

    virtual void setSecPerMb(int seconds) = 0;

    virtual void setSourceMaxActive(const std::string & source_hostname, int maxActive) = 0;

    virtual void setDestMaxActive(const std::string & destination_hostname, int maxActive) = 0;

    virtual void setFixActive(const std::string & source, const std::string & destination, int active) = 0;

    virtual int getBufferOptimization() = 0;

    virtual void getTransferJobStatusDetailed(std::string job_id, std::vector<boost::tuple<std::string, std::string, int, std::string, std::string> >& files) = 0;

    virtual void getVOPairs(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct) = 0;

    virtual void getVOPairsWithReuse(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct) = 0;


    //NEW deletions and staging API
    //deletions						 //file_id / state / reason
    virtual void updateDeletionsState(std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files) = 0;

    //vo_name, source_url, job_id, file_id, user_dn, cred_id
    virtual void getFilesForDeletion(std::vector< boost::tuple<std::string, std::string, std::string, int, std::string, std::string> >& files) = 0;

    virtual void revertDeletionToStarted() = 0;

    //staging						//file_id / state / reason / token
    virtual void updateStagingState(std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files) = 0;

    //															//	job_id / file_id / token
    virtual void updateBringOnlineToken(std::map< std::string, std::map<std::string, std::vector<int> > > const & jobs, std::string const & token) = 0;

    //vo / file_id / surl / proxy / pinlifetime / bringonlineTimeout / spacetoken / job_id
    virtual void getFilesForStaging(std::vector< boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string> >& files) = 0;

    virtual void getAlreadyStartedStaging(std::vector< boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string, std::string> >& files) = 0;

    //file_id / surl / token
    virtual void getStagingFilesForCanceling(std::set< std::pair<std::string, std::string> >& files) = 0;

    virtual bool isDmJob(std::string const & job) = 0;

    // Cloud storage API
    virtual bool getOauthCredentials(const std::string& user_dn,
                                     const std::string& vo, const std::string& cloud_name,
                                     OAuth& oauth) = 0;

    virtual void setCloudStorageCredential(std::string const & dn, std::string const & vo, std::string const & storage, std::string const & accessKey, std::string const & secretKey) = 0;

    virtual void setCloudStorage(std::string const & storage, std::string const & appKey, std::string const & appSecret, std::string const & apiUrl) = 0;

    virtual void cancelDmJobs(std::vector<std::string> const & jobs) = 0;

    virtual bool getUserDnVisible() = 0;
};
