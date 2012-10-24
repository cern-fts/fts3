#include <error.h>
#include <mysql/soci-mysql.h>
#include "MySqlAPI.h"
#include "MySqlMonitoring.h"
#include "sociConversions.h"

using namespace FTS3_COMMON_NAMESPACE;


MySqlAPI::MySqlAPI(): poolSize(8), connectionPool(poolSize)  {
}



MySqlAPI::~MySqlAPI() {
}



void MySqlAPI::init(std::string username, std::string password, std::string connectString) {
    for (size_t i = 0; i < poolSize; ++i) {
        std::ostringstream connParams;

        connParams << connectString
                   << " user='" << username << "'"
                   << " pass='" << password << "'";

        soci::session& sql = connectionPool.at(i);
        sql.open(soci::mysql, connParams.str());
    }
}



void MySqlAPI::getSubmittedJobs(std::vector<TransferJobs*>& jobs, const std::string & vos) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlAPI::getSeCreditsInUse(
        int &creditsInUse,
        std::string srcSeName,
        std::string destSeName,
        std::string voName
        ) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlAPI::getGroupCreditsInUse(
        int &creditsInUse,
        std::string srcGroupName,
        std::string destGroupName,
        std::string voName
        ) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



unsigned int MySqlAPI::updateFileStatus(TransferFiles* file, const std::string status) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlAPI::updateJObStatus(std::string jobId, const std::string status) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlAPI::getByJobId(std::vector<TransferJobs*>& jobs, std::vector<TransferFiles*>& files) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlAPI::submitPhysical(const std::string & jobId, std::vector<src_dest_checksum_tupple> src_dest_pair, const std::string & paramFTP,
        const std::string & DN, const std::string & cred, const std::string & voName, const std::string & myProxyServer,
        const std::string & delegationID, const std::string & spaceToken, const std::string & overwrite,
        const std::string & sourceSpaceToken, const std::string &, const std::string & lanConnection, int copyPinLifeTime,
        const std::string & failNearLine, const std::string & checksumMethod, const std::string & reuse,
        const std::string & sourceSE, const std::string & destSE) {

    char hostname[512];
    gethostname(hostname, sizeof(hostname));
    const std::string currenthost = hostname;
    const std::string initialState = "SUBMITTED";
    const int priority = 3;
    time_t nowTimestamp = time(NULL);
    struct tm now;
    const std::string params;

    gmtime_r(&nowTimestamp, &now);

    soci::session sql(connectionPool);
    sql.begin();

    try {
        // Insert job
        sql << "INSERT INTO t_job (job_id, job_state, job_params, user_dn, user_cred, priority,       "
               "                   vo_name, submit_time, internal_job_params, submit_host, cred_id,   "
               "                   myproxy_server, space_token, overwrite_flag, source_space_token,   "
               "                   copy_pin_lifetime, lan_connection, fail_nearline, checksum_method, "
               "                   reuse_job, source_se, dest_se)                                     "
               "VALUES (:jobId, :jobState, :jobParams, :userDn, :userCred, :priority,                 "
               "        :voName, :submitTime, :internalParams, :submitHost, :credId,                  "
               "        :myproxyServer, :spaceToken, :overwriteFlag, :sourceSpaceToken,               "
               "        :copyPinLifetime, :lanConnection, :failNearline, :checksumMethod,             "
               "        :reuseJob, :sourceSE, :destSE)",
               soci::use(jobId), soci::use(initialState), soci::use(paramFTP), soci::use(DN), soci::use(cred), soci::use(priority),
               soci::use(voName), soci::use(now), soci::use(params), soci::use(currenthost), soci::use(delegationID),
               soci::use(myProxyServer), soci::use(spaceToken), soci::use(overwrite), soci::use(sourceSpaceToken),
               soci::use(copyPinLifeTime), soci::use(lanConnection), soci::use(failNearLine), soci::use(checksumMethod),
               soci::use(reuse), soci::use(sourceSE), soci::use(destSE);

        // Insert src/dest pair
        std::string sourceSurl, destSurl, checksum;
        soci::statement pairStmt = (sql.prepare << "INSERT INTO t_file (job_id, file_state, source_surl, dest_surl,checksum) "
                                                    "VALUES (:jobId, :fileState, :sourceSurl, :destSurl, :checksum)",
                                                    soci::use(jobId), soci::use(initialState), soci::use(sourceSurl),
                                                    soci::use(destSurl), soci::use(checksum));
        std::vector<src_dest_checksum_tupple>::const_iterator iter;
        for (iter = src_dest_pair.begin(); iter != src_dest_pair.end(); ++iter) {
            sourceSurl = iter->source;
            destSurl   = iter->destination;
            checksum   = iter->checksum;
            pairStmt.execute();
        }

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": " +  e.what());
    }
}



