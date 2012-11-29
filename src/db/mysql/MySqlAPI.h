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
 
    virtual void init(std::string username, std::string password, std::string connectString);

/**
 * Submit a transfer request to be stored in the database
 **/ 
    virtual void submitPhysical(const std::string & jobId, std::vector<src_dest_checksum_tupple> src_dest_pair, const std::string & paramFTP,
                                 const std::string & DN, const std::string & cred, const std::string & voName, const std::string & myProxyServer,
                                 const std::string & delegationID, const std::string & spaceToken, const std::string & overwrite, 
                                 const std::string & sourceSpaceToken, const std::string & sourceSpaceTokenDescription, const std::string & lanConnection, int copyPinLifeTime,
                                 const std::string & failNearLine, const std::string & checksumMethod, const std::string & reuse,
				 const std::string & sourceSE, const std::string & destSe);

    virtual void getTransferJobStatus(std::string requestID, std::vector<JobStatus*>& jobs);
    
    virtual void getTransferFileStatus(std::string requestID, std::vector<FileTransferStatus*>& files);    
    
    virtual void listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates, std::string restrictToClientDN, std::string forDN, std::string VOname);
 
    virtual TransferJobs* getTransferJob(std::string jobId);

    virtual void getSubmittedJobs(std::vector<TransferJobs*>& jobs, const std::string & vos);
    
    virtual void getByJobId(std::vector<TransferJobs*>& jobs, std::vector<TransferFiles*>& files);
   
    virtual void getSe(Se* &se, std::string seName);
   
    virtual unsigned int updateFileStatus(TransferFiles* file, const std::string status);

    virtual void getAllSeInfoNoCritiria(std::vector<Se*>& se);
    
    virtual std::set<std::string> getAllMatchingSeNames(std::string name); 

    virtual void addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
            std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID);

    virtual void updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
            std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID);

    virtual void deleteSe(std::string NAME);
       
    virtual void updateFileTransferStatus(std::string job_id, std::string file_id, std::string transfer_status, std::string transfer_message, int process_id, double filesize, double duration);    
    
    virtual void updateJobTransferStatus(std::string file_id, std::string job_id, const std::string status);
    
    virtual void updateJObStatus(std::string jobId, const std::string status);  
    
    virtual void cancelJob(std::vector<std::string>& requestIDs);
    
    virtual void getCancelJob(std::vector<int>& requestIDs);        
    
    virtual std::vector<std::string> get_group_names();

    virtual std::vector<std::string> get_group_members(std::string name);

    virtual std::string get_group_name(std::string se);    
    
    /*check if a SE is already member of a group*/
    virtual bool is_se_group_member(std::string se);
    
    /*check if a group already exists*/
    virtual bool is_se_group_exist(std::string group);    
      
    /*se group operations*/
    /*add a SE to a group*/
    virtual void add_se_to_group(std::string sd, std::string group);
    
    /*remove se from a group*/
    virtual void remove_se_from_group(std::string sd, std::string group);	
    
    /*delete a group*/
    virtual void delete_group(std::string group);	
 
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
    
    virtual void updateOptimizer(std::string file_id , double filesize, int timeInSecs, int nostreams, int timeout, int buffersize,std::string source_hostname, std::string destin_hostname);
    
    virtual void addOptimizer(time_t when, double throughput, const std::string & source_hostname, const std::string & destin_hostname, int file_id, int nostreams, int timeout, int buffersize, int noOfActiveTransfers);    
    
    virtual void initOptimizer(const std::string & source_hostname, const std::string & destin_hostname, int file_id); 
    
    virtual bool isCredentialExpired(const std::string & dlg_id, const std::string & dn);
    
    virtual bool isTrAllowed(const std::string & source_se, const std::string & dest);   
    
    virtual void setAllowed(const std::string & job_id, int file_id, const std::string & source_se, const std::string & dest, int nostreams, int timeout, int buffersize);               
    
    virtual void setAllowedNoOptimize(const std::string & job_id, int file_id, const std::string & params);
    
    virtual void terminateReuseProcess(const std::string & jobId);
    
    virtual void forceFailTransfers();
    
    virtual void setPid(const std::string & jobId, const std::string & fileId, int pid);
    
    virtual void setPidV(int pid, std::map<int,std::string>& pids);        
    
    virtual void revertToSubmitted();
    
    virtual void revertToSubmittedTerminate();
       
    virtual void backup();
    
    virtual void forkFailedRevertState(const std::string & jobId, int fileId);
    
    virtual void forkFailedRevertStateV(std::map<int,std::string>& pids); 
    
    virtual void retryFromDead(std::map<int,std::string>& pids);
    
    virtual void blacklistSe(std::string se, std::string msg, std::string adm_dn);

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
    
    //t_se_protocol
    virtual SeProtocolConfig*  getProtocol(std::string symbolicName);
    
    virtual int getProtocolIdFromConfig(const std::string & symbolicName,const std::string & vo);
    
    virtual int getProtocolIdFromConfig(const std::string & source, const std::string & dest, const std::string & vo);   
    
    virtual void addProtocol(SeProtocolConfig* seProtocolConfig);    
    
    virtual void deleteProtocol(std::string symbolicName);       
    
    virtual void updateProtocol(SeProtocolConfig* config);                       
    
    //t_config
    virtual std::vector<SeConfig*>  getConfig(const std::string & source,const std::string & dest, const std::string & vo);
    
    virtual std::vector<SeConfig*>  getConfig(const std::string & symbolicName, const std::string & vo);    
    
    virtual void addNewConfig(SeConfig* config);    
    
    virtual void deleteConfig(SeConfig* config); 
    
    virtual void updateConfig(SeConfig* config);              
    
    //general purpose
    virtual std::string checkConfigExists(const std::string & source, const std::string & dest, const std::string & vo);		    	          
    
    virtual bool isTransferAllowed(const std::string & src, const std::string & dest, const std::string & vo); 
    
    virtual void allocateToConfig(const std::string & jobId, const std::string & src, const std::string & dest, const std::string & vo);
    
    virtual void submitHost(const std::string & jobId);     
    
    virtual std::string transferHost(int fileId);         
    
    virtual std::string transferHostV(std::map<int,std::string>& fileIds);  
    
    //t_config_symbolic
    virtual std::pair<std::string, std::string> getSymbolicName(const std::string & src, const std::string & dest);
    
    virtual void addSymbolic(const std::string & symbolicName, const std::string & src, const std::string & dest, const std::string & status);    
    
    virtual void updateSymbolic(const std::string & symbolicName, const std::string & src, const std::string & dest, const std::string & status);        
    
    virtual void deleteSymbolic(const std::string & symbolicName);  
    
   //check the number of active is not > than in configuration pair
    virtual bool checkCreditsForMemberOfGroup(const std::string & symbolicName, const std::string & vo, int active);
    
    //there if a share in configure pair for this vo exist
    virtual bool checkVOForMemberOfGroup(const std::string & symbolicName, const std::string & vo); 
    
    virtual bool checkIfSymbolicNameExists(const std::string & symbolicName, const std::string & vo);
    
    virtual bool checkIfSeIsMemberOfGroup(const std::string & groupName, const std::string & member); 
    
    virtual bool checkIfSymbolicNameExistsForSrcDest(const std::string & symbolicName, const std::string & src, const std::string & dest);
    
    virtual bool checkIfSeIsMemberOfAnotherGroup( const std::string & member);
    
    
    virtual bool checkSePair(const std::string & src, const std::string & dest, const std::string & vo, const std::string & config);
        
	//check if both src and dest are GROUPS
    virtual bool checkGroupToGroup(const std::string & src, const std::string & dest, const std::string & vo, const std::string & config);
	
	//check if source is GROUP
    virtual bool checkSourceGroup(const std::string & src, const std::string & dest, const std::string & vo, const std::string & config);
	
	//check if dest is GROUP
    virtual bool checkDestinationGroup(const std::string & src, const std::string & dest, const std::string & vo, const std::string & config);    
      
    virtual bool checkSeExist(const std::string & se);
    
    virtual void addLinkConfig(LinkConfig* cfg);
    virtual void updateLinkConfig(LinkConfig* cfg);
    virtual void deleteLinkConfig(std::string source, std::string destination);
    virtual LinkConfig* getLinkConfig(std::string source, std::string destination);
    virtual std::pair<std::string, std::string>* getSourceAndDestination(std::string symbolic_name);
    virtual bool isGrInPair(std::string group);

    virtual void addShareConfig(ShareConfig* cfg);
    virtual void updateShareConfig(ShareConfig* cfg);
    virtual void deleteShareConfig(std::string source, std::string destination, std::string vo);
    virtual void deleteShareConfig(std::string source, std::string destination);
    virtual ShareConfig* getShareConfig(std::string source, std::string destination, std::string vo);
    virtual std::vector<ShareConfig*> getShareConfig(std::string source, std::string destination); 
    
    virtual void addJobShareConfig(std::string job_id, std::string source, std::string destination, std::string vo);  
    
    virtual std::string getGroupForSe(const std::string se);
    
    virtual bool isThereLinkConfig(std::string source, std::string destination);  
    
    virtual std::vector< boost::tuple<std::string, std::string, std::string> > getJobShareConfig(std::string job_id);

    virtual int countActiveTransfers(std::string source, std::string destination, std::string vo);

    virtual int countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo);

    virtual int countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo);
    
    virtual bool isThereJobShareConfig(std::string job_id);                   

private:
    size_t                poolSize;
    soci::connection_pool connectionPool;

    bool getInOutOfSe(const std::string& sourceSe, const std::string& destSe);
};
