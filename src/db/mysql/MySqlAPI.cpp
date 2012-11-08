#include <error.h>
#include <logger.h>
#include <mysql/soci-mysql.h>
#include <mysql/mysql.h>
#include <signal.h>
#include "MySqlAPI.h"
#include "MySqlMonitoring.h"
#include "sociConversions.h"

using namespace FTS3_COMMON_NAMESPACE;

static double convertBtoM( double byte,  double duration) {
    return ceil((((byte / duration) / 1024) / 1024) * 100 + 0.5) / 100;
}


static int extractTimeout(std::string & str) {
    size_t found;
    found = str.find("timeout:");
    if (found != std::string::npos) {
        str = str.substr(found, str.length());
        size_t found2;
        found2 = str.find(",buffersize:");
        if (found2 != std::string::npos) {
            str = str.substr(0, found2);
            str = str.substr(8, str.length());
            return atoi(str.c_str());
        }

    }
    return 0;
}


MySqlAPI::MySqlAPI(): poolSize(8), connectionPool(poolSize)  {
}



MySqlAPI::~MySqlAPI() {
}



void MySqlAPI::init(std::string username, std::string password, std::string connectString) {
    std::ostringstream connParams;
    std::string host, db;

    // From connectString, get host and db
    size_t slash = connectString.find('/');
    if (slash != std::string::npos) {
        connParams << "host='" << connectString.substr(0, slash) << "' "
                   << "db='" << connectString.substr(slash + 1, std::string::npos) << "'";
    }
    else {
        connParams << "db='" << connectString << "'";
    }
    connParams << " ";

    // Build connection string
    connParams << "user='" << username << "' "
               << "pass='" << password << "'";

    std::string connStr = connParams.str();

    // Connect
    static const my_bool reconnect = 1;
    for (size_t i = 0; i < poolSize; ++i) {
        soci::session& sql = connectionPool.at(i);
        sql.open(soci::mysql, connStr);

        soci::mysql_session_backend* be = static_cast<soci::mysql_session_backend*>(sql.get_backend());
        mysql_options(be->conn_, MYSQL_OPT_RECONNECT, &reconnect);
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
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
        query << " AND t_job.source_se = :source ";
        stmt.exchange(soci::use(srcSeName, "source"));
    }

    if (!destSeName.empty()) {
        query << " AND t_job.dest_se = :dest ";
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
                  << "                     t_se_vo_share.share_id = CONCAT('%\"share_id\":\"', t_job.vo_name, '\"%') "
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::getGroupCreditsInUse(int &creditsInUse,
                                    std::string srcGroupName,
                                    std::string destGroupName,
                                    std::string voName) {
    std::ostringstream query;

    try {
        soci::session sql(connectionPool);
        soci::statement stmt(sql);
        std::string groupName;

        query << "SELECT COUNT(*) FROM t_job, t_file, t_se "
                  "WHERE t_job.job_id = t_file.job_id AND "
                  "      t_file.file_state = 'ACTIVE' ";

        if (!srcGroupName.empty()) {
            query << " AND t_job.source_se = t_se.name "
                     " AND t_se.name IN (SELECT se_name FROM t_se_group_contains, t_se_group "
                     "                   WHERE se_group_name = :srcGroupName AND"
                     "                         t_se_group_contains.se_group_id = t_se_group.se_group_id) ";
            stmt.exchange(soci::use(srcGroupName, "srcGroupName"));
        }

        if (!destGroupName.empty()) {
            query << " AND t_job.dest_se = t_se.name "
                     " AND t_se.name IN (SELECT se_name FROM t_se_group_contains, t_se_group "
                     "                   WHERE se_group_name = :destGroupName AND "
                     "                         t_se_group_contains.se_group_id = t_se_group.se_group_id) ";
            stmt.exchange(soci::use(destGroupName, "destGroupName"));
        }

        if (srcGroupName.empty() || destGroupName.empty()) {
            if (!voName.empty()) {
                query << " AND t_job.vo_name = :voName ";;
                stmt.exchange(soci::use(voName, "voName"));
            }
            else {
                groupName = srcGroupName.empty() ? destGroupName : srcGroupName;
                // voName not in those who have voshare for this SE
                query << " AND NOT EXISTS ("
                         "          SELECT NULL FROM t_se_vo_share WHERE "
                         "              t_se_vo_share.se_name = :groupName AND "
                         "              t_se_vo_share.share_type = 'group' AND "
                         "              t_se_vo_share.share_id = CONCAT('%\"share_id\":\"', t_job.vo_name, '\"%') "
                         "   )";
                stmt.exchange(soci::use(groupName, "groupName"));
            }
        }

        stmt.exchange(soci::into(creditsInUse));
        stmt.alloc();
        stmt.prepare(query.str());
        stmt.define_and_bind();
        stmt.execute(true);

    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



unsigned int MySqlAPI::updateFileStatus(TransferFiles* file, const std::string status) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        soci::statement stmt(sql);

        stmt.exchange(soci::use(status, "state"));
        stmt.exchange(soci::use(file->FILE_ID, "fileId"));
        stmt.alloc();
        stmt.prepare("UPDATE t_file SET "
                     "    file_state = :state, start_time = UTC_TIMESTAMP() "
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::updateJObStatus(std::string jobId, const std::string status) {
    soci::session sql(connectionPool);

    try {
        sql.begin();

        sql << "UPDATE t_job SET job_state = :state WHERE job_id = :jobId AND job_state = 'SUBMITTED'",
                soci::use(status, "state"), soci::use(jobId, "jobId");

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
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
    const std::string params;

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
               "        :voName, UTC_TIMESTAMP(), :internalParams, :submitHost, :credId,                  "
               "        :myproxyServer, :spaceToken, :overwriteFlag, :sourceSpaceToken,               "
               "        :copyPinLifetime, :lanConnection, :failNearline, :checksumMethod,             "
               "        :reuseJob, :sourceSE, :destSE)",
               soci::use(jobId), soci::use(initialState), soci::use(paramFTP), soci::use(DN), soci::use(cred), soci::use(priority),
               soci::use(voName), soci::use(params), soci::use(currenthost), soci::use(delegationID),
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
}



std::vector<std::string> MySqlAPI::getSiteGroupNames() {
    soci::session sql(connectionPool);

    try {
        std::vector<std::string> groups;
        sql << "SELECT DISTINCT group_name FROM t_site_group", soci::into(groups);
        return groups;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
}



std::vector<std::string> MySqlAPI::getSiteGroupMembers(std::string GroupName) {
    soci::session sql(connectionPool);

    try {
        std::vector<std::string> members;
        sql << "SELECT site_name FROM t_site_group where group_name = :group",
                soci::use(GroupName), soci::into(members);
        return members;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
}



void MySqlAPI::removeGroupMember(std::string groupName, std::string siteName) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "DELETE FROM t_site_group WHERE group_name = :groupName AND site_Name = :site",
                soci::use(groupName), soci::use(siteName);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
}



void MySqlAPI::addGroupMember(std::string groupName, std::string siteName) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "INSERT INTO t_site_group (group_name, site_name) VALUES (:group, :site)",
                soci::use(groupName), soci::use(siteName);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
}



/*
 * Return a list of jobs based on the status requested
 * std::vector<JobStatus*> jobs: the caller will deallocate memory JobStatus instances and clear the vector
 * std::vector<std::string> inGivenStates: order doesn't really matter, more than one states supported
 */
void MySqlAPI::listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates,
                            std::string restrictToClientDN, std::string forDN, std::string VOname) {
    soci::session sql(connectionPool);

    try {
        std::ostringstream query;
        soci::statement stmt(sql);
        bool searchForCanceling = false;

        query << "SELECT DISTINCT job_id, job_state, reason, submit_time, user_dn, vo_name, priority, cancel_job "
                 "FROM t_job ";

        //joins
        if (!restrictToClientDN.empty()) {
            query << "LEFT OUTER JOIN t_vo_acl ON t_vo_acl.vo_name = t_job.vo_name ";
        }

        //gain the benefit from the statement pooling
        std::sort(inGivenStates.begin(), inGivenStates.end());

        if (inGivenStates.size() > 0) {
            std::vector<std::string>::const_iterator i;
            i = std::find_if(inGivenStates.begin(), inGivenStates.end(),
                             std::bind2nd(std::equal_to<std::string>(), std::string("Canceling")));
            searchForCanceling = (i != inGivenStates.end());

            std::string jobStatusesIn = "'" + inGivenStates[0] + "'";
            for (unsigned i = 1; i < inGivenStates.size(); ++i) {
                jobStatusesIn += (",'" + inGivenStates[i] + "'");
            }
            query << "WHERE job_state IN (" << jobStatusesIn << ") ";
        }
        else {
            query << "WHERE 1 ";
        }

        if (!restrictToClientDN.empty()) {
           query << " AND (t_job.user_dn = :clientDn OR t_vo_acl.principal = :clientDn) ";
           stmt.exchange(soci::use(restrictToClientDN, "clientDn"));
        }

        if (!VOname.empty()) {
            query << " AND vo_name = :vo ";
            stmt.exchange(soci::use(VOname, "vo"));
        }

        if (!forDN.empty()) {
            query << " AND user_dn = :userDn ";
            stmt.exchange(soci::use(forDN, "userDn"));
        }

        if (searchForCanceling) {
            query << " AND cancel_job = 'Y' ";
        }

        JobStatus job;
        stmt.exchange(soci::into(job));
        stmt.alloc();
        stmt.prepare(query.str());
        stmt.define_and_bind();

        if (stmt.execute(true)) {
            do {
                jobs.push_back(new JobStatus(job));
            } while (stmt.fetch());
        }

    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
}



void MySqlAPI::getTransferFileStatus(std::string requestID, std::vector<FileTransferStatus*>& files) {
    soci::session sql(connectionPool);
     try {
         soci::rowset<FileTransferStatus> rs = (sql.prepare  << "SELECT t_file.source_surl, t_file.dest_surl, t_file.file_state, "
                                                                "       t_file.reason, t_file.start_time, t_file.finish_time "
                                                                "FROM t_file WHERE t_file.job_id = :jobId",
                                                                soci::use(requestID));

         for (soci::rowset<FileTransferStatus>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
             FileTransferStatus const& transfer = *i;
             files.push_back(new FileTransferStatus(transfer));
         }

     }
     catch (std::exception& e) {
         throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
     }
}



void MySqlAPI::getSe(Se* &se, std::string seName) {
    soci::session sql(connectionPool);
    se = NULL;

    try {
        se = new Se();
        sql << "SELECT * FROM t_se WHERE name = :name",
                soci::use(seName), soci::into(*se);

        if (!sql.got_data()) {
            delete se;
            se = NULL;
        }

    }
    catch (std::exception& e) {
        delete se;
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
}



std::set<std::string> MySqlAPI::getAllMatchingSeNames(std::string name) {
    soci::session sql(connectionPool);

    try {
        std::set<std::string> matches;

        soci::rowset<std::string> rs = (sql.prepare << "SELECT t_se.name FROM t_se WHERE t_se.NAME LIKE :name",
                                        soci::use(name));

        for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
            matches.insert(*i);

        return matches;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
}



std::set<std::string> MySqlAPI::getAllMatchingSeGroupNames(std::string name) {
    soci::session sql(connectionPool);

    try {
        std::set<std::string> matches;

        soci::rowset<std::string> rs = (sql.prepare << "SELECT t_se_group.se_group_name "
                                                       "FROM t_se_group "
                                                       "WHERE t_se_group.se_group_name LIKE :name",
                                                       soci::use(name));

        for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                    matches.insert(*i);

        return matches;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
}



void MySqlAPI::getAllSeInfoNoCritiria(std::vector<Se*>& se) {
    soci::session sql(connectionPool);

    try {
        soci::rowset<Se> rs = (sql.prepare << "SELECT * FROM t_se");
        for (soci::rowset<Se>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
            se.push_back(new Se(*i));
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
}



void MySqlAPI::getAllShareConfigNoCritiria(std::vector<SeConfig*>& seConfig) {
    soci::session sql(connectionPool);

    try {
        soci::rowset<SeConfig> rs = (sql.prepare << "SELECT * FROM t_se_vo_share");
        for (soci::rowset<SeConfig>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
            seConfig.push_back(new SeConfig(*i));
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
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
            query << " WHERE t_se_vo_share.se_name LIKE :se";
            stmt.exchange(soci::use(SE_NAME, "se"));
        }

        if (SHARE_ID.length() > 0) {
            if (first)
                query << " WHERE ";
            else
                query << " AND ";
            first = false;
            query << "t_se_vo_share.share_id LIKE :shareId";
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

        if (stmt.execute(true)) {
            do {
                seAndConfig.push_back(new SeAndConfig(seandconf));
            } while (stmt.fetch());
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
                     std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "INSERT INTO t_se (endpoint, se_type, site, name, state, version, host, se_transfer_type, "
               "                  se_transfer_protocol, se_control_protocol, gocdb_id) VALUES "
               "                 (:endpoint, :seType, :site, :name, :state, :version, :host, :seTransferType, "
               "                  :seTransferProtocol, :seControlProtocol, :gocdbId)",
               soci::use(ENDPOINT), soci::use(SE_TYPE), soci::use(SITE), soci::use(NAME), soci::use(STATE), soci::use(VERSION),
               soci::use(HOST), soci::use(SE_TRANSFER_TYPE), soci::use(SE_TRANSFER_PROTOCOL), soci::use(SE_CONTROL_PROTOCOL),
               soci::use(GOCDB_ID);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }

}



void MySqlAPI::updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
                        std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID) {
    soci::session sql(connectionPool);

    try {
        sql.begin();

        std::ostringstream query;
        soci::statement stmt(sql);

        query << "UPDATE t_se SET ";

        if (ENDPOINT.length() > 0) {
            query << "ENDPOINT=:endpoint,";
            stmt.exchange(soci::use(ENDPOINT, "endpoint"));
        }

        if (SE_TYPE.length() > 0) {
            query << " SE_TYPE = :seType,";
            stmt.exchange(soci::use(SE_TYPE, "seType"));
        }

        if (SITE.length() > 0) {
            query << " SITE = :site,";
            stmt.exchange(soci::use(SITE, "site"));
        }

        if (STATE.length() > 0) {
            query << " STATE = :state,";
            stmt.exchange(soci::use(STATE, "state"));
        }

        if (VERSION.length() > 0) {
            query << " VERSION = :version,";
            stmt.exchange(soci::use(VERSION, "version"));
        }

        if (HOST.length() > 0) {
            query << " HOST = :host,";
            stmt.exchange(soci::use(HOST, "host"));
        }

        if (SE_TRANSFER_TYPE.length() > 0) {
            query << " SE_TRANSFER_TYPE = :transferType,";
            stmt.exchange(soci::use(SE_TRANSFER_TYPE, "transferType"));
        }

        if (SE_TRANSFER_PROTOCOL.length() > 0) {
            query << " SE_TRANSFER_PROTOCOL = :transferProtocol,";
            stmt.exchange(soci::use(SE_TRANSFER_PROTOCOL, "transferProtocol"));
        }

        if (SE_CONTROL_PROTOCOL.length() > 0) {
            query << " SE_CONTROL_PROTOCOL = :controlProtocol,";
            stmt.exchange(soci::use(SE_CONTROL_PROTOCOL, "controlProtocol"));
        }

        if (GOCDB_ID.length() > 0) {
            query << " GOCDB_ID = :gocdbId,";
            stmt.exchange(soci::use(GOCDB_ID, "gocdbId"));
        }

        // There is always a comma at the end, so truncate
        std::string queryStr = query.str();
        query.str(queryStr.substr(0, queryStr.length() - 1));

        query << " WHERE name = :name";
        stmt.exchange(soci::use(NAME, "name"));

        stmt.alloc();
        stmt.prepare(query.str());
        stmt.define_and_bind();
        stmt.execute(true);

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::addSeConfig(std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE, std::string SHARE_VALUE) {
    soci::session sql(connectionPool);
    try {
        sql.begin();

        sql << "INSERT INTO t_se_vo_share (se_name, share_id, share_type, share_value) VALUES "
               "                          (:seName, :shareId, :shareType, :shareValue)",
               soci::use(SE_NAME), soci::use(SHARE_ID), soci::use(SHARE_TYPE), soci::use(SHARE_VALUE);

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



/*
Update the config of an SE
REQUIRED: SE_NAME / VO_NAME
OPTIONAL; the rest
set int to -1 so as NOT to be changed
 */
void MySqlAPI::updateSeConfig(std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE, std::string SHARE_VALUE) {
    // Do not bother if it is empty
    if (SHARE_VALUE.empty())
        return;

    soci::session sql(connectionPool);
    try {
        sql.begin();
        sql << "UPDATE t_se_vo_share SET share_value = :shareValue WHERE "
               "    share_id = :shareId AND se_name = :seName AND share_type = :shareType",
               soci::use(SHARE_VALUE), soci::use(SHARE_ID), soci::use(SE_NAME), soci::use(SHARE_TYPE);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



/*
Delete a SE
REQUIRED: NAME
 */
void MySqlAPI::deleteSe(std::string NAME) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "DELETE FROM t_se WHERE name = :name", soci::use(NAME);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



/*
Delete the config of an SE for a specific VO
REQUIRED: SE_NAME
OPTIONAL: VO_NAME
 */
void MySqlAPI::deleteSeConfig(std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "DELETE FROM t_se_vo_share WHERE "
               "    se_name LIKE :seName AND "
               "    share_id LIKE :shareId AND "
               "    share_type = :shareType",
               soci::use(SE_NAME), soci::use(SHARE_ID), soci::use(SHARE_TYPE);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::updateFileTransferStatus(std::string /*job_id*/, std::string file_id, std::string transfer_status, std::string transfer_message, int process_id,
                                        double filesize, double duration) {
    soci::session sql(connectionPool);

    try {
        double throughput;

        sql.begin();

        soci::statement stmt(sql);
        std::ostringstream query;

        query << "UPDATE t_file SET "
                 "    file_state = :state, reason = :reason";
        stmt.exchange(soci::use(transfer_status, "state"));
        stmt.exchange(soci::use(transfer_message, "reason"));

        if (transfer_status == "FINISHED" || transfer_status == "FAILED" || transfer_status == "CANCELED") {
            query << ", FINISH_TIME = UTC_TIMESTAMP()";
        }
        if (transfer_status == "ACTIVE") {
            query << ", START_TIME = UTC_TIMESTAMP()";
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::updateJobTransferStatus(std::string /*file_id*/, std::string job_id, const std::string status) {
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
                   "    job_state = :state, job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), reason = :reason "
                   "WHERE job_id = :jobId AND job_state = 'ACTIVE'",
                   soci::use(state, "state"), soci::use(reason, "reason"),
                   soci::use(job_id, "jobId");

            // And file finish timestamp
            sql << "UPDATE t_file SET job_finished = UTC_TIMESTAMP() WHERE job_id = :jobId",
                    soci::use(job_id, "jobId");
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::cancelJob(std::vector<std::string>& requestIDs) {
    soci::session sql(connectionPool);

    try {
        const std::string reason = "Job canceled by the user";

        sql.begin();

        for (std::vector<std::string>::const_iterator i = requestIDs.begin(); i != requestIDs.end(); ++i) {
            // Cancel job
            sql << "UPDATE t_job SET job_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                   "                 reason = :reason "
                   "WHERE job_id = :jobId AND job_state NOT IN ('FINISHEDDIRTY', 'FINISHED', 'FAILED')",
                   soci::use(reason, "reason"), soci::use(*i, "jobId");
            // Cancel files
            sql << "UPDATE t_file SET file_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                   "                  reason = :reason "
                   "WHERE job_id = :jobId AND file_state NOT IN ('FINISHEDDIRTY','FINISHED','FAILED')",
                   soci::use(reason, "reason"), soci::use(*i, "jobId");
        }

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }


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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



bool MySqlAPI::is_se_group_exist(std::string group) {
    soci::session sql(connectionPool);

    try {
        unsigned count = 0;
        sql << "SELECT count(se_group_name) FROM t_se_group WHERE se_group_name LIKE :group",
                soci::use(group, "group"), soci::into(count);

        return count > 0;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



bool MySqlAPI::is_group_protocol_exist(std::string group) {
    soci::session sql(connectionPool);

    try {
        unsigned count = 0;
        sql << "SELECT se_group_name FROM t_se_protocol WHERE se_group_name = :group",
                soci::use(group, "group"), soci::into(count);

        return count > 0;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
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
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



SeProtocolConfig* MySqlAPI::get_se_group_protocol_config(std::string se) {
    soci::session sql(connectionPool);

    try {
        SeProtocolConfig protoConfig;
        sql << "SELECT * FROM t_se_protocol WHERE se_name = :se",
                soci::use(se, "se"), soci::into(protoConfig);
        return new SeProtocolConfig(protoConfig);
    }
    catch (std::exception& e) {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



SeProtocolConfig* MySqlAPI::get_group_protocol_config(std::string group) {
    soci::session sql(connectionPool);

    try {
        SeProtocolConfig protoConfig;
        sql << "SELECT * FROM t_se_protocol WHERE se_group_name = :group",
                soci::use(group, "group"), soci::into(protoConfig);
        return new SeProtocolConfig(protoConfig);
    }
    catch (std::exception& e) {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



bool MySqlAPI::add_se_protocol_config(SeProtocolConfig* seProtocolConfig) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        SeProtocolConfig protoConfig;
        sql << "INSERT INTO t_se_protocol (se_name, nostreams, urlcopy_tx_to) VALUES "
               "                          (:seName, :nostreams, :urlCopyTxTo)",
                soci::use(seProtocolConfig->SE_NAME),
                soci::use(seProtocolConfig->NOSTREAMS),
                soci::use(seProtocolConfig->URLCOPY_TX_TO);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
    return true;
}



bool MySqlAPI::add_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        SeProtocolConfig protoConfig;
        sql << "INSERT INTO t_se_protocol (se_group_name, nostreams, urlcopy_tx_to) VALUES"
               "                          (:seGroupName, :nostreams, :urlCopyTxTo)",
                soci::use(seGroupProtocolConfig->SE_GROUP_NAME),
                soci::use(seGroupProtocolConfig->NOSTREAMS),
                soci::use(seGroupProtocolConfig->URLCOPY_TX_TO);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
    return true;
}



void MySqlAPI::delete_se_protocol_config(SeProtocolConfig* seProtocolConfig) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "DELETE FROM t_se_protocol WHERE se_name LIKE :seName AND se_group_name IS NULL",
                soci::use(seProtocolConfig->SE_NAME);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::delete_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "DELETE FROM t_se_protocol WHERE se_group_name LIKE :seGroupName",
                soci::use(seGroupProtocolConfig->SE_GROUP_NAME);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::update_se_protocol_config(SeProtocolConfig* seProtocolConfig) {
    soci::session sql(connectionPool);

    try {
        sql.begin();

        soci::statement stmt(sql);
        std::ostringstream query;
        unsigned index = 0;

        query << "UPDATE t_se_protocol SET ";
        if (seProtocolConfig->NOSTREAMS > 0) {
            query << " nostreams = :nostreams ";
            stmt.exchange(soci::use(seProtocolConfig->NOSTREAMS, "nostreams"));
            ++index;
        }

        if (seProtocolConfig->URLCOPY_TX_TO > 0) {
            if (index) query << ", ";
            query << " urlcopy_tx_to = :urlCopyTxTo ";
            stmt.exchange(soci::use(seProtocolConfig->URLCOPY_TX_TO, "urlCopyTxTo"));
            ++index;
        }

        query << "WHERE se_name = :seName";
        stmt.exchange(soci::use(seProtocolConfig->SE_NAME, "seName"));

        stmt.alloc();
        stmt.prepare(query.str());
        stmt.define_and_bind();
        stmt.execute(true);

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::update_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig) {
    soci::session sql(connectionPool);

    try {
        sql.begin();

        soci::statement stmt(sql);
        std::ostringstream query;
        unsigned index = 0;

        query << "UPDATE t_se_protocol SET ";
        if (seGroupProtocolConfig->NOSTREAMS > 0) {
            query << " nostreams = :nostreams ";
            stmt.exchange(soci::use(seGroupProtocolConfig->NOSTREAMS, "nostreams"));
            ++index;
        }

        if (seGroupProtocolConfig->URLCOPY_TX_TO > 0) {
            if (index) query << ", ";
            query << " urlcopy_tx_to = :urlCopyTxTo ";
            stmt.exchange(soci::use(seGroupProtocolConfig->URLCOPY_TX_TO, "urlCopyTxTo"));
            ++index;
        }

        query << "WHERE se_group_name = :seGroupName";
        stmt.exchange(soci::use(seGroupProtocolConfig->SE_GROUP_NAME, "seGroupName"));

        stmt.alloc();
        stmt.prepare(query.str());
        stmt.define_and_bind();
        stmt.execute(true);

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
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
    soci::session sql(connectionPool);

    try {
        sql.begin();

        if (!is_se_group_exist(group)) {
            sql << "INSERT INTO t_se_group (se_group_name) VALUES (:group)",
                    soci::use(group);
        }

        std::string groupId;
        sql << "SELECT se_group_id FROM t_se_group WHERE se_group_name = :group",
                soci::use(group), soci::into(groupId);


        sql << "INSERT INTO t_se_group (se_group_id, se_name) VALUES (:seGroupId, :seName)",
                soci::use(groupId), soci::use(se);

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::remove_se_from_group(std::string se, std::string group) {
    soci::session sql(connectionPool);

    try {
        sql.begin();

        std::string groupId;
        sql << "SELECT se_group_id FROM t_se_group WHERE se_group_name = :group",
                soci::use(group), soci::into(groupId);

        sql << "DELETE FROM t_se_group_contains WHERE se_name = :seName AND se_group_id = :seGroupId",
                soci::use(se), soci::use(groupId);

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::delete_group(std::string group) {
    soci::session sql(connectionPool);

    try {
        sql.begin();

        std::string groupId;
        sql << "SELECT se_group_id FROM t_se_group WHERE se_group_name = :group",
                soci::use(group), soci::into(groupId);

        sql << "DELETE FROM t_se_group_contains WHERE se_group_id = :groupId",
                soci::use(groupId);
        sql << "DELETE FROM t_se_group WHERE se_group_id = :groupId",
                soci::use(groupId);

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
    return group;
}



std::vector<std::string> MySqlAPI::get_group_names() {
    soci::session sql(connectionPool);

    try {
        std::vector<std::string> groups;

        soci::rowset<std::string> rs = (sql.prepare << "SELECT DICTINCT se_group_name FROM t_se_group");
        for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
            groups.push_back(*i);

        return groups;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



std::vector<std::string> MySqlAPI::get_group_members(std::string name) {
    soci::session sql(connectionPool);

    try {
        std::vector<std::string> members;

        soci::rowset<std::string> rs = (sql.prepare << "SELECT DICTINCT se_name "
                                        "FROM t_se_group_contains, t_se_group "
                                        "WHERE t_se_group_contains.se_group_id = t_se_group.se_group_id AND "
                                        "      t_se_group.se_group_name = :group",
                                        soci::use(name));
        for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
            members.push_back(*i);

        return members;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }

    return cred;
}



void MySqlAPI::deleteGrDPStorageCacheElement(std::string delegationID, std::string dn) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "DELETE FROM t_credential_cache WHERE dlg_id = :dlgId AND dn = :dn",
                soci::use(delegationID), soci::use(dn);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }

    return cred;
}



void MySqlAPI::deleteGrDPStorageElement(std::string delegationID, std::string dn) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "DELETE FROM t_credential WHERE dlg_id = :dlgId AND dn = :dn",
                soci::use(delegationID), soci::use(dn);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
}



void MySqlAPI::setDebugMode(std::string source_hostname, std::string destin_hostname, std::string mode) {
    soci::session sql(connectionPool);

    try {
        sql.begin();

        if (destin_hostname.length() == 0) {
            sql << "DELETE FROM t_debug WHERE source_se = :source AND dest_se IS NULL",
                    soci::use(source_hostname);
            sql << "INSERT INTO t_debug (source_se, debug) VALUES (:source, :debug)",
                    soci::use(source_hostname), soci::use(mode);
        } else {
            sql << "DELETE FROM t_debug WHERE source_se = :source AND dest_se = :dest",
                    soci::use(source_hostname), soci::use(destin_hostname);
            sql << "INSERT INTO t_debug (source_se, dest_se, debug) VALUES (:source, :dest, :mode)",
                    soci::use(source_hostname), soci::use(destin_hostname), soci::use(mode);
        }

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
    }
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::auditConfiguration(const std::string & dn, const std::string & config, const std::string & action) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "INSERT INTO t_config_audit (datetime, dn, config, action ) VALUES "
               "                           (UTC_TIMESTAMP(), :dn, :config, :action)",
               soci::use(dn), soci::use(config), soci::use(action);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::setGroupOrSeState(const std::string & se, const std::string & group, const std::string & state) {
    soci::session sql(connectionPool);

    try {
        sql.begin();

        if (se.length() > 0) {
            sql << "UPDATE t_se SET state = :state WHERE name = :name",
                    soci::use(state), soci::use(se);
        }
        else {
            sql << "UPDATE t_se_group SET state = :state WHERE se_group_name = :name",
                    soci::use(state), soci::use(group);
        }

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



/*custom optimization stuff*/

void MySqlAPI::fetchOptimizationConfig2(OptimizerSample* ops, const std::string & source_hostname, const std::string & destin_hostname) {
    soci::session sql(connectionPool);

    try {
        unsigned numberOfTimeouts;
        unsigned numberOfRecords;
        unsigned numberOfNullThroughput;
        bool loadDefaults = false;

        // check the last records between src/dest which failed with timeout
        sql << "SELECT COUNT(*) FROM t_file, t_job WHERE "
               "    t_job.job_id = t_file.job_id AND t_file.file_state = 'FAILED' AND "
               "    t_file.reason LIKE '%operation timeout%' AND "
               "    t_job.source_se = :source AND t_job.dest_se = :dest AND"
               "    t_file.finish_time > (CURRENT_TIMESTAMP - interval '30' minute) "
               "ORDER BY t_file.finish_time DESC",
               soci::use(source_hostname), soci::use(destin_hostname),
               soci::into(numberOfTimeouts);

        sql << "SELECT COUNT(*) FROM t_optimize WHERE source_se = :source AND dest_se=:dest",
                soci::use(source_hostname), soci::use(destin_hostname),
                soci::into(numberOfRecords);

        sql << " SELECT COUNT(*) FROM t_optimize WHERE "
               "     throughput is NULL AND "
               "     source_se = :source AND dest_se = :dest AND "
               "     file_id = 0",
               soci::use(source_hostname), soci::use(destin_hostname),
               soci::into(numberOfNullThroughput);


        if (numberOfRecords > 0 && numberOfNullThroughput == 0) { //ALL records for this SE/DEST are having throughput
            unsigned numberOfActivesBetween;
            sql << "SELECT COUNT(*) FROM t_file, t_job WHERE "
                   "    t_file.file_state = 'ACTIVE' AND t_job.job_id = t_file.job_id AND "
                   "    t_job.source_se = :source AND t_job.dest_se = :dest",
                   soci::use(source_hostname), soci::use(destin_hostname),
                   soci::into(numberOfActivesBetween);

            sql << "SELECT nostreams, timeout, buffer FROM t_optimize "
                   "WHERE source_se = :source and dest_se = :dest "
                   "ORDER BY ABS(:nActives - active), ABS(500 - throughput) DESC "
                   "LIMIT 1",
                    soci::use(source_hostname), soci::use(destin_hostname), soci::use(numberOfActivesBetween),
                    soci::into(ops->streamsperfile), soci::into(ops->timeout), soci::into(ops->bufsize);

            if (sql.got_data())
                ops->file_id = 1;
            else
                loadDefaults = true;
        }
        else if (numberOfRecords > 0 && numberOfNullThroughput > 0) { //found records in the DB but are some without throughput
            sql << "SELECT nostreams, timeout, buffer FROM t_optimize "
                   "WHERE source_se = :source AND dest_se = :dest AND "
                   "      throughput IS NULL AND file_id = 0 "
                   "LIMIT 1",
                   soci::use(source_hostname), soci::use(destin_hostname),
                   soci::into(ops->streamsperfile), soci::into(ops->timeout), soci::into(ops->bufsize);

            if (sql.got_data())
                ops->file_id = 1;
            else
                loadDefaults = true;
        }
        else {
            loadDefaults = true;
        }

        // If not found, set default values
        if (loadDefaults) {
            if (numberOfTimeouts == 0) {
                ops->streamsperfile = DEFAULT_NOSTREAMS;
                ops->timeout = DEFAULT_TIMEOUT;
                ops->bufsize = DEFAULT_BUFFSIZE;
                ops->file_id = 0;
            }
            else {
                ops->streamsperfile = DEFAULT_NOSTREAMS;
                ops->timeout = MID_TIMEOUT;
                ops->bufsize = DEFAULT_BUFFSIZE;
                ops->file_id = 0;
            }
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::updateOptimizer(std::string, double filesize, int timeInSecs, int nostreams, int timeout, int buffersize, std::string source_hostname, std::string destin_hostname) {
    soci::session sql(connectionPool);

    try {
        bool activeExists;
        int active;
        double throughput;

        sql << "SELECT active FROM t_optimize WHERE "
               "    active = (SELECT COUNT(*) FROM t_file, t_job WHERE "
               "                  t_file.file_state = 'ACTIVE' AND "
               "                  t_file.job_id = t_job.job_id AND "
               "                  t_job.source_se = :source AND t_job.dest_se = :dest) AND "
               "    nostreams = :nstreams AND timeout = :timeout AND buffer = :buffer AND "
               "    source_se = :source AND dest_se = :dest",
               soci::use(source_hostname, "source"), soci::use(destin_hostname, "dest"),
               soci::use(nostreams, "nstreams"), soci::use(timeout, "timeout"),
               soci::use(buffersize, "buffer"),
               soci::into(active);
        activeExists = sql.got_data();

        if (filesize > 0 && timeInSecs > 0)
            throughput = convertBtoM(filesize, timeInSecs);
        else
            throughput = convertBtoM(filesize, 1);

        if (filesize <= 0)
            filesize = 0;

        if (nostreams <= 0)
            nostreams = DEFAULT_NOSTREAMS;

        if (buffersize <= 0)
            buffersize = 0;

        if (activeExists) {
            int newTimeout = timeout;
            if (timeInSecs <= DEFAULT_TIMEOUT)
                newTimeout = DEFAULT_TIMEOUT;

            sql.begin();
            sql << "UPDATE t_optimize SET "
                   "    filesize = :filesize, throughput = :throughput, active = :active, "
                   "    datetime = UTC_TIMESTAMP(), timeout = :timeout "
                   "WHERE nostreams = :nstreams AND timeout = :timeout AND buffer = :buffer AND "
                   "      source_se = :source AND dest_se = :dest",
                    soci::use(filesize), soci::use(throughput), soci::use(active),
                    soci::use(newTimeout),
                    soci::use(nostreams), soci::use(timeout), soci::use(buffersize),
                    soci::use(source_hostname), soci::use(destin_hostname);
            sql.commit();
        }
        else {
                if (timeInSecs <= DEFAULT_TIMEOUT)
                    timeout = DEFAULT_TIMEOUT;
                addOptimizer(time(NULL), throughput, source_hostname, destin_hostname,
                             1, nostreams, timeout, buffersize, active);
        }
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::addOptimizer(time_t when, double throughput, const std::string & source_hostname, const std::string & destin_hostname,
                            int file_id, int nostreams, int timeout, int buffersize, int) {
    soci::session sql(connectionPool);

    try {
        struct tm timest;
        gmtime_r(&when, &timest);
        unsigned actives = 0;

        sql << "SELECT COUNT(*) FROM t_file, t_job WHERE "
               "    t_file.file_state = 'ACTIVE' AND t_job.job_id = t_file.job_id AND "
               "    t_job.source_se = :source AND t_job.dest_se = :dest",
               soci::use(source_hostname), soci::use(destin_hostname),
               soci::into(actives);

        sql.begin();

        sql << "INSERT INTO t_optimize (file_id, source_se, dest_se, nostreams, timeout, active, buffer, throughput, datetime) "
               "                VALUES (:fileId, :sourceSe, :destSe, :nStreams, :timeout, :active, :buffer, :throughput, :datetime)",
               soci::use(file_id), soci::use(source_hostname), soci::use(destin_hostname), soci::use(nostreams), soci::use(timeout),
               soci::use(actives), soci::use(buffersize), soci::use(throughput), soci::use(timest);

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::initOptimizer(const std::string & source_hostname, const std::string & destin_hostname, int) {
    soci::session sql(connectionPool);

    try {
        unsigned foundRecords = 0;
        sql.begin();

        sql << "SELECT COUNT(*) FROM t_optimize WHERE source_se = :source AND dest_se=:dest",
                soci::use(source_hostname), soci::use(destin_hostname),
                soci::into(foundRecords);

        if (foundRecords == 0) {
            int timeout, nStreams, bufferSize;

            soci::statement stmt = (sql.prepare << "INSERT INTO t_optimize (source_se, dest_se, timeout, nostreams, buffer, file_id) "
                                                   "                VALUES (:source, :dest, :timeout, :nostreams, :buffer, 0)",
                                                   soci::use(source_hostname), soci::use(destin_hostname), soci::use(timeout),
                                                   soci::use(nStreams), soci::use(bufferSize));

            for (unsigned register int x = 0; x < timeoutslen; x++) {
                for (unsigned register int y = 0; y < nostreamslen; y++) {
                    for (unsigned register int z = 0; z < buffsizeslen; z++) {
                        timeout    = timeouts[x];
                        nStreams   = nostreams[y];
                        bufferSize = buffsizes[z];
                        stmt.execute(true);
                    }
                }
            }
        }

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



bool MySqlAPI::isTrAllowed(const std::string & source_hostname, const std::string & destin_hostname) {
    soci::session sql(connectionPool);
    bool allowed = false;

    try {
        int numberOfActiveTransfersBetween;
        int numberOfReadyOrActiveTransfersBetween;
        int numberOfActiveTransfersToDest;
        int numberOfFailedBetween;
        double averageThroughput;

        sql << "SELECT COUNT(*) FROM t_file, t_job "
               "WHERE t_file.file_state = 'ACTIVE' AND "
               "    t_job.job_id = t_file.job_id AND "
               "    t_job.source_se = :sourceSe and t_job.dest_se = :destSe ",
               soci::use(source_hostname), soci::use(destin_hostname),
               soci::into(numberOfActiveTransfersBetween);

        sql << "SELECT COUNT(*) FROM t_file, t_job "
               "WHERE t_file.file_state IN ('ACTIVE', 'READY') AND "
               "      t_job.job_id = t_file.job_id AND "
               "      t_job.source_se = :sourceSe AND "
               "      t_job.dest_se = :destSe",
               soci::use(source_hostname), soci::use(destin_hostname),
               soci::into(numberOfReadyOrActiveTransfersBetween);

        sql << "SELECT COALESCE(AVG(throughput), 0) FROM t_optimize "
               "WHERE (active = :nActiveTransfersBetween OR "
               "       active = (SELECT MAX(active) FROM t_optimize WHERE active < :nActiveOrReady)) AND "
                "     source_se = :sourceSe  AND "
                "     dest_se = :destSe AND "
                "     t_optimize.datetime IS NOT NULL "
                "ORDER BY datetime DESC",
                soci::use(numberOfActiveTransfersBetween), soci::use(numberOfReadyOrActiveTransfersBetween),
                soci::use(source_hostname), soci::use(source_hostname),
                soci::into(averageThroughput);

        sql << "SELECT COUNT(*) FROM t_file, t_job "
               "WHERE t_file.file_state = 'ACTIVE' AND "
               "      t_job.job_id = t_file.job_id AND "
               "      t_job.dest_se = :destSe",
               soci::use(destin_hostname), soci::into(numberOfActiveTransfersToDest);

        sql << "SELECT COUNT(*) FROM t_file, t_job "
               "WHERE t_job.job_id = t_file.job_id AND "
               "      t_job.source_se = :sourceSe AND "
               "      t_job.dest_se = :destSe AND "
               "      t_file.finish_time IS NOT NULL AND "
               "      t_file.file_state = 'FAILED' "
               "LIMIT 10",
               soci::use(destin_hostname), soci::use(destin_hostname), soci::into(numberOfFailedBetween);

        if (numberOfFailedBetween == 0) { //no failures in the last transfers
            if (numberOfActiveTransfersBetween == 0 && numberOfActiveTransfersToDest <= 30) {
                allowed = true;
            } else if (numberOfActiveTransfersBetween > 0 && numberOfActiveTransfersToDest <= 30 && numberOfActiveTransfersBetween <= 20) {
                allowed = true;
            } else if (averageThroughput > 1 && numberOfActiveTransfersToDest <= 30 && numberOfActiveTransfersBetween <= 20) {
                allowed = true;
            } else if (averageThroughput < 1 && numberOfActiveTransfersToDest > 30 && numberOfActiveTransfersBetween <= 20) {
                allowed = true;
            } else if (averageThroughput < 1 && numberOfActiveTransfersToDest > 30 && numberOfActiveTransfersBetween > 20) {
                allowed = false;
            } else if (averageThroughput < 1 && numberOfActiveTransfersToDest > 5 && numberOfActiveTransfersBetween > 5) {
                allowed = false;
            }
            else {
                allowed = true;
                return allowed;
            }
        } else { //failures
            if (numberOfFailedBetween > 4 && numberOfActiveTransfersBetween > 5) {
                allowed = false;
            } else if (numberOfFailedBetween < 4 && numberOfActiveTransfersBetween > 8) {
                allowed = false;
            }
            else if (averageThroughput > 1 && numberOfActiveTransfersToDest <= 20 && numberOfActiveTransfersBetween < 6) {
                allowed = true;
            } else if (averageThroughput < 1 && numberOfActiveTransfersToDest <= 20 && numberOfActiveTransfersBetween < 4) {
                allowed = true;
            } else if (averageThroughput < 1 && numberOfActiveTransfersToDest > 10 && numberOfActiveTransfersBetween > 5) {
                allowed = false;
            } else {
                allowed = true;
            }
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }

    return allowed;
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}


/* REUSE CASE ???*/
void MySqlAPI::forceFailTransfers() {
    soci::session sql(connectionPool);

    try {
        std::string jobId, params;
        int fileId, pid, timeout;
        struct tm startTimeSt;
        time_t lifetime = time(NULL);
        time_t startTime;
        double diff;

        soci::statement stmt = (sql.prepare << "SELECT job_id, file_id, start_time ,pid, internal_file_params "
                                               "FROM t_file "
                                               "WHERE file_state = 'ACTIVE' AND pid IS NOT NULL",
                                               soci::into(jobId), soci::into(fileId), soci::into(startTimeSt),
                                               soci::into(pid), soci::into(params));

        if (stmt.execute(true)) {
            do {
                startTime = mktime(&startTimeSt);
                timeout = extractTimeout(params);

                diff = difftime(lifetime, startTime);
                if (timeout != 0 && diff > (timeout + 1000)) {
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Killing pid:" << pid << ", jobid:" << jobId << ", fileid:" << fileId << " because it was stalled" << commit;
                    std::stringstream ss;
                    ss << fileId;
                    kill(pid, SIGUSR1);
                    updateFileTransferStatus(jobId, ss.str(),
                                             "FAILED", "Transfer has been forced-killed because it was stalled",
                                             pid, 0, 0);
                    updateJobTransferStatus(ss.str(), jobId, "FAILED");
                }

            } while (stmt.fetch());
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::setAllowed(const std::string & job_id, int file_id, const std::string & source_se, const std::string & dest,
                          int nostreams, int timeout, int buffersize) {
    soci::session sql(connectionPool);

    try {
        sql.begin();

        sql << "UPDATE t_optimize SET "
               "    file_id = 1 "
               "WHERE nostreams = :nStreams AND buffer = :bufferSize AND timeout = :timeout AND "
               "      source_se = :source AND dest_se = :dest",
               soci::use(nostreams), soci::use(buffersize), soci::use(timeout),
               soci::use(source_se), soci::use(dest);

        std::stringstream params;
        params << "nostreams:" << nostreams << ",timeout:" << timeout << ",buffersize:" << buffersize;

        if (file_id != -1) {
            sql << "UPDATE t_file SET internal_file_params = :params WHERE file_id = :fileId AND job_id = :jobId",
                    soci::use(params.str()), soci::use(file_id), soci::use(job_id);
        } else {
            sql << "UPDATE t_file SET internal_file_params = :params WHERE job_id = :jobId",
                    soci::use(params.str()), soci::use(job_id);
        }

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::terminateReuseProcess(const std::string & jobId) {
    soci::session sql(connectionPool);

    try {
        sql.begin();

        std::string reuse;
        sql << "SELECT reuse_job FROM t_job WHERE job_id = :jobId AND reuse_job IS NOT NULL",
                soci::use(jobId), soci::into(reuse);

        if (sql.got_data()) {
            sql << "UPDATE t_file SET file_state = 'FAILED' WHERE job_id = :jobId AND file_state != 'FINISHED'",
                    soci::use(jobId);
        }

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
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
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::setPidV(int pid, std::map<int, std::string>& pids) {
    soci::session sql(connectionPool);

    try {
        sql.begin();
        std::string jobId;
        int fileId;
        soci::statement stmt = (sql.prepare << "UPDATE t_file SET pid = :pid WHERE job_id = :jobId AND file_id = :fileId",
                                               soci::use(pid), soci::use(jobId), soci::use(fileId));

        for (std::map<int, std::string>::const_iterator i = pids.begin(); i != pids.end(); ++i) {
            fileId = i->first;
            jobId  = i->second;
            stmt.execute(true);
        }

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::revertToSubmitted(){
    soci::session sql(connectionPool);

    try {
        struct tm startTime;
        int fileId;
        std::string jobId, reuseJob;
        time_t now = time(NULL);

        sql.begin();

        soci::indicator reuseInd;
        soci::statement readyStmt = (sql.prepare << "SELECT t_file.start_time, t_file.file_id, t_file.job_id, t_job.reuse_job "
                                                    "FROM t_file, t_job "
                                                    "WHERE t_file.file_state = 'READY' AND t_file.finish_time IS NULL AND "
                                                    "      t_file.job_finished IS NULL AND t_file.job_id = t_job.job_id",
                                                    soci::into(startTime), soci::into(fileId), soci::into(jobId), soci::into(reuseJob, reuseInd));

        if (readyStmt.execute(true)) {
            do {
                time_t startTimestamp = mktime(&startTime);
                double diff = difftime(now, startTimestamp);

                if (diff > 15000) { /*~5h*/
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "The transfer with file id " << fileId << " seems to be stalled, restart it" << commit;

                    sql << "UPDATE t_file SET file_state = 'SUBMITTED' "
                           "WHERE file_state = 'READY' AND finish_time IS NULL AND "
                           "      job_finished IS NULL AND file_id = :fileId",
                           soci::use(fileId);

                    if (reuseJob == "Y") {
                        sql << "UPDATE t_job SET job_state='SUBMITTED' WHERE job_state IN ('READY','ACTIVE') AND "
                               "    finish_time IS NULL AND job_finished IS NULL AND reuse_job = 'Y' AND "
                               "    job_id = :jobId",
                               soci::use(jobId);
                    }
                }

            } while (readyStmt.fetch());
        }

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::revertToSubmittedTerminate(){
    soci::session sql(connectionPool);

    try {
        sql.begin();

        sql << "UPDATE t_file SET file_state = 'SUBMITTED' "
               "WHERE file_state = 'READY' AND finish_time IS NULL AND job_finished IS NULL";

        sql << "UPDATE t_job SET job_state = 'SUBMITTED' WHERE job_state IN ('READY','ACTIVE') AND "
                "   finish_time IS NULL AND job_finished IS NULL AND reuse_job = 'Y' AND "
                "   job_id IN (SELECT DISTINCT t_file.job_id FROM t_file "
                "              WHERE t_file.job_id = t_job.job_id AND t_file.file_state = 'READY')";

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



bool MySqlAPI::configExists(const std::string & src, const std::string & dest, const std::string & vo) {
    soci::session sql(connectionPool);

    try {
        unsigned count;
        std::string srcGroup, destGroup;
        std::string se1, se2, shareType;

        // VO shares
        soci::statement stmt = (sql.prepare << "SELECT COUNT(*) FROM t_se_vo_share "
                                               "WHERE (se_name = :se1 AND share_type = :shareType AND "
                                               "       (share_id = '\"type\":\"public\"' OR "
                                               "        share_id = CONCAT('\"type\":\"vo\",\"id\":\"', :vo, '\"') OR "
                                               "        share_id = CONCAT('\"type\":\"pair\",\"id\":\"', :se2, '\"')))",
                                soci::use(se1), soci::use(shareType), soci::use(vo), soci::use(se2), soci::into(count));

        shareType = "se";
        {
            count = 0;
            se1 = src;
            se2 = dest;
            stmt.execute(true);
            if (count) return true;

            count = 0;
            se1 = dest;
            se2 = src;
            stmt.execute(true);
            if (count) return true;
        }

        shareType = "group";
        {
            // Get the groups names
            sql << "SELECT se_group_name FROM t_se_group, t_se_group_contains "
                   "WHERE t_se_group_contains.se_group_id = t_se_group.se_group_id AND "
                   "      se_name = :seName", soci::use(src), soci::into(srcGroup);
            sql << "SELECT se_group_name FROM t_se_group, t_se_group_contains "
                   "WHERE t_se_group_contains.se_group_id = t_se_group.se_group_id AND "
                   "      se_name = :seName", soci::use(dest), soci::into(destGroup);

            if (!srcGroup.empty() && !destGroup.empty()) {
                count = 0;
                se1 = srcGroup;
                se2 = destGroup;
                stmt.execute(true);
                if (count) return true;

                count = 0;
                se1 = destGroup;
                se2 = srcGroup;
                stmt.execute(true);
                if (count) return true;
            }
        }

        // No luck with the vo shares. Try with the protocol
        sql << "SELECT COUNT(*) FROM t_se_protocol WHERE "
               "    se_name = :source OR se_name = :dest OR "
               "    se_group_name = :sourceGroup OR se_group_name = :destGroup",
               soci::use(src), soci::use(dest), soci::use(srcGroup), soci::use(destGroup),
               soci::into(count);
        if (count) return true;

        return false;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::backup(){
    soci::session sql(connectionPool);

    try {
        sql.begin();

        sql << "INSERT INTO t_job_backup SELECT * FROM t_job "
               "WHERE job_state IN ('FINISHED', 'FAILED', 'CANCELED', 'FINISHEDDIRTY') AND "
               "      job_finished < (CURRENT_TIMESTAMP() - interval '7' DAY )";

        sql << "INSERT INTO t_file_backup SELECT * FROM t_file WHERE job_id IN (SELECT job_id FROM t_job_backup)";
        sql << "DELETE FROM t_file WHERE file_id IN (SELECT file_id FROM t_file_backup)";
        sql << "DELETE FROM t_job WHERE job_id IN (SELECT job_id FROM t_job_backup)";

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::forkFailedRevertState(const std::string & jobId, int fileId){
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "UPDATE t_file SET file_state = 'SUBMITTED' "
               "WHERE file_id = :fileId AND job_id = :jobId",
               soci::use(fileId), soci::use(jobId);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::forkFailedRevertStateV(std::map<int,std::string>& pids){
    std::string query = "update t_file set file_state='SUBMITTED' where file_id=:1 and job_id=:2";
    soci::session sql(connectionPool);

    try {
        int fileId;
        std::string jobId;

        sql.begin();
        soci::statement stmt = (sql.prepare << "UPDATE t_file SET file_state = 'SUBMITTED'"
                                               "WHERE file_id = :fileId AND job_id = :jobId",
                                               soci::use(fileId), soci::use(jobId));

        for (std::map<int, std::string>::const_iterator i = pids.begin(); i != pids.end(); ++i) {
            fileId = i->first;
            jobId  = i->second;
            stmt.execute(true);
        }

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}


TransferJobs* MySqlAPI::getTransferJob(std::string jobId) {
    soci::session sql(connectionPool);

    try {
        TransferJobs* job = new TransferJobs();

        sql << "SELECT t_job.vo_name, t_job.user_dn "
               "FROM t_job WHERE t_job.job_id = ::jobId",
               soci::use(jobId),
               soci::into(job->VO_NAME), soci::into(job->USER_DN);

        if (!sql.got_data()) {
            delete job;
            job = NULL;
        }

        return job;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}

void MySqlAPI::retryFromDead(std::map<int,std::string>& pids){
    soci::session sql(connectionPool);

    try {
        int fileId;
        std::string jobId;

        sql.begin();

        soci::statement stmt = (sql.prepare << "UPDATE t_file SET file_state = 'SUBMITTED' "
                                               "WHERE file_id = :fileId AND"
                                               "      job_id = :jobId AND "
                                               "      file_state NOT IN ('FINISHED','FAILED','CANCELED')",
                                               soci::use(fileId), soci::use(jobId));
        for (std::map<int, std::string>::const_iterator i = pids.begin(); i != pids.end(); ++i) {
            fileId = i->first;
            jobId  = i->second;
            stmt.execute(true);
        }

        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::blacklistSe(std::string se, std::string msg, std::string adm_dn)
{
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "INSERT INTO t_bad_ses (se, message, addition_time, admin_dn) "
               "               VALUES (:se, :message, UTC_TIMESTAMP(), :admin)",
               soci::use(se), soci::use(msg), soci::use(adm_dn);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::blacklistDn(std::string dn, std::string msg, std::string adm_dn)
{
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "INSERT INTO t_bad_dns (dn, message, addition_time, admin_dn) "
               "               VALUES (:dn, :message, UTC_TIMESTAMP(), :admin)",
               soci::use(dn), soci::use(msg), soci::use(adm_dn);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::unblacklistSe(std::string se)
{
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "DELETE FROM t_bad_ses WHERE se = :se",
               soci::use(se);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



void MySqlAPI::unblacklistDn(std::string dn)
{
    soci::session sql(connectionPool);

    try {
        sql.begin();
        sql << "DELETE FROM t_bad_dns WHERE dn = :dn",
               soci::use(dn);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



bool MySqlAPI::isSeBlacklisted(std::string se)
{
    soci::session sql(connectionPool);

    try {
        sql << "SELECT * FROM t_bad_ses WHERE se = :se", soci::use(se);
        return sql.got_data();
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
}



bool MySqlAPI::isDnBlacklisted(std::string dn)
{
    soci::session sql(connectionPool);

    try {
        sql << "SELECT * FROM t_bad_dns WHERE dn = :dn", soci::use(dn);
        return sql.got_data();
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
    }
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
