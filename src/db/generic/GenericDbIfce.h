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
#include "JobStatus.h"
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

#include <utility>

#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>

/**
 * GenericDbIfce class declaration
 **/


/**
 * Map source/destination with the checksum provided
 **/
struct src_dest_checksum_tupple{
    std::string source;
    std::string destination;
    std::string checksum;
};
 
class GenericDbIfce {
public:

    virtual ~GenericDbIfce() {};

/**
 * Intialize database connection  by providing information from fts3config file
 **/
 
    virtual void init(std::string username, std::string password, std::string connectString) = 0;

/**
 * Submit a transfer request to be stored in the database
 **/ 
    virtual void submitPhysical(const std::string & jobId, std::vector<src_dest_checksum_tupple> src_dest_pair, const std::string & paramFTP,
                                 const std::string & DN, const std::string & cred, const std::string & voName, const std::string & myProxyServer,
                                 const std::string & delegationID, const std::string & spaceToken, const std::string & overwrite, 
                                 const std::string & sourceSpaceToken, const std::string & sourceSpaceTokenDescription, const std::string & lanConnection, int copyPinLifeTime,
                                 const std::string & failNearLine, const std::string & checksumMethod, const std::string & reuse,
				 const std::string & sourceSE, const std::string & destSe) = 0;

    virtual void getTransferJobStatus(std::string requestID, std::vector<JobStatus*>& jobs) = 0;
    
    virtual void getTransferFileStatus(std::string requestID, std::vector<FileTransferStatus*>& files) = 0;    

