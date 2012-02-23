#include "OracleAPI.h"
#include <fstream>
#include "error.h"
#include "OracleTypeConversions.h"

using namespace FTS3_COMMON_NAMESPACE;

OracleAPI::OracleAPI(): conn(NULL)  {
}

OracleAPI::~OracleAPI() {
	if(conn)
		delete conn;
}

void OracleAPI::init(std::string username, std::string password, std::string connectString){
	if(!conn)
		conn = new OracleConnection(username, password, connectString);
}

void OracleAPI::submitPhysical(const std::string & jobId, std::map<std::string, std::string> src_dest_pair, const std::string & paramFTP,
                                 const std::string & DN, const std::string & cred, const std::string & voName, const std::string & myProxyServer,
                                 const std::string & delegationID, const std::string & spaceToken, const std::string & overwrite, 
                                 const std::string & sourceSpaceToken, const std::string & sourceSpaceTokenDescription, const std::string & lanConnection, int copyPinLifeTime,
                                 const std::string & failNearLine, const std::string & checksum, const std::string & checksumMethod) {
/*
	Required fields
	JOB_ID 				   NOT NULL CHAR(36)
	JOB_STATE			   NOT NULL VARCHAR2(32)
	USER_DN				   NOT NULL VARCHAR2(1024)
*/
   std::string source;
   std::string destination;
   const std::string initial_state = "SUBMITTED";
   time_t timed = time (NULL);
   char hostname[512] = {0};
   gethostname(hostname, 512);
   const std::string currenthost = hostname; //current hostname
   const std::string tag_job_statement = "tag_job_statement";	    
   const std::string tag_file_statement = "tag_file_statement";	       
   const std::string job_statement = "INSERT INTO t_job(job_id, job_state, job_params, user_dn, user_cred, priority, vo_name,submit_time,internal_job_params,submit_host, cred_id, myproxy_server, storage_class, overwrite_flag,source_token_description,copy_pin_lifetime, lan_connection,fail_nearline, checksum_method) VALUES (:1,:2,:3,:4,:5,:6,:7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17, :18, :19)";
   const std::string file_statement = "INSERT INTO t_file (job_id, file_state, source_surl, dest_surl,checksum) VALUES (:1,:2,:3,:4,:5)";
	    
    try {
        oracle::occi::Statement* s_job_statement = conn->createStatement(job_statement, tag_job_statement);
	s_job_statement->setString(1, jobId); //job_id
	s_job_statement->setString(2, initial_state); //job_state
	s_job_statement->setString(3, paramFTP); //job_params
	s_job_statement->setString(4, DN); //user_dn
	s_job_statement->setString(5, cred); //user_cred
	s_job_statement->setInt(6, 3); //priority
	s_job_statement->setString(7, voName); //vo_name
	s_job_statement->setTimestamp(8, OracleTypeConversions::toTimestamp(timed, conn->getEnv())); //submit_time
	s_job_statement->setString(9, ""); //internal_job_params
	s_job_statement->setString(10, currenthost); //submit_host
	s_job_statement->setString(11, delegationID); //cred_id
	s_job_statement->setString(12, myProxyServer); //myproxy_server
	s_job_statement->setString(13, spaceToken); //storage_class
	s_job_statement->setString(14, overwrite); //overwrite_flag
	s_job_statement->setString(15, sourceSpaceToken); //source_token_description
	s_job_statement->setInt(16, copyPinLifeTime); //copy_pin_lifetime
	s_job_statement->setString(17, lanConnection); //lan_connection
	s_job_statement->setString(18, failNearLine); //fail_nearline	
	if(checksumMethod.length() == 0)
		s_job_statement->setNull(19, oracle::occi::OCCICHAR); 
	else
		s_job_statement->setString(19, "Y"); //checksum_method		
	s_job_statement->executeUpdate();
	
	//now insert each src/dest pair for this job id
        std::map<std::string, std::string>::iterator iter;
        oracle::occi::Statement* s_file_statement = conn->createStatement(file_statement, tag_file_statement);	
	
	for (iter = src_dest_pair.begin(); iter != src_dest_pair.end(); ++iter) {
                source = std::string(iter->first);
		destination = std::string(iter->second);
		s_file_statement->setString(1, jobId);
		s_file_statement->setString(2, initial_state);
		s_file_statement->setString(3, source);
		s_file_statement->setString(4, destination);
		s_file_statement->setString(5, checksum);	                
   		s_file_statement->executeUpdate();
        }	
        conn->commit();
        conn->destroyStatement(s_job_statement, tag_job_statement);
        conn->destroyStatement(s_file_statement, tag_file_statement);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	    
	    
}

JobStatus* OracleAPI::getTransferJobStatus(std::string requestID) {

    const std::string query = "SELECT job_id, job_state, se_pair_name, user_dn, reason, submit_time, priority, vo_name, "
            "(SELECT count(*) from t_file where t_file.job_id = t_job.job_id) "
            "FROM t_job WHERE job_id = :1";
    const std::string tag = "getTransferJobStatus";	    

    JobStatus* js = NULL;
    try {
        js = new JobStatus();
	js->priority = 0;
	js->numFiles = 0;	
	js->submitTime = (std::time_t)-1;	
        oracle::occi::Statement* s = conn->createStatement(query, tag);
	s->setString(1, requestID);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        while (r->next()) {
            js->jobID = r->getString(1);
            js->jobStatus = r->getString(2);
	    js->channelName = r->getString(3);	    
	    js->clientDN = r->getString(4);	    	    
	    js->reason = r->getString(5);	    	    	    
	    js->submitTime = OracleTypeConversions::toTimeT(r->getTimestamp(6));	    	    	    	    
	    js->priority = r->getInt(7);	    	    	    	    	    
	    js->voName = r->getString(8);
	    js->numFiles = r->getInt(9);	    	    	    	    	    	    	    	    	                
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);        
    } catch (oracle::occi::SQLException const &e) {
	if(js)
		delete js;
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
    return js;
}


std::vector<std::string> OracleAPI::getSiteGroupNames() {
    std::string query = "SELECT distinct group_name FROM t_site_group";
    std::string tag = "getSiteGroupNames";
    std::vector<std::string> groups;

    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        while (r->next()) {
            std::string groupName = r->getString(1);
            groups.push_back(groupName);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
       FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
    return groups;    
}

std::vector<std::string> OracleAPI::getSiteGroupMembers(std::string GroupName) {
    std::string query = "SELECT site_Name FROM t_site_group where group_Name = :1";
    std::string tag = "getSiteGroupMembers";
    std::vector<std::string> groups;

    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
	s->setString(1, GroupName);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        while (r->next()) {
            std::string groupName = r->getString(1);
            groups.push_back(groupName);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);        
    } catch (oracle::occi::SQLException const &e) {
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
    return groups;
}

void OracleAPI::removeGroupMember(std::string groupName, std::string siteName) {
    std::string query = "DELETE FROM t_site_group WHERE group_Name = :1 AND site_Name = :2";
    std::string tag = "removeGroupMember";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
	s->setString(1,groupName);
	s->setString(2,siteName);		
        oracle::occi::ResultSet* r = conn->createResultset(s);
        conn->commit();
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::addGroupMember(std::string groupName, std::string siteName) {

    std::string query = "INSERT INTO t_site_group (group_Name, site_Name) VALUES (:1,:2)";
    std::string tag = "addGroupMember";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
	s->setString(1,groupName);
	s->setString(2,siteName);	
        oracle::occi::ResultSet* r = conn->createResultset(s);
        conn->commit();
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}


/*
std::vector<JobStatus> OracleAPI::listRequests(std::vector<std::string> inGivenStates,
        std::string channelName, std::string restrictToClientDN, std::string forDN, std::string VOname) {
	std::vector<JobStatus> test;
	return test;
}

std::vector<FileTransferStatus> OracleAPI::getFileStatus(std::string requestID, int offset, int limit) {
	std::vector<FileTransferStatus> test;
	return test;
}


TransferJobSummary* OracleAPI::getTransferJobSummary(std::string requestID) {
	return new TransferJobSummary;
}

SePair* OracleAPI::getSEPairName(std::string sePairName) {
	return new SePair;
}

void OracleAPI::cancel(std::vector<std::string> requestIDs) {
}

void OracleAPI::addChannel(std::string channelName, std::string sourceSite, std::string destSite, std::string contact, int numberOfStreams,
        int numberOfFiles, int bandwidth, int nominalThroughput, std::string state) {
}

void OracleAPI::setJobPriority(std::string requestID, int priority) {
}

void OracleAPI::dropChannel(std::string name) {
}

void OracleAPI::setState(std::string channelName, std::string state, std::string message) {
}

std::vector<std::string> OracleAPI::listChannels() {
	std::vector<std::string> test;
	return test;
}

void OracleAPI::setNumberOfStreams(std::string channelName, int numberOfStreams, std::string message) {
}

void OracleAPI::setNumberOfFiles(std::string channelName, int numberOfFiles, std::string message) {
}

void OracleAPI::setBandwidth(std::string channelName, int utilisation, std::string message) {
}

void OracleAPI::setContact(std::string channelName, std::string contact, std::string message) {
}

void OracleAPI::setNominalThroughput(std::string channelName, int nominalThroughput, std::string message) {
}

void OracleAPI::changeStateForHeldJob(std::string jobID, std::string state) {
}

void OracleAPI::changeStateForHeldJobs(std::string channelName, std::string state) {
}

void OracleAPI::addChannelManager(std::string channelName, std::string principal) {
}

void OracleAPI::removeChannelManager(std::string channelName, std::string principal) {
}

std::vector<std::string> OracleAPI::listChannelManagers(std::string channelName) {
	std::vector<std::string> test;
	return test;
}

std::map<std::string, std::string> OracleAPI::getChannelManager(std::string channelName, std::vector<std::string> principals) {
	std::map<std::string, std::string> test;
	return test;
}

void OracleAPI::addVOManager(std::string VOName, std::string principal) {
}

void OracleAPI::removeVOManager(std::string VOName, std::string principal) {
}

std::vector<std::string> OracleAPI::listVOManagers(std::string VOName) {
	std::vector<std::string> test;
	return test;

}

std::map<std::string, std::string> OracleAPI::getVOManager(std::string VOName, std::vector<std::string> principals) {
	std::map<std::string, std::string> test;
	return test;
}

bool OracleAPI::isRequestManager(std::string requestID, std::string clientDN, std::vector<std::string> principals, bool includeOwner) {
	return true;
}

void OracleAPI::removeVOShare(std::string channelName, std::string VOName) {
}

void OracleAPI::setVOShare(std::string channelName, std::string VOName, int share) {
}

bool OracleAPI::isAgentAvailable(std::string name, std::string type) {
	return true;
}

std::string OracleAPI::getSchemaVersion() {
	return std::string("");
}

void OracleAPI::setTcpBufferSize(std::string channelName, std::string bufferSize, std::string message) {
}

void OracleAPI::setTargetDirCheck(std::string channelName, int targetDirCheck, std::string message) {
}

void OracleAPI::setUrlCopyFirstTxmarkTo(std::string channelName, int urlCopyFirstTxmarkTo, std::string message) {
}

void OracleAPI::setChannelType(std::string channelName, std::string channelType, std::string message) {
}

void OracleAPI::setBlockSize(std::string channelName, std::string blockSize, std::string message) {
}

void OracleAPI::setHttpTimeout(std::string channelName, int httpTimeout, std::string message) {
}

void OracleAPI::setTransferLogLevel(std::string channelName, std::string transferLogLevel, std::string message) {
}

void OracleAPI::setPreparingFilesRatio(std::string channelName, double preparingFilesRatio, std::string message) {
}

void OracleAPI::setUrlCopyPutTimeout(std::string channelName, int urlCopyPutTimeout, std::string message) {
}

void OracleAPI::setUrlCopyPutDoneTimeout(std::string channelName, int urlCopyPutDoneTimeout, std::string message) {
}

void OracleAPI::setUrlCopyGetTimeout(std::string channelName, int urlCopyGetTimeout, std::string message) {
}

void OracleAPI::setUrlCopyGetDoneTimeout(std::string channelName, int urlCopyGetDoneTimeout, std::string message) {
}

void OracleAPI::setUrlCopyTransferTimeout(std::string channelName, int urlCopyTransferTimeout, std::string message) {
}

void OracleAPI::setUrlCopyTransferMarkersTimeout(std::string channelName, int urlCopyTransferMarkersTimeout, std::string message) {
}

void OracleAPI::setUrlCopyNoProgressTimeout(std::string channelName, int urlCopyNoProgressTimeout, std::string message) {
}

void OracleAPI::setUrlCopyTransferTimeoutPerMB(std::string channelName, double urlCopyTransferTimeoutPerMB, std::string message) {
}

void OracleAPI::setSrmCopyDirection(std::string channelName, std::string srmCopyDirection, std::string message) {
}

void OracleAPI::setSrmCopyTimeout(std::string channelName, int srmCopyTimeout, std::string message) {
}

void OracleAPI::setSrmCopyRefreshTimeout(std::string channelName, int srmCopyRefreshTimeout, std::string message) {
}

void OracleAPI::removeVOLimit(std::string channelUpperName, std::string voName) {
}

void OracleAPI::setVOLimit(std::string channelUpperName, std::string voName, int limit) {
}
*/
/* ********************************* NEW API FTS3 *********************************/
/*void OracleAPI::addSe( std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
    std::string  SE_TRANSFER_TYPE, std::string   SE_TRANSFER_PROTOCOL, std::string   SE_CONTROL_PROTOCOL, std::string GOCDB_ID){
}

Se* OracleAPI::getSeInfo(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
    std::string  SE_TRANSFER_TYPE, std::string   SE_TRANSFER_PROTOCOL, std::string   SE_CONTROL_PROTOCOL, std::string GOCDB_ID){
	return new Se;
}

void OracleAPI::updateSe( std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
    std::string  SE_TRANSFER_TYPE, std::string   SE_TRANSFER_PROTOCOL, std::string   SE_CONTROL_PROTOCOL, std::string GOCDB_ID){
}

void OracleAPI::deleteSe( std::string ENDPOINT, std::string SITE, std::string NAME){
}
*/


// the class factories
extern "C" GenericDbIfce* create() {
    return new OracleAPI;
}

extern "C" void destroy(GenericDbIfce* p) {
    delete p;
}