void MySqlAPI::getTransferJobStatus(std::string requestID, std::vector<JobStatus*>& jobs) {
    soci::session sql(connectionPool);

    // Get job
//    soci::rowset<JobStatus> rs = (sql.prepare << "SELECT * "
//                                                 "FROM t_job "
//                                                 "WHERE t_job.job_id = :jobId", soci::use(requestID));
//    for (soci::rowset<JobStatus>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
//        JobStatus* status = new JobStatus();
//        *status = *i;
//        jobs.push_back(status);
//    }


    // Get number of transfers
//    sql << "SELECT COUNT(*) FROM t_file WHERE t_file.job_id = :jobId",
//            soci::use(requestID), soci::into(status->numFiles);
}



std::vector<std::string> MySqlAPI::getSiteGroupNames() {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

std::vector<std::string> MySqlAPI::getSiteGroupMembers(std::string GroupName) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::removeGroupMember(std::string groupName, std::string siteName) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::addGroupMember(std::string groupName, std::string siteName) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

/*
 * Return a list of jobs based on the status requested
 * std::vector<JobStatus*> jobs: the caller will deallocate memory JobStatus instances and clear the vector
 * std::vector<std::string> inGivenStates: order doesn't really matter, more than one states supported
 */
void MySqlAPI::listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates, std::string restrictToClientDN, std::string forDN, std::string VOname) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::getTransferFileStatus(std::string requestID, std::vector<FileTransferStatus*>& files) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::getSe(Se* &se, std::string seName) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

std::set<std::string> MySqlAPI::getAllMatchingSeNames(std::string name) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

std::set<std::string> MySqlAPI::getAllMatchingSeGroupNames(std::string name) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::getAllSeInfoNoCritiria(std::vector<Se*>& se) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::getAllShareConfigNoCritiria(std::vector<SeConfig*>& seConfig) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::getAllShareAndConfigWithCritiria(std::vector<SeAndConfig*>& seAndConfig, std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE,std::string SHARE_VALUE) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
                     std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
                        std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::addSeConfig(std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE, std::string SHARE_VALUE) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

/*
Update the config of an SE
REQUIRED: SE_NAME / VO_NAME
OPTIONAL; the rest
set int to -1 so as NOT to be changed
 */
void MySqlAPI::updateSeConfig(std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE, std::string SHARE_VALUE) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

/*
Delete a SE
REQUIRED: NAME
 */
void MySqlAPI::deleteSe(std::string NAME) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

/*
Delete the config of an SE for a specific VO
REQUIRED: SE_NAME
OPTIONAL: VO_NAME
 */
void MySqlAPI::deleteSeConfig(std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::updateFileTransferStatus(std::string job_id, std::string file_id, std::string transfer_status, std::string transfer_message, int process_id,
                                        double filesize, double duration) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::updateJobTransferStatus(std::string file_id, std::string job_id, const std::string status) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::cancelJob(std::vector<std::string>& requestIDs) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::getCancelJob(std::vector<int>& requestIDs) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}


/*Protocol config*/

