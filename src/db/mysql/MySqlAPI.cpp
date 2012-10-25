#include <error.h>
#include <mysql/soci-mysql.h>
#include "MySqlAPI.h"
#include "MySqlMonitoring.h"
#include "sociConversions.h"

using namespace FTS3_COMMON_NAMESPACE;

static double convertBtoM( double byte,  double duration) {
    return ceil((((byte / duration) / 1024) / 1024) * 100 + 0.5) / 100;
}



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



bool MySqlAPI::getInOutOfSe(const std::string& sourceSe, const std::string& destSe) {
    soci::session sql(connectionPool);

    unsigned nSE;
    sql << "SELECT COUNT(*) FROM t_se WHERE "
           "    (t_se.name = :source OR t_se.name = :dest) AND "
           "    t_se.state='false'",
           soci::use(sourceSe), soci::use(destSe), soci::into(nSE);

    unsigned nGroup;
    sql << "SELECT COUNT(*) FROM t_se_group_contains WHERE "
           "    (t_se_group_contains.se_name = :source OR t_se_group_contains.se_name = :dest) AND "
           "     t_se_group_contains.state='false'",
           soci::use(sourceSe), soci::use(destSe), soci::into(nGroup);

    return nSE == 0 && nGroup == 0;
}



void MySqlAPI::getSubmittedJobs(std::vector<TransferJobs*>& jobs, const std::string & vos) {
    soci::session sql(connectionPool);

    try {
        // Get unique SE pairs
        std::multimap<std::string, std::string> sePairs;
        soci::rowset<soci::row> rs = (sql.prepare << "SELECT DISTINCT source_se, dest_se "
                                                     "FROM t_job "
                                                     "WHERE t_job.job_finished IS NULL AND "
                                                     "      t_job.cancel_job IS NULL AND "
                                                     "      (t_job.reuse_job = 'N' OR t_job.reuse_job IS NULL) AND "
                                                     "      t_job.job_state IN ('ACTIVE', 'READY', 'SUBMITTED')");

        for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
            soci::row const& row = *i;
            sePairs.insert(std::make_pair(row.get<std::string>("source_se"),
                                          row.get<std::string>("dest_se")));
        }

        // Query depends on vos
        std::string query;
        if (vos.empty()) {
            query = "SELECT t_job.* FROM t_job "
                    "WHERE t_job.job_finished IS NULL AND "
                    "      t_job.cancel_job IS NULL AND "
                    "      t_job.source_se = :source AND t_job.dest_se = :dest AND "
                    "      (t_job.reuse_job = 'N' OR t_job.reuse_job IS NULL) AND "
                    "      t_job.job_state IN ('ACTIVE', 'READY', 'SUBMITTED') AND "
                    "      EXISTS ( SELECT NULL FROM t_file WHERE t_file.job_id = t_job.job_id AND t_file.file_state = 'SUBMITTED') "
                    "ORDER BY t_job.priority DESC, t_job.submit_time ASC "
                    "LIMIT 25";
        }
        else {
            query = "SELECT t_job.* FROM t_job "
                                "WHERE t_job.job_finished IS NULL AND "
                                "      t_job.cancel_job IS NULL AND "
                                "      t_job.source_se = :source AND t_job.dest_se = :dest AND "
                                "      (t_job.reuse_job = 'N' OR t_job.reuse_job IS NULL) AND "
                                "      t_job.job_state IN ('ACTIVE', 'READY', 'SUBMITTED') AND "
                                "      t_job.vo_name IN " + vos + " AND "
                                "      EXISTS ( SELECT NULL FROM t_file WHERE t_file.job_id = t_job.job_id AND t_file.file_state = 'SUBMITTED') "
                                "ORDER BY t_job.priority DESC, t_job.submit_time ASC "
                                "LIMIT 25";
        }

        // Iterate through pairs, getting jobs IF the VO has not run out of credits
        // AND there are pending file transfers within the job
        for (std::multimap<std::string, std::string>::const_iterator i = sePairs.begin(); i != sePairs.end(); ++i) {
            soci::rowset<TransferJobs> jobRs = (sql.prepare << query ,
                                                               soci::use(i->first), soci::use(i->second));
            for (soci::rowset<TransferJobs>::const_iterator ji = jobRs.begin(); ji != jobRs.end(); ++ji) {
                TransferJobs const & job = *ji;

                if (getInOutOfSe(job.SOURCE_SE, job.DEST_SE))
                    jobs.push_back(new TransferJobs(job));
            }
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



void MySqlAPI::getSeCreditsInUse(int &creditsInUse,
                                 std::string srcSeName,
                                 std::string destSeName,
                                 std::string voName) {
    std::ostringstream query;
    std::string seName;
    soci::session sql(connectionPool);
    soci::statement stmt(sql);

    query << "SELECT COUNT(*) FROM t_job, t_file WHERE "
          << "    t_job.job_id = t_file.job_id AND "
          << "    t_file.file_state = 'ACTIVE' ";

    if (!srcSeName.empty()) {
        query << " AND t_file.source_surl like '%://' || :source || '%' ";
        stmt.exchange(soci::use(srcSeName, "source"));
    }

    if (!destSeName.empty()) {
        query << " AND t_file.dest_surl like '%://' || :dest || '%' ";
        stmt.exchange(soci::use(destSeName, "dest"));
    }

    if (srcSeName.empty() || destSeName.empty()) {
        if (!voName.empty()) {
            query << " AND t_job.vo_name = :vo ";
            stmt.exchange(soci::use(voName, "vo"));
        }
        else {
            seName = srcSeName.empty() ? destSeName : srcSeName;

            query << " AND NOT EXISTS ( SELECT NULL FROM t_se_vo_share WHERE "
                  << "                     t_se_vo_share.se_name = :seName AND "
                  << "                     t_se_vo_share.share_id = '%\"share_id\":\"' || t_job.vo_name || '\"%' "
                  << "                 )";
            stmt.exchange(soci::use(seName, "seName"));
        }
    }

    try {
        stmt.exchange(soci::into(creditsInUse));
        stmt.alloc();
        stmt.prepare(query.str());
        stmt.define_and_bind();
        stmt.execute(true);
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
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
    soci::session sql(connectionPool);

    try {
        sql.begin();
        soci::statement stmt(sql);

        time_t nowTimestamp = time(NULL);
        struct tm now;
        gmtime_r(&nowTimestamp, &now);

        stmt.exchange(soci::use(status, "state"));
        stmt.exchange(soci::use(now, "now"));
        stmt.exchange(soci::use(file->FILE_ID, "fileId"));
        stmt.alloc();
        stmt.prepare("UPDATE t_file SET "
                     "    file_state = :state, start_time = :now "
                     "WHERE file_id = :fileId AND file_state = 'SUBMITTED'");
        stmt.define_and_bind();
        stmt.execute(true);

        unsigned updated = stmt.get_affected_rows();
        if (updated != 0) {
            soci::statement jobStmt(sql);
            jobStmt.exchange(soci::use(status, "state"));
            jobStmt.exchange(soci::use(file->JOB_ID, "jobId"));
            jobStmt.alloc();
            jobStmt.prepare("UPDATE t_job SET "
                            "    job_state = :state "
                            "WHERE job_id = :jobId AND job_state = 'SUBMITTED'");
            jobStmt.define_and_bind();
            jobStmt.execute(true);
        }
        sql.commit();
        return updated;
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



void MySqlAPI::updateJObStatus(std::string jobId, const std::string status) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlAPI::getByJobId(std::vector<TransferJobs*>& jobs, std::vector<TransferFiles*>& files) {
    soci::session sql(connectionPool);

    try {
        for (std::vector<TransferJobs*>::const_iterator i = jobs.begin(); i != jobs.end(); ++i) {
            std::string jobId = (*i)->JOB_ID;

            soci::rowset<TransferFiles> rs = (sql.prepare << "SELECT t_file.*, t_job.vo_name, t_job.overwrite_flag, "
                                                             "    t_job.user_dn, t_job.cred_id, t_job.checksum_method, "
                                                             "    t_job.source_space_token, t_job.space_token "
                                                             "FROM t_file, t_job WHERE "
                                                             "    t_file.job_id = t_job.job_id AND "
                                                             "    t_file.job_finished IS NULL AND "
                                                             "    t_file.file_state = 'SUBMITTED' AND "
                                                             "    t_job.job_id = :jobId "
                                                             "ORDER BY t_file.file_id DESC", soci::use(jobId));

            for (soci::rowset<TransferFiles>::const_iterator ti = rs.begin(); ti != rs.end(); ++ti) {
                TransferFiles const& tfile = *ti;
                files.push_back(new TransferFiles(tfile));
            }

        }
    }
    catch (std::exception& e) {
        for (std::vector<TransferFiles*>::iterator i = files.begin(); i != files.end(); ++i)
            delete *i;
        files.clear();
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
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
        soci::indicator reuseIndicator = soci::i_ok;
        if (reuse.empty())
            reuseIndicator = soci::i_null;
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
               soci::use(reuse, reuseIndicator), soci::use(sourceSE), soci::use(destSE);

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

    try {
        // Get number first (always the same)
        int nFiles = 0;
        sql << "SELECT COUNT(*) FROM t_file WHERE t_file.job_id = :jobId",
                soci::use(requestID, "jobId"), soci::into(nFiles);

        soci::rowset<JobStatus> rs = (sql.prepare << "SELECT t_job.job_id, t_job.job_state, t_file.file_state, "
                                                     "    t_job.user_dn, t_job.reason, t_job.submit_time, t_job.priority, "
                                                     "    t_job.vo_name "
                                                     "FROM t_job, t_file "
                                                     "WHERE t_file.job_id = t_job.job_id and t_file.job_id = :jobId",
                                                     soci::use(requestID, "jobId"));

        for (soci::rowset<JobStatus>::iterator i = rs.begin(); i != rs.end(); ++i) {
            JobStatus& job = *i;
            job.numFiles = nFiles;
            jobs.push_back(new JobStatus(job));
        }
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": " +  e.what());
    }
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
    soci::session sql(connectionPool);

    try {
        soci::statement stmt(sql);
        std::ostringstream query;

        query << "SELECT t_se_vo_share.se_name, t_se_vo_share.share_id, t_se_vo_share.share_type, t_se_vo_share.share_value ";
        query << "FROM t_se_vo_share ";

        bool first = true;

        if (SE_NAME.length() > 0) {
            first = false;
            query << " WHERE t_se_vo_share.se_name like :se";
            stmt.exchange(soci::use(SE_NAME, "se"));
        }

        if (SHARE_ID.length() > 0) {
            if (first)
                query << " WHERE ";
            else
                query << " AND ";
            first = false;
            query << "t_se_vo_share.share_id like :shareId";
            stmt.exchange(soci::use(SHARE_ID, "shareId"));
        }

        if (SHARE_TYPE.length() > 0) {
            if (first)
                query << " WHERE ";
            else
                query << " AND ";
            first = false;
            query << "t_se_vo_share.share_type = :shareType";
            stmt.exchange(soci::use(SHARE_TYPE, "shareType"));
        }

        if (SHARE_VALUE.length() > 0) {
            if (first)
                query << " WHERE ";
            else
                query << " AND ";
            first = false;
            query << " t_se_vo_share.share_value LIKE :shareValue";
            stmt.exchange(soci::use(SHARE_VALUE, "shareValue"));
        }

        SeAndConfig seandconf;
        stmt.exchange(soci::into(seandconf));
        stmt.alloc();
        stmt.prepare(query.str());
        stmt.define_and_bind();
        stmt.execute(true);

        while (stmt.fetch()) {
            seAndConfig.push_back(new SeAndConfig(seandconf));
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
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
    soci::session sql(connectionPool);

    try {
        double throughput;
        struct tm now;
        time_t timestamp = time(NULL);
        gmtime_r(&timestamp, &now);

        sql.begin();

        soci::statement stmt(sql);
        std::ostringstream query;

        query << "UPDATE t_file SET "
                 "    file_state = :state, reason = :reason";
        stmt.exchange(soci::use(transfer_status, "state"));
        stmt.exchange(soci::use(transfer_message, "reason"));

        if (transfer_status == "FINISHED" || transfer_status == "FAILED" || transfer_status == "CANCELED") {
            query << ", FINISH_TIME = :finishTime";
            stmt.exchange(soci::use(now, "finishTime"));
        }
        if (transfer_status == "ACTIVE") {
            query << ", START_TIME = :startTime";
            stmt.exchange(soci::use(now, "startTime"));
        }

        if (filesize > 0 && duration > 0 && transfer_status == "FINISHED") {
            throughput = convertBtoM(filesize, duration);
        }
        else if (filesize > 0 && duration <= 0 && transfer_status == "FINISHED") {
            throughput = convertBtoM(filesize, 1);
        }
        else {
            throughput = 0;
        }

        query << "   , pid = :pid, filesize = :filesize, tx_duration = :duration, throughput = :throughput "
                 "WHERE file_id = :fileId AND file_state IN ('READY', 'ACTIVE')";
        stmt.exchange(soci::use(process_id, "pid"));
        stmt.exchange(soci::use(filesize, "filesize"));
        stmt.exchange(soci::use(duration, "duration"));
        stmt.exchange(soci::use(throughput, "throughput"));
        stmt.exchange(soci::use(file_id, "fileId"));
        stmt.alloc();
        stmt.prepare(query.str());
        stmt.define_and_bind();
        stmt.execute(true);

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



void MySqlAPI::updateJobTransferStatus(std::string file_id, std::string job_id, const std::string status) {
    soci::session sql(connectionPool);

    try {
        int numberOfFilesInJob = 0;
        int numberOfFilesTerminal = 0;
        int numberOfFilesFinished = 0;
        int numberOfFilesFailed = 0;

        sql << "SELECT nFiles, nTerminal, nFinished, nFailed FROM "
               "    (SELECT COUNT(*) AS nFiles FROM t_file WHERE job_id = :jobId) as DTableTotal, "
               "    (SELECT COUNT(*) AS nTerminal FROM t_file WHERE job_id = :jobId AND file_state IN ('CANCELED', 'FINISHED', 'FAILED')) as DTableTerminal, "
               "    (SELECT COUNT(*) AS nFinished FROM t_file WHERE job_id = :jobId AND file_state = 'FINISHED') AS DTableFinished, "
               "    (SELECT COUNT(*) AS nFailed FROM t_file WHERE job_id = :jobId AND file_state IN ('CANCELED', 'FAILED')) AS DTableFailed",
               soci::use(job_id, "jobId"),
               soci::into(numberOfFilesInJob), soci::into(numberOfFilesTerminal),
               soci::into(numberOfFilesFinished), soci::into(numberOfFilesFailed);

        bool jobFinished = (numberOfFilesInJob == numberOfFilesTerminal);

        sql.begin();

        if (jobFinished) {
            time_t timestamp = time(NULL);
            struct tm now;
            gmtime_r(&timestamp, &now);

            std::string state;
            std::string reason = "One or more files failed. Please have a look at the details for more information";
            if (numberOfFilesFinished > 0 && numberOfFilesFailed > 0) {
                state = "FINISHEDDIRTY";
            }
            else if(numberOfFilesInJob == numberOfFilesFinished) {
                state = "FINISHED";
                reason.clear();
            }
            else if(numberOfFilesFailed > 0) {
                state = "FAILED";
            }
            else {
                state = "FAILED";
                reason = "Inconsistent internal state!";
            }

            // Update job
            sql << "UPDATE t_job SET "
                   "    job_state = :state, job_finished = :finishTime, finish_time = :finishTime, reason = :reason "
                   "WHERE job_id = :jobId AND job_state = 'ACTIVE'",
                   soci::use(state, "state"), soci::use(now, "finishTime"), soci::use(reason, "reason"),
                   soci::use(job_id, "jobId");

            // And file finish timestamp
            sql << "UPDATE t_file SET job_finished = :finishTime WHERE job_id = :jobId",
                    soci::use(now, "finishTime"), soci::use(job_id, "jobId");
        }
        // Job not finished yet
        else {
            if (status == "ACTIVE") {
                sql << "UPDATE t_job "
                       "SET job_state = :state WHERE job_id = :jobId AND job_state IN ('READY','ACTIVE')",
                       soci::use(status, "state"), soci::use(job_id, "jobId");
            }
        }

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



void MySqlAPI::cancelJob(std::vector<std::string>& requestIDs) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlAPI::getCancelJob(std::vector<int>& requestIDs) {
    soci::session select(connectionPool);
    soci::session update(connectionPool);

    try {
        soci::rowset<soci::row> rs = (select.prepare << "SELECT t_file.pid, t_job.job_id FROM t_file, t_job "
                                                        "WHERE t_file.job_id = t_job.job_id AND "
                                                        "      t_file.FILE_STATE = 'CANCELED' AND "
                                                        "      t_file.PID IS NOT NULL AND "
                                                        "      t_job.cancel_job IS NULL");

        std::string jobId;
        soci::statement updateStmt = (select.prepare << "UPDATE t_job SET cancel_job='Y' WHERE job_id = :jobId",
                                                        soci::use(jobId));

        update.begin();
        for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
            soci::row const& row = *i;

            int pid = row.get<int>("pid");
            jobId = row.get<std::string>("job_id");

            requestIDs.push_back(pid);
            updateStmt.execute();
        }
        update.commit();
    }
    catch (std::exception& e) {
        update.rollback();
        requestIDs.clear();
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



/*Protocol config*/

/*check if a SE belongs to a group*/
bool MySqlAPI::is_se_group_member(std::string se) {
    soci::session sql(connectionPool);

    try {
        unsigned count = 0;
        sql << "SELECT count(se_name) FROM t_se_group_contains WHERE se_name = :se",
                soci::use(se, "se"), soci::into(count);

        return count > 0;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



bool MySqlAPI::is_se_group_exist(std::string group) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}

/*check if a SE belongs to a group*/
bool MySqlAPI::is_se_protocol_exist(std::string se) {
    soci::session sql(connectionPool);

    try {
        unsigned count = 0;
        sql << "SELECT count(se_name) FROM t_se_protocol WHERE se_name = :se AND se_group_name IS NULL",
                soci::use(se, "se"), soci::into(count);

        return count > 0;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



bool MySqlAPI::is_group_protocol_exist(std::string group) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



SeProtocolConfig* MySqlAPI::get_se_protocol_config(std::string se) {
    soci::session sql(connectionPool);

    try {
        SeProtocolConfig protoConfig;
        sql << "SELECT * FROM t_se_protocol WHERE se_name = :se AND se_group_name IS NULL",
                soci::use(se, "se"), soci::into(protoConfig);
        return new SeProtocolConfig(protoConfig);
    }
    catch (std::exception& e) {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



SeProtocolConfig* MySqlAPI::get_se_group_protocol_config(std::string se) {
    soci::session sql(connectionPool);

    try {
        SeProtocolConfig protoConfig;
        sql << "SELECT * FROM t_se_protocol WHERE se_group_name = :group",
                soci::use(se, "group"), soci::into(protoConfig);
        return new SeProtocolConfig(protoConfig);
    }
    catch (std::exception& e) {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
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
    bool is_dest_group = is_se_group_member(se2);

    if (is_dest_group) {
        return get_se_group_protocol_config(se2);
    } else {
        bool is_se = is_se_protocol_exist(se2);
        if (is_se) {
            return get_se_protocol_config(se2);
        }
    }

    return NULL;
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
    soci::session sql(connectionPool);

    std::string group;
    try {
        sql << "SELECT t_se_group.se_group_name FROM t_se_group, t_se_group_contains WHERE "
               "    t_se_group.se_group_id = t_se_group_contains.se_group_id AND "
               "    t_se_group_contains.se_name = :se", soci::use(se), soci::into(group);
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
    return group;
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

    try {
        struct tm tTime;
        gmtime_r(&termination_time, &tTime);

        sql.begin();
        sql << "INSERT INTO t_credential "
               "    (dlg_id, dn, termination_time, proxy, voms_attrs) VALUES "
               "    (:dlgId, :dn, :terminationTime, :proxy, :vomsAttrs)",
               soci::use(dlg_id), soci::use(dn), soci::use(tTime), soci::use(proxy), soci::use(voms_attrs);
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
        struct tm tTime;
        gmtime_r(&termination_time, &tTime);

        sql << "UPDATE t_credential SET "
               "    proxy = :proxy, "
               "    voms_attrs = :vomsAttrs, "
               "    termination_time = :terminationTime "
               "WHERE dlg_id = :dlgId AND dn = :dn",
               soci::use(proxy), soci::use(voms_attrs), soci::use(tTime),
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
    soci::session sql(connectionPool);

    try {
        std::string debug;
        sql << "SELECT debug FROM t_debug WHERE source_se = :source AND (dest_se = :dest OR dest_se IS NULL)",
                soci::use(source_hostname), soci::use(destin_hostname), soci::into(debug);

        return sql.got_data() && debug == "on";
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " +  e.what());
    }
}



void MySqlAPI::setDebugMode(std::string source_hostname, std::string destin_hostname, std::string mode) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlAPI::getSubmittedJobsReuse(std::vector<TransferJobs*>& jobs, const std::string & vos) {
    soci::session sql(connectionPool);

    try {
        std::string query;
        if (vos.empty()) {
            query = "SELECT t_job.* FROM t_job WHERE "
                    "    t_job.job_finished IS NULL AND "
                    "    t_job.cancel_job IS NULL AND "
                    "    t_job.reuse_job='Y' AND "
                    "    t_job.job_state IN ('ACTIVE', 'READY','SUBMITTED') "
                    "ORDER BY t_job.priority DESC, t_job.submit_time ASC "
                    "LIMIT 1";
        }
        else {
            query = "SELECT t_job.* FROM t_job WHERE "
                                "    t_job.job_finished IS NULL AND "
                                "    t_job.cancel_job IS NULL AND "
                                "    t_job.reuse_job='Y' AND "
                                "    t_job.job_state IN ('ACTIVE', 'READY','SUBMITTED') AND "
                                "    t_job.vo_name IN " + vos + " "
                                "ORDER BY t_job.priority DESC, t_job.submit_time ASC "
                                "LIMIT 1";
        }

        soci::rowset<TransferJobs> rs = (sql.prepare << query);
        for (soci::rowset<TransferJobs>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
            TransferJobs const& tjob = *i;
            jobs.push_back(new TransferJobs(tjob));
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
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
    soci::session sql(connectionPool);

    try {
        struct tm termTime;
        bool valid = false;

        sql << "SELECT termination_time FROM t_credential WHERE dlg_id = :dlgId AND dn = :dn",
                soci::use(dlg_id), soci::use(dn), soci::into(termTime);

        if (sql.got_data()) {
            time_t termTimestamp = mktime(&termTime);
            valid = (difftime(termTimestamp, time(NULL)) > 0);
        }

        return valid;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



bool MySqlAPI::isTrAllowed(const std::string & source_hostname, const std::string & destin_hostname) {
    throw Err_System(std::string("Not implemented: ") + __func__);
}



void MySqlAPI::setAllowedNoOptimize(const std::string & job_id, int file_id, const std::string & params) {
    soci::session sql(connectionPool);

    try {
        sql.begin();

        if (file_id)
            sql << "UPDATE t_file SET internal_file_params = :params WHERE file_id = :fileId AND job_id = :jobId",
                    soci::use(params), soci::use(file_id), soci::use(job_id);
        else
            sql << "UPDATE t_file SET internal_file_params = :params WHERE job_id = :jobId",
                   soci::use(params), soci::use(job_id);

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
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
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "UPDATE t_file SET pid = :pid WHERE job_id = :jobId AND file_id = :fileId",
                soci::use(pid), soci::use(jobId), soci::use(fileId);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
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



bool MySqlAPI::configExists(const std::string & src, const std::string & dest, const std::string & vo) {
    // TODO
    return false;
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
