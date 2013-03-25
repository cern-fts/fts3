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

#pragma once

#include <common_dev.h>
#include <soci.h>
#include "GenericDbIfce.h"

using namespace FTS3_COMMON_NAMESPACE;

class MySqlAPI : public GenericDbIfce {
public:
    MySqlAPI();
    virtual ~MySqlAPI();

/**
 * Intialize database connection  by providing information from fts3config file
 **/
 
    virtual void init(std::string username, std::string password, std::string connectString, int pooledConn);

/**
 * Submit a transfer request to be stored in the database
 **/ 
    virtual void submitPhysical(const std::string & jobId, std::vector<job_element_tupple> src_dest_pair, const std::string & paramFTP,
                                const std::string & DN, const std::string & cred, const std::string & voName, const std::string & myProxyServer,
                                const std::string & delegationID, const std::string & spaceToken, const std::string & overwrite,
                                const std::string & sourceSpaceToken, const std::string & sourceSpaceTokenDescription, int copyPinLifeTime,
                                const std::string & failNearLine, const std::string & checksumMethod, const std::string & reuse,
                                int bringonline, std::string metadata,
                                int retry, int retryDelay, std::string sourceSe, std::string destinationSe);

    virtual void getTransferJobStatus(std::string requestID, std::vector<JobStatus*>& jobs);

    virtual void getTransferFileStatus(std::string requestID, std::vector<FileTransferStatus*>& files);