/*check if a SE belongs to a group*/
bool MySqlAPI::is_se_group_member(std::string se) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

bool MySqlAPI::is_se_group_exist(std::string group) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

/*check if a SE belongs to a group*/
bool MySqlAPI::is_se_protocol_exist(std::string se) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

bool MySqlAPI::is_group_protocol_exist(std::string group) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

SeProtocolConfig* MySqlAPI::get_se_protocol_config(std::string se) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

SeProtocolConfig* MySqlAPI::get_se_group_protocol_config(std::string se) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

SeProtocolConfig* MySqlAPI::get_group_protocol_config(std::string group) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

bool MySqlAPI::add_se_protocol_config(SeProtocolConfig* seProtocolConfig) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

bool MySqlAPI::add_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::delete_se_protocol_config(SeProtocolConfig* seProtocolConfig) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::delete_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

/*
void MySqlAPI::update_se_pair_protocol_config(SeProtocolConfig* sePairProtocolConfig){
}
 */


void MySqlAPI::update_se_protocol_config(SeProtocolConfig* seProtocolConfig) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::update_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

SeProtocolConfig* MySqlAPI::getProtocol(std::string, std::string se2) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

/*se group operations*/
void MySqlAPI::add_se_to_group(std::string se, std::string group) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::remove_se_from_group(std::string se, std::string group) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::delete_group(std::string group) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

std::string MySqlAPI::get_group_name(std::string se) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

std::vector<std::string> MySqlAPI::get_group_names() {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

std::vector<std::string> MySqlAPI::get_group_members(std::string name) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

/*t_credential API*/
void MySqlAPI::insertGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs) {
    soci::session sql(connectionPool);
    sql.begin();

    try {
        sql << "INSERT INTO t_credential_cache "
               "    (dlg_id, dn, cert_request, priv_key, voms_attrs) VALUES "
               "    (:dlgId, :dn, :certRequest, :privKey, :vomsAttrs)",
               soci::use(dlg_id), soci::use(dn), soci::use(cert_request), soci::use(priv_key), soci::use(voms_attrs);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": " +  e.what());
    }
}



void MySqlAPI::updateGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs) {
    soci::session sql(connectionPool);
    sql.begin();

    try {
        sql << "UPDATE t_credential_cache SET "
               "    cert_request = :certRequest, "
               "    priv_key = :privKey, "
               "    voms_attrs = :vomsAttrs "
               "WHERE dlg_id = :dlgId AND dn=:dn",
               soci::use(cert_request), soci::use(priv_key), soci::use(voms_attrs),
               soci::use(dlg_id), soci::use(dn);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": " +  e.what());
    }
}



CredCache* MySqlAPI::findGrDPStorageCacheElement(std::string delegationID, std::string dn) {
    CredCache* cred = NULL;
    soci::session sql(connectionPool);

    try {
        cred = new CredCache();
        sql << "SELECT dlg_id, dn, voms_attrs, cert_request, priv_key "
               "FROM t_credential_cache "
               "WHERE dlg_id = :dlgId and dn = :dn",
               soci::use(delegationID), soci::use(dn), soci::into(*cred);
        if (!sql.got_data()) {
            delete cred;
            cred = NULL;
        }
    }
    catch (std::exception& e) {
        delete cred;
        throw Err_Custom(std::string(__func__) + ": " +  e.what());
    }

    return cred;
}



void MySqlAPI::deleteGrDPStorageCacheElement(std::string delegationID, std::string dn) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlAPI::insertGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time) {
    soci::session sql(connectionPool);
    sql.begin();

    try {
        sql << "INSERT INTO t_credential "
               "    (dlg_id, dn, termination_time, proxy, voms_attrs) VALUES "
               "    (:dlgId, :dn, :terminationTime, :proxy, :vomsAttrs)",
               soci::use(dlg_id), soci::use(dn), soci::use(termination_time), soci::use(proxy), soci::use(voms_attrs);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": " +  e.what());
    }
}



