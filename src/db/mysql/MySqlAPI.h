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

    virtual void init(std::string username, std::string password, std::string connectString);

    virtual void submitPhysical(const std::string & jobId, std::vector<src_dest_checksum_tupple> src_dest_pair, const std::string & paramFTP,
                                 const std::string & DN, const std::string & cred, const std::string & voName, const std::string & myProxyServer,
                                 const std::string & delegationID, const std::string & spaceToken, const std::string & overwrite,
                                 const std::string & sourceSpaceToken, const std::string & sourceSpaceTokenDescription, const std::string & lanConnection, int copyPinLifeTime,
                                 const std::string & failNearLine, const std::string & checksumMethod, const std::string & reuse,
				 const std::string & sourceSE, const std::string & destSe);

    virtual TransferJobs* getTransferJob(std::string jobId);

    virtual void getTransferJobStatus(std::string requestID, std::vector<JobStatus*>& jobs);

    virtual void getTransferFileStatus(std::string requestID, std::vector<FileTransferStatus*>& files);

    virtual std::vector<std::string> getSiteGroupNames();

    virtual std::vector<std::string> getSiteGroupMembers(std::string GroupName);

    virtual void removeGroupMember(std::string groupName, std::string siteName);

    virtual void addGroupMember(std::string groupName, std::string siteName);

    virtual void listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates, std::string restrictToClientDN, std::string forDN, std::string VOname);

    virtual void getSubmittedJobs(std::vector<TransferJobs*>& jobs, const std::string & vos);

    virtual void getByJobId(std::vector<TransferJobs*>& jobs, std::vector<TransferFiles*>& files);


    virtual void getSeCreditsInUse(int &creditsInUse, std::string srcSeName, std::string destSeName, std::string voName);

    virtual void getGroupCreditsInUse(int &creditsInUse, std::string srcGroupName, std::string destGroupName, std::string voName);

    virtual unsigned int updateFileStatus(TransferFiles* file, const std::string status);



    /*NEW API*/
    virtual std::set<std::string> getAllMatchingSeNames(std::string name);

    virtual std::set<std::string> getAllMatchingSeGroupNames(std::string name);

    virtual void getAllSeInfoNoCritiria(std::vector<Se*>& se);

    virtual void getSe(Se* &se, std::string seName);

    virtual void getAllShareConfigNoCritiria(std::vector<SeConfig*>& seConfig);

    virtual void getAllShareAndConfigWithCritiria(std::vector<SeAndConfig*>& seAndConfig, std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE, std::string SHARE_VALUE);

    virtual void addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
            std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID);

    virtual void updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
            std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID);

    virtual void deleteSe(std::string NAME);

    virtual void addSeConfig( std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE, std::string SHARE_VALUE);

    virtual void updateSeConfig(std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE, std::string SHARE_VALUE);

    virtual void deleteSeConfig(std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE);

    virtual void updateFileTransferStatus(std::string job_id, std::string file_id, std::string transfer_status, std::string transfer_message, int process_id, double filesize, double duration);

    virtual void updateJobTransferStatus(std::string file_id, std::string job_id, const std::string status);

    virtual void updateJObStatus(std::string jobId, const std::string status);

    virtual void cancelJob(std::vector<std::string>& requestIDs);

    virtual void getCancelJob(std::vector<int>& requestIDs);

    /*protocol config*/
    virtual std::vector<std::string> get_group_names();

    virtual std::vector<std::string> get_group_members(std::string name);

    virtual std::string get_group_name(std::string se);

    virtual bool is_se_group_member(std::string se);

    virtual bool is_se_group_exist(std::string group);

    virtual bool is_se_protocol_exist(std::string se);

    /*virtual bool is_se_pair_protocol_exist(std::string se1, std::string se2);*/


    virtual SeProtocolConfig* get_se_protocol_config(std::string se);

    /*virtual SeProtocolConfig* get_se_pair_protocol_config(std::string se1, std::string se2);*/

    virtual SeProtocolConfig* get_se_group_protocol_config(std::string se);

    virtual SeProtocolConfig* get_group_protocol_config(std::string group);

    virtual bool add_se_protocol_config(SeProtocolConfig* seProtocolConfig);

    /*virtual bool add_se_pair_protocol_config(SeProtocolConfig* sePairProtocolConfig);*/

    virtual bool add_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig);

    virtual void delete_se_protocol_config(SeProtocolConfig* seProtocolConfig);

    /*virtual void delete_se_pair_protocol_config(SeProtocolConfig* sePairProtocolConfig);*/

    virtual void delete_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig);

    virtual void update_se_protocol_config(SeProtocolConfig* seProtocolConfig);

    /*virtual void update_se_pair_protocol_config(SeProtocolConfig* sePairProtocolConfig);*/

    virtual void update_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig);

    virtual SeProtocolConfig* getProtocol(std::string se1, std::string se2);

    /*se group operations*/
    virtual void add_se_to_group(std::string sd, std::string group);
    virtual void remove_se_from_group(std::string sd, std::string group);
    virtual void delete_group(std::string group);

    virtual bool is_group_protocol_exist(std::string group);


    /*t_credential API*/
    virtual void insertGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs);
    virtual void updateGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs);
    virtual CredCache* findGrDPStorageCacheElement(std::string delegationID, std::string dn);
    virtual void deleteGrDPStorageCacheElement(std::string delegationID, std::string dn);

    virtual void insertGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time);
    virtual void updateGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time);
    virtual Cred* findGrDPStorageElement(std::string delegationID, std::string dn);
    virtual void deleteGrDPStorageElement(std::string delegationID, std::string dn);

    virtual bool getDebugMode(std::string source_hostname, std::string destin_hostname);
    virtual void setDebugMode(std::string source_hostname, std::string destin_hostname, std::string mode);

    virtual void getSubmittedJobsReuse(std::vector<TransferJobs*>& jobs, const std::string & vos);

    virtual void auditConfiguration(const std::string & dn, const std::string & config, const std::string & action);

    virtual void setGroupOrSeState(const std::string & se, const std::string & group, const std::string & state);

    virtual void fetchOptimizationConfig2(OptimizerSample* ops, const std::string & source_hostname, const std::string & destin_hostname);

    virtual void updateOptimizer(std::string file_id , double filesize, int timeInSecs, int nostreams, int timeout, int buffersize, std::string source_hostname, std::string destin_hostname);

    virtual void addOptimizer(time_t when, double throughput, const std::string & source_hostname, const std::string & destin_hostname, int file_id, int nostreams, int timeout, int buffersize, int noOfActiveTransfers);

    virtual void initOptimizer(const std::string & source_hostname, const std::string & destin_hostname, int file_id);

    virtual bool isCredentialExpired(const std::string & dlg_id, const std::string & dn);

    virtual bool isTrAllowed(const std::string & source_se, const std::string & dest);

    virtual void setAllowed(const std::string & job_id, int file_id, const std::string & source_se, const std::string & dest, int nostreams, int timeout, int buffersize);

    virtual void terminateReuseProcess(const std::string & jobId);

    virtual void setAllowedNoOptimize(const std::string & job_id, int file_id, const std::string & params);

    virtual void forceFailTransfers();

    virtual void setPid(const std::string & jobId, const std::string & fileId, int pid);

    virtual void setPidV(int pid, std::map<int,std::string>& pids);

    virtual void revertToSubmitted();

    virtual void revertToSubmittedTerminate();

    virtual bool configExists(const std::string & src, const std::string & dest, const std::string & vo);

    virtual void backup();
    
    virtual void forkFailedRevertState(const std::string & jobId, int fileId);
    
    virtual void forkFailedRevertStateV(std::map<int,std::string>& pids);
    
    virtual void retryFromDead(std::map<int,std::string>& pids);        

private:
    size_t                poolSize;
    soci::connection_pool connectionPool;

    bool getInOutOfSe(const std::string& sourceSe, const std::string& destSe);
};
