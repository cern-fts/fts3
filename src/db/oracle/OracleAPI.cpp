#include "OracleAPI.h"
#include <fstream>
#include "ReadConfigFile.h"
#include "Logger.h"
#include "OracleTypeConversions.h"

OracleAPI::OracleAPI()  {
}

OracleAPI::~OracleAPI() {
	if(conn)
		delete conn;
}


void OracleAPI::init(std::string username, std::string password, std::string connectString){
	conn = new OracleConnection(username, password, connectString);

}

std::string OracleAPI::submitPhysical(std::string jobId, std::map<std::string, std::string> src_dest_pair, std::string paramFTP, std::string DN, std::string cred, std::string voName,
            std::string delegationID, std::string spaceToken, std::string overwrite, std::string sourceSpaceToken,
            std::string sourceSpaceTokenDescription,
             int copyPingLifeTime, std::string failNearLine,
            std::vector<std::string> checksum, std::string checksumMode) {
	    
	    /*
	    
	    */
}

std::vector<JobStatus> OracleAPI::listRequests(std::vector<std::string> inGivenStates,
        std::string channelName, std::string restrictToClientDN, std::string forDN, std::string VOname) {
}

std::vector<FileTransferStatus> OracleAPI::getFileStatus(std::string requestID, int offset, int limit) {
}

JobStatus* OracleAPI::getTransferJobStatus(std::string requestID) {

    std::string query = "SELECT job_id, job_state, se_pair_name, user_dn, reason, submit_time, priority, vo_name, "
            "(SELECT count(*) from t_file where t_file.job_id = t_job.job_id) "
            "FROM t_job WHERE job_id = '" + requestID + "'";

    JobStatus* js = NULL;
    try {
        js = new JobStatus();
	js->priority = 0;
	js->numFiles = 0;	
	js->submitTime = (std::time_t)-1;	
        oracle::occi::Statement* s = conn->createStatement(query);
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
        conn->destroyStatement(s);        
    } catch (oracle::occi::SQLException const &e) {
        Logger::instance().error(e.what());
	if(js)
		delete js;
	throw std::string(e.what());
    }
    return js;
}

TransferJobSummary* OracleAPI::getTransferJobSummary(std::string requestID) {
}

SePair* OracleAPI::getSEPairName(std::string sePairName) {
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
}

std::map<std::string, std::string> OracleAPI::getChannelManager(std::string channelName, std::vector<std::string> principals) {
}

void OracleAPI::addVOManager(std::string VOName, std::string principal) {
}

void OracleAPI::removeVOManager(std::string VOName, std::string principal) {
}

std::vector<std::string> OracleAPI::listVOManagers(std::string VOName) {
}

std::map<std::string, std::string> OracleAPI::getVOManager(std::string VOName, std::vector<std::string> principals) {
}

bool OracleAPI::isRequestManager(std::string requestID, std::string clientDN, std::vector<std::string> principals, bool includeOwner) {
}

void OracleAPI::removeVOShare(std::string channelName, std::string VOName) {
}

void OracleAPI::setVOShare(std::string channelName, std::string VOName, int share) {
}

bool OracleAPI::isAgentAvailable(std::string name, std::string type) {
}

std::string OracleAPI::getSchemaVersion() {
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

std::vector<std::string> OracleAPI::getSiteGroupNames() {
    std::string query = "SELECT distinct group_name FROM t_site_group";
    std::vector<std::string> groups;

    try {
        oracle::occi::Statement* s = conn->createStatement(query);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        while (r->next()) {
            std::string groupName = r->getString(1);
            groups.push_back(groupName);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s);
    } catch (oracle::occi::SQLException const &e) {
        Logger::instance().error(e.what());
	throw std::string(e.what());
    }	
    return groups;    
}

std::vector<std::string> OracleAPI::getSiteGroupMembers(std::string GroupName) {
    std::string query = "SELECT site_Name FROM t_site_group where group_Name = '" + GroupName + "' ";
    std::vector<std::string> groups;

    try {
        oracle::occi::Statement* s = conn->createStatement(query);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        while (r->next()) {
            std::string groupName = r->getString(1);
            groups.push_back(groupName);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s);        
    } catch (oracle::occi::SQLException const &e) {
        Logger::instance().error(e.what());
	throw std::string(e.what());
    }
    return groups;
}

void OracleAPI::removeGroupMember(std::string groupName, std::string siteName) {
    std::string query = "DELETE FROM t_site_group WHERE group_Name = '" + groupName + "' AND site_Name = '" + siteName + "'";
    try {
        oracle::occi::Statement* s = conn->createStatement(query);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        conn->commit();
        conn->destroyResultset(s, r);
        conn->destroyStatement(s);

    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        Logger::instance().error(e.what());
	throw std::string(e.what());
    }
}

void OracleAPI::addGroupMember(std::string groupName, std::string siteName) {

    std::string query = "INSERT INTO t_site_group (group_Name, site_Name) VALUES ('" + groupName + "','" + siteName + "')";
    try {
        oracle::occi::Statement* s = conn->createStatement(query);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        conn->commit();
        conn->destroyResultset(s, r);
        conn->destroyStatement(s);

    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        Logger::instance().error(e.what());	
	throw std::string(e.what());
    }
}


/* ********************************* NEW API FTS3 *********************************/
void OracleAPI::addSe( std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
    std::string  SE_TRANSFER_TYPE, std::string   SE_TRANSFER_PROTOCOL, std::string   SE_CONTROL_PROTOCOL, std::string GOCDB_ID){
}

Se* OracleAPI::getSeInfo(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
    std::string  SE_TRANSFER_TYPE, std::string   SE_TRANSFER_PROTOCOL, std::string   SE_CONTROL_PROTOCOL, std::string GOCDB_ID){
}

void OracleAPI::updateSe( std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
    std::string  SE_TRANSFER_TYPE, std::string   SE_TRANSFER_PROTOCOL, std::string   SE_CONTROL_PROTOCOL, std::string GOCDB_ID){
}

void OracleAPI::deleteSe( std::string ENDPOINT, std::string SITE, std::string NAME){
}



// the class factories
extern "C" GenericDbIfce* create() {
    return new OracleAPI;
}

extern "C" void destroy(GenericDbIfce* p) {
    delete p;
}