    virtual void listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates, std::string restrictToClientDN, std::string forDN, std::string VOname) = 0;
 
    virtual TransferJobs* getTransferJob(std::string jobId) = 0;

    virtual void getSubmittedJobs(std::vector<TransferJobs*>& jobs, const std::string & vos) = 0;
    
    virtual void getByJobId(std::vector<TransferJobs*>& jobs, std::vector<TransferFiles*>& files) = 0;
   
    virtual void getSe(Se* &se, std::string seName) = 0;
   
    virtual unsigned int updateFileStatus(TransferFiles* file, const std::string status) = 0;

    virtual void addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
            std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID) = 0;

    virtual void updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
            std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID) = 0;

    virtual void deleteSe(std::string NAME) = 0;
       
    virtual bool updateFileTransferStatus(std::string job_id, std::string file_id, std::string transfer_status, std::string transfer_message, int process_id, double filesize, double duration) = 0;    
    
    virtual bool updateJobTransferStatus(std::string file_id, std::string job_id, const std::string status) = 0;
    
    virtual void updateJObStatus(std::string jobId, const std::string status) = 0;  
    
    virtual void cancelJob(std::vector<std::string>& requestIDs) = 0;
    
    virtual void getCancelJob(std::vector<int>& requestIDs) = 0;        

 
    /*t_credential API*/
    virtual void insertGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs) = 0;
    
    virtual void updateGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs) = 0;
    
    virtual CredCache* findGrDPStorageCacheElement(std::string delegationID, std::string dn) = 0;
    
    virtual void deleteGrDPStorageCacheElement(std::string delegationID, std::string dn) = 0;
    
    virtual void insertGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time) = 0;
    
    virtual void updateGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time) = 0;
    
    virtual  Cred* findGrDPStorageElement(std::string delegationID, std::string dn) = 0;
    
    virtual void deleteGrDPStorageElement(std::string delegationID, std::string dn) = 0;
        
    virtual bool getDebugMode(std::string source_hostname, std::string destin_hostname) = 0;
    
    virtual void setDebugMode(std::string source_hostname, std::string destin_hostname, std::string mode) = 0;
    
    virtual void getSubmittedJobsReuse(std::vector<TransferJobs*>& jobs, const std::string & vos) = 0;    
    
    virtual void auditConfiguration(const std::string & dn, const std::string & config, const std::string & action) = 0;
        
    virtual void fetchOptimizationConfig2(OptimizerSample* ops, const std::string & source_hostname, const std::string & destin_hostname) = 0;
    
    virtual bool updateOptimizer(std::string file_id , double filesize, int timeInSecs, int nostreams, int timeout, int buffersize,std::string source_hostname, std::string destin_hostname) = 0;
    
    virtual void addOptimizer(time_t when, double throughput, const std::string & source_hostname, const std::string & destin_hostname, int file_id, int nostreams, int timeout, int buffersize, int noOfActiveTransfers) = 0;    
    
    virtual void initOptimizer(const std::string & source_hostname, const std::string & destin_hostname, int file_id) = 0; 
    
    virtual bool isCredentialExpired(const std::string & dlg_id, const std::string & dn) = 0;
    
    virtual bool isTrAllowed(const std::string & source_se, const std::string & dest) = 0;   
    
    virtual void setAllowed(const std::string & job_id, int file_id, const std::string & source_se, const std::string & dest, int nostreams, int timeout, int buffersize) = 0;               
    
    virtual void setAllowedNoOptimize(const std::string & job_id, int file_id, const std::string & params) = 0;
    
    virtual bool terminateReuseProcess(const std::string & jobId) = 0;
    
    virtual void forceFailTransfers() = 0;
    
    virtual void setPid(const std::string & jobId, const std::string & fileId, int pid) = 0;
    
    virtual void setPidV(int pid, std::map<int,std::string>& pids) = 0;        
    
    virtual void revertToSubmitted() = 0;
    
    virtual void revertToSubmittedTerminate() = 0;
       
    virtual void backup() = 0;
    
    virtual void forkFailedRevertState(const std::string & jobId, int fileId) = 0;
    
    virtual void forkFailedRevertStateV(std::map<int,std::string>& pids) = 0; 
    
    virtual bool retryFromDead(std::map<int,std::string>& pids) = 0;
    
    virtual void blacklistSe(std::string se, std::string msg, std::string adm_dn) = 0;

    virtual void blacklistDn(std::string dn, std::string msg, std::string adm_dn) = 0;

    virtual void unblacklistSe(std::string se) = 0;

    virtual void unblacklistDn(std::string dn) = 0;

    virtual bool isSeBlacklisted(std::string se) = 0;

    virtual bool isDnBlacklisted(std::string dn) = 0;
    
    virtual bool isFileReadyState(int fileID) = 0;
    
    virtual bool isFileReadyStateV(std::map<int,std::string>& fileIds) = 0;
    
    //t_group_members
    virtual  bool checkGroupExists(const std::string & groupName) = 0;
    
    virtual void getGroupMembers(const std::string & groupName, std::vector<std::string>& groupMembers) = 0;
    
    virtual void addMemberToGroup(const std::string & groupName, std::vector<std::string>& groupMembers) = 0;
    
    virtual void deleteMembersFromGroup(const std::string & groupName, std::vector<std::string>& groupMembers) = 0;
    
    virtual std::string getGroupForSe(const std::string se) = 0;
      
    
    virtual void submitHost(const std::string & jobId) = 0;     
    
    virtual std::string transferHost(int fileId) = 0;         
    
    virtual std::string transferHostV(std::map<int,std::string>& fileIds) = 0;  
    
    //t_config_symbolic
    virtual void addLinkConfig(LinkConfig* cfg) = 0;
    virtual void updateLinkConfig(LinkConfig* cfg) = 0;
    virtual void deleteLinkConfig(std::string source, std::string destination) = 0;
    virtual LinkConfig* getLinkConfig(std::string source, std::string destination) = 0;
    virtual bool isThereLinkConfig(std::string source, std::string destination) = 0;
    virtual std::pair<std::string, std::string>* getSourceAndDestination(std::string symbolic_name) = 0;
    virtual bool isGrInPair(std::string group) = 0;

    virtual void addShareConfig(ShareConfig* cfg) = 0;
    virtual void updateShareConfig(ShareConfig* cfg) = 0;
    virtual void deleteShareConfig(std::string source, std::string destination, std::string vo) = 0;
    virtual void deleteShareConfig(std::string source, std::string destination) = 0;
    virtual ShareConfig* getShareConfig(std::string source, std::string destination, std::string vo) = 0;
    virtual std::vector<ShareConfig*> getShareConfig(std::string source, std::string destination) = 0;  
     
    virtual bool checkIfSeIsMemberOfAnotherGroup( const std::string & member) = 0; 

    virtual void addJobShareConfig(std::string job_id, std::string source, std::string destination, std::string vo) = 0;

    virtual void delJobShareConfig(std::string job_id) = 0;

    virtual std::vector< boost::tuple<std::string, std::string, std::string> > getJobShareConfig(std::string job_id) = 0;

    virtual unsigned int countJobShareConfig(std::string job_id) = 0;

    virtual int countActiveTransfers(std::string source, std::string destination, std::string vo) = 0;

    virtual int countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo) = 0;

    virtual int countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo) = 0;

    virtual boost::optional<unsigned int> getJobConfigCount(std::string job_id) = 0;

    virtual void setJobConfigCount(std::string job_id, int count) = 0;

    virtual bool checkConnectionStatus() = 0;

    virtual void setPriority(std::string jobId, int priority) = 0;
};





