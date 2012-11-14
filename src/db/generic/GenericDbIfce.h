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
    
    virtual std::vector<std::string> getSiteGroupNames() = 0;

    virtual std::vector<std::string> getSiteGroupMembers(std::string GroupName) = 0;

    virtual void removeGroupMember(std::string groupName, std::string siteName) = 0;

    virtual void addGroupMember(std::string groupName, std::string siteName) = 0;    

    virtual void listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates, std::string restrictToClientDN, std::string forDN, std::string VOname) = 0;
 
    virtual TransferJobs* getTransferJob(std::string jobId) = 0;

    virtual void getSubmittedJobs(std::vector<TransferJobs*>& jobs, const std::string & vos) = 0;
    
    virtual void getByJobId(std::vector<TransferJobs*>& jobs, std::vector<TransferFiles*>& files) = 0;
   
    virtual void getSe(Se* &se, std::string seName) = 0;
   
    virtual unsigned int updateFileStatus(TransferFiles* file, const std::string status) = 0;

    virtual void getAllSeInfoNoCritiria(std::vector<Se*>& se) = 0;
    
    virtual std::set<std::string> getAllMatchingSeNames(std::string name) = 0; 

    virtual void addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
            std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID) = 0;

    virtual void updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
            std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID) = 0;

    virtual void deleteSe(std::string NAME) = 0;
       
    virtual void updateFileTransferStatus(std::string job_id, std::string file_id, std::string transfer_status, std::string transfer_message, int process_id, double filesize, double duration) = 0;    
    
    virtual void updateJobTransferStatus(std::string file_id, std::string job_id, const std::string status) = 0;
    
    virtual void updateJObStatus(std::string jobId, const std::string status) = 0;  
    
    virtual void cancelJob(std::vector<std::string>& requestIDs) = 0;
    
    virtual void getCancelJob(std::vector<int>& requestIDs) = 0;        
    
    virtual std::vector<std::string> get_group_names() = 0;

    virtual std::vector<std::string> get_group_members(std::string name) = 0;

    virtual std::string get_group_name(std::string se) = 0;    
    
    /*check if a SE is already member of a group*/
    virtual bool is_se_group_member(std::string se) = 0;
    
    /*check if a group already exists*/
    virtual bool is_se_group_exist(std::string group) = 0;    
      
    /*se group operations*/
    /*add a SE to a group*/
    virtual void add_se_to_group(std::string sd, std::string group) = 0;
    
    /*remove se from a group*/
    virtual void remove_se_from_group(std::string sd, std::string group) = 0;	
    
    /*delete a group*/
    virtual void delete_group(std::string group) = 0;	
 
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
    
    virtual void updateOptimizer(std::string file_id , double filesize, int timeInSecs, int nostreams, int timeout, int buffersize,std::string source_hostname, std::string destin_hostname) = 0;
    
    virtual void addOptimizer(time_t when, double throughput, const std::string & source_hostname, const std::string & destin_hostname, int file_id, int nostreams, int timeout, int buffersize, int noOfActiveTransfers) = 0;    
    
    virtual void initOptimizer(const std::string & source_hostname, const std::string & destin_hostname, int file_id) = 0; 
    
    virtual bool isCredentialExpired(const std::string & dlg_id, const std::string & dn) = 0;
    
    virtual bool isTrAllowed(const std::string & source_se, const std::string & dest) = 0;   
    
    virtual void setAllowed(const std::string & job_id, int file_id, const std::string & source_se, const std::string & dest, int nostreams, int timeout, int buffersize) = 0;               
    
    virtual void setAllowedNoOptimize(const std::string & job_id, int file_id, const std::string & params) = 0;
    
    virtual void terminateReuseProcess(const std::string & jobId) = 0;
    
    virtual void forceFailTransfers() = 0;
    
    virtual void setPid(const std::string & jobId, const std::string & fileId, int pid) = 0;
    
    virtual void setPidV(int pid, std::map<int,std::string>& pids) = 0;        
    
    virtual void revertToSubmitted() = 0;
    
    virtual void revertToSubmittedTerminate() = 0;
       
    virtual void backup() = 0;
    
    virtual void forkFailedRevertState(const std::string & jobId, int fileId) = 0;
    
    virtual void forkFailedRevertStateV(std::map<int,std::string>& pids) = 0; 
    
    virtual void retryFromDead(std::map<int,std::string>& pids) = 0;
    
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
    
    //t_se_protocol
    virtual SeProtocolConfig*  getProtocol(int protocolId) = 0;
    
    virtual int getProtocolIdFromConfig(const std::string & symbolicName,const std::string & vo) = 0;
    
    virtual int getProtocolIdFromConfig(const std::string & source, const std::string & dest, const std::string & vo) = 0;   
    
    virtual int addProtocol(SeProtocolConfig* seProtocolConfig) = 0;    
    
    virtual void deleteProtocol(int protocolId) = 0;       
    
    virtual void updateProtocol(SeProtocolConfig* config, int protocolId) = 0;           
    
    //t_group_config
    virtual SeGroup* getGroupConfig(const std::string & symbolicName, const std::string & groupName, const std::string & member, const std::string & vo) = 0;
    
    virtual void addGroupConfig(SeGroup* seGroup) = 0;    
    
    virtual void deleteGroupConfig(SeGroup* seGroup) = 0;
    
    virtual void updateGroupConfig(SeGroup* seGroup) = 0;            
    
    //t_config
    virtual SeConfig* getConfig(const std::string & source,const std::string & dest, const std::string & vo) = 0;
    
    virtual SeConfig* getConfig(const std::string & symbolicName, const std::string & vo) = 0;    
    
    virtual void addNewConfig(SeConfig* config) = 0;    
    
    virtual void deleteConfig(SeConfig* config) = 0; 
    
    virtual void updateConfig(SeConfig* config) = 0;              
    
    //general purpose
    virtual std::string checkConfigExists(const std::string & source, const std::string & dest, const std::string & vo) = 0;		    	          
    
    virtual bool isTransferAllowed(const std::string & src, const std::string & dest, const std::string & vo) = 0; 
    
    virtual void allocateToConfig(const std::string & jobId, const std::string & src, const std::string & dest, const std::string & vo) = 0;
    
    virtual void submitHost(const std::string & jobId) = 0;     
    
    virtual void transferHost(int fileId) = 0;         
    
    virtual void transferHostV(std::map<int,std::string>& fileIds) = 0;  
    
    //t_config_symbolic
    virtual std::string getSymbolicName(const std::string & src, const std::string & dest) = 0;
    
    virtual void addSymbolic(const std::string & symbolicName, const std::string & src, const std::string & dest) = 0;    
    
    virtual void updateSymbolic(const std::string & symbolicName, const std::string & src, const std::string & dest) = 0;        
    
    virtual void deleteSymbolic(const std::string & symbolicName) = 0;
    
    //check the number of active is not > than in configuration pair
    virtual bool checkCreditsForMemberOfGroup(const std::string & symbolicName, const std::string & vo, int active) = 0;
    
    //there if a share in configure pair for this vo exist
    virtual bool checkVOForMemberOfGroup(const std::string & symbolicName, const std::string & vo) = 0;    
};