    virtual void listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates, std::string restrictToClientDN, std::string forDN, std::string VOname);

    virtual TransferJobs* getTransferJob(std::string jobId);

    virtual void getSubmittedJobs(std::vector<TransferJobs*>& jobs, const std::string & vos);

    virtual void getByJobId(std::vector<TransferJobs*>& jobs, std::map< std::string, std::list<TransferFiles*> >& files);

    virtual void getSe(Se* &se, std::string seName);

    virtual unsigned int updateFileStatus(TransferFiles* file, const std::string status);

    virtual void addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
            std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID);

    virtual void updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
            std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID);

    virtual void deleteSe(std::string NAME);

    virtual bool updateFileTransferStatus(std::string job_id, int file_id, std::string transfer_status, std::string transfer_message, int process_id, double filesize, double duration);

    virtual bool updateJobTransferStatus(int file_id, std::string job_id, const std::string status);

    virtual void updateJObStatus(std::string jobId, const std::string status);
    
    virtual void cancelJob(std::vector<std::string>& requestIDs);
    
    virtual void getCancelJob(std::vector<int>& requestIDs);


    /*t_credential API*/
    virtual void insertGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs);

    virtual void updateGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs);

    virtual CredCache* findGrDPStorageCacheElement(std::string delegationID, std::string dn);

    virtual void deleteGrDPStorageCacheElement(std::string delegationID, std::string dn);

    virtual void insertGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time);

    virtual void updateGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time);

    virtual  Cred* findGrDPStorageElement(std::string delegationID, std::string dn);

    virtual void deleteGrDPStorageElement(std::string delegationID, std::string dn);

    virtual bool getDebugMode(std::string source_hostname, std::string destin_hostname);

    virtual void setDebugMode(std::string source_hostname, std::string destin_hostname, std::string mode);

    virtual void getSubmittedJobsReuse(std::vector<TransferJobs*>& jobs, const std::string & vos);

    virtual void auditConfiguration(const std::string & dn, const std::string & config, const std::string & action);

    virtual void fetchOptimizationConfig2(OptimizerSample* ops, const std::string & source_hostname, const std::string & destin_hostname);
    
    virtual bool updateOptimizer(int file_id , double filesize, double timeInSecs, int nostreams, int timeout, int buffersize,std::string source_hostname, std::string destin_hostname);
    
    virtual void addOptimizer(time_t when, double throughput, const std::string & source_hostname, const std::string & destin_hostname, int file_id, int nostreams, int timeout, int buffersize, int noOfActiveTransfers);
    
    virtual void initOptimizer(const std::string & source_hostname, const std::string & destin_hostname, int file_id);
    
    virtual bool isCredentialExpired(const std::string & dlg_id, const std::string & dn);
    
    virtual bool isTrAllowed(const std::string & source_se, const std::string & dest);
    
    virtual void setAllowed(const std::string & job_id, int file_id, const std::string & source_se, const std::string & dest, int nostreams, int timeout, int buffersize);
    
    virtual void setAllowedNoOptimize(const std::string & job_id, int file_id, const std::string & params);
    
    virtual bool terminateReuseProcess(const std::string & jobId);
    
    virtual void forceFailTransfers();
    
    virtual void setPid(const std::string & jobId, int fileId, int pid);
    
    virtual void setPidV(int pid, std::map<int,std::string>& pids);
    
    virtual void revertToSubmitted();
    
    virtual void revertToSubmittedTerminate();

    virtual void backup();
    
    virtual void forkFailedRevertState(const std::string & jobId, int fileId);
    
    virtual void forkFailedRevertStateV(std::map<int,std::string>& pids);
    
    virtual bool retryFromDead(std::vector<struct message_updater>& messages);
    
    virtual void blacklistSe(std::string se, std::string vo, std::string status, int timeout, std::string msg, std::string adm_dn);

    virtual void blacklistDn(std::string dn, std::string msg, std::string adm_dn);

    virtual void unblacklistSe(std::string se);

    virtual void unblacklistDn(std::string dn);

    virtual bool isSeBlacklisted(std::string se);

    virtual bool isDnBlacklisted(std::string dn);

    virtual bool isFileReadyState(int fileID);

    virtual bool isFileReadyStateV(std::map<int,std::string>& fileIds);

    //t_group_members
    virtual  bool checkGroupExists(const std::string & groupName);

    virtual void getGroupMembers(const std::string & groupName, std::vector<std::string>& groupMembers);

    virtual void addMemberToGroup(const std::string & groupName, std::vector<std::string>& groupMembers);

    virtual void deleteMembersFromGroup(const std::string & groupName, std::vector<std::string>& groupMembers);

    virtual std::string getGroupForSe(const std::string se);


    virtual void submitHost(const std::string & jobId);

    virtual std::string transferHost(int fileId);

    virtual std::string transferHostV(std::map<int,std::string>& fileIds);

    //t_config_symbolic
    virtual void addLinkConfig(LinkConfig* cfg);
    virtual void updateLinkConfig(LinkConfig* cfg);
    virtual void deleteLinkConfig(std::string source, std::string destination);
    virtual LinkConfig* getLinkConfig(std::string source, std::string destination);
    virtual bool isThereLinkConfig(std::string source, std::string destination);
    virtual std::pair<std::string, std::string>* getSourceAndDestination(std::string symbolic_name);
    virtual bool isGrInPair(std::string group);

    virtual void addShareConfig(ShareConfig* cfg);
    virtual void updateShareConfig(ShareConfig* cfg);
    virtual void deleteShareConfig(std::string source, std::string destination, std::string vo);
    virtual void deleteShareConfig(std::string source, std::string destination);
    virtual ShareConfig* getShareConfig(std::string source, std::string destination, std::string vo);
    virtual std::vector<ShareConfig*> getShareConfig(std::string source, std::string destination);

    virtual bool checkIfSeIsMemberOfAnotherGroup( const std::string & member);

    virtual void addJobShareConfig(std::string job_id, std::string source, std::string destination, std::string vo);

    virtual void delJobShareConfig(std::string job_id);

    virtual std::vector< boost::tuple<std::string, std::string, std::string> > getJobShareConfig(std::string job_id);

    virtual unsigned int countJobShareConfig(std::string job_id);

    virtual int countActiveTransfers(std::string source, std::string destination, std::string vo);

    virtual int countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo);

    virtual int countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo);

    virtual boost::optional<unsigned int> getJobConfigCount(std::string job_id);

    virtual void setJobConfigCount(std::string job_id, int count);

    virtual void setPriority(std::string jobId, int priority);

    virtual bool checkConnectionStatus();
    
    virtual void setRetry(int retry);
    
    virtual int getRetry(const std::string & jobId);     
    
    virtual void setRetryTimes(int retry, const std::string & jobId, int fileId);
    
    virtual int getRetryTimes(const std::string & jobId, int fileId);  
    
    virtual void setRetryTransfer(const std::string & jobId, int fileId); 
    
    virtual int getMaxTimeInQueue();
    
    virtual void setMaxTimeInQueue(int afterXHours); 
    
    virtual void setToFailOldQueuedJobs();

    virtual std::vector<std::string> getAllStandAlloneCfgs();

    virtual std::vector< std::pair<std::string, std::string> > getAllPairCfgs();
    
    virtual int activeProcessesForThisHost();    

    virtual void setFilesToNotUsed(std::string jobId, int fileIndex);
    
    virtual std::vector< boost::tuple<std::string, std::string, int> >  getVOBringonlimeMax();
    
    virtual std::vector<struct message_bringonline> getBringOnlineFiles(std::string voName, std::string hostName, int maxValue);
    
    virtual void bringOnlineReportStatus(const std::string & state, const std::string & message, struct message_bringonline msg);
    
    virtual void addToken(const std::string & job_id, int file_id, const std::string & token);
    
    virtual void getCredentials(std::string & vo_name, const std::string & job_id, int file_id, std::string & dn, std::string & dlg_id);

    virtual void setMaxStageOp(const std::string& se, const std::string& vo, int val);

    virtual void useFileReplica(std::string jobId, int fileId);
    
    virtual void setRetryTimestamp(const std::string& jobId, int fileId);    

    virtual int countActiveTransfers(std::string source, std::string destination);

    virtual int getFailureRate(std::string source, std::string destination);

    virtual int getAvgThroughput(std::string source, std::string destination, int activeTransfers);
    
    virtual void updateProtocol(const std::string& jobId, int fileId, int nostreams, int timeout, int buffersize);    

private:
    size_t                poolSize;
    soci::connection_pool connectionPool;
    OptimizerSample       optimizerObject;
    std::string           hostname;

    bool getInOutOfSe(const std::string& sourceSe, const std::string& destSe);
};
