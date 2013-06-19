/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 */

#include <boost/lexical_cast.hpp>
#include <error.h>
#include <logger.h>
#include <mysql/soci-mysql.h>
#include <mysql/mysql.h>
#include <signal.h>
#include <sys/param.h>
#include <unistd.h>
#include "MySqlAPI.h"
#include "MySqlMonitoring.h"
#include "sociConversions.h"

using namespace FTS3_COMMON_NAMESPACE;


static time_t convertToUTC(int advance)
{
    time_t now;
    if(advance > 0)
        now = time(NULL) + advance;
    else
        now = time(NULL);

    struct tm *utc;
    utc = gmtime(&now);
    return timegm(utc);
}

static std::string _getTrTimestampUTC()
{
    time_t now = time(NULL);
    struct tm* tTime;
    tTime = gmtime(&now);
    time_t msec = timegm(tTime) * 1000; //the number of milliseconds since the epoch
    std::ostringstream oss;
    oss << fixed << msec;
    return oss.str();
}


static double convertBtoM( double byte,  double duration)
{
    return ceil((((byte / duration) / 1024) / 1024) * 100 + 0.5) / 100;
}


static int extractTimeout(std::string & str)
{
    size_t found;
    found = str.find("timeout:");
    if (found != std::string::npos)
        {
            str = str.substr(found, str.length());
            size_t found2;
            found2 = str.find(",buffersize:");
            if (found2 != std::string::npos)
                {
                    str = str.substr(0, found2);
                    str = str.substr(8, str.length());
                    return atoi(str.c_str());
                }

        }
    return 0;
}



MySqlAPI::MySqlAPI(): poolSize(10), connectionPool(NULL)
{
    char chname[MAXHOSTNAMELEN]= {0};
    gethostname(chname, sizeof(chname));
    hostname.assign(chname);
}



MySqlAPI::~MySqlAPI()
{
    if(connectionPool)
        {
            delete connectionPool;
        }
}