void MySqlAPI::updateGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time) {
    soci::session sql(connectionPool);
    sql.begin();

    try {
        sql << "UPDATE t_credential SET "
               "    proxy = :proxy, "
               "    voms_attrs = :vomsAttrs, "
               "    termination_time = :terminationTime "
               "WHERE dlg_id = :dlgId AND dn = :dn",
               soci::use(proxy), soci::use(voms_attrs), soci::use(termination_time),
               soci::use(dlg_id), soci::use(dn);

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



Cred* MySqlAPI::findGrDPStorageElement(std::string delegationID, std::string dn) {
    Cred* cred = NULL;
    soci::session sql(connectionPool);

    try {
        cred = new Cred();
        sql << "SELECT dlg_id, dn, voms_attrs, proxy, termination_time "
               "FROM t_credential "
               "WHERE dlg_id = :dlgId AND dn =:dn",
               soci::use(delegationID), soci::use(dn),
               soci::into(*cred);

        if (!sql.got_data()) {
            delete cred;
            cred = NULL;
        }
    }
    catch (std::exception& e) {
        delete cred;
        throw Err_Custom(std::string(__func__) + ": " +  e.what());
    }

    return cred;
}



void MySqlAPI::deleteGrDPStorageElement(std::string delegationID, std::string dn) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

bool MySqlAPI::getDebugMode(std::string source_hostname, std::string destin_hostname) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::setDebugMode(std::string source_hostname, std::string destin_hostname, std::string mode) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::getSubmittedJobsReuse(std::vector<TransferJobs*>& jobs, const std::string & vos) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::auditConfiguration(const std::string & dn, const std::string & config, const std::string & action) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::setGroupOrSeState(const std::string & se, const std::string & group, const std::string & state) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

/*custom optimization stuff*/

void MySqlAPI::fetchOptimizationConfig2(OptimizerSample* ops, const std::string & source_hostname, const std::string & destin_hostname) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::updateOptimizer(std::string, double filesize, int timeInSecs, int nostreams, int timeout, int buffersize, std::string source_hostname, std::string destin_hostname) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::addOptimizer(time_t when, double throughput, const std::string & source_hostname, const std::string & destin_hostname, int file_id, int nostreams, int timeout, int buffersize, int) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::initOptimizer(const std::string & source_hostname, const std::string & destin_hostname, int) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

bool MySqlAPI::isCredentialExpired(const std::string & dlg_id, const std::string & dn) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

bool MySqlAPI::isTrAllowed(const std::string & source_hostname, const std::string & destin_hostname) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::setAllowedNoOptimize(const std::string & job_id, int file_id, const std::string & params) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

/* REUSE CASE ???*/
void MySqlAPI::forceFailTransfers() {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::setAllowed(const std::string & job_id, int file_id, const std::string & source_se, const std::string & dest, int nostreams, int timeout, int buffersize) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::terminateReuseProcess(const std::string & jobId) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::setPid(const std::string & jobId, const std::string & fileId, int pid) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::setPidV(int pid, std::map<int, std::string>& pids) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::revertToSubmitted(){
    throw Err_System(std::string("Not implemented: ") + __func__);
}

void MySqlAPI::revertToSubmittedTerminate(){
    throw Err_System(std::string("Not implemented: ") + __func__);
}


bool MySqlAPI::configExists(const std::string & src, const std::string & dest, const std::string & vo){
    throw Err_System(std::string("Not implemented: ") + __func__);
}


void MySqlAPI::backup(){
    throw Err_System(std::string("Not implemented: ") + __func__);
}

// the class factories
extern "C" GenericDbIfce* create() {
    return new MySqlAPI;
}

extern "C" void destroy(GenericDbIfce* p) {
    if (p)
        delete p;
}


extern "C" MonitoringDbIfce* create_monitoring() {
    return new MySqlMonitoring;
}

extern "C" void destroy_monitoring(MonitoringDbIfce* p) {
    if (p)
        delete p;
}