void MySqlAPI::init(std::string username, std::string password, std::string connectString, int pooledConn)
{
    std::ostringstream connParams;
    std::string host, db, port;

    try
        {

            connectionPool = new soci::connection_pool(pooledConn);

            // From connectString, get host and db
            size_t slash = connectString.find('/');
            if (slash != std::string::npos)
                {
                    host = connectString.substr(0, slash);
                    db   = connectString.substr(slash + 1, std::string::npos);

                    size_t colon = host.find(':');
                    if (colon != std::string::npos)
                        {
                            port = host.substr(colon + 1, std::string::npos);
                            host = host.substr(0, colon);
                        }

                    connParams << "host='" << host << "' "
                               << "db='" << db << "' ";

                    if (!port.empty())
                        connParams << "port=" << port << " ";
                }
            else
                {
                    connParams << "db='" << connectString << "' ";
                }
            connParams << " ";

            // Build connection string
            connParams << "user='" << username << "' "
                       << "pass='" << password << "'";

            std::string connStr = connParams.str();

            // Connect
            static const my_bool reconnect = 1;

            poolSize = (size_t) pooledConn;

            for (size_t i = 0; i < poolSize; ++i)
                {
                    soci::session& sql = (*connectionPool).at(i);
                    sql.open(soci::mysql, connStr);

                    (*connectionPool).at(i) << "SET tx_isolation = 'READ-COMMITTED'";

                    soci::mysql_session_backend* be = static_cast<soci::mysql_session_backend*>(sql.get_backend());
                    mysql_options(static_cast<MYSQL*>(be->conn_), MYSQL_OPT_RECONNECT, &reconnect);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



bool MySqlAPI::getInOutOfSe(const std::string & sourceSe, const std::string & destSe)
{
    soci::session sql(*connectionPool);

    unsigned nSE = 0;
    sql << "SELECT COUNT(*) FROM t_se "
        "WHERE (t_se.name = :source OR t_se.name = :dest) AND "
        "      t_se.state = 'off'",
        soci::use(sourceSe), soci::use(destSe), soci::into(nSE);

    return nSE == 0;
}



TransferJobs* MySqlAPI::getTransferJob(std::string jobId)
{
    soci::session sql(*connectionPool);

    TransferJobs* job = NULL;
    try
        {
            job = new TransferJobs();

            sql << "SELECT t_job.vo_name, t_job.user_dn "
                "FROM t_job WHERE t_job.job_id = :jobId",
                soci::use(jobId),
                soci::into(job->VO_NAME), soci::into(job->USER_DN);

            if (!sql.got_data())
                {
                    delete job;
                    job = NULL;
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return job;
}



void MySqlAPI::getSubmittedJobs(std::vector<TransferJobs*>& jobs, const std::string & vos)
{
    soci::session sql(*connectionPool);

    try
        {

            // Get uniqueue VOs
            std::vector< boost::tuple<std::string, std::string, std::string> > distinct;
            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT DISTINCT v_file.source_se, v_file.dest_se, t_job.vo_name "
                                             " FROM t_job, ( "
                                             "	SELECT DISTINCT source_se, dest_se "
                                             "	FROM t_file WHERE file_state='SUBMITTED' "
                                             " ) AS v_file "
                                             " WHERE "
                                             "	t_job.job_finished IS NULL "
                                             "	AND t_job.CANCEL_JOB IS NULL "
                                             "	AND (t_job.reuse_job='N' OR t_job.reuse_job is NULL) "
                                             "	AND t_job.job_state IN ('ACTIVE', 'READY','SUBMITTED') "


                                             <<
                                             (vos == "*" ? "" : " AND t_job.vo_name IN " + vos)
                                         );

            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    soci::row const& r = *i;
                    distinct.push_back(
                        boost::tuple< std::string, std::string, std::string>(
                            r.get<std::string>("source_se"),
                            r.get<std::string>("dest_se"),
                            r.get<std::string>("vo_name")
                        )

                    );
                }

            // Query depends on vos
            std::string query;
            query =
                "SELECT DISTINCT t_job.* FROM t_job "
                "WHERE t_job.job_finished IS NULL AND "
                "	t_job.cancel_job IS NULL AND "
                "   (t_job.reuse_job = 'N' OR t_job.reuse_job IS NULL) AND "
                "   t_job.job_state IN ('ACTIVE', 'READY', 'SUBMITTED') AND "
                "   t_job.vo_name = :vo AND "
                "   EXISTS ( "
                "		SELECT NULL "
                "		FROM t_file "
                "		WHERE t_file.job_id = t_job.job_id AND "
                "		    t_file.source_se = :source AND "
                "			t_file.dest_se = :dest AND "
                "			t_file.file_state = 'SUBMITTED'"
                "	) "
                "ORDER BY t_job.priority DESC, t_job.submit_time ASC LIMIT 10 ";


            std::set<std::string> jobIds;

            // Iterate through pairs, getting jobs IF the VO has not run out of credits
            // AND there are pending file transfers within the job
            std::vector< boost::tuple<std::string, std::string, std::string> >::iterator it;
            for (it = distinct.begin(); it != distinct.end(); ++it)
                {

                    boost::tuple<std::string, std::string, std::string>& triplet = *it;

                    soci::rowset<TransferJobs> rs = (
                                                        sql.prepare <<
                                                        query,
                                                        soci::use(boost::get<2>(triplet)),
                                                        soci::use(boost::get<0>(triplet)),
                                                        soci::use(boost::get<1>(triplet))
                                                    );

                    for (soci::rowset<TransferJobs>::const_iterator ji = rs.begin(); ji != rs.end(); ++ji)
                        {

                            TransferJobs const & job = *ji;

                            if (!jobIds.count(job.JOB_ID))
                                {
                                    jobIds.insert(job.JOB_ID);
                                    jobs.push_back(new TransferJobs(job));
                                }
                        }
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}


void MySqlAPI::setFilesToNotUsed(std::string jobId, int fileIndex, std::vector<int>& files)
{
    soci::session sql(*connectionPool);

    try
        {

            // first really check if it is a multi-source/destination submission
            // count the alternative replicas, if there is more than one it makes sense to set the NOT_USED state

            int count = 0;

            sql <<
                "SELECT COUNT(*) "
                "FROM t_file "
                "WHERE job_id = :jobId AND file_index = :fileIndex",
                soci::use(jobId),
                soci::use(fileIndex),
                soci::into(count)
                ;

            if (count < 2) return;

            sql.begin();
            soci::statement stmt(sql);

            stmt.exchange(soci::use(jobId, "jobId"));
            stmt.exchange(soci::use(fileIndex, "fileIndex"));
            stmt.alloc();
            stmt.prepare(
                "UPDATE t_file "
                "SET file_state = 'NOT_USED' "
                "WHERE "
                "	job_id = :jobId AND "
                "	file_index = :fileIndex "
                "	AND file_state = 'SUBMITTED' "
            );
            stmt.define_and_bind();
            stmt.execute(true);
            long long  affected_rows = stmt.get_affected_rows();
            sql.commit();

            if (affected_rows > 0)
                {

                    soci::rowset<soci::row> rs = (
                                                     sql.prepare <<
                                                     " SELECT file_id "
                                                     " FROM t_file "
                                                     " WHERE job_id = :jobId "
                                                     "	AND file_index = :fileIndex "
                                                     "	AND file_state = 'NOT_USED' ",
                                                     soci::use(jobId),
                                                     soci::use(fileIndex)
                                                 );

                    soci::rowset<soci::row>::iterator it;
                    for (it = rs.begin(); it != rs.end(); ++it)
                        {
                            files.push_back(
                                it->get<int>("file_id")
                            );
                        }
                }


        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::useFileReplica(std::string jobId, int fileId)
{
    soci::session sql(*connectionPool);

    try
        {

            soci::indicator ind;
            int fileIndex=0;

            sql.begin();

            sql <<
                " SELECT file_index "
                " FROM t_file "
                " WHERE file_id = :fileId ",
                soci::use(fileId),
                soci::into(fileIndex, ind)
                ;

            // make sure it's not NULL
            if (ind == soci::i_ok)
                {
                    sql <<
                        " UPDATE t_file "
                        " SET file_state = 'SUBMITTED' "
                        " WHERE job_id = :jobId "
                        "	AND file_index = :fileIndex "
                        "	AND file_state = 'NOT_USED'",
                        soci::use(jobId),
                        soci::use(fileIndex)
                        ;
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

}

unsigned int MySqlAPI::updateFileStatus(TransferFiles* file, const std::string status)
{
    soci::session sql(*connectionPool);

    unsigned int updated = 0;
    try
        {
            sql.begin();
            soci::statement stmt(sql);

            stmt.exchange(soci::use(status, "state"));
            stmt.exchange(soci::use(file->FILE_ID, "fileId"));
            stmt.exchange(soci::use(hostname, "hostname"));
            stmt.alloc();
            stmt.prepare("UPDATE t_file SET "
                         "    file_state = :state, start_time = UTC_TIMESTAMP(), transferHost = :hostname "
                         "WHERE file_id = :fileId AND file_state = 'SUBMITTED'");
            stmt.define_and_bind();
            stmt.execute(true);

            updated = (unsigned int) stmt.get_affected_rows();
            if (updated != 0)
                {
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
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return updated;
}



void MySqlAPI::updateJObStatus(std::string jobId, const std::string status)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "UPDATE t_job SET job_state = :state WHERE job_id = :jobId AND job_state = 'SUBMITTED'",
                soci::use(status, "state"), soci::use(jobId, "jobId");

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::getByJobId(std::vector<TransferJobs*>& jobs, std::map< std::string, std::list<TransferFiles*> >& files)
{
    soci::session sql(*connectionPool);

    time_t now = time(NULL);
    struct tm tTime;
    gmtime_r(&now, &tTime);

    try
        {
            for (std::vector<TransferJobs*>::const_iterator i = jobs.begin(); i != jobs.end(); ++i)
                {
                    std::string jobId = (*i)->JOB_ID;


                    soci::rowset<TransferFiles> rs = (
                                                         sql.prepare <<
                                                         "SELECT f1.*, j.vo_name, j.overwrite_flag, "
                                                         "    j.user_dn, j.cred_id, j.checksum_method, "
                                                         "    j.source_space_token, j.space_token, j.job_metadata, "
                                                         "    j.copy_pin_lifetime, j.bring_online "
                                                         "FROM t_file f1, t_job j "
                                                         "WHERE j.job_id = :jobId AND"
                                                         "    f1.job_id = j.job_id AND "
                                                         "    f1.job_finished IS NULL AND "
                                                         "    f1.file_state = 'SUBMITTED' AND "
                                                         " 	 (f1.retry_timestamp is NULL OR f1.retry_timestamp < :tTime) "
                                                         "     AND "
                                                         "	 NOT EXISTS ( "
                                                         "		SELECT NULL "
                                                         "		FROM t_file f2 "
                                                         "		WHERE "
                                                         "			f2.job_id = :jobId AND f2.job_id = f1.job_id AND "
                                                         "			f2.file_index = f1.file_index AND "
                                                         "			f2.file_state='READY' "
                                                         "	 ) ORDER BY f1.file_id ASC LIMIT 50 ",soci::use(jobId), soci::use(tTime), soci::use(jobId)


                                                     );


                    for (soci::rowset<TransferFiles>::const_iterator ti = rs.begin(); ti != rs.end(); ++ti)
                        {
                            TransferFiles const& tfile = *ti;
                            files[tfile.VO_NAME].push_back(new TransferFiles(tfile));
                        }
                }
        }
    catch (std::exception& e)
        {
            for (std::map< std::string, std::list<TransferFiles*> >::iterator i = files.begin(); i != files.end(); ++i)
                {
                    std::list<TransferFiles*>& l = i->second;
                    for (std::list<TransferFiles*>::iterator it = l.begin(); it != l.end(); ++it)
                        {
                            delete *it;
                        }
                    l.clear();
                }
            files.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::submitPhysical(const std::string & jobId, std::vector<job_element_tupple> src_dest_pair, const std::string & paramFTP,
                              const std::string & DN, const std::string & cred, const std::string & voName, const std::string & myProxyServer,
                              const std::string & delegationID, const std::string & spaceToken, const std::string & overwrite,
                              const std::string & sourceSpaceToken, const std::string &, int copyPinLifeTime,
                              const std::string & failNearLine, const std::string & checksumMethod, const std::string & reuse,
                              int bring_online, std::string metadata,
                              int retry, int retryDelay, std::string sourceSe, std::string destinationSe)
{

    const std::string currenthost = hostname;
    const std::string initialState = bring_online > 0 || copyPinLifeTime > 0 ? "STAGING" : "SUBMITTED";
    const int priority = 3;
    const std::string params;

    soci::session sql(*connectionPool);



    try
        {
            sql.begin();
            soci::indicator reuseIndicator = soci::i_ok;
            if (reuse.empty())
                reuseIndicator = soci::i_null;
            // Insert job
            sql << "INSERT INTO t_job (job_id, job_state, job_params, user_dn, user_cred, priority,       "
                "                   vo_name, submit_time, internal_job_params, submit_host, cred_id,   "
                "                   myproxy_server, space_token, overwrite_flag, source_space_token,   "
                "                   copy_pin_lifetime, fail_nearline, checksum_method, "
                "                   reuse_job, bring_online, retry, retry_delay, job_metadata,		  "
                "			       source_se, dest_se)         										  "
                "VALUES (:jobId, :jobState, :jobParams, :userDn, :userCred, :priority,                 "
                "        :voName, UTC_TIMESTAMP(), :internalParams, :submitHost, :credId,              "
                "        :myproxyServer, :spaceToken, :overwriteFlag, :sourceSpaceToken,               "
                "        :copyPinLifetime, :failNearline, :checksumMethod,             "
                "        :reuseJob, :bring_online, :retry, :retryDelay, :job_metadata,				  "
                "		:sourceSe, :destSe)															  ",
                soci::use(jobId), soci::use(initialState), soci::use(paramFTP), soci::use(DN), soci::use(cred), soci::use(priority),
                soci::use(voName), soci::use(params), soci::use(currenthost), soci::use(delegationID),
                soci::use(myProxyServer), soci::use(spaceToken), soci::use(overwrite), soci::use(sourceSpaceToken),
                soci::use(copyPinLifeTime), soci::use(failNearLine), soci::use(checksumMethod),
                soci::use(reuse, reuseIndicator), soci::use(bring_online),
                soci::use(retry), soci::use(retryDelay), soci::use(metadata),
                soci::use(sourceSe), soci::use(destinationSe);

            // Insert src/dest pair
            std::string sourceSurl, destSurl, checksum, metadata, selectionStrategy, sourceSe, destSe;
            double filesize = 0.0;
            int fileIndex = 0;
            soci::statement pairStmt = (
                                           sql.prepare <<
                                           "INSERT INTO t_file (job_id, file_state, source_surl, dest_surl, checksum, user_filesize, file_metadata, selection_strategy, file_index, source_se, dest_se) "
                                           "VALUES (:jobId, :fileState, :sourceSurl, :destSurl, :checksum, :filesize, :metadata, :ss, :fileIndex, :source_se, :dest_se)",
                                           soci::use(jobId),
                                           soci::use(initialState),
                                           soci::use(sourceSurl),
                                           soci::use(destSurl),
                                           soci::use(checksum),
                                           soci::use(filesize),
                                           soci::use(metadata),
                                           soci::use(selectionStrategy),
                                           soci::use(fileIndex),
                                           soci::use(sourceSe),
                                           soci::use(destSe)
                                       );
            std::vector<job_element_tupple>::const_iterator iter;
            for (iter = src_dest_pair.begin(); iter != src_dest_pair.end(); ++iter)
                {
                    sourceSurl = iter->source;
                    destSurl   = iter->destination;
                    checksum   = iter->checksum;
                    filesize   = iter->filesize;
                    metadata   = iter->metadata;
                    selectionStrategy = iter->selectionStrategy;
                    fileIndex = iter->fileIndex;
                    sourceSe = iter->source_se;
                    destSe = iter->dest_se;
                    pairStmt.execute();
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
}



void MySqlAPI::getTransferJobStatus(std::string requestID, std::vector<JobStatus*>& jobs)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::rowset<JobStatus> rs = (
                                             sql.prepare <<
                                             "SELECT t_job.job_id, t_job.job_state, t_file.file_state, "
                                             "    t_job.user_dn, t_job.reason, t_job.submit_time, t_job.priority, "
                                             "    t_job.vo_name, t_file.file_index, "
                                             "    (SELECT COUNT(DISTINCT file_index) FROM t_file WHERE t_file.job_id = t_job.job_id) as numFiles "
                                             "FROM t_job, t_file "
                                             "WHERE t_file.job_id = t_job.job_id and t_file.job_id = :jobId",
                                             soci::use(requestID, "jobId"));

            for (soci::rowset<JobStatus>::iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    JobStatus& job = *i;
                    jobs.push_back(new JobStatus(job));
                }
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
}



/*
 * Return a list of jobs based on the status requested
 * std::vector<JobStatus*> jobs: the caller will deallocate memory JobStatus instances and clear the vector
 * std::vector<std::string> inGivenStates: order doesn't really matter, more than one states supported
 */
void MySqlAPI::listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates, std::string restrictToClientDN, std::string forDN, std::string VOname)
{
    soci::session sql(*connectionPool);

    try
        {
            std::ostringstream query;
            soci::statement stmt(sql);
            bool searchForCanceling = false;

            query << "SELECT DISTINCT job_id, job_state, reason, submit_time, user_dn, "
                  "                 vo_name, priority, cancel_job, "
                  "                 (SELECT COUNT(DISTINCT t_file.file_index) FROM t_file WHERE t_file.job_id = t_job.job_id) as numFiles "
                  "FROM t_job ";

            //joins
            if (!restrictToClientDN.empty())
                {
                    query << "LEFT OUTER JOIN t_vo_acl ON t_vo_acl.vo_name = t_job.vo_name ";
                }

            //gain the benefit from the statement pooling
            std::sort(inGivenStates.begin(), inGivenStates.end());

            if (inGivenStates.size() > 0)
                {
                    std::vector<std::string>::const_iterator i;
                    i = std::find_if(inGivenStates.begin(), inGivenStates.end(),
                                     std::bind2nd(std::equal_to<std::string>(), std::string("Canceling")));
                    searchForCanceling = (i != inGivenStates.end());

                    std::string jobStatusesIn = "'" + inGivenStates[0] + "'";
                    for (unsigned i = 1; i < inGivenStates.size(); ++i)
                        {
                            jobStatusesIn += (",'" + inGivenStates[i] + "'");
                        }
                    query << "WHERE job_state IN (" << jobStatusesIn << ") ";
                }
            else
                {
                    query << "WHERE 1 ";
                }

            if (!restrictToClientDN.empty())
                {
                    query << " AND (t_job.user_dn = :clientDn OR t_vo_acl.principal = :clientDn) ";
                    stmt.exchange(soci::use(restrictToClientDN, "clientDn"));
                }

            if (!VOname.empty())
                {
                    query << " AND vo_name = :vo ";
                    stmt.exchange(soci::use(VOname, "vo"));
                }

            if (!forDN.empty())
                {
                    query << " AND user_dn = :userDn ";
                    stmt.exchange(soci::use(forDN, "userDn"));
                }

            if (searchForCanceling)
                {
                    query << " AND cancel_job = 'Y' ";
                }

            JobStatus job;
            stmt.exchange(soci::into(job));
            stmt.alloc();
            std::string test = query.str();
            stmt.prepare(query.str());
            stmt.define_and_bind();

            if (stmt.execute(true))
                {
                    do
                        {
                            jobs.push_back(new JobStatus(job));
                        }
                    while (stmt.fetch());
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
}



void MySqlAPI::getTransferFileStatus(std::string requestID, std::vector<FileTransferStatus*>& files)
{
    soci::session sql(*connectionPool);
    try
        {
            soci::rowset<FileTransferStatus> rs = (sql.prepare  << "SELECT t_file.source_surl, t_file.dest_surl, t_file.file_state, "
                                                   "       t_file.reason, t_file.start_time, t_file.finish_time, t_file.retry "
                                                   "FROM t_file WHERE t_file.job_id = :jobId",
                                                   soci::use(requestID));

            for (soci::rowset<FileTransferStatus>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    FileTransferStatus const& transfer = *i;
                    files.push_back(new FileTransferStatus(transfer));
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
}



void MySqlAPI::getSe(Se* &se, std::string seName)
{
    soci::session sql(*connectionPool);
    se = NULL;

    try
        {
            se = new Se();
            sql << "SELECT * FROM t_se WHERE name = :name",
                soci::use(seName), soci::into(*se);

            if (!sql.got_data())
                {
                    delete se;
                    se = NULL;
                }

        }
    catch (std::exception& e)
        {
            delete se;
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
}



void MySqlAPI::addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
                     std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID)
{
    soci::session sql(*connectionPool);

    try
        {
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
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
                        std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            std::ostringstream query;
            soci::statement stmt(sql);

            query << "UPDATE t_se SET ";

            if (ENDPOINT.length() > 0)
                {
                    query << "ENDPOINT=:endpoint,";
                    stmt.exchange(soci::use(ENDPOINT, "endpoint"));
                }

            if (SE_TYPE.length() > 0)
                {
                    query << " SE_TYPE = :seType,";
                    stmt.exchange(soci::use(SE_TYPE, "seType"));
                }

            if (SITE.length() > 0)
                {
                    query << " SITE = :site,";
                    stmt.exchange(soci::use(SITE, "site"));
                }

            if (STATE.length() > 0)
                {
                    query << " STATE = :state,";
                    stmt.exchange(soci::use(STATE, "state"));
                }

            if (VERSION.length() > 0)
                {
                    query << " VERSION = :version,";
                    stmt.exchange(soci::use(VERSION, "version"));
                }

            if (HOST.length() > 0)
                {
                    query << " HOST = :host,";
                    stmt.exchange(soci::use(HOST, "host"));
                }

            if (SE_TRANSFER_TYPE.length() > 0)
                {
                    query << " SE_TRANSFER_TYPE = :transferType,";
                    stmt.exchange(soci::use(SE_TRANSFER_TYPE, "transferType"));
                }

            if (SE_TRANSFER_PROTOCOL.length() > 0)
                {
                    query << " SE_TRANSFER_PROTOCOL = :transferProtocol,";
                    stmt.exchange(soci::use(SE_TRANSFER_PROTOCOL, "transferProtocol"));
                }

            if (SE_CONTROL_PROTOCOL.length() > 0)
                {
                    query << " SE_CONTROL_PROTOCOL = :controlProtocol,";
                    stmt.exchange(soci::use(SE_CONTROL_PROTOCOL, "controlProtocol"));
                }

            if (GOCDB_ID.length() > 0)
                {
                    query << " GOCDB_ID = :gocdbId,";
                    stmt.exchange(soci::use(GOCDB_ID, "gocdbId"));
                }

            // There is always a comma at the end, so truncate
            std::string queryStr = query.str();
            query.str(std::string());

            query << queryStr.substr(0, queryStr.length() - 1);
            query << " WHERE name = :name";
            stmt.exchange(soci::use(NAME, "name"));

            stmt.alloc();
            stmt.prepare(query.str());
            stmt.define_and_bind();
            stmt.execute(true);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



/*
Delete a SE
REQUIRED: NAME
 */
void MySqlAPI::deleteSe(std::string NAME)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql << "DELETE FROM t_se WHERE name = :name", soci::use(NAME);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



bool MySqlAPI::updateFileTransferStatus(std::string job_id, int file_id, std::string transfer_status, std::string transfer_message,
                                        int process_id, double filesize, double duration)
{

    bool ok = true;
    soci::session sql(*connectionPool);

    try
        {
            double throughput = 0;

            sql.begin();

            bool staging = false;

            // query for the file state in DB
            soci::rowset<soci::row> rs = (
                                             sql.prepare << "SELECT file_state FROM t_file WHERE file_id=:fileId and job_id=:jobId",
                                             soci::use(file_id),
                                             soci::use(job_id)
                                         );

            // check if the state is SATGING, there should be just one row
            soci::rowset<soci::row>::const_iterator it = rs.begin();
            if (it != rs.end())
                {
                    soci::row const& r = *it;
                    std::string st = r.get<std::string>("file_state");
                    staging = (st == "STAGING");
                }

            soci::statement stmt(sql);
            std::ostringstream query;

            query << "UPDATE t_file SET "
                  "    file_state = :state, reason = :reason";
            stmt.exchange(soci::use(transfer_status, "state"));
            stmt.exchange(soci::use(transfer_message, "reason"));

            if (transfer_status == "FINISHED" || transfer_status == "FAILED" || transfer_status == "CANCELED")
                {
                    query << ", FINISH_TIME = UTC_TIMESTAMP()";
                }
            if (transfer_status == "ACTIVE")
                {
                    query << ", START_TIME = UTC_TIMESTAMP()";
                }

            if (transfer_status == "STAGING")
                {
                    if (staging)
                        {
                            query << ", STAGING_FINISHED = UTC_TIMESTAMP()";
                        }
                    else
                        {
                            query << ", STAGING_START = UTC_TIMESTAMP()";
                        }
                }

            if (filesize > 0 && duration > 0 && transfer_status == "FINISHED")
                {
                    throughput = convertBtoM(filesize, duration);
                }
            else if (filesize > 0 && duration <= 0 && transfer_status == "FINISHED")
                {
                    throughput = convertBtoM(filesize, 1);
                }
            else
                {
                    throughput = 0;
                }

            query << "   , pid = :pid, filesize = :filesize, tx_duration = :duration, throughput = :throughput "
                  "WHERE file_id = :fileId AND file_state NOT IN ('FAILED', 'FINISHED', 'CANCELED')";
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
    catch (std::exception& e)
        {
            ok = false;
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return ok;
}



bool MySqlAPI::updateJobTransferStatus(int /*fileId*/, std::string job_id, const std::string status)
{
    bool ok = true;
    soci::session sql(*connectionPool);

    try
        {

            int numberOfFilesInJob = 0;
            int numberOfFilesCanceled = 0;
            int numberOfFilesFinished = 0;
            int numberOfFilesFailed = 0;

            sql.begin();

            sql << "SELECT nFiles, nCanceled, nFinished, nFailed FROM "
                // the total number of files within a job
                "    (SELECT COUNT(DISTINCT file_index) AS nFiles FROM t_file WHERE job_id = :jobId) as DTableTotal, "
                // the number of canceled files in the job, files counts as canceled if all replicas has been canceled
                "    (SELECT COUNT(DISTINCT file_index) AS nCanceled FROM t_file f1 WHERE job_id = :jobId "
                "																	AND NOT EXISTS (SELECT NULL FROM t_file f2 WHERE f1.job_id = f2.job_id AND f1.file_index = f2.file_index AND f2.file_state <> 'CANCELED') "
                "		) as DTableCanceled, "
                // the number of finished files in the job, file counts as finished if at least one replica went to the finished state
                "    (SELECT COUNT(DISTINCT file_index) AS nFinished FROM t_file WHERE job_id = :jobId AND file_state = 'FINISHED') AS DTableFinished, "
                // the number of failed files in the job, file counts as failed if all the replicas went to failed state except replicas that were cancelled
                "    (SELECT COUNT(DISTINCT f1.file_index) AS nFailed FROM t_file f1 WHERE f1.job_id = :jobId "
                "																	AND NOT EXISTS (SELECT NULL FROM t_file f2 WHERE f1.job_id = f2.job_id AND f1.file_index = f2.file_index AND (f2.file_state <> 'FAILED' AND f2.file_state <> 'CANCELED')) "
                "		) AS DTableFailed ",
                soci::use(job_id, "jobId"),
                soci::into(numberOfFilesInJob), soci::into(numberOfFilesCanceled),
                soci::into(numberOfFilesFinished), soci::into(numberOfFilesFailed);

            int numberOfFilesTerminal = numberOfFilesCanceled + numberOfFilesFailed + numberOfFilesFinished;

            bool jobFinished = (numberOfFilesInJob == numberOfFilesTerminal);

            if (jobFinished)
                {
                    std::string state;
                    std::string reason = "One or more files failed. Please have a look at the details for more information";
                    if (numberOfFilesFinished > 0 && numberOfFilesFailed > 0)
                        {
                            state = "FINISHEDDIRTY";
                        }
                    else if(numberOfFilesInJob == numberOfFilesFinished)
                        {
                            state = "FINISHED";
                            reason.clear();
                        }
                    else if(numberOfFilesFailed > 0)
                        {
                            state = "FAILED";
                        }
                    else if(numberOfFilesCanceled > 0)
                        {
                            state = "CANCELED";
                        }
                    else
                        {
                            state = "FAILED";
                            reason = "Inconsistent internal state!";
                        }

                    // Update job
                    sql << "UPDATE t_job SET "
                        "    job_state = :state, job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                        "    reason = :reason "
                        "WHERE job_id = :jobId AND "
                        "      job_state NOT IN ('FINISHEDDIRTY','CANCELED','FINISHED','FAILED')",
                        soci::use(state, "state"), soci::use(reason, "reason"),
                        soci::use(job_id, "jobId");

                    // And file finish timestamp
                    sql << "UPDATE t_file SET job_finished = UTC_TIMESTAMP() WHERE job_id = :jobId ",
                        soci::use(job_id, "jobId");
                }
            // Job not finished yet
            else
                {
                    if (status == "ACTIVE" || status == "STAGING" || status == "SUBMITTED")
                        {
                            sql << "UPDATE t_job "
                                "SET job_state = :state "
                                "WHERE job_id = :jobId AND"
                                "      job_state NOT IN ('FINISHEDDIRTY','CANCELED','FINISHED','FAILED')",
                                soci::use(status, "state"), soci::use(job_id, "jobId");
                        }
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            ok = false;
            sql.rollback();
            std::string msg = e.what();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + msg);
        }

    return ok;
}



void MySqlAPI::cancelJob(std::vector<std::string>& requestIDs)
{
    soci::session sql(*connectionPool);

    try
        {
            const std::string reason = "Job canceled by the user";

            sql.begin();

            for (std::vector<std::string>::const_iterator i = requestIDs.begin(); i != requestIDs.end(); ++i)
                {
                    // Cancel job
                    sql << "UPDATE t_job SET job_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                        "                 reason = :reason "
                        "WHERE job_id = :jobId AND job_state NOT IN ('FINISHEDDIRTY', 'FINISHED', 'FAILED')",
                        soci::use(reason, "reason"), soci::use(*i, "jobId");
                    // Cancel files
                    sql << "UPDATE t_file SET file_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                        "                  reason = :reason "
                        "WHERE job_id = :jobId AND file_state NOT IN ('FINISHED','FAILED')",
                        soci::use(reason, "reason"), soci::use(*i, "jobId");
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::getCancelJob(std::vector<int>& requestIDs)
{
    soci::session select(*connectionPool);
    soci::session update(*connectionPool);

    try
        {
            soci::rowset<soci::row> rs = (select.prepare << "SELECT t_file.pid, t_file.job_id FROM t_file, t_job "
                                          "WHERE t_file.job_id = t_job.job_id AND "
                                          "      t_file.FILE_STATE = 'CANCELED' AND "
                                          "      t_file.PID IS NOT NULL AND "
                                          "      t_job.cancel_job IS NULL AND "
                                          "      t_file.transferHost = :thost",
                                          soci::use(hostname));

            std::string jobId;
            soci::statement updateStmt = (select.prepare << "UPDATE t_job SET cancel_job='Y' WHERE job_id = :jobId AND cancel_job IS NULL",
                                          soci::use(jobId));

            update.begin();
            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    soci::row const& row = *i;

                    int pid = row.get<int>("pid");
                    jobId = row.get<std::string>("job_id");

                    requestIDs.push_back(pid);
                    updateStmt.execute();
                }
            update.commit();
        }
    catch (std::exception& e)
        {
            update.rollback();
            requestIDs.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



/*t_credential API*/
void MySqlAPI::insertGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs)
{
    soci::session sql(*connectionPool);


    try
        {
            sql.begin();
            sql << "INSERT INTO t_credential_cache "
                "    (dlg_id, dn, cert_request, priv_key, voms_attrs) VALUES "
                "    (:dlgId, :dn, :certRequest, :privKey, :vomsAttrs)",
                soci::use(dlg_id), soci::use(dn), soci::use(cert_request), soci::use(priv_key), soci::use(voms_attrs);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
}



void MySqlAPI::updateGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs)
{
    soci::session sql(*connectionPool);
    sql.begin();

    try
        {
            sql << "UPDATE t_credential_cache SET "
                "    cert_request = :certRequest, "
                "    priv_key = :privKey, "
                "    voms_attrs = :vomsAttrs "
                "WHERE dlg_id = :dlgId AND dn=:dn",
                soci::use(cert_request), soci::use(priv_key), soci::use(voms_attrs),
                soci::use(dlg_id), soci::use(dn);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
}



CredCache* MySqlAPI::findGrDPStorageCacheElement(std::string delegationID, std::string dn)
{
    CredCache* cred = NULL;
    soci::session sql(*connectionPool);

    try
        {
            cred = new CredCache();
            sql << "SELECT dlg_id, dn, voms_attrs, cert_request, priv_key "
                "FROM t_credential_cache "
                "WHERE dlg_id = :dlgId and dn = :dn",
                soci::use(delegationID), soci::use(dn), soci::into(*cred);
            if (!sql.got_data())
                {
                    delete cred;
                    cred = NULL;
                }
        }
    catch (std::exception& e)
        {
            delete cred;
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }

    return cred;
}



void MySqlAPI::deleteGrDPStorageCacheElement(std::string delegationID, std::string dn)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql << "DELETE FROM t_credential_cache WHERE dlg_id = :dlgId AND dn = :dn",
                soci::use(delegationID), soci::use(dn);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
}



void MySqlAPI::insertGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time)
{
    soci::session sql(*connectionPool);

    try
        {
            struct tm tTime;
            gmtime_r(&termination_time, &tTime);

            sql.begin();
            sql << "INSERT INTO t_credential "
                "    (dlg_id, dn, termination_time, proxy, voms_attrs) VALUES "
                "    (:dlgId, :dn, :terminationTime, :proxy, :vomsAttrs)",
                soci::use(dlg_id), soci::use(dn), soci::use(tTime), soci::use(proxy), soci::use(voms_attrs);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
}



void MySqlAPI::updateGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time)
{
    soci::session sql(*connectionPool);


    try
        {
            sql.begin();
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
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



Cred* MySqlAPI::findGrDPStorageElement(std::string delegationID, std::string dn)
{
    Cred* cred = NULL;
    soci::session sql(*connectionPool);

    try
        {
            cred = new Cred();
            sql << "SELECT dlg_id, dn, voms_attrs, proxy, termination_time "
                "FROM t_credential "
                "WHERE dlg_id = :dlgId AND dn =:dn",
                soci::use(delegationID), soci::use(dn),
                soci::into(*cred);

            if (!sql.got_data())
                {
                    delete cred;
                    cred = NULL;
                }
        }
    catch (std::exception& e)
        {
            delete cred;
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }

    return cred;
}



void MySqlAPI::deleteGrDPStorageElement(std::string delegationID, std::string dn)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql << "DELETE FROM t_credential WHERE dlg_id = :dlgId AND dn = :dn",
                soci::use(delegationID), soci::use(dn);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
}



bool MySqlAPI::getDebugMode(std::string source_hostname, std::string destin_hostname)
{
    soci::session sql(*connectionPool);

    bool isDebug = false;
    try
        {
            std::string debug;
            sql << "SELECT debug FROM t_debug WHERE source_se = :source AND (dest_se = :dest OR dest_se IS NULL)",
                soci::use(source_hostname), soci::use(destin_hostname), soci::into(debug);

            isDebug = (debug == "on");
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    return isDebug;
}



void MySqlAPI::setDebugMode(std::string source_hostname, std::string destin_hostname, std::string mode)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            if (destin_hostname.length() == 0)
                {
                    sql << "DELETE FROM t_debug WHERE source_se = :source AND dest_se IS NULL",
                        soci::use(source_hostname);
                    sql << "INSERT INTO t_debug (source_se, debug) VALUES (:source, :debug)",
                        soci::use(source_hostname), soci::use(mode);
                }
            else
                {
                    sql << "DELETE FROM t_debug WHERE source_se = :source AND dest_se = :dest",
                        soci::use(source_hostname), soci::use(destin_hostname);
                    sql << "INSERT INTO t_debug (source_se, dest_se, debug) VALUES (:source, :dest, :mode)",
                        soci::use(source_hostname), soci::use(destin_hostname), soci::use(mode);
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
}



void MySqlAPI::getSubmittedJobsReuse(std::vector<TransferJobs*>& jobs, const std::string & vos)
{
    soci::session sql(*connectionPool);

    try
        {
            std::string query;
            if (vos == "*")
                {
                    query = "SELECT t_job.* FROM t_job WHERE "
                            "    t_job.job_finished IS NULL AND "
                            "    t_job.cancel_job IS NULL AND "
                            "    t_job.reuse_job='Y' AND "
                            "    t_job.job_state = 'SUBMITTED' "
                            "ORDER BY t_job.priority DESC, t_job.submit_time ASC "
                            "LIMIT 1";
                }
            else
                {
                    query = "SELECT t_job.* FROM t_job WHERE "
                            "    t_job.job_finished IS NULL AND "
                            "    t_job.cancel_job IS NULL AND "
                            "    t_job.reuse_job='Y' AND "
                            "    t_job.job_state = 'SUBMITTED' AND"
                            "    t_job.vo_name IN " + vos + " "
                            "ORDER BY t_job.priority DESC, t_job.submit_time ASC "
                            "LIMIT 1";
                }

            soci::rowset<TransferJobs> rs = (sql.prepare << query);
            for (soci::rowset<TransferJobs>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    TransferJobs const& tjob = *i;
                    jobs.push_back(new TransferJobs(tjob));
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}




void MySqlAPI::auditConfiguration(const std::string & dn, const std::string & config, const std::string & action)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql << "INSERT INTO t_config_audit (datetime, dn, config, action ) VALUES "
                "                           (UTC_TIMESTAMP(), :dn, :config, :action)",
                soci::use(dn), soci::use(config), soci::use(action);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



/*custom optimization stuff*/

void MySqlAPI::fetchOptimizationConfig2(OptimizerSample* ops, const std::string & source_hostname, const std::string & destin_hostname)
{
    soci::session sql(*connectionPool);

    try
        {

            int numberOfSamples = 0;

            sql <<
                " SELECT COUNT(*) "
                " FROM t_optimize "
                " WHERE throughput is NULL "
                "	AND source_se = :source "
                "	AND dest_se=:destination "
                "	AND file_id=0 ",
                soci::use(source_hostname),
                soci::use(destin_hostname),
                soci::into(numberOfSamples);

            int timeoutTr = 0;

            sql <<
                " SELECT COUNT(*) "
                " FROM t_file "
                " WHERE  "
                "	t_file.file_state = 'FAILED' "
                " 	AND t_file.reason LIKE '%operation timeout%' "
                "	AND t_file.source_se = :sourceSe "
                " 	AND t_file.dest_se = :destSe  "
                " 	AND (t_file.finish_time > (UTC_TIMESTAMP - INTERVAL '30' minute)) "
                " ORDER BY t_file.finish_time DESC",
                soci::use(source_hostname),
                soci::use(destin_hostname),
                soci::into(timeoutTr)
                ;

            if (numberOfSamples > 0 && timeoutTr == 0)
                {

                    soci::rowset<soci::row> rs = (
                                                     sql.prepare <<	" SELECT nostreams, timeout, buffer "
                                                     " FROM t_optimize "
                                                     " WHERE source_se = :source "
                                                     "	AND dest_se = :dest "
                                                     "	AND throughput is NULL "
                                                     "	AND file_id = 0 "
                                                     " ORDER BY nostreams ASC, timeout ASC, buffer ASC "
                                                     " LIMIT 1",
                                                     soci::use(source_hostname),
                                                     soci::use(destin_hostname)
                                                 );

                    soci::rowset<soci::row>::iterator r = rs.begin();

                    if (r != rs.end())
                        {
                            // we are expecting just one row (LIMIT 1)
                            ops->streamsperfile = r->get<int>("nostreams");
                            ops->timeout = r->get<int>("timeout");
                            ops->bufsize = r->get<int>("buffer");
                            ops->file_id = 1;

                        }
                    else
                        {
                            // or no row at all
                            ops->streamsperfile = DEFAULT_NOSTREAMS;
                            ops->timeout = MID_TIMEOUT;
                            ops->bufsize = DEFAULT_BUFFSIZE;
                            ops->file_id = 0;
                        }

                }
            else if (timeoutTr > 0)     //tr's started timing out, use decent defaults
                {

                    ops->streamsperfile = DEFAULT_NOSTREAMS;
                    ops->timeout = 0;
                    ops->bufsize = DEFAULT_BUFFSIZE;
                    ops->file_id = 0;

                }
            else
                {

                    unsigned numberOfActives;

                    sql << " SELECT COUNT(*) "
                        " FROM t_file "
                        " WHERE t_file.file_state = 'ACTIVE' "
                        "	AND t_file.source_se = :source "
                        "	AND t_file.dest_se = :dest",
                        soci::use(source_hostname),
                        soci::use(destin_hostname),
                        soci::into(numberOfActives);

                    soci::rowset<soci::row> rs = (
                                                     sql.prepare <<
                                                     " SELECT throughput, active, nostreams, timeout, buffer "
                                                     " FROM ( "
                                                     "	SELECT throughput, active, nostreams, timeout, buffer "
                                                     "	FROM t_optimize "
                                                     "	WHERE source_se = :source "
                                                     "		AND dest_se = :destination "
                                                     "		AND throughput IS NOT NULL "
                                                     "	ORDER BY ABS(active - :active), throughput DESC"
                                                     " ) sub "
                                                     " LIMIT 1 ",
                                                     soci::use(source_hostname),
                                                     soci::use(destin_hostname),
                                                     soci::use(numberOfActives)
                                                 );

                    soci::rowset<soci::row>::iterator r = rs.begin();

                    if (r != rs.end())
                        {
                            // we are expecting just one row (LIMIT 1)
                            ops->streamsperfile = r->get<int>("nostreams");
                            ops->timeout = r->get<int>("timeout");
                            ops->bufsize = r->get<int>("buffer");
                            ops->file_id = 1;

                        }
                    else
                        {
                            // or no row at all
                            ops->streamsperfile = DEFAULT_NOSTREAMS;
                            ops->timeout = MID_TIMEOUT;
                            ops->bufsize = DEFAULT_BUFFSIZE;
                            ops->file_id = 0;
                        }
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::recordOptimizerUpdate(int active, double filesize,
                                     double throughput, int nostreams, int timeout, int buffersize,
                                     std::string source_hostname, std::string destin_hostname)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql << "INSERT INTO t_optimizer_evolution "
                " (datetime, source_se, dest_se, nostreams, timeout, active, throughput, buffer, filesize) "
                " VALUES "
                " (UTC_TIMESTAMP(), :source, :dest, :nostreams, :timeout, :active, :throughput, :buffer, :filesize)",
                soci::use(source_hostname), soci::use(destin_hostname), soci::use(nostreams),
                soci::use(timeout), soci::use(active), soci::use(throughput), soci::use(buffersize), soci::use(filesize);
            sql.commit();
        }
    catch (...)
        {
            sql.rollback();
        }
}

bool MySqlAPI::updateOptimizer(int, double filesize, double timeInSecs, int nostreams,
                               int timeout, int buffersize,
                               std::string source_hostname, std::string destin_hostname)
{
    bool ok = true;
    soci::session sql(*connectionPool);

    try
        {
            double throughput=0;
            int active=0;

            sql <<
                " SELECT COUNT(*) "
                " FROM t_file "
                " WHERE t_file.source_se = :source_se "
                "	AND t_file.dest_se = :dest_se "
                "	AND file_state = 'ACTIVE' ",
                soci::use(source_hostname),
                soci::use(destin_hostname),
                soci::into(active);

            if (filesize > 0 && timeInSecs > 0)
                throughput = convertBtoM(filesize, timeInSecs);
            else
                throughput = convertBtoM(filesize, 1);

            if (filesize <= 0)
                filesize = 0;
            if (buffersize <= 0)
                buffersize = 0;

            sql.begin();

            soci::statement stmt (sql);

            stmt.exchange(soci::use(filesize, "fsize"));
            stmt.exchange(soci::use(throughput, "throughput"));
            stmt.exchange(soci::use(active, "active"));
            stmt.exchange(soci::use(timeout, "timeout"));
            stmt.exchange(soci::use(nostreams, "nstreams"));
            stmt.exchange(soci::use(buffersize, "buffer"));
            stmt.exchange(soci::use(source_hostname, "source_se"));
            stmt.exchange(soci::use(destin_hostname, "dest_se"));

            stmt.alloc();

            stmt.prepare(
                " UPDATE t_optimize "
                " SET filesize = :fsize, throughput = :throughput, active = :active, datetime = UTC_TIMESTAMP(), timeout= :timeout "
                " WHERE nostreams = :nstreams "
                "	AND buffer = :buffer "
                "	AND source_se = :source_se "
                "	AND dest_se = :dest_se "
                " 	AND (throughput IS NULL OR throughput<=:throughput) "
                "	AND (active<=:active OR active IS NULL)"
            );

            stmt.define_and_bind();
            stmt.execute(true);
            long long affected_rows = stmt.get_affected_rows();

            sql.commit();

            if ( affected_rows == 0)
                {

                    sql.begin();

                    soci::statement stmt2(sql);

                    stmt2.exchange(soci::use(active, "active"));
                    stmt2.exchange(soci::use(nostreams, "nstreams"));
                    stmt2.exchange(soci::use(buffersize, "buffer"));
                    stmt2.exchange(soci::use(source_hostname, "source_se"));
                    stmt2.exchange(soci::use(destin_hostname, "dest_se"));

                    stmt2.alloc();

                    stmt2.prepare("UPDATE t_optimize "
                                  " SET datetime = UTC_TIMESTAMP() "
                                  " WHERE nostreams = :nstreams "
                                  "	AND buffer = :buffer "
                                  "	AND source_se = :source_se "
                                  "	AND dest_se = :dest_se "
                                  " 	AND (active <= :active OR active IS NULL)");

                    stmt2.define_and_bind();
                    stmt2.execute(true);
                    affected_rows += stmt2.get_affected_rows();
                    sql.commit();
                }

            // check the number of updated > 0 (2853)

            //sql.begin();

            //sql << " delete from t_optimize USING t_optimize, t_optimize as vtable WHERE (t_optimize.auto_number < vtable.auto_number) AND "
            //    " (t_optimize.nostreams=vtable.nostreams AND t_optimize.active=vtable.active and t_optimize.throughput=vtable.throughput)";

            //sql.commit();

            // Historical data
            if (affected_rows)
                recordOptimizerUpdate(active, filesize, throughput, nostreams,
                                      timeout, buffersize,
                                      source_hostname, destin_hostname);

        }
    catch (std::exception& e)
        {
            ok = false;
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return ok;
}



void MySqlAPI::addOptimizer(time_t when, double throughput,
                            const std::string & source_hostname,
                            const std::string & destin_hostname, int file_id, int nostreams,
                            int timeout, int buffersize, int /*noOfActiveTransfers*/)
{
    soci::session sql(*connectionPool);

    try
        {
            struct tm timest;
            gmtime_r(&when, &timest);
            unsigned actives = 0;

            sql << "SELECT COUNT(*) FROM t_file WHERE "
                "    t_file.file_state = 'ACTIVE' AND "
                "    t_file.source_se = :source AND t_file.dest_se = :dest",
                soci::use(source_hostname), soci::use(destin_hostname),
                soci::into(actives);

            sql.begin();

            sql << "INSERT INTO t_optimize (file_id, source_se, dest_se, nostreams, timeout, active, buffer, throughput, datetime) "
                "                VALUES (:fileId, :sourceSe, :destSe, :nStreams, :timeout, :active, :buffer, :throughput, :datetime)",
                soci::use(file_id), soci::use(source_hostname), soci::use(destin_hostname), soci::use(nostreams), soci::use(timeout),
                soci::use(actives), soci::use(buffersize), soci::use(throughput), soci::use(timest);

            recordOptimizerUpdate(actives, 0, throughput, nostreams,
                                  timeout, buffersize,
                                  source_hostname, destin_hostname);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::initOptimizer(const std::string & source_hostname, const std::string & destin_hostname, int)
{
    soci::session sql(*connectionPool);

    try
        {
            unsigned foundRecords = 0;
            sql.begin();

            sql << "SELECT COUNT(*) FROM t_optimize WHERE source_se = :source AND dest_se=:dest",
                soci::use(source_hostname), soci::use(destin_hostname),
                soci::into(foundRecords);

            if (foundRecords == 0)
                {
                    int timeout=0, nStreams=0, bufferSize=0;

                    soci::statement stmt = (sql.prepare << "INSERT INTO t_optimize (source_se, dest_se, timeout, nostreams, buffer, file_id) "
                                            "                VALUES (:source, :dest, :timeout, :nostreams, :buffer, 0)",
                                            soci::use(source_hostname), soci::use(destin_hostname), soci::use(timeout),
                                            soci::use(nStreams), soci::use(bufferSize));

                    for (unsigned register int x = 0; x < timeoutslen; x++)
                        {
                            for (unsigned register int y = 0; y < nostreamslen; y++)
                                {
                                    timeout    = timeouts[x];
                                    nStreams   = nostreams[y];
                                    bufferSize = 0;
                                    stmt.execute(true);
                                }
                        }
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



bool MySqlAPI::isCredentialExpired(const std::string & dlg_id, const std::string & dn)
{
    soci::session sql(*connectionPool);

    bool expired = true;
    try
        {
            struct tm termTime;

            sql << "SELECT termination_time FROM t_credential WHERE dlg_id = :dlgId AND dn = :dn",
                soci::use(dlg_id), soci::use(dn), soci::into(termTime);

            if (sql.got_data())
                {
                    time_t termTimestamp = timegm(&termTime);
                    time_t now = convertToUTC(0);
                    expired = (difftime(termTimestamp, now) <= 0);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return !expired;
}



bool MySqlAPI::isTrAllowed(const std::string & source_hostname, const std::string & destin_hostname)
{
    soci::session sql(*connectionPool);

    bool allowed = false;
    try
        {
            int nActiveSource=0, nActiveDest=0;
            double nFailedLastHour=0, nFinishedLastHour=0;
            int nActive=0;
            double nFailedAll=0.0, nFinishedAll=0.0, throughput=0.0;
            soci::indicator isNull;


            sql << "SELECT COUNT(*) FROM t_file "
                "WHERE t_file.file_state in ('READY','ACTIVE') AND "
                "      t_file.source_se = :source ",
                soci::use(source_hostname), soci::into(nActiveSource);

            sql << "SELECT COUNT(*) FROM t_file "
                "WHERE t_file.file_state in ('READY','ACTIVE') AND "
                "      t_file.dest_se = :dst",
                soci::use(destin_hostname), soci::into(nActiveDest);


            sql <<
                " select throughput "
                " from t_file "
                " where source_se = :source "
                " and dest_se = :dest and throughput is not NULL and throughput != 0 "
                " order by FINISH_TIME DESC "
                " LIMIT 1 ",
                soci::use(source_hostname), soci::use(destin_hostname),
                soci::into(throughput, isNull);

            if (isNull == soci::i_null)
                {
                    throughput = 0.0;
                }

            soci::rowset<std::string> rs = (sql.prepare << "SELECT file_state FROM t_file "
                                            "WHERE "
                                            "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                                            "      file_state IN ('FAILED','FINISHED') AND "
                                            "      (t_file.FINISH_TIME > (UTC_TIMESTAMP - interval '5' minute))",
                                            soci::use(source_hostname), soci::use(destin_hostname));

            for (soci::rowset<std::string>::const_iterator i = rs.begin();
                    i != rs.end(); ++i)
                {
                    if      (i->compare("FAILED") == 0)   nFailedLastHour+=1.0;
                    else if (i->compare("FINISHED") == 0) ++nFinishedLastHour+=1.0;
                }


            sql << "SELECT COUNT(*) FROM t_file "
                "WHERE "
                "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                "      file_state in ('READY','ACTIVE')",
                soci::use(source_hostname), soci::use(destin_hostname),
                soci::into(nActive);

            sql << "SELECT COUNT(*) FROM t_file "
                "WHERE "
                "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                "      file_state = 'FINISHED' AND (t_file.FINISH_TIME > (UTC_TIMESTAMP - interval '5' minute)) ",
                soci::use(source_hostname), soci::use(destin_hostname),
                soci::into(nFinishedAll);

            sql << "SELECT COUNT(*) FROM t_file "
                "WHERE "
                "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                "      file_state = 'FAILED' AND (t_file.FINISH_TIME > (UTC_TIMESTAMP - interval '5' minute)) ",
                soci::use(source_hostname), soci::use(destin_hostname),
                soci::into(nFailedAll);

            double ratioSuccessFailure = 0;
            if(nFinishedLastHour > 0)
                {
                    ratioSuccessFailure = nFinishedLastHour/(nFinishedLastHour + nFailedLastHour) * (100.0/1.0);
                }

            allowed = optimizerObject.transferStart((int) nFinishedLastHour, (int) nFailedLastHour,
                                                    source_hostname, destin_hostname,
                                                    nActive, nActiveSource, nActiveDest,
                                                    ratioSuccessFailure,
                                                    nFinishedAll, nFailedAll,throughput);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return allowed;
}


int MySqlAPI::getSeOut(const std::string & source, const std::set<std::string> & destination)
{
    soci::session sql(*connectionPool);

    // total number of allowed active for the source (both currently in use and free credits)
    int ret = 0;

    try
        {
            int nActiveSource=0, nActiveDest=0;
            double nFailedLastHour=0, nFinishedLastHour=0;
            int nActive=0;
            double nFailedAll=0, nFinishedAll=0, throughput=0;
            soci::indicator isNull;

            std::set<std::string>::iterator it;

            std::string source_hostname = source;

            sql << "SELECT COUNT(*) FROM t_file "
                "WHERE t_file.file_state in ('READY','ACTIVE') AND "
                "      t_file.source_se = :source ",
                soci::use(source_hostname), soci::into(nActiveSource);

            ret += nActiveSource;

            for (it = destination.begin(); it != destination.end(); ++it)
                {

                    std::string destin_hostname = *it;

                    sql << "SELECT COUNT(*) FROM t_file "
                        "WHERE t_file.file_state in ('READY','ACTIVE') AND "
                        "      t_file.dest_se = :dst",
                        soci::use(destin_hostname), soci::into(nActiveDest);

                    sql <<
                        " select throughput "
                        " from t_file "
                        " where source_se = :source "
                        " 	and dest_se = :dest "
                        "	and throughput is not NULL "
                        "	and throughput != 0 "
                        " order by FINISH_TIME DESC "
                        " LIMIT 1 ",
                        soci::use(source_hostname), soci::use(destin_hostname),
                        soci::into(throughput, isNull);

                    if (isNull == soci::i_null)
                        {
                            throughput = 0.0;
                        }

                    soci::rowset<std::string> rs = (sql.prepare << "SELECT file_state FROM t_file "
                                                    "WHERE "
                                                    "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                                                    "      file_state IN ('FAILED','FINISHED') AND "
                                                    "      (t_file.FINISH_TIME > (UTC_TIMESTAMP - interval '1' hour))",
                                                    soci::use(source_hostname), soci::use(destin_hostname));

                    for (soci::rowset<std::string>::const_iterator i = rs.begin();
                            i != rs.end(); ++i)
                        {
                            if      (i->compare("FAILED") == 0)   nFailedLastHour+=1.0;
                            else if (i->compare("FINISHED") == 0) nFinishedLastHour+=1.0;
                        }


                    sql << "SELECT COUNT(*) FROM t_file "
                        "WHERE "
                        "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                        "      file_state in ('READY','ACTIVE')",
                        soci::use(source_hostname), soci::use(destin_hostname),
                        soci::into(nActive);

                    sql << "SELECT COUNT(*) FROM t_file "
                        "WHERE "
                        "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                        "      file_state = 'FINISHED'",
                        soci::use(source_hostname), soci::use(destin_hostname),
                        soci::into(nFinishedAll);

                    sql << "SELECT COUNT(*) FROM t_file "
                        "WHERE "
                        "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                        "      file_state = 'FAILED'",
                        soci::use(source_hostname), soci::use(destin_hostname),
                        soci::into(nFailedAll);

                    double ratioSuccessFailure = 0;
                    if(nFinishedLastHour > 0)
                        {
                            ratioSuccessFailure = nFinishedLastHour/(nFinishedLastHour + nFailedLastHour) * (100.0/1.0);
                        }

                    ret += optimizerObject.getFreeCredits((int) nFinishedLastHour, (int) nFailedLastHour,
                                                          source_hostname, destin_hostname,
                                                          nActive, nActiveSource, nActiveDest,
                                                          ratioSuccessFailure,
                                                          nFinishedAll, nFailedAll,throughput);
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return ret;
}

int MySqlAPI::getSeIn(const std::set<std::string> & source, const std::string & destination)
{
    soci::session sql(*connectionPool);

    // total number of allowed active for the source (both currently in use and free credits)
    int ret = 0;

    try
        {
            int nActiveSource=0, nActiveDest=0;
            double nFailedLastHour=0, nFinishedLastHour=0;
            int nActive=0;
            double nFailedAll=0, nFinishedAll=0, throughput=0;
            soci::indicator isNull;

            std::set<std::string>::iterator it;

            std::string destin_hostname = destination;

            sql << "SELECT COUNT(*) FROM t_file "
                "WHERE t_file.file_state in ('READY','ACTIVE') AND "
                "      t_file.dest_se = :dst",
                soci::use(destin_hostname), soci::into(nActiveDest);

            ret += nActiveDest;

            for (it = source.begin(); it != source.end(); ++it)
                {

                    std::string source_hostname = *it;

                    sql << "SELECT COUNT(*) FROM t_file "
                        "WHERE t_file.file_state in ('READY','ACTIVE') AND "
                        "      t_file.source_se = :source ",
                        soci::use(source_hostname), soci::into(nActiveSource);

                    sql <<
                        " select throughput "
                        " from t_file "
                        " where source_se = :source "
                        " 	and dest_se = :dest "
                        "	and throughput is not NULL "
                        "	and throughput != 0 "
                        " order by FINISH_TIME DESC "
                        " LIMIT 1 ",
                        soci::use(source_hostname), soci::use(destin_hostname),
                        soci::into(throughput, isNull);

                    if (isNull == soci::i_null)
                        {
                            throughput = 0.0;
                        }

                    soci::rowset<std::string> rs = (sql.prepare << "SELECT file_state FROM t_file "
                                                    "WHERE "
                                                    "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                                                    "      file_state IN ('FAILED','FINISHED') AND "
                                                    "      (t_file.FINISH_TIME > (UTC_TIMESTAMP - interval '1' hour))",
                                                    soci::use(source_hostname), soci::use(destin_hostname));

                    for (soci::rowset<std::string>::const_iterator i = rs.begin();
                            i != rs.end(); ++i)
                        {
                            if      (i->compare("FAILED") == 0)   nFailedLastHour+=1.0;
                            else if (i->compare("FINISHED") == 0) ++nFinishedLastHour+=1.0;
                        }


                    sql << "SELECT COUNT(*) FROM t_file "
                        "WHERE "
                        "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                        "      file_state in ('READY','ACTIVE')",
                        soci::use(source_hostname), soci::use(destin_hostname),
                        soci::into(nActive);

                    sql << "SELECT COUNT(*) FROM t_file "
                        "WHERE "
                        "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                        "      file_state = 'FINISHED'",
                        soci::use(source_hostname), soci::use(destin_hostname),
                        soci::into(nFinishedAll);

                    sql << "SELECT COUNT(*) FROM t_file "
                        "WHERE "
                        "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                        "      file_state = 'FAILED'",
                        soci::use(source_hostname), soci::use(destin_hostname),
                        soci::into(nFailedAll);

                    double ratioSuccessFailure = 0;
                    if(nFinishedLastHour > 0)
                        {
                            ratioSuccessFailure = nFinishedLastHour/(nFinishedLastHour + nFailedLastHour) * (100.0/1.0);
                        }

                    ret += optimizerObject.getFreeCredits((int) nFinishedLastHour, (int) nFailedLastHour,
                                                          source_hostname, destin_hostname,
                                                          nActive, nActiveSource, nActiveDest,
                                                          ratioSuccessFailure,
                                                          nFinishedAll, nFailedAll,throughput);
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return ret;
}

void MySqlAPI::setAllowedNoOptimize(const std::string & job_id, int file_id, const std::string & params)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            if (file_id)
                sql << "UPDATE t_file SET internal_file_params = :params WHERE file_id = :fileId AND job_id = :jobId",
                    soci::use(params), soci::use(file_id), soci::use(job_id);
            else
                sql << "UPDATE t_file SET internal_file_params = :params WHERE job_id = :jobId",
                    soci::use(params), soci::use(job_id);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::forceFailTransfers(std::map<int, std::string>& collectJobs)
{
    soci::session sql(*connectionPool);

    try
        {
            std::string jobId, params, tHost,reuse;
            int fileId=0, pid=0, timeout=0;
            struct tm startTimeSt;
            time_t now2 = convertToUTC(0);
            time_t startTime;
            double diff = 0.0;
            soci::indicator isNull;

            soci::statement stmt = (
                                       sql.prepare <<
                                       " SELECT t_file.job_id, t_file.file_id, t_file.start_time, t_file.pid, t_file.internal_file_params, "
                                       " t_file.transferHost, t_job.reuse_job "
                                       " FROM t_file, t_job "
                                       " WHERE t_job.job_id = t_file.job_id AND t_file.file_state='ACTIVE' AND pid IS NOT NULL "
                                       " and t_file.finish_time IS NULL AND t_file.job_finished IS NULL ",
                                       soci::into(jobId), soci::into(fileId), soci::into(startTimeSt),
                                       soci::into(pid), soci::into(params), soci::into(tHost), soci::into(reuse, isNull)
                                   );



            if (stmt.execute(true))
                {

                    do
                        {
                            startTime = timegm(&startTimeSt); //from db
                            timeout = extractTimeout(params);

                            int terminateTime = timeout + 3600;

                            if (isNull != soci::i_null && reuse == "Y")
                                {

                                    int count = 0;

                                    sql << " SELECT COUNT(*) FROM t_file WHERE job_id = :jobId ", soci::use(jobId), soci::into(count);
                                    if(count > 0)
                                        terminateTime = count * 360;
                                }

                            diff = difftime(now2, startTime);
                            if (diff > terminateTime)
                                {
                                    if(tHost == hostname)
                                        {
                                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Killing pid:" << pid << ", jobid:" << jobId << ", fileid:" << fileId << " because it was stalled" << commit;
                                            kill(pid, SIGUSR1);
                                        }
                                    collectJobs.insert(std::make_pair<int, std::string > (fileId, jobId));
                                    updateFileTransferStatus(jobId, fileId,
                                                             "FAILED", "Transfer has been forced-killed because it was stalled",
                                                             pid, 0, 0);
                                    updateJobTransferStatus(fileId, jobId, "FAILED");
                                }

                        }
                    while (stmt.fetch());
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::setAllowed(const std::string & job_id, int file_id, const std::string & source_se, const std::string & dest,
                          int nostreams, int timeout, int buffersize)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "UPDATE t_optimize SET "
                "    file_id = 1 "
                "WHERE nostreams = :nStreams AND buffer = :bufferSize AND timeout = :timeout AND "
                "      source_se = :source AND dest_se = :dest",
                soci::use(nostreams), soci::use(buffersize), soci::use(timeout),
                soci::use(source_se), soci::use(dest);

            std::stringstream params;
            params << "nostreams:" << nostreams << ",timeout:" << timeout << ",buffersize:" << buffersize;

            if (file_id != -1)
                {
                    sql << "UPDATE t_file SET internal_file_params = :params WHERE file_id = :fileId AND job_id = :jobId",
                        soci::use(params.str()), soci::use(file_id), soci::use(job_id);
                }
            else
                {
                    sql << "UPDATE t_file SET internal_file_params = :params WHERE job_id = :jobId",
                        soci::use(params.str()), soci::use(job_id);
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



bool MySqlAPI::terminateReuseProcess(const std::string & jobId)
{
    bool ok = true;
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            std::string reuse;
            sql << "SELECT reuse_job FROM t_job WHERE job_id = :jobId AND reuse_job IS NOT NULL",
                soci::use(jobId), soci::into(reuse);

            if (sql.got_data())
                {
                    sql << "UPDATE t_file SET file_state = 'FAILED' WHERE job_id = :jobId AND file_state != 'FINISHED'",
                        soci::use(jobId);
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            ok = false;
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return ok;
}



void MySqlAPI::setPid(const std::string & jobId, int fileId, int pid)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql << "UPDATE t_file SET pid = :pid WHERE job_id = :jobId AND file_id = :fileId",
                soci::use(pid), soci::use(jobId), soci::use(fileId);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::setPidV(int pid, std::map<int, std::string>& pids)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            std::string jobId;
            int fileId=0;
            soci::statement stmt = (sql.prepare << "UPDATE t_file SET pid = :pid WHERE job_id = :jobId AND file_id = :fileId",
                                    soci::use(pid), soci::use(jobId), soci::use(fileId));

            for (std::map<int, std::string>::const_iterator i = pids.begin(); i != pids.end(); ++i)
                {
                    fileId = i->first;
                    jobId  = i->second;
                    stmt.execute(true);
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::revertToSubmitted()
{
    soci::session sql(*connectionPool);

    try
        {
            struct tm startTime;
            int fileId=0;
            std::string jobId, reuseJob;
            time_t now2 = convertToUTC(0);
            sql.begin();

            soci::indicator reuseInd;
            soci::statement readyStmt = (sql.prepare << "SELECT t_file.start_time, t_file.file_id, t_file.job_id, t_job.reuse_job "
                                         "FROM t_file, t_job "
                                         "WHERE t_file.file_state = 'READY' AND t_file.finish_time IS NULL AND "
                                         "      t_file.job_finished IS NULL AND t_file.job_id = t_job.job_id",
                                         soci::into(startTime), soci::into(fileId), soci::into(jobId), soci::into(reuseJob, reuseInd));

            if (readyStmt.execute(true))
                {
                    do
                        {
                            time_t startTimestamp = timegm(&startTime);
                            double diff = difftime(now2, startTimestamp);
                            if (diff > 100 && reuseJob != "Y")
                                {
                                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "The transfer with file id " << fileId << " seems to be stalled, restart it" << commit;

                                    sql << "UPDATE t_file SET file_state = 'SUBMITTED', reason='' "
                                        "WHERE file_state = 'READY' AND finish_time IS NULL AND "
                                        "      job_finished IS NULL AND file_id = :fileId",
                                        soci::use(fileId);
                                }
                            else
                                {
                                    if(reuseJob == "Y")
                                        {
                                            int count = 0;
                                            int terminateTime = 0;

                                            sql << " SELECT COUNT(*) FROM t_file WHERE job_id = :jobId ", soci::use(jobId), soci::into(count);
                                            if(count > 0)
                                                terminateTime = count * 360;

                                            if(diff > terminateTime)
                                                {
                                                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "The transfer with file id (reuse) " << fileId << " seems to be stalled, restart it" << commit;

                                                    sql << "UPDATE t_job SET job_state = 'SUBMITTED' where job_id = :jobId ", soci::use(jobId);

                                                    sql << "UPDATE t_file SET file_state = 'SUBMITTED', reason='' "
                                                        "WHERE file_state = 'READY' AND finish_time IS NULL AND "
                                                        "      job_finished IS NULL AND file_id = :fileId",
                                                        soci::use(fileId);
                                                }
                                        }
                                }
                        }
                    while (readyStmt.fetch());
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::revertToSubmittedTerminate()
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "UPDATE t_file SET file_state = 'SUBMITTED' "
                "WHERE file_state = 'READY' AND finish_time IS NULL AND job_finished IS NULL";

            sql << "UPDATE t_job SET job_state = 'SUBMITTED' WHERE job_state IN ('READY','ACTIVE') AND "
                "   finish_time IS NULL AND job_finished IS NULL AND reuse_job = 'Y' AND "
                "   job_id IN (SELECT DISTINCT t_file.job_id FROM t_file "
                "              WHERE t_file.job_id = t_job.job_id AND t_file.file_state = 'READY')";

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::backup()
{
    soci::session sql(*connectionPool);

    try
        {
            sql << "SET autocommit=0";
            sql << "SET tx_isolation = 'READ-COMMITTED'";
            sql.commit();

            sql << "INSERT INTO t_job_backup SELECT * FROM t_job "
                "WHERE job_finished < (UTC_TIMESTAMP - interval '7' DAY ) AND "
                "job_state IN ('FINISHED', 'FAILED', 'CANCELED', 'FINISHEDDIRTY')";
            sql.commit();


            sql << "INSERT INTO t_file_backup SELECT * FROM t_file WHERE job_id IN (SELECT job_id FROM t_job_backup)";
            sql.commit();

            sql << "DELETE FROM t_file WHERE file_id IN (SELECT file_id FROM t_file_backup)";
            sql.commit();

            sql << "DELETE FROM t_job WHERE job_id IN (SELECT job_id FROM t_job_backup)";
            sql.commit();


        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::forkFailedRevertState(const std::string & jobId, int fileId)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql << "UPDATE t_file SET file_state = 'SUBMITTED' "
                "WHERE file_id = :fileId AND job_id = :jobId AND "
                "      file_state NOT IN ('FINISHED','FAILED','CANCELED')",
                soci::use(fileId), soci::use(jobId);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::forkFailedRevertStateV(std::map<int, std::string>& pids)
{
    soci::session sql(*connectionPool);

    try
        {
            int fileId=0;
            std::string jobId;

            sql.begin();
            soci::statement stmt = (sql.prepare << "UPDATE t_file SET file_state = 'SUBMITTED'"
                                    "WHERE file_id = :fileId AND job_id = :jobId AND "
                                    "      file_state NOT IN ('FINISHED','FAILED','CANCELED')",
                                    soci::use(fileId), soci::use(jobId));

            for (std::map<int, std::string>::const_iterator i = pids.begin(); i != pids.end(); ++i)
                {
                    fileId = i->first;
                    jobId  = i->second;
                    stmt.execute(true);
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



bool MySqlAPI::retryFromDead(std::vector<struct message_updater>& messages)
{
    soci::session sql(*connectionPool);

    bool ok = true;
    std::vector<struct message_updater>::const_iterator iter;
    const std::string transfer_status = "FAILED";
    const std::string transfer_message = "no FTS server had updated the transfer status the last 300 seconds, probably stalled in " + hostname;
    const std::string status = "FAILED";

    try
        {
            for (iter = messages.begin(); iter != messages.end(); ++iter)
                {

                    soci::rowset<int> rs = (
                                               sql.prepare <<
                                               " SELECT file_id FROM t_file "
                                               " WHERE job_id = :jobId AND file_id = :fileId AND file_state='ACTIVE' and transferhost=:hostname ",
                                               soci::use(std::string(iter->job_id)),
                                               soci::use(iter->file_id), soci::use(hostname)
                                           );
                    if (rs.begin() != rs.end())
                        {
                            updateFileTransferStatus((*iter).job_id, (*iter).file_id, transfer_status, transfer_message, (*iter).process_id, 0, 0);
                            updateJobTransferStatus((*iter).file_id, (*iter).job_id, status);
                        }
                }
        }
    catch (std::exception& e)
        {
            ok = false;
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return ok;
}



void MySqlAPI::blacklistSe(std::string se, std::string vo, std::string status, int timeout, std::string msg, std::string adm_dn)
{
    soci::session sql(*connectionPool);

    try
        {

            int count = 0;

            sql <<
                " SELECT COUNT(*) FROM t_bad_ses WHERE se = :se ",
                soci::use(se),
                soci::into(count)
                ;

            sql.begin();

            if (count)
                {

                    sql << " UPDATE t_bad_ses SET "
                        "	addition_time = UTC_TIMESTAMP(), "
                        "	admin_dn = :admin, "
                        "  	vo = :vo, "
                        "	status = :status, "
                        "  	wait_timeout = :timeout "
                        " WHERE se = :se ",
                        soci::use(adm_dn),
                        soci::use(vo),
                        soci::use(status),
                        soci::use(timeout),
                        soci::use(se)
                        ;

                }
            else
                {

                    sql << "INSERT INTO t_bad_ses (se, message, addition_time, admin_dn, vo, status, wait_timeout) "
                        "               VALUES (:se, :message, UTC_TIMESTAMP(), :admin, :vo, :status, :timeout)",
                        soci::use(se), soci::use(msg), soci::use(adm_dn), soci::use(vo), soci::use(status), soci::use(timeout);
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::blacklistDn(std::string dn, std::string msg, std::string adm_dn)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql << "INSERT IGNORE INTO t_bad_dns (dn, message, addition_time, admin_dn) "
                "               VALUES (:dn, :message, UTC_TIMESTAMP(), :admin)",
                soci::use(dn), soci::use(msg), soci::use(adm_dn);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::unblacklistSe(std::string se)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            // delete the entry from DB
            sql << "DELETE FROM t_bad_ses WHERE se = :se",
                soci::use(se)
                ;
            // set to null pending fields in respective files
            sql <<
                " UPDATE t_file f, t_job j SET f.wait_timestamp = NULL, f.wait_timeout = NULL "
                " WHERE (f.source_se = :src OR f.dest_se = :dest) "
                "	AND f.file_state IN ('ACTIVE', 'READY', 'SUBMITTED') "
                "	AND f.job_id = j.job_id "
                "	AND NOT EXISTS ( "
                "		SELECT NULL "
                "		FROM t_bad_dns "
                "		WHERE dn = j.user_dn AND status = 'WAIT' "
                "	)",
                soci::use(se),
                soci::use(se)
                ;

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::unblacklistDn(std::string dn)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            // delete the entry from DB
            sql << "DELETE FROM t_bad_dns WHERE dn = :dn",
                soci::use(dn)
                ;
            // set to null pending fields in respective files
            sql <<
                " UPDATE t_file f, t_job j SET f.wait_timestamp = NULL, f.wait_timeout = NULL "
                " WHERE f.job_id = j.job_id "
                "	AND f.file_state IN ('ACTIVE', 'READY', 'SUBMITTED') "
                "	AND j.user_dn = :dn "
                "	AND NOT EXISTS ( "
                "		SELECT NULL "
                "		FROM t_bad_ses "
                "		WHERE (se = f.source_se OR se = f.dest_se) AND status = 'WAIT' "
                "	)",
                soci::use(dn)
                ;

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



bool MySqlAPI::isSeBlacklisted(std::string se, std::string vo)
{
    soci::session sql(*connectionPool);

    bool blacklisted = false;
    try
        {
            sql << "SELECT * FROM t_bad_ses WHERE se = :se AND (vo IS NULL OR vo='' OR vo = :vo)", soci::use(se), soci::use(vo);
            blacklisted = sql.got_data();
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return blacklisted;
}



bool MySqlAPI::isDnBlacklisted(std::string dn)
{
    soci::session sql(*connectionPool);

    bool blacklisted = false;
    try
        {
            sql << "SELECT * FROM t_bad_dns WHERE dn = :dn", soci::use(dn);
            blacklisted = sql.got_data();
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return blacklisted;
}



/********* section for the new config API **********/
bool MySqlAPI::isFileReadyState(int fileID)
{
    soci::session sql(*connectionPool);
    bool isReady = false;

    try
        {
            std::string state;
            sql << "SELECT file_state FROM t_file WHERE file_id = :fileId",
                soci::use(fileID), soci::into(state);

            isReady = (state == "READY");
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return isReady;
}



bool MySqlAPI::checkGroupExists(const std::string & groupName)
{
    soci::session sql(*connectionPool);

    bool exists = false;
    try
        {
            std::string grp;
            sql << "SELECT groupName FROM t_group_members WHERE groupName = :group",
                soci::use(groupName), soci::into(grp);

            exists = sql.got_data();
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return exists;
}

//t_group_members

void MySqlAPI::getGroupMembers(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::rowset<std::string> rs = (sql.prepare << "SELECT member FROM t_group_members "
                                            "WHERE groupName = :group",
                                            soci::use(groupName));
            for (soci::rowset<std::string>::const_iterator i = rs.begin();
                    i != rs.end(); ++i)
                groupMembers.push_back(*i);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



std::string MySqlAPI::getGroupForSe(const std::string se)
{
    soci::session sql(*connectionPool);

    std::string group;
    try
        {
            sql << "SELECT groupName FROM t_group_members "
                "WHERE member = :member",
                soci::use(se), soci::into(group);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return group;
}



void MySqlAPI::addMemberToGroup(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            std::string member;
            soci::statement stmt = (sql.prepare << "INSERT INTO t_group_members(member, groupName) "
                                    "                    VALUES (:member, :group)",
                                    soci::use(member), soci::use(groupName));
            for (std::vector<std::string>::const_iterator i = groupMembers.begin();
                    i != groupMembers.end(); ++i)
                {
                    member = *i;
                    stmt.execute(true);
                }


            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::deleteMembersFromGroup(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            std::string member;
            soci::statement stmt = (sql.prepare << "DELETE FROM t_group_members "
                                    "WHERE groupName = :group AND member = :member",
                                    soci::use(groupName), soci::use(member));
            for (std::vector<std::string>::const_iterator i = groupMembers.begin();
                    i != groupMembers.end(); ++i)
                {
                    member = *i;
                    stmt.execute(true);
                }
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::addLinkConfig(LinkConfig* cfg)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "INSERT INTO t_link_config (source, destination, state, symbolicName, "
                "                          nostreams, tcp_buffer_size, urlcopy_tx_to, no_tx_activity_to, auto_tuning)"
                "                  VALUES (:src, :dest, :state, :sname, :nstreams, :tcp, :txto, :txactivity, :auto_tuning)",
                soci::use(cfg->source), soci::use(cfg->destination),
                soci::use(cfg->state), soci::use(cfg->symbolic_name),
                soci::use(cfg->NOSTREAMS), soci::use(cfg->TCP_BUFFER_SIZE),
                soci::use(cfg->URLCOPY_TX_TO), soci::use(cfg->URLCOPY_TX_TO),
                soci::use(cfg->auto_tuning);


            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::updateLinkConfig(LinkConfig* cfg)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "UPDATE t_link_config SET "
                "  state = :state, symbolicName = :sname, "
                "  nostreams = :nostreams, tcp_buffer_size = :tcp, "
                "  urlcopy_tx_to = :txto, no_tx_activity_to = :txactivity, auto_tuning = :auto_tuning "
                "WHERE source = :source AND destination = :dest",
                soci::use(cfg->state), soci::use(cfg->symbolic_name),
                soci::use(cfg->NOSTREAMS), soci::use(cfg->TCP_BUFFER_SIZE),
                soci::use(cfg->URLCOPY_TX_TO), soci::use(cfg->NO_TX_ACTIVITY_TO),
                soci::use(cfg->auto_tuning),
                soci::use(cfg->source), soci::use(cfg->destination);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::deleteLinkConfig(std::string source, std::string destination)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "DELETE FROM t_link_config WHERE source = :source AND destination = :destination",
                soci::use(source), soci::use(destination);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



LinkConfig* MySqlAPI::getLinkConfig(std::string source, std::string destination)
{
    soci::session sql(*connectionPool);

    LinkConfig* lnk = NULL;
    try
        {
            LinkConfig config;

            sql << "SELECT * FROM t_link_config WHERE source = :source AND destination = :dest",
                soci::use(source), soci::use(destination),
                soci::into(config);

            if (sql.got_data())
                lnk = new LinkConfig(config);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return lnk;
}



bool MySqlAPI::isThereLinkConfig(std::string source, std::string destination)
{
    soci::session sql(*connectionPool);

    bool exists = false;
    try
        {
            int count = 0;
            sql << "SELECT COUNT(*) FROM t_link_config WHERE "
                "  source = :source AND destination = :dest",
                soci::use(source), soci::use(destination),
                soci::into(count);
            exists = (count > 0);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return exists;
}



std::pair<std::string, std::string>* MySqlAPI::getSourceAndDestination(std::string symbolic_name)
{
    soci::session sql(*connectionPool);

    std::pair<std::string, std::string>* pair = NULL;
    try
        {
            std::string source, destination;
            sql << "SELECT source, destination FROM t_link_config WHERE symbolicName = :sname",
                soci::use(symbolic_name), soci::into(source), soci::into(destination);
            if (sql.got_data())
                pair = new std::pair<std::string, std::string>(source, destination);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return pair;
}



bool MySqlAPI::isGrInPair(std::string group)
{
    soci::session sql(*connectionPool);

    bool inPair = false;
    try
        {
            sql << "SELECT * FROM t_link_config WHERE "
                "  (source = :group AND destination <> '*') OR "
                "  (source <> '*' AND destination = :group)",
                soci::use(group, "group");
            inPair = sql.got_data();
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return inPair;
}



void MySqlAPI::addShareConfig(ShareConfig* cfg)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "INSERT INTO t_share_config (source, destination, vo, active) "
                "                    VALUES (:source, :destination, :vo, :active)",
                soci::use(cfg->source), soci::use(cfg->destination), soci::use(cfg->vo),
                soci::use(cfg->active_transfers);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::updateShareConfig(ShareConfig* cfg)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "UPDATE t_share_config SET "
                "  active = :active "
                "WHERE source = :source AND destination = :dest AND vo = :vo",
                soci::use(cfg->active_transfers),
                soci::use(cfg->source), soci::use(cfg->destination), soci::use(cfg->vo);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::deleteShareConfig(std::string source, std::string destination, std::string vo)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "DELETE FROM t_share_config WHERE source = :source AND destination = :dest AND vo = :vo",
                soci::use(destination), soci::use(source), soci::use(vo);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::deleteShareConfig(std::string source, std::string destination)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "DELETE FROM t_share_config WHERE source = :source AND destination = :dest",
                soci::use(destination), soci::use(source);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



ShareConfig* MySqlAPI::getShareConfig(std::string source, std::string destination, std::string vo)
{
    soci::session sql(*connectionPool);

    ShareConfig* cfg = NULL;
    try
        {
            ShareConfig config;
            sql << "SELECT * FROM t_share_config WHERE "
                "  source = :source AND destination = :dest AND vo = :vo",
                soci::use(source), soci::use(destination), soci::use(vo),
                soci::into(config);
            if (sql.got_data())
                cfg = new ShareConfig(config);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return cfg;
}



std::vector<ShareConfig*> MySqlAPI::getShareConfig(std::string source, std::string destination)
{
    soci::session sql(*connectionPool);

    std::vector<ShareConfig*> cfg;
    try
        {
            soci::rowset<ShareConfig> rs = (sql.prepare << "SELECT * FROM t_share_config WHERE "
                                            "  source = :source AND destination = :dest",
                                            soci::use(source), soci::use(destination));
            for (soci::rowset<ShareConfig>::const_iterator i = rs.begin();
                    i != rs.end(); ++i)
                {
                    ShareConfig* newCfg = new ShareConfig(*i);
                    cfg.push_back(newCfg);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return cfg;
}



void MySqlAPI::submitHost(const std::string & jobId)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "UPDATE t_job SET submit_host = :host WHERE job_id = :jobId",
                soci::use(hostname), soci::use(jobId);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



std::string MySqlAPI::transferHost(int fileId)
{
    soci::session sql(*connectionPool);

    std::string host;
    try
        {
            sql << "SELECT transferHost FROM t_file WHERE file_id = :fileId",
                soci::use(fileId), soci::into(host);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return host;
}



/*for session reuse check only*/
bool MySqlAPI::isFileReadyStateV(std::map<int, std::string>& fileIds)
{
    soci::session sql(*connectionPool);

    bool isReady = false;
    try
        {
            std::string state;
            sql << "SELECT file_state FROM t_file WHERE file_id = :fileId",
                soci::use(fileIds.begin()->first), soci::into(state);

            isReady = (state == "READY");
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return isReady;
}



std::string MySqlAPI::transferHostV(std::map<int, std::string>& fileIds)
{
    soci::session sql(*connectionPool);

    std::string host;
    try
        {
            sql << "SELECT transferHost FROM t_file WHERE file_id = :fileId",
                soci::use(fileIds.begin()->first), soci::into(host);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return host;
}

/*
    we need to check if a member already belongs to another group
    true: it is member of another group
    false: it is not member of another group
 */
bool MySqlAPI::checkIfSeIsMemberOfAnotherGroup(const std::string & member)
{
    soci::session sql(*connectionPool);

    bool isMember = false;
    try
        {
            std::string group;
            sql << "SELECT groupName FROM t_group_members WHERE member = :member",
                soci::use(member), soci::into(group);

            isMember = sql.got_data();
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return isMember;
}



void MySqlAPI::addFileShareConfig(int file_id, std::string source, std::string destination, std::string vo)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << " insert into t_file_share_config (file_id, source, destination, vo) "
                " select :fileId, :source, :dest, :vo from dual where not exists(select file_id, source, destination, vo "
                " from t_file_share_config WHERE file_id = :fileId AND source = :source AND destination = :dest AND vo = :vo)",
                soci::use(file_id),
                soci::use(source),
                soci::use(destination),
                soci::use(vo),
                soci::use(file_id),
                soci::use(source),
                soci::use(destination),
                soci::use(vo);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



//void MySqlAPI::delJobShareConfig(std::string job_id) {
//    soci::session sql(*connectionPool);
//
//    try {
//        sql.begin();
//
//        sql << "DELETE FROM t_job_share_config WHERE job_id = :jobId",
//                soci::use(job_id);
//
//        sql.commit();
//    }
//    catch (std::exception& e) {
//        sql.rollback();
//        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
//    }
//}
//
//
//
//std::vector< boost::tuple<std::string, std::string, std::string> > MySqlAPI::getJobShareConfig(std::string job_id) {
//    soci::session sql(*connectionPool);
//
//    std::vector< boost::tuple<std::string, std::string, std::string> > vConfig;
//    try {
//        std::string source, dest, vo;
//        soci::statement stmt = (sql.prepare << "SELECT source, destination, vo FROM t_job_share_config WHERE job_id = :jobId",
//                                               soci::use(job_id), soci::into(source), soci::into(dest), soci::into(vo));
//       if (stmt.execute(true)) {
//            do {
//            	boost::tuple<std::string, std::string, std::string> tmp;
//            	boost::get<0>(tmp) = source;
//            	boost::get<1>(tmp) = dest;
//            	boost::get<2>(tmp) = vo;
//            	vConfig.push_back(tmp);
//            } while (stmt.fetch());
//        }
//
//    }
//    catch (std::exception& e) {
//        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
//    }
//    return vConfig;
//}
//
//
//
//unsigned int MySqlAPI::countJobShareConfig(std::string job_id) {
//    soci::session sql(*connectionPool);
//
//    unsigned int count = 0;
//    try {
//        sql << "SELECT COUNT(*) FROM t_job_share_config WHERE job_id = :jobId",
//                soci::use(job_id), soci::into(count);
//    }
//    catch (std::exception& e) {
//        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
//    }
//    return count;
//}



int MySqlAPI::countActiveTransfers(std::string source, std::string destination, std::string vo)
{
    soci::session sql(*connectionPool);

    int nActive = 0;
    try
        {
            sql << "SELECT COUNT(*) FROM t_file, t_file_share_config "
                "WHERE (t_file.file_state = 'ACTIVE'  OR t_file.file_state = 'READY') AND "
                "      t_file_share_config.file_id = t_file.file_id AND "
                "      t_file_share_config.source = :source AND "
                "      t_file_share_config.destination = :dest AND "
                "      t_file_share_config.vo = :vo",
                soci::use(source), soci::use(destination), soci::use(vo),
                soci::into(nActive);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return nActive;
}



int MySqlAPI::countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo)
{
    soci::session sql(*connectionPool);

    int nActiveOutbound = 0;
    try
        {
            sql << "SELECT COUNT(*) FROM t_file, t_file_share_config "
                "WHERE (t_file.file_state = 'ACTIVE' OR t_file.file_state = 'READY') AND "
                "      t_file.source_se = :source AND "
                "      t_file.file_id = t_file_share_config.file_id AND "
                "      t_file_share_config.source = '(*)' AND "
                "      t_file_share_config.destination = '*' AND "
                "      t_file_share_config.vo = :vo",
                soci::use(se), soci::use(vo), soci::into(nActiveOutbound);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return nActiveOutbound;
}



int MySqlAPI::countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo)
{
    soci::session sql(*connectionPool);

    int nActiveInbound = 0;
    try
        {
            sql << "SELECT COUNT(*) FROM t_file, t_file_share_config "
                "WHERE (t_file.file_state = 'ACTIVE'  OR t_file.file_state = 'READY') AND "
                "      t_file.dest_se = :dest AND "
                "      t_file.file_id = t_file_share_config.file_id AND "
                "      t_file_share_config.source = '*' AND "
                "      t_file_share_config.destination = '(*)' AND "
                "      t_file_share_config.vo = :vo",
                soci::use(se), soci::use(vo), soci::into(nActiveInbound);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return nActiveInbound;
}

int MySqlAPI::sumUpVoShares (std::string source, std::string destination, std::set<std::string> vos)
{
    soci::session sql(*connectionPool);

    int sum = 0;
    try
        {

            std::set<std::string>::iterator it = vos.begin();

            while (it != vos.end())
                {

                    std::set<std::string>::iterator remove = it;
                    it++;

                    sql <<
                        " SELECT vo "
                        " FROM t_share_config "
                        " WHERE source = :source "
                        "	AND destination = :destination "
                        "	AND vo = :vo ",
                        soci::use(source),
                        soci::use(destination),
                        soci::use(*remove)
                        ;

                    if (!sql.got_data() && *remove != "public")
                        {
                            // if there is no configuration for this VO replace it with 'public'
                            vos.erase(remove);
                            vos.insert("'public'");
                        }
                }

            std::string vos_str = "(";

            for (it = vos.begin(); it != vos.end(); ++it)
                {

                    vos_str += *it + ",";
                }

            // replace the last ',' with closing ')'
            vos_str[vos_str.size() - 1] = ')';

            soci::indicator isNull;

            sql <<
                " SELECT SUM(active) "
                " FROM t_share_config "
                " WHERE source = :source "
                "	AND destination = :destination "
                "	AND vo IN " + vos_str,
                soci::use(source),
                soci::use(destination),
                soci::into(sum, isNull)
                ;

            if (isNull == soci::i_null) sum = 0;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return sum;
}

//boost::optional<unsigned int> MySqlAPI::getJobConfigCount(std::string job_id) {
//    soci::session sql(*connectionPool);
//
//    boost::optional<unsigned int> opCount;
//    try {
//        int count = 0;
//        soci::indicator isNull;
//
//        sql << "SELECT configuration_count FROM t_job WHERE job_id = :jobId",
//                soci::use(job_id), soci::into(count, isNull);
//
//        if (isNull != soci::i_null) {
//            opCount = count;
//        }
//
//    }
//    catch (std::exception& e) {
//        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
//    }
//
//    return opCount;
//}



//void MySqlAPI::setJobConfigCount(std::string job_id, int count) {
//    soci::session sql(*connectionPool);
//
//    try {
//        sql.begin();
//
//        sql << "UPDATE t_job SET "
//               "  configuration_count = :count "
//               "WHERE job_id = :jobId",
//               soci::use(count), soci::use(job_id);
//
//        sql.commit();
//    }
//    catch (std::exception& e) {
//        sql.rollback();
//        throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
//    }
//}

void MySqlAPI::setPriority(std::string job_id, int priority)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "UPDATE t_job SET "
                "  priority = :priority "
                "WHERE job_id = :jobId",
                soci::use(priority), soci::use(job_id);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

bool MySqlAPI::checkConnectionStatus()
{
    soci::session sql(*connectionPool);

    bool couldConnect = false;
    try
        {
            soci::mysql_session_backend* be = static_cast<soci::mysql_session_backend*>(sql.get_backend());
            couldConnect = (mysql_ping(static_cast<MYSQL*>(be->conn_)) == 0);
        }
    catch (std::exception& e)
        {
            // Pass
        }
    catch (...)
        {
            // Pass
        }

    return couldConnect;
}



void MySqlAPI::setRetry(int retry)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "UPDATE t_server_config SET retry = :retry",
                soci::use(retry);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



int MySqlAPI::getRetry(const std::string & jobId)
{
    soci::session sql(*connectionPool);

    int nRetries = 0;
    try
        {

            sql <<
                " SELECT retry "
                " FROM t_job "
                " WHERE job_id = :jobId ",
                soci::use(jobId),
                soci::into(nRetries)
                ;

            if (nRetries == 0)
                {

                    sql <<
                        " SELECT retry "
                        " FROM t_server_config",
                        soci::into(nRetries)
                        ;
                }

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return nRetries;
}



void MySqlAPI::setRetryTimes(int retry, const std::string & jobId, int fileId)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "UPDATE t_file SET "
                "    retry = :retry "
                "WHERE job_id = :jobId AND file_id = :fileId",
                soci::use(retry), soci::use(jobId), soci::use(fileId);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



int MySqlAPI::getRetryTimes(const std::string & jobId, int fileId)
{
    soci::session sql(*connectionPool);

    int nRetries = 0;
    try
        {
            sql << "SELECT retry FROM t_file WHERE job_id = :jobId AND file_id = :fileId",
                soci::use(jobId), soci::use(fileId), soci::into(nRetries);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return nRetries;
}



void MySqlAPI::setRetryTransfer(const std::string & jobId, int fileId)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "UPDATE t_job SET "
                "    job_state = 'ACTIVE' "
                "WHERE job_id = :jobId AND "
                "      job_state NOT IN ('FAILED','CANCELED') AND "
                "      reuse_job = 'Y'",
                soci::use(jobId);

            sql << "UPDATE t_file SET file_state = 'SUBMITTED' "
                "WHERE job_id = :jobId AND file_id = :fileId AND file_state NOT IN ('FAILED','CANCELED')",
                soci::use(jobId), soci::use(fileId);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



int MySqlAPI::getMaxTimeInQueue()
{
    soci::session sql(*connectionPool);

    int maxTime = 0;
    try
        {
            sql << "SELECT max_time_queue FROM t_server_config",
                soci::into(maxTime);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return maxTime;
}



void MySqlAPI::setMaxTimeInQueue(int afterXHours)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "UPDATE t_server_config SET max_time_queue = :maxTime",
                soci::use(afterXHours);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}



void MySqlAPI::setToFailOldQueuedJobs(std::vector<std::string>& jobs)
{
    const static std::string message = "Job has been canceled because it stayed in the queue for too long";

    soci::session sql(*connectionPool);

    try
        {
            int maxTime = getMaxTimeInQueue();
            if (maxTime == 0)
                return;

            sql.begin();

            soci::rowset<std::string> rs = (sql.prepare << "SELECT job_id FROM t_job WHERE "
                                            "    (submit_time < (UTC_TIMESTAMP - interval :interval hour) AND "
                                            "    job_state in ('SUBMITTED', 'READY')",
                                            soci::use(maxTime));

            for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    sql << "UPDATE t_file SET "
                        "    file_state = 'CANCELED', reason = :reason "
                        "WHERE job_id = :jobId AND "
                        "      file_state IN ('SUBMITTED','READY')",
                        soci::use(message), soci::use(*i);

                    sql << "UPDATE t_job SET "
                        "    job_state = 'CANCELED', reason = :reason "
                        "WHERE job_id = :jobId AND job_state IN ('SUBMITTED','READY')",
                        soci::use(message), soci::use(*i);
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}


std::vector< std::pair<std::string, std::string> > MySqlAPI::getPairsForSe(std::string se)
{
    soci::session sql(*connectionPool);

    std::vector< std::pair<std::string, std::string> > ret;

    try
        {
            soci::rowset<soci::row> rs = (
                                               sql.prepare <<
                                               " select source, destination "
                                               " from t_link_config "
                                               " where (source = :source and destination <> '*') "
                                               "	or (source <> '*' and destination = :dest) ",
                                               soci::use(se),
                                               soci::use(se)
                                           );

            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    ret.push_back(
                    		make_pair(i->get<std::string>("source"), i->get<std::string>("destination"))
                    	);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return ret;
}


std::vector<std::string> MySqlAPI::getAllStandAlloneCfgs()
{

    soci::session sql(*connectionPool);

    std::vector<std::string> ret;

    try
        {
            soci::rowset<std::string> rs = (
                                               sql.prepare <<
                                               " select source "
                                               " from t_link_config "
                                               " where destination = '*' and auto_tuning <> 'all' "
                                           );

            for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    ret.push_back(*i);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return ret;
}

std::vector< std::pair<std::string, std::string> > MySqlAPI::getAllPairCfgs()
{

    soci::session sql(*connectionPool);

    std::vector< std::pair<std::string, std::string> > ret;

    try
        {
            soci::rowset<soci::row> rs = (sql.prepare << "select source, destination from t_link_config where source <> '*' and destination <> '*'");

            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    soci::row const& row = *i;
                    ret.push_back(
                        std::make_pair(row.get<std::string>("source"), row.get<std::string>("destination"))
                    );
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return ret;
}

int MySqlAPI::activeProcessesForThisHost()
{
    soci::session sql(*connectionPool);

    unsigned active = 0;
    try
        {
            sql << "select count(*) from t_file where file_state in ('READY','ACTIVE') and TRANSFERHOST=:host", soci::use(hostname), soci::into(active);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return active;
}


std::vector< boost::tuple<std::string, std::string, int> >  MySqlAPI::getVOBringonlimeMax()
{

    soci::session sql(*connectionPool);

    std::vector< boost::tuple<std::string, std::string, int> > ret;

    try
        {
            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             "SELECT vo_name, host, concurrent_ops FROM t_stage_req"
                                         );

            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    soci::row const& row = *i;

                    boost::tuple<std::string, std::string, int> item (
                        row.get<std::string>("vo_name"),
                        row.get<std::string>("host"),
                        row.get<int>("concurrent_ops")
                    );

                    ret.push_back(item);
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return ret;

}

std::vector<message_bringonline> MySqlAPI::getBringOnlineFiles(std::string voName, std::string hostName, int maxValue)
{

    soci::session sql(*connectionPool);

    std::vector<message_bringonline> ret;

    try
        {

            if (voName.empty())
                {

                    soci::rowset<soci::row> rs = (
                                                     sql.prepare <<
                                                     " SELECT distinct(t_file.source_se) "
                                                     " FROM t_file, t_job "
                                                     " WHERE t_job.job_id = t_file.job_id "
                                                     "	AND (t_job.BRING_ONLINE > 0 OR t_job.COPY_PIN_LIFETIME > 0) "
                                                     "	AND t_file.file_state = 'STAGING' "
                                                     "	AND t_file.staging_start IS NULL and t_file.source_surl like 'srm%' "
                                                 );

                    for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                        {
                            soci::row const& row = *i;

                            std::string hostV = row.get<std::string>("source_se");

                            unsigned int currentStagingFilesNoConfig = 0;

                            sql <<
                                " SELECT COUNT(*) "
                                " FROM t_file, t_job "
                                " WHERE t_job.job_id = t_file.job_id "
                                " 	AND (t_job.BRING_ONLINE > 0 OR t_job.COPY_PIN_LIFETIME > 0) "
                                "	AND t_file.file_state = 'STAGING' "
                                "	AND t_file.staging_start IS NOT NULL "
                                "	AND t_file.source_se = :hostV  and t_file.source_surl like 'srm%'  ",
                                soci::use(hostV),
                                soci::into(currentStagingFilesNoConfig)
                                ;

                            unsigned int maxNoConfig = currentStagingFilesNoConfig > 0 ? maxValue - currentStagingFilesNoConfig : maxValue;

                            soci::rowset<soci::row> rs2 = (
                                                              sql.prepare <<
                                                              " SELECT t_file.source_surl, t_file.job_id, t_file.file_id, t_job.copy_pin_lifetime, t_job.bring_online "
                                                              " FROM t_file, t_job "
                                                              " WHERE t_job.job_id = t_file.job_id "
                                                              " 	AND (t_job.BRING_ONLINE > 0 OR t_job.COPY_PIN_LIFETIME > 0) "
                                                              "	AND t_file.staging_start IS NULL "
                                                              "	AND t_file.file_state = 'STAGING' "
                                                              "	AND t_file.source_se = :source_se and t_file.source_surl like 'srm%'   "
                                                              "	AND t_job.submit_host = :hostname "
                                                              " ORDER BY t_file.file_id "
                                                              " LIMIT :limit",
                                                              soci::use(hostV),
                                                              soci::use(hostname),
                                                              soci::use(maxNoConfig)

                                                          );

                            for (soci::rowset<soci::row>::const_iterator i2 = rs2.begin(); i2 != rs2.end(); ++i2)
                                {
                                    soci::row const& row2 = *i2;

                                    struct message_bringonline msg;
                                    msg.url = row2.get<std::string>("source_surl");
                                    msg.job_id = row2.get<std::string>("job_id");
                                    msg.file_id = row2.get<int>("file_id");
                                    msg.pinlifetime = row2.get<int>("copy_pin_lifetime");
                                    msg.bringonlineTimeout = row2.get<int>("bring_online");

                                    ret.push_back(msg);
                                    bringOnlineReportStatus("STARTED", "", msg);
                                }
                        }
                }
            else
                {

                    unsigned currentStagingFilesConfig = 0;

                    sql <<
                        " SELECT COUNT(*) FROM t_file, t_job "
                        " WHERE t_job.job_id = t_file.job_id "
                        " 	AND (t_job.BRING_ONLINE > 0 OR t_job.COPY_PIN_LIFETIME > 0) "
                        "	AND t_file.file_state = 'STAGING' "
                        "	AND t_file.STAGING_START IS NOT NULL "
                        " 	AND t_job.vo_name = :vo_name "
                        "	AND t_file.source_se = :source_se and t_file.source_surl like 'srm%'   ",
                        soci::use(voName),
                        soci::use(hostName),
                        soci::into(currentStagingFilesConfig)
                        ;

                    unsigned int maxConfig = currentStagingFilesConfig > 0 ? maxValue - currentStagingFilesConfig : maxValue;

                    soci::rowset<soci::row> rs = (
                                                     sql.prepare <<
                                                     " SELECT t_file.source_surl, t_file.job_id, t_file.file_id, t_job.copy_pin_lifetime, t_job.bring_online "
                                                     " FROM t_file, t_job "
                                                     " WHERE t_job.job_id = t_file.job_id "
                                                     "	AND (t_job.BRING_ONLINE > 0 OR t_job.COPY_PIN_LIFETIME > 0) "
                                                     " 	AND t_file.staging_START IS NULL "
                                                     "	AND t_file.file_state = 'STAGING' "
                                                     "	AND t_file.source_se = :source_se "
                                                     "	AND t_job.vo_name = :vo_name and t_file.source_surl like 'srm%'   "
                                                     "	AND t_job.SUBMIT_HOST = :hostname "
                                                     " ORDER BY t_file.file_id "
                                                     " LIMIT :limit",
                                                     soci::use(hostName),
                                                     soci::use(voName),
                                                     soci::use(hostname),
                                                     soci::use(maxConfig)
                                                 );

                    for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                        {
                            soci::row const& row = *i;

                            struct message_bringonline msg;
                            msg.url = row.get<std::string>("source_surl");
                            msg.job_id = row.get<std::string>("job_id");
                            msg.file_id = row.get<int>("file_id");
                            msg.pinlifetime = row.get<int>("copy_pin_lifetime");
                            msg.bringonlineTimeout = row.get<int>("bring_online");

                            ret.push_back(msg);
                            bringOnlineReportStatus("STARTED", "", msg);
                        }
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return ret;
}

void MySqlAPI::bringOnlineReportStatus(const std::string & state, const std::string & message, struct message_bringonline msg)
{

    if (state != "STARTED" && state != "FINISHED" && state != "FAILED") return;

    soci::session sql(*connectionPool);

    try
        {


            if (state == "STARTED")
                {
                    sql.begin();
                    sql <<
                        " UPDATE t_file "
                        " SET staging_start = UTC_TIMESTAMP() "
                        " WHERE job_id = :jobId "
                        "	AND file_id= :fileId "
                        "	AND file_state='STAGING'",
                        soci::use(msg.job_id),
                        soci::use(msg.file_id)
                        ;
                    sql.commit();
                }
            else
                {
                    sql.begin();
                    std::string dbState = state == "FINISHED" ? "SUBMITTED" : state;
                    std::string dbReason = state == "FINISHED" ? string() : message;

                    sql <<
                        " UPDATE t_file "
                        " SET staging_finished = UTC_TIMESTAMP(), reason = :reason, file_state = :fileState "
                        " WHERE job_id = :jobId "
                        "	AND file_id = :fileId "
                        "	AND file_state = 'STAGING'",
                        soci::use(dbReason),
                        soci::use(dbState),
                        soci::use(msg.job_id),
                        soci::use(msg.file_id)
                        ;
                    sql.commit();

                    //check if session reuse has been issued
                    soci::indicator isNull;
                    std::string reuse("");
                    sql << " select reuse_job from t_job where job_id=:jobId", soci::use(msg.job_id), soci::into(reuse, isNull);
                    if (isNull != soci::i_null && reuse == "Y")
                        {
                            int countTr = 0;
                            sql << " select count(*) from t_file where job_id=:jobId and file_state='STAGING' ", soci::use(msg.job_id), soci::into(countTr);
                            if(countTr == 0)
                                {
                                    updateJobTransferStatus(0, msg.job_id, "SUBMITTED");
                                }
                        }
                    else
                        {
                            updateJobTransferStatus(0, msg.job_id, dbState);
                        }
                }



        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::addToken(const std::string & job_id, int file_id, const std::string & token)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql <<
                " UPDATE t_file "
                " SET bringonline_token = :token "
                " WHERE job_id = :jobId "
                "	AND file_id = :fileId "
                "	AND file_state = 'STAGING' ",
                soci::use(token),
                soci::use(job_id),
                soci::use(file_id);

            sql.commit();

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}


void MySqlAPI::getCredentials(std::string & vo_name, const std::string & job_id, int, std::string & dn, std::string & dlg_id)
{

    soci::session sql(*connectionPool);

    try
        {
            sql <<
                " SELECT vo_name, user_dn, cred_id FROM t_job WHERE job_id = :jobId ",
                soci::use(job_id),
                soci::into(vo_name),
                soci::into(dn),
                soci::into(dlg_id)
                ;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::setMaxStageOp(const std::string& se, const std::string& vo, int val)
{
    soci::session sql(*connectionPool);

    try
        {
            int exist = 0;

            sql <<
                " SELECT COUNT(*) "
                " FROM t_stage_req "
                " WHERE vo_name = :vo AND host = :se ",
                soci::use(vo),
                soci::use(se),
                soci::into(exist)
                ;

            sql.begin();

            // if the record already exist ...
            if (exist)
                {
                    // update
                    sql <<
                        " UPDATE t_stage_req "
                        " SET concurrent_ops = :value "
                        " WHERE vo_name = :vo AND host = :se ",
                        soci::use(val),
                        soci::use(vo),
                        soci::use(se)
                        ;
                }
            else
                {
                    // otherwise insert
                    sql <<
                        " INSERT "
                        " INTO t_stage_req (host, vo_name, concurrent_ops) "
                        " VALUES (:se, :vo, :value)",
                        soci::use(se),
                        soci::use(vo),
                        soci::use(val)
                        ;
                }

            sql.commit();

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}


void MySqlAPI::setRetryTimestamp(const std::string& jobId, int fileId)
{
    soci::session sql(*connectionPool);

    try
        {
            //expressed in secs
            int retry_delay = 0;

            sql <<
                " select RETRY_DELAY from t_job where job_id=:jobId ",
                soci::use(jobId),
                soci::into(retry_delay)
                ;

            sql.begin();

            if (retry_delay > 0)
                {
                    // update
                    time_t now = convertToUTC(retry_delay);
                    struct tm tTime;
                    gmtime_r(&now, &tTime);

                    sql <<
                        " update t_file set retry_timestamp=:1 where job_id=:jobId and file_id=:fileId ",
                        soci::use(tTime),
                        soci::use(jobId),
                        soci::use(fileId)
                        ;
                }

            sql.commit();

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::updateProtocol(const std::string& jobId, int fileId, int nostreams, int timeout, int buffersize, double filesize)
{

    std::stringstream internalParams;
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            internalParams << "nostreams:" << nostreams << ",timeout:" << timeout << ",buffersize:" << buffersize;

            sql <<
                " UPDATE t_file set INTERNAL_FILE_PARAMS=:1, FILESIZE=:2 where job_id=:jobId and file_id=:fileId ",
                soci::use(internalParams.str()),
                soci::use(filesize),
                soci::use(jobId),
                soci::use(fileId);

            sql.commit();

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}


int MySqlAPI::countActiveTransfers(std::string source, std::string destination)
{
    // TODO
    return 0;
}

int MySqlAPI::getFailureRate(std::string source, std::string destination)
{
    //TODO

    return 0;
}

int MySqlAPI::getAvgThroughput(std::string source, std::string destination, int activeTransfers)
{
    // TODO
    return 0;
}

void MySqlAPI::cancelFilesInTheQueue(const std::string& se, const std::string& vo, std::set<std::string>& jobs)
{
    soci::session sql(*connectionPool);

    try
        {

            sql.begin();

            soci::rowset<soci::row> rs = vo.empty() ?
                                         (
                                             sql.prepare <<
                                             " SELECT file_id, job_id, file_index "
                                             " FROM t_file "
                                             " WHERE (source_se = :se OR dest_se = :se) "
                                             "	AND file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED')",
                                             soci::use(se),
                                             soci::use(se)
                                         )
                                         :
                                         (
                                             sql.prepare <<
                                             " SELECT f.file_id, f.job_id, file_index "
                                             " FROM t_job j, t_file f "
                                             " WHERE (f.source_se = :se OR f.dest_se = :se) "
                                             "	AND j.vo_name = :vo "
                                             " 	AND f.file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
                                             "	AND j.job_id = f.job_id ",
                                             soci::use(se),
                                             soci::use(se),
                                             soci::use(vo)
                                         );

            soci::rowset<soci::row>::iterator it;
            for (it = rs.begin(); it != rs.end(); ++it)
                {

                    std::string jobId = it->get<std::string>("job_id");
                    int fileId = it->get<int>("file_id");

                    jobs.insert(jobId);

                    sql <<
                        " UPDATE t_file "
                        " SET file_state = 'CANCELED' "
                        " WHERE file_id = :fileId",
                        soci::use(fileId)
                        ;

                    int fileIndex = it->get<int>("file_index");

                    sql <<
                        " UPDATE t_file "
                        " SET file_state = 'SUBMITTED' "
                        " WHERE file_state = 'NOT_USED' "
                        "	AND job_id = :jobId "
                        "	AND file_index = :fileIndex "
                        "	AND NOT EXISTS ( "
                        "		SELECT NULL "
                        "		FROM ( "
                        "			SELECT NULL "
                        "			FROM t_file "
                        "			WHERE job_id = :jobId "
                        "				AND file_index = :fileIndex "
                        "				AND file_state <> 'NOT_USED' "
                        "				AND file_state <> 'FAILED' "
                        "				AND file_state <> 'CANCELED' "
                        "		) AS tmp "
                        "	) ",
                        soci::use(jobId),
                        soci::use(fileIndex),
                        soci::use(jobId),
                        soci::use(fileIndex)
                        ;
                }

            sql.commit();

            std::set<std::string>::iterator job_it;
            for (job_it = jobs.begin(); job_it != jobs.end(); ++job_it)
                {
                    updateJobTransferStatus(int(), *job_it, string());
                }

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::cancelJobsInTheQueue(const std::string& dn, std::vector<std::string>& jobs)
{
    soci::session sql(*connectionPool);

    try
        {

            // bare in mind that in this case we do not care about NOT_USED because we are operating at the level of a job

            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT job_id "
                                             " FROM t_job "
                                             " WHERE user_dn = :dn "
                                             "	AND job_state IN ('ACTIVE', 'READY', 'SUBMITTED')",
                                             soci::use(dn)
                                         );

            soci::rowset<soci::row>::iterator it;
            for (it = rs.begin(); it != rs.end(); ++it)
                {

                    jobs.push_back(it->get<std::string>("job_id"));
                }

            cancelJob(jobs);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::transferLogFile(const std::string& filePath, const std::string& jobId, int fileId, bool debug)
{
    soci::session sql(*connectionPool);

    //soci doesn't access bool
    unsigned int debugFile = debug;

    try
        {
            sql.begin();

            sql <<
                " update t_file set t_log_file=:filePath, t_log_file_debug=:debugFile where job_id=:jobId and file_id=:fileId ",
                soci::use(filePath),
                soci::use(debugFile),
                soci::use(jobId),
                soci::use(fileId);

            sql.commit();

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}


struct message_state MySqlAPI::getStateOfTransfer(const std::string& jobId, int fileId)
{
    soci::session sql(*connectionPool);

    message_state ret;

    try
        {

            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT "
                                             "	t_job.job_id, t_job.job_state, t_job.vo_name, "
                                             "	t_job.job_metadata, t_job.retry AS retry_max, t_file.file_id, "
                                             "	t_file.file_state, t_file.retry AS retry_counter, t_file.file_metadata, "
                                             "	t_file.source_se, t_file.dest_se "
                                             " FROM t_file, t_job "
                                             " WHERE t_job.job_id = t_file.job_id "
                                             " 	AND t_job.job_id = :jobId "
                                             "	AND t_file.file_id = :fileId ",
                                             soci::use(jobId),
                                             soci::use(fileId)
                                         );

            soci::rowset<soci::row>::iterator it = rs.begin();
            if (it != rs.end())
                {
                    ret.job_id = it->get<std::string>("job_id");
                    ret.job_state = it->get<std::string>("job_state");
                    ret.vo_name = it->get<std::string>("vo_name");
                    ret.job_metadata = it->get<std::string>("job_metadata");
                    ret.retry_max = it->get<int>("retry_max");
                    ret.file_id = it->get<int>("file_id");
                    ret.file_state = it->get<std::string>("file_state");
                    ret.retry_counter = it->get<int>("retry_counter");
                    ret.file_metadata = it->get<std::string>("file_metadata");
                    ret.source_se = it->get<std::string>("source_se");
                    ret.dest_se = it->get<std::string>("dest_se");
                    ret.timestamp = _getTrTimestampUTC();

                    if(ret.retry_max == 0)
                        {
                            sql << " select retry from t_server_config", soci::into(ret.retry_max);
                        }
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return ret;
}

void MySqlAPI::getFilesForJob(const std::string& jobId, std::vector<int>& files)
{
    soci::session sql(*connectionPool);

    try
        {

            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT file_id "
                                             " FROM t_file "
                                             " WHERE job_id = :jobId",
                                             soci::use(jobId)
                                         );

            soci::rowset<soci::row>::iterator it;
            for (it = rs.begin(); it != rs.end(); ++it)
                {
                    files.push_back(
                        it->get<int>("file_id")
                    );
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::getFilesForJobInCancelState(const std::string& jobId, std::vector<int>& files)
{
    soci::session sql(*connectionPool);

    try
        {

            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT file_id "
                                             " FROM t_file "
                                             " WHERE job_id = :jobId "
                                             "	AND file_state = 'CANCELED' ",
                                             soci::use(jobId)
                                         );

            soci::rowset<soci::row>::iterator it;
            for (it = rs.begin(); it != rs.end(); ++it)
                {
                    files.push_back(
                        it->get<int>("file_id")
                    );
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}


void MySqlAPI::setFilesToWaiting(const std::string& se, const std::string& vo, int timeout)
{
    soci::session sql(*connectionPool);

    try
        {

            sql.begin();

            if (vo.empty())
                {

                    sql <<
                        " UPDATE t_file "
                        " SET wait_timestamp = UTC_TIMESTAMP(), wait_timeout = :timeout "
                        " WHERE (source_se = :src OR dest_se = :dest) "
                        "	AND file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
                        "	AND (wait_timestamp IS NULL OR wait_timeout IS NULL) ",
                        soci::use(timeout),
                        soci::use(se),
                        soci::use(se)
                        ;

                }
            else
                {

                    sql <<
                        " UPDATE t_file f, t_job j "
                        " SET f.wait_timestamp = UTC_TIMESTAMP(), f.wait_timeout = :timeout "
                        " WHERE (f.source_se = :src OR f.dest_se = :dest) "
                        "	AND j.vo_name = :vo "
                        "	AND f.file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
                        "	AND f.job_id = j.job_id "
                        "	AND (f.wait_timestamp IS NULL OR f.wait_timeout IS NULL) ",
                        soci::use(timeout),
                        soci::use(se),
                        soci::use(se),
                        soci::use(vo)
                        ;
                }

            sql.commit();

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::setFilesToWaiting(const std::string& dn, int timeout)
{
    soci::session sql(*connectionPool);

    try
        {

            sql.begin();

            sql <<
                " UPDATE t_file f, t_job j "
                " SET f.wait_timestamp = UTC_TIMESTAMP(), f.wait_timeout = :timeout "
                " WHERE j.user_dn = :dn "
                "	AND f.file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
                "	AND f.job_id = j.job_id "
                "	AND (f.wait_timestamp IS NULL OR f.wait_timeout IS NULL) ",
                soci::use(timeout),
                soci::use(dn)
                ;

            sql.commit();

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::cancelWaitingFiles(std::set<std::string>& jobs)
{

    soci::session sql(*connectionPool);

    try
        {

            sql.begin();

            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT file_id, job_id "
                                             " FROM t_file "
                                             " WHERE TIMESTAMPDIFF(SECOND, wait_timestamp, UTC_TIMESTAMP()) > wait_timeout "
                                             "	AND file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
                                         );

            soci::rowset<soci::row>::iterator it;
            for (it = rs.begin(); it != rs.end(); ++it)
                {

                    jobs.insert(it->get<std::string>("job_id"));

                    sql <<
                        " UPDATE t_file "
                        " SET file_state = 'CANCELED' "
                        " WHERE file_id = :fileId ",
                        soci::use(it->get<int>("file_id"))
                        ;
                }

            sql.commit();

            std::set<std::string>::iterator job_it;
            for (job_it = jobs.begin(); job_it != jobs.end(); ++job_it)
                {
                    updateJobTransferStatus(int(), *job_it, string());
                }
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::revertNotUsedFiles()
{

    soci::session sql(*connectionPool);

    try
        {

            sql.begin();

            sql <<
                " UPDATE t_file f1 "
                " SET f1.file_state = 'SUBMITTED' "
                " WHERE f1.file_state = 'NOT_USED' "
                "	AND NOT EXISTS ( "
                "		SELECT NULL "
                "		FROM ( "
                "			SELECT job_id, file_index, file_state "
                "			FROM t_file f2 "
                "			WHERE EXISTS ( "
                "					SELECT NULL "
                "					FROM t_file f3 "
                "					WHERE f3.job_id = f2.job_id "
                "						AND f3.file_index = f2.file_index "
                "						AND f3.file_state = 'NOT_USED' "
                "				) "
                "				AND f2.file_state <> 'NOT_USED' "
                "				AND f2.file_state <> 'CANCELED' "
                "				AND f2.file_state <> 'FAILED' "
                "		) AS t_file_tmp "
                "		WHERE t_file_tmp.job_id = f1.job_id "
                "			AND t_file_tmp.file_index = f1.file_index "
                "	) "
                ;

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

bool MySqlAPI::isShareOnly(std::string se)
{

    soci::session sql(*connectionPool);

    bool ret = true;
    try
        {
            sql <<
                " select * from t_link_config "
                " where source = :source and destination = '*' and auto_tuning = 'all'",
                soci::use(se)
                ;

            ret = sql.got_data();

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return ret;
}

std::vector<std::string> MySqlAPI::getAllShareOnlyCfgs()
{

    soci::session sql(*connectionPool);

    std::vector<std::string> ret;

    try
        {
            soci::rowset<std::string> rs = (
                                               sql.prepare <<
                                               " select source "
                                               " from t_link_config "
                                               " where destination = '*' and auto_tuning = 'all' "
                                           );

            for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    ret.push_back(*i);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return ret;
}

void MySqlAPI::checkSanityState()
{
    //TERMINAL STATES:  "FINISHED" FAILED" "CANCELED"
    soci::session sql(*connectionPool);

    std::vector<std::string> ret;
    unsigned int numberOfFiles = 0;
    unsigned int terminalState = 0;
    int allFinished = 0;
    int allFailed = 0;
    int allCanceled = 0;
    unsigned int allNotUsedStaging = 0;
    unsigned int numberOfFilesRevert = 0;
    std::string canceledMessage = "Transfer canceled by the user";
    std::string failed = "One or more files failed. Please have a look at the details for more information";


    try
        {
            soci::rowset<std::string> rs = (
                                               sql.prepare <<
                                               " select job_id from t_job where job_state in ('ACTIVE','READY','SUBMITTED','STAGING') "
                                           );

            for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    sql << "SELECT COUNT(DISTINCT file_index) FROM t_file where job_id=:jobId ", soci::use(*i), soci::into(numberOfFiles);

                    if(numberOfFiles > 0)
                        {
                            countFileInTerminalStates(*i, allFinished, allCanceled, allFailed);
                            terminalState = allFinished + allCanceled + allFailed;

                            if(numberOfFiles == terminalState)  /* all files terminal state but job in ('ACTIVE','READY','SUBMITTED','STAGING') */
                                {
                                    if(allCanceled > 0)
                                        {
                                            sql.begin();
                                            sql << "UPDATE t_job SET "
                                                "    job_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                                                "    reason = :canceledMessage "
                                                "    WHERE job_id = :jobId ", soci::use(canceledMessage), soci::use(*i);
                                            sql.commit();
                                        }
                                    else   //non canceled, check other states: "FINISHED" and FAILED"
                                        {
                                            if(numberOfFiles == allFinished)  /*all files finished*/
                                                {
                                                    sql.begin();
                                                    sql << "UPDATE t_job SET "
                                                        "    job_state = 'FINISHED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP() "
                                                        "    WHERE job_id = :jobId", soci::use(*i);
                                                    sql.commit();
                                                }
                                            else
                                                {
                                                    if(numberOfFiles == allFailed)  /*all files failed*/
                                                        {
                                                            sql.begin();
                                                            sql << "UPDATE t_job SET "
                                                                "    job_state = 'FAILED', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                                                                "    reason = :failed "
                                                                "    WHERE job_id = :jobId", soci::use(failed), soci::use(*i);
                                                            sql.commit();
                                                        }
                                                    else   // otherwise it is FINISHEDDIRTY
                                                        {
							    sql.begin();
                                                            sql << "UPDATE t_job SET "
                                                                "    job_state = 'FINISHEDDIRTY', job_finished = UTC_TIMESTAMP(), finish_time = UTC_TIMESTAMP(), "
                                                                "    reason = :failed "
                                                                "    WHERE job_id = :jobId", soci::use(failed), soci::use(*i);
							    sql.commit();
                                                        }
                                                }
                                        }
                                }
                        }
                    //reset
                    numberOfFiles = 0;
                }

            //now check reverse sanity checks, JOB can't be FINISH,  FINISHEDDIRTY, FAILED is at least one tr is in SUBMITTED, READY, ACTIVE
            soci::rowset<std::string> rs2 = (
                                                sql.prepare <<
                                                " select job_id from t_job where job_state in ('FINISHED','FAILED','FINISHEDDIRTY') "
                                            );
            for (soci::rowset<std::string>::const_iterator i2 = rs2.begin(); i2 != rs2.end(); ++i2)
                {
                    sql << "SELECT COUNT(*) FROM t_file where job_id=:jobId AND file_state in ('ACTIVE','READY','SUBMITTED','STAGING') ", soci::use(*i2), soci::into(numberOfFilesRevert);
                    if(numberOfFilesRevert > 0)
                        {
                            sql.begin();
                            sql << "UPDATE t_job SET "
                                "    job_state = 'ACTIVE', job_finished = NULL, finish_time = NULL, "
                                "    reason = NULL "
                                "    WHERE job_id = :jobId", soci::use(*i2);
                            sql.commit();
                        }
                    //reset
                    numberOfFilesRevert = 0;
                }

        }
    catch (std::exception& e)
        {
  	    sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::countFileInTerminalStates(std::string jobId, int& finished, int& canceled, int& failed)
{
    soci::session sql(*connectionPool);

    try
        {
            sql <<
                " select count(*)  "
                " from t_file "
                " where job_id = :jobId "
                "	and  file_state = 'FINISHED' ",
                soci::use(jobId),
                soci::into(finished)
                ;

            sql <<
                " select count(distinct f1.file_index) "
                " from t_file f1 "
                " where f1.job_id = :jobId "
                "	and NOT EXISTS ( "
                "		select null "
                "		from t_file f2 "
                "		where job_id = :jobId "
                "			and f2.file_index = f1.file_index "
                "			and f2.file_state <> 'CANCELED' "
                " 	) ",
                soci::use(jobId),
                soci::use(jobId),
                soci::into(canceled)
                ;

            sql <<
                " select count(distinct f1.file_index) "
                " from t_file f1 "
                " where f1.job_id = :jobId "
                "	and f1.file_state = 'FAILED' "
                "	and NOT EXISTS ( "
                "		select null "
                "		from t_file f2 "
                "		where job_id = :jobId "
                "			and f2.file_index = f1.file_index "
                "			and f2.file_state NOT IN ('CANCELED', 'FAILED') "
                " 	) ",
                soci::use(jobId),
                soci::use(jobId),
                soci::into(failed)
                ;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::getFilesForNewSeCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out) {

    soci::session sql(*connectionPool);

    try
        {
            soci::rowset<int> rs = (
								   sql.prepare <<
								   " select file_id "
								   " from t_file, t_job "
								   " where t_file.source_se like :source "
								   "	and t_file.dest_se like :destination "
								   "	and t_file.job_id = t_job.job_id "
								   "	and t_job.vo_name = :vo "
								   "	and t_file.file_state in ('READY', 'ACTIVE') ",
								   soci::use(source == "*" ? "%" : source),
								   soci::use(destination == "*" ? "%" : destination),
								   soci::use(vo)
							   );

            for (soci::rowset<int>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    out.push_back(*i);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

void MySqlAPI::getFilesForNewGrCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out) {
    soci::session sql(*connectionPool);

    std::string select;
	select +=
			" select file_id "
			" from t_file, t_job "
			" where t_file.job_id = t_job.job_id "
			"	and t_job.vo_name = :vo "
			"	and t_file.file_state in ('READY', 'ACTIVE')";
	if (source != "*")
		select +=
			"	and t_file.source_se in ( "
			"		select member "
			"		from t_group_members "
			"		where groupName = :source "
			"	) ";
	if (destination != "*")
		select +=
			"	and t_file.dest_se in ( "
			"		select member "
			"		from t_group_members "
			"		where groupName = :dest "
			"	) ";

    try
        {
    		int id;

    		soci::statement stmt(sql);
    		stmt.exchange(soci::use(vo, "vo"));
    		if (source != "*") stmt.exchange(soci::use(source, "source"));
    		if (destination != "*") stmt.exchange(soci::use(destination, "dest"));
    		stmt.exchange(soci::into(id));
    		stmt.alloc();
    		stmt.prepare(select);
    		stmt.define_and_bind();

            if (stmt.execute(true))
                {
                    do
                        {
                            out.push_back(id);
                        }
                    while (stmt.fetch());
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}


void MySqlAPI::delFileShareConfig(int file_id, std::string source, std::string destination, std::string vo) {
    soci::session sql(*connectionPool);

    try
        {
	    sql.begin();
            sql <<
                " delete from t_file_share_config  "
                " where file_id = :id "
                "	and source = :src "
                "	and destination = :dest "
                "	and vo = :vo",
                soci::use(file_id),
                soci::use(source),
                soci::use(destination),
                soci::use(vo)
                ;
	    sql.commit();
        }
    catch (std::exception& e)
        {
	    sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}


void MySqlAPI::delFileShareConfig(std::string group, std::string se) {
    soci::session sql(*connectionPool);

    try
        {
	    sql.begin();
	    
            sql <<
                " delete from t_file_share_config  "
                " where (source = :gr or destination = :gr) "
                "	and file_id IN ( "
                "		select file_id "
                "		from t_file "
                "		where (source_se = :se or dest_se = :se) "
                "			and file_state in ('READY', 'ACTIVE')"
                "	) ",
                soci::use(group),
                soci::use(group),
                soci::use(se),
                soci::use(se)
                ;
		
	    sql.commit();
        }
    catch (std::exception& e)
        {
 	    sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}


bool MySqlAPI::hasStandAloneSeCfgAssigned(int file_id, std::string vo) {
    soci::session sql(*connectionPool);

    int count = 0;

    try
        {
            sql <<
                " select count(*) "
                " from t_file_share_config fc "
                " where fc.file_id = :id "
                "	and fc.vo = :vo "
                "	and ((fc.source <> '(*)' and fc.destination = '*') or (fc.source = '*' and fc.destination <> '(*)')) "
                "	and not exists ( "
                "		select null "
                "		from t_group_members g "
                "		where g.groupName = fc.source or g.groupName = fc.destination "
                "	) ",
                soci::use(file_id),
                soci::use(vo),
                soci::into(count)
                ;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return count > 0;
}

bool MySqlAPI::hasPairSeCfgAssigned(int file_id, std::string vo) {
    soci::session sql(*connectionPool);

    int count = 0;

    try
        {
            sql <<
                " select count(*) "
                " from t_file_share_config fc "
                " where fc.file_id = :id "
                "	and fc.vo = :vo "
                "	and (fc.source <> '(*)' and fc.source <> '*' and fc.destination <> '*' and fc.destination <> '(*)') "
				"	and not exists ( "
				"		select null "
				"		from t_group_members g "
				"		where g.groupName = fc.source or g.groupName = fc.destination "
				"	) ",
                soci::use(file_id),
                soci::use(vo),
                soci::into(count)
                ;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return count > 0;
}

bool MySqlAPI::hasStandAloneGrCfgAssigned(int file_id, std::string vo) {
    soci::session sql(*connectionPool);

    int count = 0;

    try
        {
            sql <<
                " select count(*) "
                " from t_file_share_config fc "
                " where fc.file_id = :id "
                "	and fc.vo = :vo "
                "	and ((fc.source <> '(*)' and fc.destination = '*') or (fc.source = '*' and fc.destination <> '(*)')) "
                "	and exists ( "
                "		select null "
                "		from t_group_members g "
                "		where g.groupName = fc.source or g.groupName = fc.destination "
                "	) ",
                soci::use(file_id),
                soci::use(vo),
                soci::into(count)
                ;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return count > 0;
}

bool MySqlAPI::hasPairGrCfgAssigned(int file_id, std::string vo) {
    soci::session sql(*connectionPool);

    int count = 0;

    try
        {
            sql <<
                " select count(*) "
                " from t_file_share_config fc "
                " where fc.file_id = :id "
                "	and fc.vo = :vo "
                "	and (fc.source <> '(*)' and fc.source <> '*' and fc.destination <> '*' and fc.destination <> '(*)') "
				"	and exists ( "
				"		select null "
				"		from t_group_members g "
				"		where g.groupName = fc.source or g.groupName = fc.destination "
				"	) ",
                soci::use(file_id),
                soci::use(vo),
                soci::into(count)
                ;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return count > 0;
}

// the class factories

extern "C" GenericDbIfce* create()
{
    return new MySqlAPI;
}

extern "C" void destroy(GenericDbIfce* p)
{
    if (p)
        delete p;
}

extern "C" MonitoringDbIfce* create_monitoring()
{
    return new MySqlMonitoring;
}

extern "C" void destroy_monitoring(MonitoringDbIfce* p)
{
    if (p)
        delete p;
}
