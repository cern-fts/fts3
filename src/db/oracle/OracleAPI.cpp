/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "OracleAPI.h"
#include "OracleMonitoring.h"
#include <fstream>
#include "error.h"
#include <algorithm>
#include "definitions.h"
#include <string>
#include <signal.h>
#include <stdlib.h>
#include <sys/param.h>
#include <math.h>
#include "queue_updater.h"

using namespace FTS3_COMMON_NAMESPACE;


static std::string _getTrTimestampUTC()
{
    time_t now = time(NULL);
    struct tm* tTime;
    tTime = gmtime(&now);
    time_t msec = mktime(tTime) * 1000; //the number of milliseconds since the epoch
    std::ostringstream oss;
    oss << fixed << msec;
    return oss.str();
}

static double convertBtoM( double byte,  double duration)
{
    return ceil((((byte / duration) / 1024) / 1024) * 100 + 0.5) / 100;
}

static double my_round(double x, unsigned int digits)
{
    if (digits > 0)
        {
            return my_round(x*10.0, digits-1)/10.0;
        }
    else
        {
            return round(x);
        }
}

static double convertKbToMb(double throughput)
{
    return throughput != 0.0? my_round((throughput / 1024), 2): 0.0;
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


OracleAPI::OracleAPI() : conn(NULL), conv(NULL), lowDefault(2), highDefault(4), jobsNum(3), filesNum(5)
{
}

OracleAPI::~OracleAPI()
{
    if (conn)
        delete conn;
    if (conv)
        delete conv;
}

void OracleAPI::init(std::string username, std::string password, std::string connectString, int pooledConn)
{
    if (!conn)
        conn = new OracleConnection(username, password, connectString,pooledConn);
    if (!conv)
        conv = new OracleTypeConversions();

    char hostname[MAXHOSTNAMELEN];
    gethostname(hostname, MAXHOSTNAMELEN);
    ftsHostName = std::string(hostname);
}

bool OracleAPI::getInOutOfSe(const std::string & sourceSe, const std::string & destSe)
{
    const std::string tagse = "getInOutOfSese";
    std::string query_stmt_se = " SELECT count(*) from t_se where "
                                " (t_se.name=:1 or t_se.name=:2) "
                                " and t_se.state='off' ";

    bool processSe = true;
    SafeStatement s_se;
    SafeResultSet rSe;
    SafeConnection pooledConnection;
    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return false;

            s_se = conn->createStatement(query_stmt_se, tagse, pooledConnection);
            s_se->setString(1, sourceSe);
            s_se->setString(2, destSe);
            rSe = conn->createResultset(s_se, pooledConnection);
            if (rSe->next())
                {
                    int count = rSe->getInt(1);
                    if (count > 0)
                        {
                            processSe = false;
                        }
                }

            conn->destroyResultset(s_se, rSe);
            conn->destroyStatement(s_se, tagse, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s_se && rSe)
                conn->destroyResultset(s_se, rSe);
            if (s_se)
                conn->destroyStatement(s_se, tagse, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch(...)
        {
            conn->rollback(pooledConnection);
            if(s_se && rSe)
                conn->destroyResultset(s_se, rSe);
            if (s_se)
                conn->destroyStatement(s_se, tagse, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return processSe;
}

TransferJobs* OracleAPI::getTransferJob(std::string jobId, bool archive)
{

    std::string tag;
    std::string query;

    if (archive)
        {
            tag = "getArchivedJob";
            query = "SELECT "
                    "   t_job_backup.vo_name,  "
                    "   t_job_backup.user_dn "
                    " FROM t_job_backup"
                    " WHERE t_job_backup.job_id = :1";
        }
    else
        {
            tag = "getTransferJob";
            query = "SELECT "
                    "   t_job.vo_name,  "
                    "   t_job.user_dn "
                    " FROM t_job"
                    " WHERE t_job.job_id = :1";
        }


    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    TransferJobs* job = NULL;
    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return job;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, jobId);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    job = new TransferJobs();
                    job->VO_NAME = r->getString(1);
                    job->USER_DN = r->getString(2);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);
    return job;
}

void OracleAPI::setFilesToNotUsed(std::string jobId, int fileIndex, std::vector<int>& files)
{

    std::string tag = "setFilesToNotUsed";
    std::string tag1 = "setFilesToNotUsedSelect";
    std::string tag2 = "setFilesToNotUsedSelectUpdated";

    std::string stmt =
        "UPDATE t_file "
        "SET file_state = 'NOT_USED' "
        "WHERE "
        "	job_id = :1 "
        "	AND file_index = :2 "
        "	AND file_state = 'SUBMITTED' "
        ;

    std::string select =
        "SELECT COUNT(*) "
        "FROM t_file "
        "WHERE job_id = :1 AND file_index = :2"
        ;

    std::string selectUpdated =
        " SELECT file_id "
        " FROM t_file "
        " WHERE job_id = :1 "
        "	AND file_index = :2 "
        "	AND file_state = 'NOT_USED' "
        ;

    SafeStatement s;
    SafeStatement s1;
    SafeResultSet r1;
    SafeStatement s2;
    SafeResultSet r2;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            // first really check if it is a multi-source/destination submission
            // count the alternative replicas, if there is more than one it makes sense to set the NOT_USED state

            s1 = conn->createStatement(select, tag1, pooledConnection);
            s1->setString(1, jobId);
            s1->setInt(2, fileIndex);
            r1 = conn->createResultset(s1, pooledConnection);

            int count = 0;
            if (r1->next())
                {
                    count = r1->getInt(1);
                }

            conn->destroyResultset(s1, r1);
            conn->destroyStatement(s1, tag1, pooledConnection);

            if (count < 2)
                {
                    conn->releasePooledConnection(pooledConnection);
                    return;
                }

            s = conn->createStatement(stmt, tag, pooledConnection);
            s->setString(1, jobId);
            s->setInt(2, fileIndex);
            unsigned int affected_rows = s->executeUpdate();
            conn->commit(pooledConnection);

            if (affected_rows > 0)
                {
                    s2 = conn->createStatement(selectUpdated, tag2, pooledConnection);
                    s2->setString(1, jobId);
                    s2->setInt(2, fileIndex);
                    r2 = conn->createResultset(s2, pooledConnection);

                    while (r2->next())
                        {
                            files.push_back(
                                r2->getInt(1)
                            );
                        }
                }

            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);

            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if(s2 && r2)
                conn->destroyResultset(s2, r2);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);


            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);

            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if(s2 && r2)
                conn->destroyResultset(s2, r2);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::useFileReplica(std::string jobId, int fileId)
{

    std::string selectTag = "selectUseFileReplica";
    std::string selectStmt =
        " SELECT file_index "
        " FROM t_file "
        " WHERE file_id = :1 "
        ;

    std::string updateTag = "updateUseFileReplica";
    std::string updateStmt =
        " UPDATE t_file "
        " SET file_state = 'SUBMITTED' "
        " WHERE job_id = :1 "
        "	AND file_index = :2 "
        "	AND file_state = 'NOT_USED'"
        ;

    SafeStatement s;
    SafeStatement s2;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            s = conn->createStatement(selectStmt, selectTag, pooledConnection);
            s->setInt(1, fileId);
            r = conn->createResultset(s, pooledConnection);

            if (r->next() && !r->isNull(1))
                {
                    int index = r->getInt(1);

                    s2 = conn->createStatement(updateStmt, updateTag, pooledConnection);
                    s2->setString(1, jobId);
                    s2->setInt(2, index);
                    s2->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s2, updateTag, pooledConnection);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, selectTag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, selectTag, pooledConnection);
            if(s2)
                conn->destroyStatement(s2, updateTag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, selectTag, pooledConnection);
            if(s2)
                conn->destroyStatement(s2, updateTag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);
}

unsigned int OracleAPI::updateFileStatus(TransferFiles* file, const std::string status)
{
    unsigned int updated = 0;
    unsigned int found = 0;
    const std::string tag1 = "updateFileStatus1";
    const std::string tag2 = "updateFileStatus2";
    const std::string tag3 = "updateFileStatus3";

    std::string query1 =
        "UPDATE t_file "
        "SET file_state =:1, start_time=:2, transferHost=:3 "
        "WHERE file_id = :4 AND FILE_STATE='SUBMITTED' ";
    std::string query2 =
        "UPDATE t_job "
        "SET job_state =:1 "
        "WHERE job_id = :2 AND JOB_STATE='SUBMITTED' ";

    std::string query3 = "select count(*) from t_file where file_state in ('READY','ACTIVE') and dest_surl=:1 ";

    SafeStatement s1;
    SafeStatement s2;
    SafeStatement s3;
    SafeResultSet r3;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return updated;

            s3 = conn->createStatement(query3, tag3, pooledConnection);
            s3->setString(1, file->DEST_SURL);
            r3 = conn->createResultset(s3, pooledConnection);
            if (r3->next())
                {
                    found = r3->getInt(1);
                }
            conn->destroyResultset(s3, r3);
            conn->destroyStatement(s3, tag3, pooledConnection);

            if(found != 0)
                return 0;

            time_t timed = time(NULL);
            s1 = conn->createStatement(query1, tag1, pooledConnection);
            s1->setString(1, status);
            s1->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv()));
            s1->setString(3, ftsHostName);
            s1->setInt(4, file->FILE_ID);
            updated = s1->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s1, tag1, pooledConnection);

            s2 = conn->createStatement(query2, tag2, pooledConnection);
            s2->setString(1, status);
            s2->setString(2, file->JOB_ID);
            s2->executeUpdate();
            conn->commit(pooledConnection);

            conn->destroyStatement(s2, tag2, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);

            if(s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if(s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            if (r3 && s3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);

            if(s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if(s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            if (r3 && s3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);
    return updated;
}

void OracleAPI::getByJobIdReuse(std::vector<TransferJobs*>& jobs, std::map< std::string, std::list<TransferFiles*> >& files, bool reuse)
{
    TransferFiles* tr_files = NULL;
    std::vector<TransferJobs*>::const_iterator iter;
    std::string selecttag = "getByJobIdReuse";

    std::string select =
        "SELECT "
        "		source_surl, dest_surl, job_id, vo_name, "
        " 		file_id, overwrite_flag, USER_DN, CRED_ID, "
        "		checksum, CHECKSUM_METHOD, SOURCE_SPACE_TOKEN, "
        " 		SPACE_TOKEN, copy_pin_lifetime, bring_online, "
        "		user_filesize, file_metadata, job_metadata, file_index, BRINGONLINE_TOKEN,"
        "		source_se, dest_se, selection_strategy  "
        "		from (select "
        "		f1.source_surl, f1.dest_surl, f1.job_id, j.vo_name, "
        " 		f1.file_id, j.overwrite_flag, j.USER_DN, j.CRED_ID, "
        "		f1.checksum, j.CHECKSUM_METHOD, j.SOURCE_SPACE_TOKEN, "
        " 		j.SPACE_TOKEN, j.copy_pin_lifetime, j.bring_online, "
        "		f1.user_filesize, f1.file_metadata, j.job_metadata, f1.file_index, f1.BRINGONLINE_TOKEN,"
        "		f1.source_se, f1.dest_se, f1.selection_strategy  "
        "FROM t_file f1, t_job j "
        "WHERE "
        "	j.job_id = :1 AND "
        "	j.job_id = f1.job_id AND "
        "	f1.file_state ='SUBMITTED' AND "
        "       j.job_finished is NULL AND "
        "	f1.wait_timestamp IS NULL AND "
        " 	(f1.retry_timestamp is NULL OR f1.retry_timestamp < :2) "
        " ORDER BY f1.file_id ASC) WHERE ROWNUM <= :3 ";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {
            int mode = getOptimizerMode();
            if(mode==1)
                {
                    filesNum = mode_1[3];
                }
            else if(mode==2)
                {
                    filesNum = mode_2[3];
                }
            else if(mode==3)
                {
                    filesNum = mode_3[3];
                }
            else
                {
                    filesNum = mode_1[3];
                }

            if(reuse)
                {
                    filesNum = 10000;
                }


            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(select, selecttag, pooledConnection);

            for (iter = jobs.begin(); iter != jobs.end(); ++iter)
                {
                    time_t timed = time(NULL);
                    TransferJobs* temp = (TransferJobs*) * iter;
                    std::string job_id = std::string(temp->JOB_ID);
                    s->setString(1, job_id);
                    s->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv())); //submit_time
                    s->setInt(3, filesNum);
                    r = conn->createResultset(s, pooledConnection);
                    while (r->next())
                        {
                            tr_files = new TransferFiles();
                            tr_files->SOURCE_SURL = r->getString(1);
                            tr_files->DEST_SURL = r->getString(2);
                            tr_files->JOB_ID = r->getString(3);
                            tr_files->VO_NAME = r->getString(4);
                            tr_files->FILE_ID = r->getInt(5);
                            tr_files->OVERWRITE = r->getString(6);
                            tr_files->DN = r->getString(7);
                            tr_files->CRED_ID = r->getString(8);
                            tr_files->CHECKSUM = r->getString(9);
                            tr_files->CHECKSUM_METHOD = r->getString(10);
                            tr_files->SOURCE_SPACE_TOKEN = r->getString(11);
                            tr_files->DEST_SPACE_TOKEN = r->getString(12);
                            tr_files->PIN_LIFETIME = r->getInt(13);
                            tr_files->BRINGONLINE = r->getInt(14);
                            tr_files->USER_FILESIZE = r->getDouble(15);
                            tr_files->FILE_METADATA = r->getString(16);
                            tr_files->JOB_METADATA = r->getString(17);
                            tr_files->FILE_INDEX = r->getInt(18);
                            tr_files->BRINGONLINE_TOKEN = r->getString(19);
                            tr_files->SOURCE_SE = r->getString(20);
                            tr_files->DEST_SE = r->getString(21);
                            tr_files->SELECTION_STRATEGY = r->getString(22);

                            files[tr_files->VO_NAME].push_back(tr_files);
                        }
                    conn->destroyResultset(s, r);
                }

            conn->destroyStatement(s, selecttag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if (r && s)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, selecttag, pooledConnection);
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if (r && s)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, selecttag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);
}


void OracleAPI::getByJobId(
    std::vector<boost::tuple<std::string, std::string, std::string> >& /*distinct*/,
    std::map<std::string, std::list<TransferFiles*> >& files)
{
    // Queries
    const std::string voQuery = " SELECT DISTINCT vo_name "
                                " FROM t_job "
                                " WHERE job_finished is NULL AND cancel_job IS NULL AND t_job.job_state IN ('ACTIVE', 'READY','SUBMITTED')";
    const std::string voTag = "getByJobId/vo";

    const std::string pairQuery = "SELECT DISTINCT f.source_se, f.dest_se from t_job j RIGHT JOIN t_file f "
                                  " ON (j.job_id = f.job_id) WHERE j.vo_name = :1 and f.file_state='SUBMITTED' ";
    const std::string pairTag = "getByJobId/pair";

    const std::string transferQuery = "SELECT * FROM ("
                                      "    SELECT "
                                      "       f.file_state, f.source_surl, f.dest_surl, f.job_id, j.vo_name, "
                                      "       f.file_id, j.overwrite_flag, j.user_dn, j.cred_id, "
                                      "       f.checksum, j.checksum_method, j.source_space_token, "
                                      "       j.space_token, j.copy_pin_lifetime, j.bring_online, "
                                      "       f.user_filesize, f.file_metadata, j.job_metadata, f.file_index, f.bringonline_token, "
                                      "       f.source_se, f.dest_se, f.selection_strategy, rownum as rw  "
                                      "    FROM t_job j RIGHT JOIN t_file f ON (j.job_id = f.job_id) "
                                      "    WHERE f.file_state = 'SUBMITTED' AND  f.source_se = :1 AND f.dest_se = :2 AND"
                                      "       j.vo_name = :3 AND "
                                      "       j.job_state IN ('ACTIVE', 'READY','SUBMITTED') AND "
                                      "       f.wait_timestamp IS NULL AND "
                                      "       (j.reuse_job = 'N' OR j.reuse_job IS NULL) AND "
                                      "       (f.retry_timestamp is NULL OR f.retry_timestamp < :4) "
                                      "       ORDER BY j.priority DESC, j.submit_time "
                                      ") WHERE rw <= :5";
    const std::string transferTag = "getByJobId/transfer";

    SafeConnection pooledConnection;

    try
        {
            SafeConnection pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            int mode = getOptimizerMode();
            if(mode == 1)
                filesNum = mode_1[3];
            else if(mode == 2)
                filesNum = mode_2[3];
            else if(mode == 3)
                filesNum = mode_3[3];
            else
                filesNum = mode_1[3];

            // Get pairs per VO
            std::vector< boost::tuple<std::string, std::string, std::string> > distinct;
            distinct.reserve(1500); //approximation

            SafeStatement voStmt = conn->createStatement(voQuery, voTag, pooledConnection);
            SafeResultSet voRs = conn->createResultset(voStmt, pooledConnection);

            while (voRs->next())
                {
                    std::string voName = voRs->getString(1);

                    SafeStatement pairStmt = conn->createStatement(pairQuery, pairTag, pooledConnection);
                    pairStmt->setString(1, voName);
                    SafeResultSet pairRs = conn->createResultset(pairStmt, pooledConnection);
                    while (pairRs->next())
                        {
                            distinct.push_back(
                                boost::tuple<std::string, std::string, std::string>
                                (pairRs->getString(1), pairRs->getString(2), voName));
                        }
                }


            // Iterate through pairs, getting jobs IF the VO has not run out of credits
            // AND there are pending file transfers within the job
            std::vector< boost::tuple<std::string, std::string, std::string> >::iterator it;
            for (it = distinct.begin(); it != distinct.end(); ++it)
                {
                    boost::tuple<std::string, std::string, std::string>& triplet = *it;

                    SafeStatement transferStmt = conn->createStatement(transferQuery, transferTag, pooledConnection);
                    transferStmt->setString(1, boost::get<0>(triplet));
                    transferStmt->setString(2, boost::get<1>(triplet));
                    transferStmt->setString(3, boost::get<2>(triplet));
                    transferStmt->setTimestamp(4, conv->toTimestamp(time(NULL), conn->getEnv()));
                    transferStmt->setInt(5, filesNum);

                    SafeResultSet transferRs = conn->createResultset(transferStmt, pooledConnection);
                    while (transferRs->next())
                        {
                            TransferFiles* tFile = new TransferFiles();

                            tFile->FILE_STATE  = transferRs->getString(1);
                            tFile->SOURCE_SURL = transferRs->getString(2);
                            tFile->DEST_SURL   = transferRs->getString(3);
                            tFile->JOB_ID      = transferRs->getString(4);
                            tFile->VO_NAME     = transferRs->getString(5);
                            tFile->FILE_ID     = transferRs->getInt(6);
                            tFile->OVERWRITE   = transferRs->getString(7);
                            tFile->DN          = transferRs->getString(8);
                            tFile->CRED_ID     = transferRs->getString(9);
                            tFile->CHECKSUM    = transferRs->getString(10);
                            tFile->CHECKSUM_METHOD = transferRs->getString(11);
                            tFile->SOURCE_SPACE_TOKEN = transferRs->getString(12);
                            tFile->DEST_SPACE_TOKEN   = transferRs->getString(13);
                            tFile->PIN_LIFETIME  = transferRs->getInt(14);
                            tFile->BRINGONLINE   = transferRs->getInt(15);
                            tFile->USER_FILESIZE = transferRs->getNumber(16);
                            tFile->FILE_METADATA = transferRs->getString(17);
                            tFile->JOB_METADATA  = transferRs->getString(18);
                            tFile->FILE_INDEX    = transferRs->getInt(19);
                            tFile->BRINGONLINE_TOKEN = transferRs->getString(20);
                            tFile->SOURCE_SE     = transferRs->getString(21);
                            tFile->DEST_SE       = transferRs->getString(22);
                            tFile->SELECTION_STRATEGY = transferRs->getString(23);

                            files[boost::get<2>(triplet)].push_back(tFile);
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



void OracleAPI::submitPhysical(const std::string & jobId, std::vector<job_element_tupple> job_elements, const std::string & paramFTP,
                               const std::string & DN, const std::string & cred, const std::string & voName, const std::string & myProxyServer,
                               const std::string & delegationID, const std::string & spaceToken, const std::string & overwrite,
                               const std::string & sourceSpaceToken, const std::string &, int copyPinLifeTime,
                               const std::string & failNearLine, const std::string & checksumMethod, const std::string & reuse,
                               int bringonline, std::string metadata,
                               int retry, int retryDelay, std::string sourceSe, std::string destinationSe)
{


    const std::string initial_state = bringonline > 0 || copyPinLifeTime > 0 ? "STAGING" : "SUBMITTED";
    time_t timed = time(NULL);
    const std::string currenthost = ftsHostName; //current hostname
    const std::string tag_job_statement = "tag_job_statement";
    const std::string tag_file_statement = "tag_file_statement";
    const std::string job_statement =
        "INSERT INTO t_job(job_id, job_state, job_params, user_dn, user_cred, priority, "
        " vo_name,submit_time,internal_job_params,submit_host, cred_id, myproxy_server, "
        " SPACE_TOKEN, overwrite_flag,SOURCE_SPACE_TOKEN,copy_pin_lifetime, "
        " fail_nearline, checksum_method, REUSE_JOB, bring_online, job_metadata, retry, retry_delay, "
        " source_se, dest_se) "
        " VALUES (:1,:2,:3,:4,:5,:6,:7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17, :18, :19, :20, :21, :22, :23, :24, :25)";

    const std::string file_statement =
        " INSERT "
        " INTO t_file ("
        "	job_id, "
        "	file_state, "
        "	source_surl, "
        "	dest_surl, "
        "	checksum, "
        "	user_filesize, "
        "	file_metadata, "
        "	selection_strategy, "
        "	file_index, "
        "	source_se, "
        "	dest_se, "
        "   wait_timestamp, "
        "   wait_timeout "
        " ) "
        " VALUES (:1,:2,:3,:4,:5,:6,:7,:8, :9, :10, :11, :12, :13)";

    SafeStatement s_job_statement;
    SafeStatement s_file_statement;
    SafeConnection pooledConnection;
    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                throw Err_Custom("Can't connect to the database");

            s_job_statement = conn->createStatement(job_statement, tag_job_statement, pooledConnection);
            s_job_statement->setString(1, jobId); //job_id
            s_job_statement->setString(2, initial_state); //job_state
            s_job_statement->setString(3, paramFTP); //job_params
            s_job_statement->setString(4, DN); //user_dn
            s_job_statement->setString(5, cred); //user_cred
            s_job_statement->setInt(6, 3); //priority
            s_job_statement->setString(7, voName); //vo_name
            s_job_statement->setTimestamp(8, conv->toTimestamp(timed, conn->getEnv())); //submit_time
            s_job_statement->setString(9, ""); //internal_job_params
            s_job_statement->setString(10, currenthost); //submit_host
            s_job_statement->setString(11, delegationID); //cred_id
            s_job_statement->setString(12, myProxyServer); //myproxy_server
            s_job_statement->setString(13, spaceToken); //space_token
            s_job_statement->setString(14, overwrite); //overwrite_flag
            s_job_statement->setString(15, sourceSpaceToken); //source_space_token
            s_job_statement->setInt(16, copyPinLifeTime); //copy_pin_lifetime
            s_job_statement->setString(17, failNearLine); //fail_nearline
            if (checksumMethod.length() == 0)
                s_job_statement->setNull(18, oracle::occi::OCCICHAR);
            else
                s_job_statement->setString(18, checksumMethod.substr(0, 1)); //checksum_method
            if (reuse.length() == 0)
                s_job_statement->setNull(19, oracle::occi::OCCISTRING);
            else
                s_job_statement->setString(19, "Y"); //reuse session for this job

            s_job_statement->setInt(20, bringonline); //reuse session for this job
            s_job_statement->setString(21, metadata); // job metadata
            s_job_statement->setInt(22, retry); // number of retries
            s_job_statement->setInt(23, retryDelay); // delay between retries
            s_job_statement->setString(24, sourceSe); // source SE
            s_job_statement->setString(25, destinationSe); // destination SE
            s_job_statement->executeUpdate();

            //now insert each src/dest pair for this job id
            std::vector<job_element_tupple>::const_iterator iter;
            s_file_statement = conn->createStatement(file_statement, tag_file_statement, pooledConnection);

            for (iter = job_elements.begin(); iter != job_elements.end(); ++iter)
                {
                    oracle::occi::Timestamp wait_timestamp;
                    oracle::occi::Number wait_timeout;

                    if (iter->wait_timeout.is_initialized())
                        {
                            time_t timed = time(NULL);
                            wait_timestamp = conv->toTimestamp(timed, conn->getEnv());
                            wait_timeout = oracle::occi::Number(*iter->wait_timeout);
                        }
                    else
                        {
                            wait_timeout.setNull();
                            wait_timestamp.setNull();
                        }

                    s_file_statement->setString(1, jobId);
                    s_file_statement->setString(2, initial_state);
                    s_file_statement->setString(3, iter->source);
                    s_file_statement->setString(4, iter->destination);
                    s_file_statement->setString(5, iter->checksum);
                    s_file_statement->setDouble(6, iter->filesize);
                    s_file_statement->setString(7, iter->metadata);
                    s_file_statement->setString(8, iter->selectionStrategy);
                    s_file_statement->setInt(9, iter->fileIndex);
                    s_file_statement->setString(10, iter->source_se);
                    s_file_statement->setString(11, iter->dest_se);
                    s_file_statement->setTimestamp(12, wait_timestamp);
                    s_file_statement->setNumber(13, wait_timeout);

                    s_file_statement->executeUpdate();
                }
            conn->commit(pooledConnection);
            conn->destroyStatement(s_job_statement, tag_job_statement, pooledConnection);
            conn->destroyStatement(s_file_statement, tag_file_statement, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);

            if(s_job_statement)
                {
                    conn->destroyStatement(s_job_statement, tag_job_statement, pooledConnection);
                }

            if(s_file_statement)
                {
                    conn->destroyStatement(s_file_statement, tag_file_statement, pooledConnection);
                }
            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {
            conn->rollback(pooledConnection);

            if(s_job_statement)
                {
                    conn->destroyStatement(s_job_statement, tag_job_statement, pooledConnection);
                }

            if(s_file_statement)
                {
                    conn->destroyStatement(s_file_statement, tag_file_statement, pooledConnection);
                }
            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }

    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::getTransferJobStatus(std::string requestID, bool archive, std::vector<JobStatus*>& jobs)
{
    std::string query;
    std::string tag;

    if (archive)
        {
            tag = "getArchivedJobStatus";
            query = "SELECT t_job_backup.job_id, t_job_backup.job_state, t_file_backup.file_state, t_job_backup.user_dn, t_job_backup.reason, "
                    "       t_job_backup.submit_time, t_job_backup.priority, t_job_backup.vo_name, t_file_backup.file_index, "
                    "       (SELECT count(DISTINCT file_index) from t_file_backup where t_file_backup.job_id=:1) "
                    "FROM t_job_backup, t_file_backup WHERE t_file_backup.job_id = t_job_backup.job_id and t_file_backup.job_id = :2";
        }
    else
        {
            tag = "getTransferJobStatus";
            query = "SELECT t_job.job_id, t_job.job_state, t_file.file_state, t_job.user_dn, t_job.reason, "
                    "       t_job.submit_time, t_job.priority, t_job.vo_name, t_file.file_index, "
                    "       (SELECT count(DISTINCT file_index) from t_file where t_file.job_id=:1) "
                    "FROM t_job, t_file WHERE t_file.job_id = t_job.job_id and t_file.job_id = :2";
        }

    JobStatus* js = NULL;
    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                throw Err_Custom("Can't connect to the database");

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, requestID);
            s->setString(2, requestID);
            r = conn->createResultset(s, pooledConnection);
            while (r->next())
                {
                    js = new JobStatus();
                    js->jobID = r->getString(1);
                    js->jobStatus = r->getString(2);
                    js->fileStatus = r->getString(3);
                    js->clientDN = r->getString(4);
                    js->reason = r->getString(5);
                    js->submitTime = conv->toTimeT(r->getTimestamp(6));
                    js->priority = r->getInt(7);
                    js->voName = r->getString(8);
                    js->fileIndex = r->getInt(9);
                    js->numFiles = r->getInt(10);
                    jobs.push_back(js);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }

    conn->releasePooledConnection(pooledConnection);
}

/*
 * Return a list of jobs based on the status requested
 * std::vector<JobStatus*> jobs: the caller will deallocate memory JobStatus instances and clear the vector
 * std::vector<std::string> inGivenStates: order doesn't really matter, more than one states supported
 */
void OracleAPI::listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates, std::string restrictToClientDN, std::string forDN, std::string VOname)
{

    JobStatus* j = NULL;
    bool checkForCanceling = false;
    unsigned int cc = 1;
    std::string jobStatuses("");

    /*this statement cannot be prepared, it's generated dynamically*/
    std::string tag = "listRequests";
    std::string sel = "SELECT DISTINCT job_id, job_state, reason, submit_time, user_dn, J.vo_name,(SELECT count(DISTINCT file_index) from t_file where t_file.job_id = J.job_id), priority, cancel_job FROM t_job J ";
    //gain the benefit from the statement pooling
    std::sort(inGivenStates.begin(), inGivenStates.end());
    std::vector<std::string>::const_iterator foundCancel;

    if (inGivenStates.size() > 0)
        {
            foundCancel = std::find_if(inGivenStates.begin(), inGivenStates.end(), bind2nd(std::equal_to<std::string > (), std::string("Canceling")));
            if (foundCancel == inGivenStates.end())   //not found
                {
                    checkForCanceling = true;
                    tag.append("1");
                }
            tag.append("2");
        }

    if (inGivenStates.size() > 0)
        {
            jobStatuses = "'" + inGivenStates[0] + "'";
            for (unsigned int i = 1; i < inGivenStates.size(); i++)
                {
                    jobStatuses += (",'" + inGivenStates[i] + "'");
                }
            tag.append("3");
        }

    if (restrictToClientDN.length() > 0)
        {
            sel.append(" LEFT OUTER JOIN T_SE_PAIR_ACL C ON J.SE_PAIR_NAME = C.SE_PAIR_NAME LEFT OUTER JOIN t_vo_acl V ON J.vo_name = V.vo_name ");
            tag.append("4");
        }

    if (inGivenStates.size() > 0)
        {
            sel.append(" WHERE J.job_state IN (" + jobStatuses + ") ");
            tag.append(jobStatuses);
        }
    else
        {
            sel.append(" WHERE J.job_state <> '0' ");
            tag.append("6");
        }

    if (restrictToClientDN.length() > 0)
        {
            sel.append(" AND (J.user_dn = :1 OR V.principal = :2 OR C.principal = :3) ");
            tag.append("7");
        }

    if (VOname.length() > 0)
        {
            sel.append(" AND J.vo_name = :4");
            tag.append("8");
        }

    if (forDN.length() > 0)
        {
            sel.append(" AND J.user_dn = :5");
            tag.append("9");
        }

    if (!checkForCanceling)
        {
            sel.append(" AND NVL(J.cancel_job,'X') <> 'Y'");
            tag.append("10");
        }

    tag.append("11");

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                throw Err_Custom("Can't connect to the database");

            s = conn->createStatement(sel, tag, pooledConnection);
            if (restrictToClientDN.length() > 0)
                {
                    s->setString(cc++, restrictToClientDN);
                    s->setString(cc++, restrictToClientDN);
                    s->setString(cc++, restrictToClientDN);
                }

            if (VOname.length() > 0)
                {
                    s->setString(cc++, VOname);
                }

            if (forDN.length() > 0)
                {
                    s->setString(cc, forDN);
                }


            r = conn->createResultset(s, pooledConnection);
            while (r->next())
                {
                    std::string jid = r->getString(1);
                    std::string jstate = r->getString(2);
                    std::string reason = r->getString(3);
                    time_t tm = conv->toTimeT(r->getTimestamp(4));
                    std::string dn = r->getString(5);
                    std::string voName = r->getString(6);
                    int fileCount = r->getInt(7);
                    int priority = r->getInt(8);

                    j = new JobStatus();
                    if (j)
                        {
                            j->jobID = jid;
                            j->jobStatus = jstate;
                            j->reason = reason;
                            j->numFiles = fileCount;
                            j->voName = voName;
                            j->submitTime = tm;
                            j->clientDN = dn;
                            j->priority = priority;
                            jobs.push_back(j);
                        }
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {
            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }

    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::getTransferFileStatus(std::string requestID, bool archive,
                                      unsigned offset, unsigned limit, std::vector<FileTransferStatus*>& files)
{
    std::string tag;
    std::string query;

    if (archive)
        {
            tag = "getArchivedFileStatus";
            query = "SELECT * FROM ("
                    "   SELECT t_file_backup.file_id, t_file_backup.SOURCE_SURL, t_file_backup.DEST_SURL, "
                    "          t_file_backup.file_state, t_file_backup.reason, "
                    "          t_file_backup.start_time, t_file_backup.finish_time, t_file_backup.retry, "
                    "          Rownum as rw "
                    "   FROM t_file_backup WHERE t_file_backup.job_id = :1) ";
        }
    else
        {
            tag = "getTransferFileStatus";
            query = "SELECT * FROM ("
                    "   SELECT t_file.file_id, t_file.SOURCE_SURL, t_file.DEST_SURL, "
                    "          t_file.file_state, t_file.reason, "
                    "          t_file.start_time, t_file.finish_time, t_file.retry, "
                    "          Rownum as rw "
                    "   FROM t_file WHERE t_file.job_id = :1) ";
        }

    if (limit)
        {
            query += "WHERE rw BETWEEN :2 AND :3";
        }
    else
        {
            query += "WHERE rw >= :2";
            tag += "Limited";
        }


    FileTransferStatus* js = 0;
    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                throw Err_Custom("Can't connect to the database");


            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, requestID);
            // Note: Rownum starts counting on 1!
            s->setInt(2, offset + 1);
            if (limit)
                s->setInt(3, offset + limit);
            r = conn->createResultset(s, pooledConnection);
            while (r->next())
                {
                    js = new FileTransferStatus();
                    js->fileId            = r->getInt(1);
                    js->sourceSURL        = r->getString(2);
                    js->destSURL          = r->getString(3);
                    js->transferFileState = r->getString(4);
                    js->reason            = r->getString(5);
                    js->start_time        = conv->toTimeT(r->getTimestamp(6));
                    js->finish_time       = conv->toTimeT(r->getTimestamp(7));
                    js->numFailures       = r->getInt(8);
                    files.push_back(js);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {
            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::getSe(Se* &se, std::string seName)
{
    const std::string tag = "getSeTest";
    std::string query_stmt =
        "SELECT "
        "	t_se.ENDPOINT, "
        " 	t_se.SE_TYPE, "
        " 	t_se.SITE,  "
        " 	t_se.NAME,  "
        " 	t_se.STATE, "
        " 	t_se.VERSION,  "
        " 	t_se.HOST, "
        " 	t_se.SE_TRANSFER_TYPE, "
        " 	t_se.SE_TRANSFER_PROTOCOL, "
        " 	t_se.SE_CONTROL_PROTOCOL, "
        " 	t_se.GOCDB_ID "
        "FROM t_se "
        "WHERE t_se.NAME=:1";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query_stmt, tag, pooledConnection);
            s->setString(1, seName);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    se = new Se();
                    se->ENDPOINT = r->getString(1);
                    se->SE_TYPE = r->getString(2);
                    se->SITE = r->getString(3);
                    se->NAME = r->getString(4);
                    se->STATE = r->getString(5);
                    se->VERSION = r->getString(6);
                    se->HOST = r->getString(7);
                    se->SE_TRANSFER_TYPE = r->getString(8);
                    se->SE_TRANSFER_PROTOCOL = r->getString(9);
                    se->SE_CONTROL_PROTOCOL = r->getString(10);
                    se->GOCDB_ID = r->getString(11);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);


            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
                      std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID)
{
    std::string query = "INSERT INTO t_se (ENDPOINT, SE_TYPE, SITE, NAME, STATE, VERSION, HOST, SE_TRANSFER_TYPE, SE_TRANSFER_PROTOCOL,SE_CONTROL_PROTOCOL,GOCDB_ID) VALUES (:1,:2,:3,:4,:5,:6,:7,:8,:9,:10,:11)";
    std::string tag = "addSe";

    SafeStatement s;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, ENDPOINT);
            s->setString(2, SE_TYPE);
            s->setString(3, SITE);
            s->setString(4, NAME);
            s->setString(5, STATE);
            s->setString(6, VERSION);
            s->setString(7, HOST);
            s->setString(8, SE_TRANSFER_TYPE);
            s->setString(9, SE_TRANSFER_PROTOCOL);
            s->setString(10, SE_CONTROL_PROTOCOL);
            s->setString(11, GOCDB_ID);
            if( s->executeUpdate() != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
                         std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID)
{
    int fields = 0;
    std::string query = "UPDATE T_SE SET ";
    if (ENDPOINT.length() > 0)
        {
            query.append(" ENDPOINT='");
            query.append(ENDPOINT);
            query.append("'");
            fields++;
        }
    if (SE_TYPE.length() > 0)
        {
            if (fields > 0)
                {
                    fields = 0;
                    query.append(", ");
                }
            query.append(" SE_TYPE='");
            query.append(SE_TYPE);
            query.append("'");
            fields++;
        }
    if (SITE.length() > 0)
        {
            if (fields > 0)
                {
                    fields = 0;
                    query.append(", ");
                }
            query.append(" SITE='");
            query.append(SITE);
            query.append("'");
            fields++;
        }
    if (STATE.length() > 0)
        {
            if (fields > 0)
                {
                    fields = 0;
                    query.append(", ");
                }
            query.append(" STATE='");
            query.append(STATE);
            query.append("'");
            fields++;
        }
    if (VERSION.length() > 0)
        {
            if (fields > 0)
                {
                    fields = 0;
                    query.append(", ");
                }
            query.append(" VERSION='");
            query.append(VERSION);
            query.append("'");
            fields++;
        }
    if (HOST.length() > 0)
        {
            if (fields > 0)
                {
                    fields = 0;
                    query.append(", ");
                }
            query.append(" HOST='");
            query.append(HOST);
            query.append("'");
            fields++;
        }
    if (SE_TRANSFER_TYPE.length() > 0)
        {
            if (fields > 0)
                {
                    fields = 0;
                    query.append(", ");
                }
            query.append(" SE_TRANSFER_TYPE='");
            query.append(SE_TRANSFER_TYPE);
            query.append("'");
            fields++;
        }
    if (SE_TRANSFER_PROTOCOL.length() > 0)
        {
            if (fields > 0)
                {
                    fields = 0;
                    query.append(", ");
                }
            query.append(" SE_TRANSFER_PROTOCOL='");
            query.append(SE_TRANSFER_PROTOCOL);
            query.append("'");
            fields++;
        }
    if (SE_CONTROL_PROTOCOL.length() > 0)
        {
            if (fields > 0)
                {
                    fields = 0;
                    query.append(", ");
                }
            query.append(" SE_CONTROL_PROTOCOL='");
            query.append(SE_CONTROL_PROTOCOL);
            query.append("'");
            fields++;
        }
    if (GOCDB_ID.length() > 0)
        {
            if (fields > 0)
                {
                    fields = 0;
                    query.append(", ");
                }
            query.append(" GOCDB_ID='");
            query.append(GOCDB_ID);
            query.append("'");
            fields++;
        }
    query.append(" WHERE NAME='");
    query.append(NAME);
    query.append("'");
    SafeStatement s;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, "", pooledConnection);

            if(s->executeUpdate()!=0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, "", pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, "", pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, "", pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}



/*
Delete a SE
REQUIRED: NAME
 */
void OracleAPI::deleteSe(std::string NAME)
{
    std::string query = "DELETE FROM T_SE WHERE NAME = :1";
    std::string tag = "deleteSe";

    SafeStatement s;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, NAME);
            if(s->executeUpdate()!=0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}



bool OracleAPI::updateFileTransferStatus(double throughputIn, std::string job_id, int file_id, std::string transfer_status, std::string transfer_message, int process_id, double
        filesize, double duration)
{
    unsigned int index = 1;
    double throughput = 0;
    bool ok = true;
    std::string tag = "updateFileTransferStatus";
    std::string tag1 = "getCurrentState";
    std::stringstream query;
    bool isStagingState = false;
    std::string query1 = "select file_state from t_file where file_id=:1 and job_id=:2";
    SafeStatement s;
    SafeStatement s1;
    SafeResultSet r1;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                {
                    ok = false;
                    return ok;
                }

            s1 = conn->createStatement(query1, tag1, pooledConnection);
            s1->setInt(1, file_id);
            s1->setString(2, job_id);
            r1 = conn->createResultset(s1, pooledConnection);
            if (r1->next())
                {
                    isStagingState = std::string(r1->getString(1)).compare("STAGING")==0? true: false;
                }
            conn->destroyResultset(s1, r1);
            conn->destroyStatement(s1, tag1, pooledConnection);

            query << "UPDATE t_file SET file_state=:" << 1 << ", REASON=:" << ++index;
            if ((transfer_status.compare("FINISHED") == 0) || (transfer_status.compare("FAILED") == 0) || (transfer_status.compare("CANCELED") == 0))
                {
                    query << ", FINISH_TIME=:" << ++index;
                    tag.append("xx");
                }
            if ((transfer_status.compare("ACTIVE") == 0))
                {
                    query << ", START_TIME=:" << ++index;
                    tag.append("xx1");
                }
            if ((transfer_status.compare("STAGING") == 0))
                {
                    if(isStagingState==false)
                        {
                            query << ", STAGING_START=:" << ++index;
                            tag.append("xx2");
                        }
                    else
                        {
                            query << ", STAGING_FINISHED=:" << ++index;
                            tag.append("xx3");
                        }
                }
            query << ", PID=:" << ++index;
            query << ", FILESIZE=:" << ++index;
            query << ", TX_DURATION=:" << ++index;
            query << ", THROUGHPUT=:" << ++index;
            query << " WHERE file_id =:" << ++index;
            query << " and file_state not in ('FAILED', 'FINISHED', 'CANCELED')";

            s = conn->createStatement(query.str(), tag, pooledConnection);
            index = 1; //reset index
            s->setString(1, transfer_status);
            ++index;
            s->setString(index, transfer_message);
            if ((transfer_status.compare("FINISHED") == 0) || (transfer_status.compare("FAILED") == 0) || (transfer_status.compare("CANCELED") == 0))
                {
                    time_t timed = time(NULL);
                    ++index;
                    s->setTimestamp(index, conv->toTimestamp(timed, conn->getEnv()));
                }
            if ((transfer_status.compare("ACTIVE") == 0))
                {
                    time_t timed = time(NULL);
                    ++index;
                    s->setTimestamp(index, conv->toTimestamp(timed, conn->getEnv()));
                }
            if ((transfer_status.compare("STAGING") == 0))
                {
                    if(isStagingState==false)
                        {
                            time_t timed = time(NULL);
                            ++index;
                            s->setTimestamp(index, conv->toTimestamp(timed, conn->getEnv()));
                        }
                    else
                        {
                            time_t timed = time(NULL);
                            ++index;
                            s->setTimestamp(index, conv->toTimestamp(timed, conn->getEnv()));
                        }
                }
            ++index;
            s->setInt(index, process_id);
            ++index;
            if(filesize <= 0)
                filesize = 0;
            s->setDouble(index, filesize);
            ++index;
            if(duration <= 0)
                duration = 1;
            s->setDouble(index, duration);
            ++index;
            if(filesize > 0 && duration> 0 && (transfer_status.compare("FINISHED") == 0))
                {
                    if(throughputIn != 0.0)
                        throughput = convertKbToMb(throughputIn);
                    else
                        throughput = convertBtoM(filesize, duration);
                    s->setDouble(index, throughput);
                }
            else if(filesize > 0 && duration<= 0 && (transfer_status.compare("FINISHED") == 0))
                {
                    if(throughputIn != 0.0)
                        throughput = convertKbToMb(throughputIn);
                    else
                        throughput = convertBtoM(filesize, 1);
                    s->setDouble(index, throughput);
                }
            else
                {
                    s->setDouble(index, 0);
                }
            ++index;
            s->setInt(index, file_id);

            if(s->executeUpdate()!=0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);
            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
            ok = false;
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);
            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
            ok = false;
        }
    conn->releasePooledConnection(pooledConnection);
    return ok;
}

bool OracleAPI::updateJobTransferStatus(int, std::string job_id, const std::string status)
{
    std::string reason = "One or more files failed. Please have a look at the details for more information";
    const std::string terminal1 = "FINISHED";
    const std::string terminal2 = "FAILED";
    const std::string terminal4 = "FINISHEDDIRTY";
    const std::string terminal5 = "CANCELED";
    bool finished = true;

    int numOfFilesInJob = 0;
    int numOfFilesCanceled = 0;
    int numOfFilesFinished = 0;
    int numOfFilesFailed = 0;
    int numOfFilesTerminal = 0;

    bool ok = true;
    std::string update =
        "UPDATE t_job "
        "SET JOB_STATE=:1, JOB_FINISHED =:2, FINISH_TIME=:3, REASON=:4 "
        "WHERE job_id = :5 AND JOB_STATE not in ('FINISHEDDIRTY','CANCELED','FINISHED','FAILED')";

    std::string updateJobNotFinished =
        "UPDATE t_job "
        "SET JOB_STATE=:1 WHERE job_id = :2 AND JOB_STATE not in ('FINISHEDDIRTY','CANCELED','FINISHED','FAILED') ";

    std::string query =
        " SELECT "
        "	COUNT(file_index) AS NUM1, "
        "	COUNT( "
        "		CASE WHEN NOT EXISTS ( "
        "				SELECT NULL "
        "				FROM t_file "
        "				WHERE job_id = tbl.job_id "
        "					AND file_index = tbl.file_index "
        "					AND file_state <> 'CANCELED' "
        "		) "
        "		THEN 1 END "
        "	) AS NUM2, "
        "	COUNT( "
        "		CASE WHEN EXISTS ( "
        "			SELECT NULL "
        "			FROM t_file "
        "			WHERE job_id = tbl.job_id "
        "				AND file_index = tbl.file_index "
        "				AND file_state = 'FINISHED' "
        "		) "
        "		THEN 1 END "
        "	) AS NUM3, "
        "	COUNT( "
        "		CASE WHEN NOT EXISTS ( "
        "			SELECT NULL "
        "			FROM t_file "
        "			WHERE job_id = tbl.job_id "
        "				AND file_index = tbl.file_index "
        "				AND (file_state <> 'FAILED' AND file_state <> 'CANCELED') "
        "		) AND EXISTS ( "
        "			SELECT NULL "
        "			FROM t_file "
        "			WHERE job_id = tbl.job_id "
        "				AND file_index = tbl.file_index "
        "				AND file_state = 'FAILED' "
        "		) "
        "		THEN 1 END "
        "	) AS NUM4 "
        " FROM "
        "	( "
        "		SELECT DISTINCT job_id, file_index "
        "		FROM t_file "
        "		WHERE job_id = :1 "
        "	) tbl "
        ;

    std::string updateFileJobFinished =
        "UPDATE t_file "
        "SET JOB_FINISHED=:1 "
        "WHERE job_id=:2 ";

    SafeStatement st;
    SafeResultSet r;
    SafeConnection pooledConnection;
    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                {
                    ok = false;
                    return ok;
                }

            st = conn->createStatement(query, "", pooledConnection);
            st->setString(1, job_id);
            r = conn->createResultset(st, pooledConnection);
            while (r->next())
                {
                    numOfFilesInJob = r->getNumber(1);
                    numOfFilesCanceled = r->getNumber(2);
                    numOfFilesFinished = r->getNumber(3);
                    numOfFilesFailed = r->getNumber(4);
                    numOfFilesTerminal = numOfFilesCanceled + numOfFilesFailed + numOfFilesFinished;

                    if (numOfFilesInJob != numOfFilesTerminal)
                        {
                            finished = false;
                            break;
                        }
                }
            conn->destroyResultset(st, r);

            //job finished
            if (finished == true)
                {
                    time_t timed = time(NULL);
                    //set job status
                    st->setSQL(update);

                    //finisheddirty
                    if ((numOfFilesFailed > 0) && (numOfFilesFinished > 0))
                        {
                            st->setString(1, terminal4);
                        }// finished
                    else if (numOfFilesInJob == numOfFilesFinished)
                        {
                            st->setString(1, terminal1);
                            reason = std::string("");
                        }
                    else if (numOfFilesFailed > 0)     //failed
                        {
                            st->setString(1, terminal2);
                        }
                    else if(numOfFilesCanceled > 0)    //unknown state
                        {
                            st->setString(1, terminal5);
                        }
                    else
                        {
                            st->setString(1, "FAILED");
                            reason = "Inconsistent internal state!";
                        }

                    //set reason and timestamps for job
                    st->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv()));
                    st->setTimestamp(3, conv->toTimestamp(timed, conn->getEnv()));
                    st->setString(4, reason);
                    st->setString(5, job_id);
                    st->executeUpdate();

                    //set timestamps for file
                    st->setSQL(updateFileJobFinished);
                    st->setTimestamp(1, conv->toTimestamp(timed, conn->getEnv()));
                    st->setString(2, job_id);
                    st->executeUpdate();
                    conn->commit(pooledConnection);
                }
            else     //job not finished
                {
                    if (status.compare("ACTIVE") == 0 || status.compare("STAGING") == 0 || status.compare("SUBMITTED") == 0)
                        {
                            st->setSQL(updateJobNotFinished);
                            st->setString(1, status);
                            st->setString(2, job_id);
                            st->executeUpdate();
                            conn->commit(pooledConnection);
                        }
                }

            conn->destroyStatement(st, "", pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (st && r)
                conn->destroyResultset(st, r);
            if (st)
                conn->destroyStatement(st, "", pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
            ok = false;
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (st && r)
                conn->destroyResultset(st, r);
            if (st)
                conn->destroyStatement(st, "", pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
            ok = false;
        }
    conn->releasePooledConnection(pooledConnection);
    return ok;
}

void OracleAPI::updateFileTransferProgress(std::string job_id, int file_id, double throughput, double /*transferred*/)
{
    const std::string queryTag = "updateFileTransferProgress";
    const std::string query    = "UPDATE t_file SET throughput = :1 "
                                 "WHERE job_id = :2 AND file_id = :3  and (throughput is NULL or throughput=0)";

    SafeStatement st;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            st = conn->createStatement(query, queryTag, pooledConnection);

            throughput = convertKbToMb(throughput);
            st->setNumber(1, throughput);
            st->setString(2, job_id);
            st->setInt(3, file_id);

            st->executeUpdate();
            conn->commit(pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

    if (st)
        conn->destroyStatement(st, queryTag, pooledConnection);
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::cancelJob(std::vector<std::string>& requestIDs)
{
    const std::string cancelReason = "Job canceled by the user";
    std::string cancelJ = "update t_job SET  cancel_job='Y', JOB_STATE=:1, JOB_FINISHED =:2, FINISH_TIME=:3, REASON=:4 WHERE job_id = :5 AND job_state NOT IN ('CANCELED','FINISHEDDIRTY', 'FINISHED', 'FAILED')";
    std::string cancelF = "update t_file set file_state=:1, JOB_FINISHED =:2, FINISH_TIME=:3, REASON=:4 WHERE JOB_ID=:5 AND file_state NOT IN ('ACTIVE','READY','CANCELED','FINISHED','FAILED')";
    const std::string cancelJTag = "cancelJTag";
    const std::string cancelFTag = "cancelFTag";
    std::vector<std::string>::const_iterator iter;
    time_t timed = time(NULL);

    SafeStatement st1;
    SafeStatement st2;
    SafeConnection pooledConnection;
    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            st1 = conn->createStatement(cancelJ, cancelJTag, pooledConnection);
            st2 = conn->createStatement(cancelF, cancelFTag, pooledConnection);

            for (iter = requestIDs.begin(); iter != requestIDs.end(); ++iter)
                {
                    std::string jobId = std::string(*iter);

                    st1->setString(1, "CANCELED");
                    st1->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv()));
                    st1->setTimestamp(3, conv->toTimestamp(timed, conn->getEnv()));
                    st1->setString(4, cancelReason);
                    st1->setString(5, jobId);
                    st1->executeUpdate();

                    st2->setString(1, "CANCELED");
                    st2->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv()));
                    st2->setTimestamp(3, conv->toTimestamp(timed, conn->getEnv()));
                    st2->setString(4, cancelReason);
                    st2->setString(5, jobId);
                    st2->executeUpdate();
                    conn->commit(pooledConnection);
                }

            conn->destroyStatement(st1, cancelJTag, pooledConnection);
            conn->destroyStatement(st2, cancelFTag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (st1)
                conn->destroyStatement(st1, cancelJTag, pooledConnection);
            if (st2)
                conn->destroyStatement(st2, cancelFTag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (st1)
                conn->destroyStatement(st1, cancelJTag, pooledConnection);
            if (st2)
                conn->destroyStatement(st2, cancelFTag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::getCancelJob(std::vector<int>& requestIDs)
{
    const std::string tag = "getCancelJob";
    const std::string tag1 = "getCancelJobUpdateCancel";

    std::string query = " select t_file.pid, t_file.job_id from t_file, t_job where t_file.job_id=t_job.job_id and "
                        " t_file.FILE_STATE IN ('ACTIVE','READY') and t_file.PID IS NOT NULL  "
                        " and t_file.TRANSFERHOST=:1 AND t_job.cancel_job = 'Y' ";


    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;
    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;
            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, ftsHostName);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {
                    int pid = r->getInt(1);
                    std::string job_id = r->getString(2);
                    requestIDs.push_back(pid);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (r && s)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (r && s)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

/*t_credential API*/
void OracleAPI::insertGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs)
{
    const std::string tag = "insertGrDPStorageCacheElement";
    std::string query = "INSERT INTO t_credential_cache (dlg_id, dn, cert_request, priv_key, voms_attrs) VALUES (:1, :2, empty_clob(), empty_clob(), empty_clob())";

    const std::string tag1 = "updateGrDPStorageCacheElementxxx";
    std::string query1 = "UPDATE t_credential_cache SET cert_request=:1, priv_key=:2, voms_attrs=:3 WHERE dlg_id=:4 AND dn=:5";


    SafeStatement s;
    SafeStatement s1;
    SafeConnection pooledConnection;
    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, dlg_id);
            s->setString(2, dn);
            s->executeUpdate();
            conn->commit(pooledConnection);

            s1 = conn->createStatement(query1, tag1, pooledConnection);
            s1->setString(1, cert_request);
            s1->setString(2, priv_key);
            s1->setString(3, voms_attrs);
            s1->setString(4, dlg_id);
            s1->setString(5, dn);
            s1->executeUpdate();
            conn->commit(pooledConnection);

            conn->destroyStatement(s, tag, pooledConnection);
            conn->destroyStatement(s1, tag1, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());

        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");

        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::updateGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs)
{
    const std::string tag = "updateGrDPStorageCacheElement";
    std::string query = "UPDATE t_credential_cache SET cert_request=:1, priv_key=:2, voms_attrs=:3 WHERE dlg_id=:4 AND dn=:5";


    SafeStatement s;
    SafeConnection pooledConnection;
    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, cert_request);
            s->setString(2, priv_key);
            s->setString(3, voms_attrs);
            s->setString(4, dlg_id);
            s->setString(5, dn);
            if (s->executeUpdate() != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

CredCache* OracleAPI::findGrDPStorageCacheElement(std::string delegationID, std::string dn)
{
    CredCache* cred = NULL;
    const std::string tag = "findGrDPStorageCacheElement";
    std::string query = "SELECT dlg_id, dn, voms_attrs, cert_request, priv_key FROM t_credential_cache WHERE dlg_id = :1 AND dn = :2 ";


    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;
    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return NULL;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, delegationID);
            s->setString(2, dn);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    cred = new CredCache();
                    cred->delegationID = r->getString(1);
                    cred->DN = r->getString(2);
                    conv->toString(r->getClob(3), cred->vomsAttributes);
                    conv->toString(r->getClob(4), cred->certificateRequest);
                    conv->toString(r->getClob(5), cred->privateKey);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (r && s)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
            conn->releasePooledConnection(pooledConnection);
            return cred;
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (r && s)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
            conn->releasePooledConnection(pooledConnection);
            return cred;
        }
    conn->releasePooledConnection(pooledConnection);
    return cred;
}

void OracleAPI::deleteGrDPStorageCacheElement(std::string delegationID, std::string dn)
{
    const std::string tag = "deleteGrDPStorageCacheElement";
    std::string query = "DELETE FROM t_credential_cache WHERE dlg_id = :1 AND dn = :2";

    SafeStatement s;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, delegationID);
            s->setString(2, dn);
            if (s->executeUpdate() != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::insertGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time)
{
    const std::string tag = "insertGrDPStorageElement";
    std::string query = "INSERT INTO t_credential (dlg_id, dn, termination_time, proxy, voms_attrs ) VALUES (:1, :2, :3, empty_clob(), empty_clob())";

    const std::string tag1 = "updateGrDPStorageElementxxx";
    std::string query1 = "UPDATE t_credential SET proxy = :1, voms_attrs = :2, termination_time = :3 WHERE dlg_id = :4 AND dn = :5";

    SafeStatement s;
    SafeStatement s1;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, dlg_id);
            s->setString(2, dn);
            s->setTimestamp(3, conv->toTimestamp(termination_time, conn->getEnv()));
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);

            s1 = conn->createStatement(query1, tag1, pooledConnection);
            s1->setString(1, proxy);
            s1->setString(2, voms_attrs);
            s1->setTimestamp(3, conv->toTimestamp(termination_time, conn->getEnv()));
            s1->setString(4, dlg_id);
            s1->setString(5, dn);
            s1->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s1, tag1, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::updateGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time)
{
    const std::string tag = "updateGrDPStorageElement";
    std::string query = "UPDATE t_credential SET proxy = :1, voms_attrs = :2, termination_time = :3 WHERE dlg_id = :4 AND dn = :5";


    SafeStatement s;
    SafeConnection pooledConnection;
    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, proxy);
            s->setString(2, voms_attrs);
            s->setTimestamp(3, conv->toTimestamp(termination_time, conn->getEnv()));
            s->setString(4, dlg_id);
            s->setString(5, dn);
            if (s->executeUpdate() != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());

        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");

        }
    conn->releasePooledConnection(pooledConnection);
}

Cred* OracleAPI::findGrDPStorageElement(std::string delegationID, std::string dn)
{
    Cred* cred = NULL;
    const std::string tag = "findGrDPStorageElement";
    std::string query = "SELECT dlg_id, dn, voms_attrs, proxy, termination_time FROM t_credential WHERE dlg_id = :1 AND dn = :2 ";


    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;
    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return NULL;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, delegationID);
            s->setString(2, dn);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    cred = new Cred();
                    cred->delegationID = r->getString(1);
                    cred->DN = r->getString(2);
                    conv->toString(r->getClob(3), cred->vomsAttributes);
                    conv->toString(r->getClob(4), cred->proxy);
                    cred->termination_time = conv->toTimeT(r->getTimestamp(5));
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
            conn->releasePooledConnection(pooledConnection);
            return cred;
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
            conn->releasePooledConnection(pooledConnection);
            return cred;
        }
    conn->releasePooledConnection(pooledConnection);
    return cred;
}

void OracleAPI::deleteGrDPStorageElement(std::string delegationID, std::string dn)
{
    const std::string tag = "deleteGrDPStorageElement";
    std::string query = "DELETE FROM t_credential WHERE dlg_id = :1 AND dn = :2";

    SafeStatement s;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, delegationID);
            s->setString(2, dn);
            if (s->executeUpdate() != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

bool OracleAPI::getDebugMode(std::string source_hostname, std::string destin_hostname)
{
    std::string tag = "getDebugMode";
    std::string query;
    bool debug = false;
    query = "SELECT source_se, dest_se, debug FROM t_debug WHERE source_se = :1 AND (dest_se = :2 or dest_se is null)";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return false;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, source_hostname);
            s->setString(2, destin_hostname);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    debug = std::string(r->getString(3)).compare("on") == 0 ? true : false;
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
            conn->releasePooledConnection(pooledConnection);
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
            conn->releasePooledConnection(pooledConnection);
        }
    conn->releasePooledConnection(pooledConnection);
    return debug;

}

void OracleAPI::setDebugMode(std::string source_hostname, std::string destin_hostname, std::string mode)
{
    std::string tag1 = "setDebugModeDelete";
    std::string tag2 = "setDebugModeInsert";
    std::string query1("");
    std::string query2("");
    unsigned int updated = 0;

    if (destin_hostname.length() == 0)
        {
            query1 = "delete from t_debug where source_se=:1 and dest_se is null";
            query2 = "insert into t_debug(source_se, debug) values(:1,:2)";
            tag1 += "1";
            tag2 += "1";
        }
    else
        {
            query1 = "delete from t_debug where source_se=:1 and dest_se =:2";
            query2 = "insert into t_debug(source_se,dest_se,debug) values(:1,:2,:3)";
        }

    SafeStatement s1;
    SafeStatement s2;
    SafeConnection pooledConnection;


    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s1 = conn->createStatement(query1, tag1, pooledConnection);
            s2 = conn->createStatement(query2, tag2, pooledConnection);
            if (destin_hostname.length() == 0)
                {
                    s1->setString(1, source_hostname);
                    s2->setString(1, source_hostname);
                    s2->setString(2, mode);
                }
            else
                {
                    s1->setString(1, source_hostname);
                    s1->setString(2, destin_hostname);
                    s2->setString(1, source_hostname);
                    s2->setString(2, destin_hostname);
                    s2->setString(3, mode);
                }
            updated += s1->executeUpdate();
            updated += s2->executeUpdate();
            if (updated != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s1, tag1, pooledConnection);
            conn->destroyStatement(s2, tag2, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s1)
                conn->destroyStatement(s1, tag1, pooledConnection);
            if(s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s1)
                conn->destroyStatement(s1, tag1, pooledConnection);
            if(s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::getSubmittedJobsReuse(std::vector<TransferJobs*>& jobs, const std::string & vos)
{
    TransferJobs* tr_jobs = NULL;
    std::string tag = "getSubmittedJobsReuse";
    std::string query_stmt("");
    bool allVos = vos.compare("*")==0? true: false;

    if (allVos)
        {
            query_stmt = "SELECT /* FIRST_ROWS(1) */ "
                         " job_id, "
                         " job_state, "
                         " vo_name,  "
                         " priority,  "
                         " source_se, "
                         " dest_se,  "
                         " agent_dn, "
                         " submit_host, "
                         " user_dn, "
                         " user_cred, "
                         " cred_id,  "
                         " space_token, "
                         " storage_class,  "
                         " job_params, "
                         " overwrite_flag, "
                         " source_space_token, "
                         " source_token_description,"
                         " copy_pin_lifetime, "
                         " checksum_method "
                         " from ( select /* FIRST_ROWS(1) */ "
                         " t_job.job_id, "
                         " t_job.job_state, "
                         " t_job.vo_name,  "
                         " t_job.priority,  "
                         " t_job.source_se, "
                         " t_job.dest_se,  "
                         " t_job.agent_dn, "
                         " t_job.submit_host, "
                         " t_job.user_dn, "
                         " t_job.user_cred, "
                         " t_job.cred_id,  "
                         " t_job.space_token, "
                         " t_job.storage_class,  "
                         " t_job.job_params, "
                         " t_job.overwrite_flag, "
                         " t_job.source_space_token, "
                         " t_job.source_token_description,"
                         " t_job.copy_pin_lifetime, "
                         " t_job.checksum_method "
                         " FROM t_job "
                         " WHERE t_job.job_finished is NULL"
                         " AND t_job.CANCEL_JOB is NULL"
                         " AND t_job.reuse_job='Y' "
                         " AND t_job.job_state = 'SUBMITTED' "
                         " ORDER BY t_job.priority DESC"
                         " , SYS_EXTRACT_UTC(t_job.submit_time) ) WHERE ROWNUM <=1 ";
        }
    else
        {
            tag += "1";
            query_stmt = "SELECT /* FIRST_ROWS(1) */ "
                         " job_id, "
                         " job_state, "
                         " vo_name,  "
                         " priority,  "
                         " source_se, "
                         " dest_se,  "
                         " agent_dn, "
                         " submit_host, "
                         " user_dn, "
                         " user_cred, "
                         " cred_id,  "
                         " space_token, "
                         " storage_class,  "
                         " job_params, "
                         " overwrite_flag, "
                         " source_space_token, "
                         " source_token_description,"
                         " copy_pin_lifetime, "
                         " checksum_method "
                         " from ( select /* FIRST_ROWS(1) */ "
                         " t_job.job_id, "
                         " t_job.job_state, "
                         " t_job.vo_name,  "
                         " t_job.priority,  "
                         " t_job.source_se, "
                         " t_job.dest_se,  "
                         " t_job.agent_dn, "
                         " t_job.submit_host, "
                         " t_job.user_dn, "
                         " t_job.user_cred, "
                         " t_job.cred_id,  "
                         " t_job.space_token, "
                         " t_job.storage_class,  "
                         " t_job.job_params, "
                         " t_job.overwrite_flag, "
                         " t_job.source_space_token, "
                         " t_job.source_token_description,"
                         " t_job.copy_pin_lifetime, "
                         " t_job.checksum_method "
                         " FROM t_job "
                         " WHERE t_job.job_finished is NULL"
                         " AND t_job.CANCEL_JOB is NULL"
                         " AND t_job.VO_NAME IN " + vos +
                         " AND t_job.reuse_job='Y' "
                         " AND t_job.job_state = 'SUBMITTED' "
                         " ORDER BY t_job.priority DESC"
                         " , SYS_EXTRACT_UTC(t_job.submit_time) ) WHERE ROWNUM <=1 ";
        }

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query_stmt, tag, pooledConnection);
            r = conn->createResultset(s, pooledConnection);
            while (r->next())
                {
                    tr_jobs = new TransferJobs();
                    tr_jobs->JOB_ID = r->getString(1);
                    tr_jobs->JOB_STATE = r->getString(2);
                    tr_jobs->VO_NAME = r->getString(3);
                    tr_jobs->PRIORITY = r->getInt(4);
                    tr_jobs->SOURCE = r->getString(5);
                    tr_jobs->DEST = r->getString(6);
                    tr_jobs->AGENT_DN = r->getString(7);
                    tr_jobs->SUBMIT_HOST = r->getString(8);
                    tr_jobs->USER_DN = r->getString(9);
                    tr_jobs->USER_CRED = r->getString(10);
                    tr_jobs->CRED_ID = r->getString(11);
                    tr_jobs->SPACE_TOKEN = r->getString(12);
                    tr_jobs->STORAGE_CLASS = r->getString(13);
                    tr_jobs->INTERNAL_JOB_PARAMS = r->getString(14);
                    tr_jobs->OVERWRITE_FLAG = r->getString(15);
                    tr_jobs->SOURCE_SPACE_TOKEN = r->getString(16);
                    tr_jobs->SOURCE_TOKEN_DESCRIPTION = r->getString(17);
                    tr_jobs->COPY_PIN_LIFETIME = r->getInt(18);
                    tr_jobs->CHECKSUM_METHOD = r->getString(19);
                    jobs.push_back(tr_jobs);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));

        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::auditConfiguration(const std::string & dn, const std::string & config, const std::string & action)
{
    const std::string tag = "auditConfiguration";
    std::string query = "INSERT INTO t_config_audit (datetime, dn, config, action ) VALUES (:1, :2, :3, :4)";

    SafeStatement s;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            time_t timed = time(NULL);
            s = conn->createStatement(query, tag, pooledConnection);
            s->setTimestamp(1, conv->toTimestamp(timed, conn->getEnv()));
            s->setString(2, dn);
            s->setString(3, config);
            s->setString(4, action);
            if (s->executeUpdate() != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}


void OracleAPI::fetchOptimizationConfig2(OptimizerSample* ops, const std::string & source_hostname, const std::string & destin_hostname)
{
    const std::string tag1 = "fetchOptimizationConfig2XXX";
    const std::string tag2 = "fetchOptimizationConfig2YYY";
    const std::string tag3 = "fetchOptimizationConfig2YYYXXX";
    const std::string tag4 = "fetchOptimizationConfig2ZZZZ";
    const std::string tag5 = "fetchOptimizationConfig2ZZZZssss";
    const std::string tagInit1 = "initOptimizerdds";
    const std::string tagInit2 = "initOptimizer2adf";

    int current_active_count = 0;
    int check_if_more_samples_exist_count = 0;
    int timeoutTr = 0;

    //TODO: check for the last 30min or last 5 transfers, add the average throughput into the picture of the last 30 mincount(*)

    std::string check_if_more_samples_exist = "  SELECT count(*) "
            " FROM t_optimize WHERE  "
            " throughput is NULL and source_se = :1 and dest_se=:2 and file_id=0 ";

    std::string current_active = " select count(*) from t_file where file_state='ACTIVE' and t_file.source_se=:1 and t_file.dest_se=:2";

    std::string pick_next_sample = " SELECT nostreams, timeout, buffer "
                                   "  FROM t_optimize WHERE  "
                                   "  source_se = :1 and dest_se=:2 "
                                   "  and throughput is NULL and file_id=0 order by nostreams asc, timeout asc, buffer asc ";

    std::string try_optimize =  " select throughput, active, nostreams, timeout, buffer from (select throughput, active, nostreams, timeout, buffer "
                                " from t_optimize where source_se=:1 and dest_se=:2 and throughput is not null order by  abs(active-:3), throughput desc) "
                                " sub where rownum < 2 ";

    std::string midRangeTimeout = " select count(*) from t_file where "
                                  " t_file.file_state='FAILED' "
                                  " and t_file.reason like '%operation timeout%' and t_file.source_se=:1 "
                                  " and t_file.dest_se=:2  "
                                  " and (t_file.finish_time > (SYS_EXTRACT_UTC(SYSTIMESTAMP) - interval '30' minute)) "
                                  " order by  SYS_EXTRACT_UTC(t_file.finish_time) desc";


    SafeStatement s1;
    SafeResultSet r1;
    SafeStatement s2;
    SafeResultSet r2;
    SafeStatement s3;
    SafeResultSet r3;
    SafeStatement s4;
    SafeResultSet r4;
    SafeStatement s5;
    SafeResultSet r5;

    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;


            //if last 30 minutes transfer timeout use decent defaults
            s5 = conn->createStatement(midRangeTimeout, tag5, pooledConnection);
            s5->setString(1, source_hostname);
            s5->setString(2, destin_hostname);
            r5 = conn->createResultset(s5, pooledConnection);
            if (r5->next())
                {
                    timeoutTr = r5->getInt(1);
                }
            conn->destroyResultset(s5, r5);
            conn->destroyStatement(s5, tag5, pooledConnection);

            //check if more samples exist for this pair to be selected
            s2 = conn->createStatement(check_if_more_samples_exist, tag2, pooledConnection);
            s2->setString(1, source_hostname);
            s2->setString(2, destin_hostname);
            r2 = conn->createResultset(s2, pooledConnection);
            if (r2->next())
                {
                    check_if_more_samples_exist_count = r2->getInt(1);
                }
            conn->destroyResultset(s2, r2);
            conn->destroyStatement(s2, tag2, pooledConnection);

            //check if more samples are available
            if(check_if_more_samples_exist_count > 0 && timeoutTr == 0)
                {
                    //select next sample
                    s3 = conn->createStatement(pick_next_sample, tag3, pooledConnection);
                    s3->setString(1, source_hostname);
                    s3->setString(2, destin_hostname);
                    r3 = conn->createResultset(s3, pooledConnection);
                    if (r3->next())
                        {
                            ops->streamsperfile = r3->getInt(1);
                            ops->timeout = r3->getInt(2);
                            ops->bufsize = r3->getInt(3);
                            ops->file_id = 1;
                        }
                    else   //nothing found, this is weird, set to defaults then
                        {
                            ops->streamsperfile = DEFAULT_NOSTREAMS;
                            ops->timeout = MID_TIMEOUT;
                            ops->bufsize = DEFAULT_BUFFSIZE;
                            ops->file_id = 0;
                        }
                    conn->destroyResultset(s3, r3);
                    conn->destroyStatement(s3, tag3, pooledConnection);
                }
            else if(timeoutTr > 0)   //tr's started timing out, use decent defaults
                {
                    ops->streamsperfile = DEFAULT_NOSTREAMS;
                    ops->timeout = 0;
                    ops->bufsize = DEFAULT_BUFFSIZE;
                    ops->file_id = 0;
                }
            else   //no more samples, try to optimize
                {
                    //get current active from t_file
                    s1 = conn->createStatement(current_active, tag1, pooledConnection);
                    s1->setString(1, source_hostname);
                    s1->setString(2, destin_hostname);
                    r1 = conn->createResultset(s1, pooledConnection);
                    if (r1->next())
                        {
                            current_active_count = r1->getInt(1);
                        }
                    conn->destroyResultset(s1, r1);
                    conn->destroyStatement(s1, tag1, pooledConnection);

                    //try to optimize
                    s4 = conn->createStatement(try_optimize, tag4, pooledConnection);
                    s4->setString(1, source_hostname);
                    s4->setString(2, destin_hostname);
                    s4->setInt(3, current_active_count);
                    r4 = conn->createResultset(s4, pooledConnection);
                    if (r4->next())
                        {
                            ops->streamsperfile = r4->getInt(3);
                            ops->timeout = r4->getInt(4);
                            ops->bufsize = r4->getInt(5);
                            ops->file_id = 1;
                        }
                    else   //nothing found, use decent defaults
                        {
                            ops->streamsperfile = DEFAULT_NOSTREAMS;
                            ops->timeout = MID_TIMEOUT;
                            ops->bufsize = DEFAULT_BUFFSIZE;
                            ops->file_id = 0;
                        }
                    conn->destroyResultset(s4, r4);
                    conn->destroyStatement(s4, tag4, pooledConnection);
                }
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if (s2 && r2)
                conn->destroyResultset(s2, r2);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            if (s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            if (s4 && r4)
                conn->destroyResultset(s4, r4);
            if (s4)
                conn->destroyStatement(s4, tag4, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if (s2 && r2)
                conn->destroyResultset(s2, r2);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            if (s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            if (s4 && r4)
                conn->destroyResultset(s4, r4);
            if (s4)
                conn->destroyStatement(s4, tag4, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::recordOptimizerUpdate(int active, double filesize, double throughput,
                                      int nostreams, int timeout, int buffersize, std::string source_hostname,
                                      std::string destin_hostname)
{
    std::string tag = "recordOptimizerUpdate";
    std::string query = "INSERT INTO t_optimizer_evolution "
                        " (datetime, source_se, dest_se, nostreams, timeout, active, throughput, buffer, filesize) "
                        " VALUES "
                        " (SYS_EXTRACT_UTC(SYSTIMESTAMP), :1, :2, :3, :4, :5, :6, :7, :8)";

    SafeConnection pooledConnection;
    SafeStatement   s;
    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;


            s = conn->createStatement(query, tag, pooledConnection);

            s->setString(1, source_hostname);
            s->setString(2, destin_hostname);
            s->setInt(3, nostreams);
            s->setInt(4, timeout);
            s->setInt(5, active);
            s->setDouble(6, throughput);
            s->setInt(7, buffersize);
            s->setDouble(8, filesize);

            s->executeUpdate();
            conn->commit(pooledConnection);

            conn->destroyStatement(s, tag, pooledConnection);
            conn->releasePooledConnection(pooledConnection);
        }
    catch (...)
        {
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            if (pooledConnection)
                {
                    conn->rollback(pooledConnection);
                    conn->releasePooledConnection(pooledConnection);
                }
        }
}

bool OracleAPI::updateOptimizer(double throughputIn, int, double filesize, double timeInSecs,
                                int nostreams, int timeout, int buffersize, std::string source_hostname,
                                std::string destin_hostname)
{
    const std::string tag2 = "updateOptimizer2";
    const std::string tag3 = "updateOptimizer3";
    const std::string tag4 = "updateOptimizer4";
    const std::string tag5 = "updateOptimizer5";
    double throughput = 0;
    bool ok = true;
    int active = 0;

    std::string query2 =
        "UPDATE t_optimize SET filesize = :1, throughput = :2, active=:3, datetime=:4, timeout=:5 "
        " WHERE nostreams = :6 and buffer=:7 and source_se=:8 and dest_se=:9 "
        " and (throughput is null or throughput<=:10) and (active<=:11 or active is null) ";

    std::string query3 =
        " select count(*) from t_file where t_file.source_se=:1 and t_file.dest_se=:2 and t_file.file_state='ACTIVE' ";

    std::string query4 =
        " DELETE FROM t_optimize a WHERE  a.rowid >  ANY ( SELECT B.rowid FROM  t_optimize B  WHERE A.NOSTREAMS = B.NOSTREAMS AND "
        " A.TIMEOUT = B.TIMEOUT AND A.ACTIVE = B.ACTIVE  AND A.THROUGHPUT = B.THROUGHPUT ) ";

    std::string query5 =
        "UPDATE t_optimize SET datetime=:1 "
        " WHERE nostreams = :2 and buffer=:3 and source_se=:4 and dest_se=:5 "
        " and (active<=:6 or active is null) ";

    SafeStatement s3;
    SafeResultSet r3;
    SafeStatement s2;
    SafeStatement s4;
    SafeStatement s5;
    SafeConnection pooledConnection;

    try
        {
            long rows_updated = 0;

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                {
                    ok = false;
                    return ok;
                }

            s3 = conn->createStatement(query3, tag3, pooledConnection);
            s3->setString(1, source_hostname);
            s3->setString(2, destin_hostname);
            r3 = conn->createResultset(s3, pooledConnection);
            if (r3->next())
                {
                    active = r3->getInt(1);
                }
            conn->destroyResultset(s3, r3);
            conn->destroyStatement(s3, tag3, pooledConnection);

            time_t now = std::time(NULL);

            if (filesize > 0 && timeInSecs > 0)
                {
                    if(throughputIn != 0.0)
                        throughput = convertKbToMb(throughputIn);
                    else
                        throughput = convertBtoM(filesize, timeInSecs);
                }
            else
                {
                    if(throughputIn != 0.0)
                        throughput = convertKbToMb(throughputIn);
                    else
                        throughput = convertBtoM(filesize, 1);
                }
            if (filesize <= 0)
                filesize = 0;
            if (buffersize <= 0)
                buffersize = 0;

            s2 = conn->createStatement(query2, tag2, pooledConnection);
            s2->setDouble(1, filesize);
            s2->setDouble(2, throughput);
            s2->setInt(3, active);
            s2->setTimestamp(4, conv->toTimestamp(now, conn->getEnv()));
            s2->setInt(5, timeout);
            s2->setInt(6, nostreams);
            s2->setInt(7, buffersize);
            s2->setString(8, source_hostname);
            s2->setString(9, destin_hostname);
            s2->setDouble(10, throughput);
            s2->setInt(11, active);
            if ((rows_updated = s2->executeUpdate()) != 0)
                {
                    conn->commit(pooledConnection);
                }
            else
                {
                    s5 = conn->createStatement(query5, tag5, pooledConnection);
                    s5->setTimestamp(1, conv->toTimestamp(now, conn->getEnv()));
                    s5->setInt(2, nostreams);
                    s5->setInt(3, buffersize);
                    s5->setString(4, source_hostname);
                    s5->setString(5, destin_hostname);
                    s5->setInt(6, active);
                    rows_updated = s5->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s5, tag5, pooledConnection);
                }
            conn->destroyStatement(s2, tag2, pooledConnection);

            s4 = conn->createStatement(query4, tag4, pooledConnection);
            if (s4->executeUpdate() != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s4, tag4, pooledConnection);

            conn->releasePooledConnection(pooledConnection);

            // Store evolution
            if (rows_updated)
                this->recordOptimizerUpdate(active, filesize, throughput,
                                            nostreams, timeout, buffersize,
                                            source_hostname, destin_hostname);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            if (s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            if (s4)
                conn->destroyStatement(s4, tag4, pooledConnection);

            if (s5)
                conn->destroyStatement(s5, tag5, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
            ok = false;
            conn->releasePooledConnection(pooledConnection);
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            if (s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            if (s4)
                conn->destroyStatement(s4, tag4, pooledConnection);

            if (s5)
                conn->destroyStatement(s5, tag5, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
            ok = false;
            conn->releasePooledConnection(pooledConnection);
        }

    return ok;
}

void OracleAPI::initOptimizer(const std::string & source_hostname, const std::string & destin_hostname, int)
{
    const std::string tag = "initOptimizer";
    const std::string tag1 = "initOptimizer2";
    std::string query = "insert into t_optimize(source_se, dest_se,timeout,nostreams,buffer, file_id ) values(:1,:2,:3,:4,:5,0) ";
    std::string query1 = "select count(*) from t_optimize where source_se=:1 and dest_se=:2";
    int foundRecords = 0;


    SafeStatement s1;
    SafeResultSet r1;
    SafeStatement s;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s1 = conn->createStatement(query1, tag1, pooledConnection);
            s1->setString(1, source_hostname);
            s1->setString(2, destin_hostname);
            r1 = conn->createResultset(s1, pooledConnection);
            if (r1->next())
                {
                    foundRecords = r1->getInt(1);
                }
            conn->destroyResultset(s1, r1);
            conn->destroyStatement(s1, tag1, pooledConnection);

            if (foundRecords == 0)
                {
                    s = conn->createStatement(query, tag, pooledConnection);

                    for (unsigned register int x = 0; x < timeoutslen; x++)
                        {
                            for (unsigned register int y = 0; y < nostreamslen; y++)
                                {
                                    s->setString(1, source_hostname);
                                    s->setString(2, destin_hostname);
                                    s->setInt(3, timeouts[x]);
                                    s->setInt(4, nostreams[y]);
                                    s->setInt(5, 0);
                                    s->executeUpdate();
                                }
                        }
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s, tag, pooledConnection);
                }
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

bool OracleAPI::isCredentialExpired(const std::string & dlg_id, const std::string & dn)
{

    bool valid = false;
    const std::string tag = "isCredentialExpired";
    std::string query = "SELECT termination_time from t_credential where dlg_id=:1 and dn=:2";
    double dif;


    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;
    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return false;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, dlg_id);
            s->setString(2, dn);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    time_t lifetime = std::time(NULL);
                    time_t term_time = conv->toTimeT(r->getTimestamp(1));
                    dif = difftime(term_time, lifetime);

                    if (dif > 0)
                        valid = true;

                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return valid;
}

bool OracleAPI::isTrAllowed2(const std::string & source_hostname, const std::string & destin_hostname)
{
    return isTrAllowed(source_hostname, destin_hostname);
}

bool OracleAPI::isTrAllowed(const std::string & source_hostname, const std::string & destin_hostname)
{
    const std::string tag1 = "isTrAllowed1";
    const std::string tag2 = "isTrAllowed2";
    const std::string tag3 = "isTrAllowed3";
    const std::string tag4 = "isTrAllowed4";
    const std::string tag5 = "isTrAllowed5";
    const std::string tag6 = "isTrAllowed6";
    const std::string tag7 = "isTrAllowed7";
    const std::string tag8 = "isTrAllowed8";

    bool allowed = true;
    int act = 0;
    int maxDest = 0;
    int maxSource = 0;
    double numberOfFinished = 0;
    double numberOfFailed = 0;
    double ratioSuccessFailure = 0;
    double numberOfFinishedAll = 0;
    double numberOfFailedAll = 0;
    double throughput = 0.0;
    double avgThr = 0.0;

    std::string query_stmt1 = " select count(*) from  t_file where t_file.file_state in ('READY','ACTIVE') and t_file.source_se=:1 ";

    std::string query_stmt2 = " select count(*) from  t_file where t_file.file_state in ('READY','ACTIVE') and t_file.dest_se=:1";

    std::string query_stmt3 = " select file_state from t_file where t_file.source_se=:1 and t_file.dest_se=:2 "
                              " and file_state in ('FAILED','FINISHED') and (t_file.FINISH_TIME > (SYS_EXTRACT_UTC(SYSTIMESTAMP) - interval '5' minute)) order by "
                              " SYS_EXTRACT_UTC(t_file.FINISH_TIME) desc ";

    std::string query_stmt4 = " select count(*) from  t_file where  t_file.source_se=:1 and t_file.dest_se=:2 "
                              " and file_state in ('READY','ACTIVE') ";

    std::string query_stmt5 = " select count(*) from  t_file where t_file.source_se=:1 and t_file.dest_se=:2 "
                              " and file_state = 'FINISHED' and (t_file.FINISH_TIME > (SYS_EXTRACT_UTC(SYSTIMESTAMP) - interval '5' minute))";

    std::string query_stmt6 = " select count(*) from  t_file where t_file.source_se=:1 and t_file.dest_se=:2 "
                              " and file_state = 'FAILED' and (t_file.FINISH_TIME > (SYS_EXTRACT_UTC(SYSTIMESTAMP) - interval '5' minute))";

    std::string query_stmt7 =   " SELECT avg(ROUND((filesize * throughput)/filesize,2)) from ( select filesize, throughput from t_file where "
                                " source_se=:1 and dest_se=:2 "
                                " and file_state in ('ACTIVE','FINISHED') and throughput > 0 and filesize > 0 "
                                " and (start_time >= (SYS_EXTRACT_UTC(SYSTIMESTAMP) - interval '10' minute) OR "
                                " job_finished >= (SYS_EXTRACT_UTC(SYSTIMESTAMP) - interval '5' minute)) "
                                " order by FILE_ID DESC)  where rownum <= 10 ";

    std::string query_stmt8 =   " SELECT avg(ROUND((filesize * throughput)/filesize,2)) from ( select filesize, throughput from t_file where "
                                " source_se=:1 and dest_se=:2 "
                                " and file_state in ('ACTIVE','FINISHED') and throughput > 0  and filesize > 0 "
                                " and (start_time >= (SYS_EXTRACT_UTC(SYSTIMESTAMP) - interval '30' minute) OR "
                                " job_finished >= (SYS_EXTRACT_UTC(SYSTIMESTAMP) - interval '30' minute)) "
                                " order by FILE_ID DESC) where rownum <= 30 ";



    SafeStatement s1;
    SafeResultSet r1;
    SafeStatement s2;
    SafeResultSet r2;
    SafeStatement s3;
    SafeResultSet r3;
    SafeStatement s4;
    SafeResultSet r4;
    SafeStatement s5;
    SafeResultSet r5;
    SafeStatement s6;
    SafeResultSet r6;
    SafeStatement s7;
    SafeResultSet r7;
    SafeStatement s8;
    SafeResultSet r8;
    SafeConnection pooledConnection;


    try
        {
            int mode = getOptimizerMode();
            if(mode==1)
                {
                    lowDefault = mode_1[0];
                    highDefault = mode_1[1];
                }
            else if(mode==2)
                {
                    lowDefault = mode_2[0];
                    highDefault = mode_2[1];
                }
            else if(mode==3)
                {
                    lowDefault = mode_3[0];
                    highDefault = mode_3[1];
                }
            else
                {
                    jobsNum = mode_1[0];
                    highDefault = mode_1[1];
                }


            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return false;

            s8 = conn->createStatement(query_stmt8, tag8, pooledConnection);
            s8->setString(1, source_hostname);
            s8->setString(2, destin_hostname);
            r8 = conn->createResultset(s8, pooledConnection);
            if (r8->next())
                {
                    avgThr = r8->getDouble(1);
                }
            conn->destroyResultset(s8, r8);
            conn->destroyStatement(s8, tag8, pooledConnection);

            s7 = conn->createStatement(query_stmt7, tag7, pooledConnection);
            s7->setString(1, source_hostname);
            s7->setString(2, destin_hostname);
            r7 = conn->createResultset(s7, pooledConnection);
            if (r7->next())
                {
                    throughput = r7->getDouble(1);
                }
            conn->destroyResultset(s7, r7);
            conn->destroyStatement(s7, tag7, pooledConnection);

            s1 = conn->createStatement(query_stmt1, tag1, pooledConnection);
            s1->setString(1, source_hostname);
            r1 = conn->createResultset(s1, pooledConnection);
            if (r1->next())
                {
                    maxSource = r1->getInt(1);
                }
            conn->destroyResultset(s1, r1);
            conn->destroyStatement(s1, tag1, pooledConnection);

            s2 = conn->createStatement(query_stmt2, tag2, pooledConnection);
            s2->setString(1, destin_hostname);
            r2 = conn->createResultset(s2, pooledConnection);
            if (r2->next())
                {
                    maxDest = r2->getInt(1);
                }
            conn->destroyResultset(s2, r2);
            conn->destroyStatement(s2, tag2, pooledConnection);

            s3 = conn->createStatement(query_stmt3, tag3, pooledConnection);
            s3->setString(1, source_hostname);
            s3->setString(2, destin_hostname);
            r3 = conn->createResultset(s3, pooledConnection);

            while (r3->next())
                {
                    std::string fileState = r3->getString(1);
                    if(fileState.compare("FAILED")==0)
                        numberOfFailed += 1.0;

                    if(fileState.compare("FINISHED")==0)
                        numberOfFinished += 1.0;
                }
            conn->destroyResultset(s3, r3);
            conn->destroyStatement(s3, tag3, pooledConnection);

            s4 = conn->createStatement(query_stmt4, tag4, pooledConnection);
            s4->setString(1, source_hostname);
            s4->setString(2, destin_hostname);
            r4 = conn->createResultset(s4, pooledConnection);
            if (r4->next())
                {
                    act = r4->getInt(1);
                }
            conn->destroyResultset(s4, r4);
            conn->destroyStatement(s4, tag4, pooledConnection);

            s5 = conn->createStatement(query_stmt5, tag5, pooledConnection);
            s5->setString(1, source_hostname);
            s5->setString(2, destin_hostname);
            r5 = conn->createResultset(s5, pooledConnection);
            if (r5->next())
                {
                    numberOfFinishedAll = r5->getInt(1);
                }
            conn->destroyResultset(s5, r5);
            conn->destroyStatement(s5, tag5, pooledConnection);

            s6 = conn->createStatement(query_stmt6, tag6, pooledConnection);
            s6->setString(1, source_hostname);
            s6->setString(2, destin_hostname);
            r6 = conn->createResultset(s6, pooledConnection);
            if (r6->next())
                {
                    numberOfFailedAll = r6->getInt(1);
                }
            conn->destroyResultset(s6, r6);
            conn->destroyStatement(s6, tag6, pooledConnection);

            if(numberOfFinished>0)
                {
                    ratioSuccessFailure = numberOfFinished/(numberOfFinished + numberOfFailed) * (100.0/1.0);
                }
            else
                {
                    ratioSuccessFailure = 0;
                }


            allowed = optimizerObject.transferStart((int) numberOfFinished, (int) numberOfFailed,source_hostname, destin_hostname, act, maxSource, maxDest,
                                                    ratioSuccessFailure,numberOfFinishedAll, numberOfFailedAll, throughput, avgThr, lowDefault, highDefault);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if (s2 && r2)
                conn->destroyResultset(s1, r1);
            if (s2)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if (s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            if (s4 && r4)
                conn->destroyResultset(s4, r4);
            if (s4)
                conn->destroyStatement(s4, tag4, pooledConnection);

            if (s5 && r5)
                conn->destroyResultset(s5, r5);
            if (s5)
                conn->destroyStatement(s5, tag5, pooledConnection);

            if (s6 && r6)
                conn->destroyResultset(s6, r6);
            if (s6)
                conn->destroyStatement(s6, tag6, pooledConnection);

            if (s7 && r7)
                conn->destroyResultset(s7, r7);
            if (s7)
                conn->destroyStatement(s7, tag7, pooledConnection);

            if (s8 && r8)
                conn->destroyResultset(s8, r8);
            if (s8)
                conn->destroyStatement(s8, tag8, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if (s2 && r2)
                conn->destroyResultset(s1, r1);
            if (s2)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if (s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            if (s4 && r4)
                conn->destroyResultset(s4, r4);
            if (s4)
                conn->destroyStatement(s4, tag4, pooledConnection);

            if (s5 && r5)
                conn->destroyResultset(s5, r5);
            if (s5)
                conn->destroyStatement(s5, tag5, pooledConnection);

            if (s6 && r6)
                conn->destroyResultset(s6, r6);
            if (s6)
                conn->destroyStatement(s6, tag6, pooledConnection);

            if (s7 && r7)
                conn->destroyResultset(s7, r7);
            if (s7)
                conn->destroyStatement(s7, tag7, pooledConnection);

            if (s8 && r8)
                conn->destroyResultset(s8, r8);
            if (s8)
                conn->destroyStatement(s8, tag8, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return allowed;
}


int OracleAPI::getSeOut(const std::string & source, const std::set<std::string> & destination)
{
    // total number of allowed active for the source (both currently in use and free credits)
    int ret = 0;

    std::string tag[8];
    std::string query[8];

    SafeStatement s[8];
    SafeResultSet r[8];

    SafeConnection pooledConnection;

    try
        {
            int nActiveSource = 0;

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return false;

            initVariablesForGetCredits(pooledConnection, s, query, tag, "getSeOut");

            std::string source_hostname = source;

            // get number of active for the destination

            s[0]->setString(1, source_hostname);
            r[0] = conn->createResultset(s[0], pooledConnection);

            if (r[0]->next())
                {
                    nActiveSource = r[0]->getInt(1);
                }

            conn->destroyResultset(s[0], r[0]);

            ret += nActiveSource;

            std::set<std::string>::iterator it;

            for (it = destination.begin(); it != destination.end(); ++it)
                {
                    std::string destin_hostname = *it;
                    ret += getCredits(pooledConnection, s, r, source_hostname, destin_hostname);
                }

            for (int i = 0; i < 8; i++)
                {
                    conn->destroyStatement(s[i], tag[i], pooledConnection);
                }
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);

            for (int i = 0; i < 8; i++)
                {
                    if (s[i] && r[i])
                        conn->destroyResultset(s[i], r[i]);
                    if (s[i])
                        conn->destroyStatement(s[i], tag[i], pooledConnection);
                }

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);

            for (int i = 0; i < 8; i++)
                {
                    if (s[i] && r[i])
                        conn->destroyResultset(s[i], r[i]);
                    if (s[i])
                        conn->destroyStatement(s[i], tag[i], pooledConnection);
                }

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);

    return ret;
}

int OracleAPI::getSeIn(const std::set<std::string> & source, const std::string & destination)
{
    // total number of allowed active for the source (both currently in use and free credits)
    int ret = 0;

    std::string tag[8];
    std::string query[8];

    SafeStatement s[8];
    SafeResultSet r[8];

    SafeConnection pooledConnection;

    try
        {
            int nActiveDest = 0;

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return false;

            initVariablesForGetCredits(pooledConnection, s, query, tag, "getSeIn");

            std::string destin_hostname = destination;

            // get number of active for the destination
            s[1]->setString(1, destin_hostname);
            r[1] = conn->createResultset(s[1], pooledConnection);

            if (r[1]->next())
                {
                    nActiveDest = r[1]->getInt(1);
                }

            conn->destroyResultset(s[1], r[1]);

            ret += nActiveDest;

            std::set<std::string>::iterator it;

            for (it = source.begin(); it != source.end(); ++it)
                {
                    std::string source_hostname = *it;
                    ret += getCredits(pooledConnection, s, r, source_hostname, destin_hostname);
                }

            for (int i = 0; i < 8; i++)
                {
                    conn->destroyStatement(s[i], tag[i], pooledConnection);
                }
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);

            for (int i = 0; i < 8; i++)
                {
                    if (s[i] && r[i])
                        conn->destroyResultset(s[i], r[i]);
                    if (s[i])
                        conn->destroyStatement(s[i], tag[i], pooledConnection);
                }

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);

            for (int i = 0; i < 8; i++)
                {
                    if (s[i] && r[i])
                        conn->destroyResultset(s[i], r[i]);
                    if (s[i])
                        conn->destroyStatement(s[i], tag[i], pooledConnection);
                }

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);

    return ret;
}

void OracleAPI::initVariablesForGetCredits(SafeConnection& pooledConnection, SafeStatement s[], std::string* query, std::string* tag, std::string basename)
{
    tag[0] = basename + "0";
    tag[1] = basename + "1";
    tag[2] = basename + "2";
    tag[3] = basename + "3";
    tag[4] = basename + "4";
    tag[5] = basename + "5";
    tag[6] = basename + "6";
    tag[7] = basename + "7";

    query[0] =
        "SELECT COUNT(*) FROM t_file "
        "WHERE t_file.file_state in ('READY','ACTIVE') AND "
        "      t_file.source_se = :1 "
        ;

    query[1] =
        "SELECT COUNT(*) FROM t_file "
        "WHERE t_file.file_state in ('READY','ACTIVE') AND "
        "      t_file.dest_se = :1"
        ;

    query[2] =
        " select throughput "
        " from ("
        "	select throughput "
        "	from  t_file "
        "	where source_se = :1 "
        "		and dest_se = :2 "
        "		and throughput is not NULL "
        " 		and throughput != 0  "
        "	order by FINISH_TIME DESC"
        " ) "
        " where rownum = 1"
        ;

    query[3] =
        "SELECT file_state "
        "FROM t_file "
        "WHERE "
        "      t_file.source_se = :1 AND t_file.dest_se = :2 AND "
        "      file_state IN ('FAILED','FINISHED') AND "
        "      (t_file.FINISH_TIME > (SYS_EXTRACT_UTC(SYSTIMESTAMP) - interval '1' hour))"
        ;

    query[4] =
        "SELECT COUNT(*) "
        "FROM t_file "
        "WHERE "
        "      t_file.source_se = :1 AND t_file.dest_se = :2 AND "
        "      file_state in ('READY','ACTIVE')"
        ;

    query[5] =
        "SELECT COUNT(*) FROM t_file "
        "WHERE "
        "      t_file.source_se = :1 AND t_file.dest_se = :2 AND "
        "      file_state = 'FINISHED'"
        ;

    query[6] =
        "SELECT COUNT(*) FROM t_file "
        "WHERE "
        "      t_file.source_se = :1 AND t_file.dest_se = :2 AND "
        "      file_state = 'FAILED'"
        ;

    query[7] = " select ROUND(AVG(throughput),2) AS Average  from t_file where source_se = :1 and dest_se = :2 "
               " and file_state = 'FINISHED' and throughput is not NULL and throughput != 0 "
               " and (t_file.FINISH_TIME > (SYS_EXTRACT_UTC(SYSTIMESTAMP) - interval '5' minute))";

    s[0] = conn->createStatement(query[0], tag[0], pooledConnection);
    s[1] = conn->createStatement(query[1], tag[1], pooledConnection);
    s[2] = conn->createStatement(query[2], tag[2], pooledConnection);
    s[3] = conn->createStatement(query[3], tag[3], pooledConnection);
    s[4] = conn->createStatement(query[4], tag[4], pooledConnection);
    s[5] = conn->createStatement(query[5], tag[5], pooledConnection);
    s[6] = conn->createStatement(query[6], tag[6], pooledConnection);
    s[7] = conn->createStatement(query[7], tag[7], pooledConnection);
}

int OracleAPI::getCredits(SafeConnection& pooledConnection, SafeStatement s[], SafeResultSet r[], const std::string & source_hostname, const std::string & destin_hostname)
{
    int nActiveSource = 0, nActiveDest = 0;
    double nFailedLastHour = 0, nFinishedLastHour = 0;
    int nActive = 0;
    double nFailedAll = 0, nFinishedAll = 0, throughput = 0;
    double avgThr = 0.0;

    s[7]->setString(1, source_hostname);
    s[7]->setString(2, destin_hostname);
    r[7] = conn->createResultset(s[7], pooledConnection);
    if (r[7]->next())
        {
            avgThr = r[7]->getDouble(1);
        }
    conn->destroyResultset(s[7], r[7]);

    // get number of active for the source
    s[0]->setString(1, source_hostname);
    r[0] = conn->createResultset(s[0], pooledConnection);

    if (r[0]->next())
        {
            nActiveSource = r[0]->getInt(1);
        }

    conn->destroyResultset(s[0], r[0]);

    // get number of active for the destination
    s[1]->setString(1, destin_hostname);
    r[1] = conn->createResultset(s[1], pooledConnection);

    if (r[1]->next())
        {
            nActiveDest = r[1]->getInt(1);
        }

    conn->destroyResultset(s[1], r[1]);

    // get the throughput
    s[2]->setString(1, source_hostname);
    s[2]->setString(2, destin_hostname);
    r[2] = conn->createResultset(s[2], pooledConnection);

    if (r[2]->next())
        {
            throughput = r[2]->getInt(1);
        }

    conn->destroyResultset(s[2], r[2]);

    // file state: FAILED and FINISHED (in last hour)
    s[3]->setString(1, source_hostname);
    s[3]->setString(2, destin_hostname);
    r[3] = conn->createResultset(s[3], pooledConnection);

    while (r[3]->next())
        {
            std::string state = r[3]->getString(1);
            if      (state.compare("FAILED") == 0)   nFailedLastHour+=1.0;
            else if (state.compare("FINISHED") == 0) ++nFinishedLastHour+=1.0;
        }

    conn->destroyResultset(s[3], r[3]);

    // number of active for the pair
    s[4]->setString(1, source_hostname);
    s[4]->setString(2, destin_hostname);
    r[4]= conn->createResultset(s[4], pooledConnection);

    if (r[4]->next())
        {
            nActive = r[4]->getInt(1);
        }

    conn->destroyResultset(s[4], r[4]);

    // all finished in total
    s[5]->setString(1, source_hostname);
    s[5]->setString(2, destin_hostname);
    r[5] = conn->createResultset(s[5], pooledConnection);

    if (r[5]->next())
        {
            nFinishedAll = r[5]->getInt(1);
        }

    conn->destroyResultset(s[5], r[5]);

    // all failed in total
    s[6]->setString(1, source_hostname);
    s[6]->setString(2, destin_hostname);
    r[6] = conn->createResultset(s[6], pooledConnection);

    if (r[6]->next())
        {
            nFailedAll = r[6]->getInt(1);
        }

    conn->destroyResultset(s[6], r[6]);

    double ratioSuccessFailure = 0;
    if(nFinishedLastHour > 0)
        {
            ratioSuccessFailure = nFinishedLastHour/(nFinishedLastHour + nFailedLastHour) * (100.0/1.0);
        }


    return optimizerObject.getFreeCredits((int) nFinishedLastHour, (int) nFailedLastHour,
                                          source_hostname, destin_hostname,
                                          nActive, nActiveSource, nActiveDest,
                                          ratioSuccessFailure,
                                          nFinishedAll, nFailedAll,throughput, avgThr);
}

void OracleAPI::setAllowedNoOptimize(const std::string & job_id, int file_id, const std::string & params)
{
    const std::string tag = "setAllowedNoOptimize";
    const std::string tag1 = "setAllowedNoOptimize1";
    std::string query_stmt = "update t_file set INTERNAL_FILE_PARAMS=:1 where file_id=:2 and job_id=:3";
    std::string query_stmt1 = "update t_file set INTERNAL_FILE_PARAMS=:1 where job_id=:2";
    SafeStatement s;
    SafeStatement s1;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            if (file_id == 0)
                {
                    s1 = conn->createStatement(query_stmt1, tag1, pooledConnection);
                    s1->setString(1, params);
                    s1->setString(2, job_id);
                    if (s1->executeUpdate() != 0)
                        conn->commit(pooledConnection);
                    conn->destroyStatement(s1, tag1, pooledConnection);
                }
            else
                {
                    s = conn->createStatement(query_stmt, tag, pooledConnection);
                    s->setString(1, params);
                    s->setInt(2, file_id);
                    s->setString(3, job_id);
                    if (s->executeUpdate() != 0)
                        conn->commit(pooledConnection);
                    conn->destroyStatement(s, tag, pooledConnection);
                }
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);

            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}


void OracleAPI::forceFailTransfers(std::map<int, std::string>& collectJobs)
{
    std::string tag = "forceFailTransfers";
    std::string tag1 = "forceFailTransfers1";

    std::string query = " select t_file.job_id, t_file.file_id, t_file.START_TIME ,t_file.PID, t_file.INTERNAL_FILE_PARAMS, t_file.TRANSFERHOST, t_job.REUSE_JOB from t_file, t_job "
                        " where t_job.job_id=t_file.job_id and t_file.file_state='ACTIVE' and t_file.pid is not null "
                        " and t_file.finish_time IS NULL AND t_file.job_finished IS NULL ";

    std::string query1 = "select count(*) from t_file where job_id=:1";

    SafeStatement s;
    SafeResultSet r;
    SafeStatement s1;
    SafeResultSet r1;
    std::string job_id("");
    int file_id = 0;
    time_t start_time;
    int pid = 0;
    std::string internalParams("");
    int timeout = 0;
    int terminateTime = 0;
    int countFiles = 0;
    std::string reuseFlag("");
    std::string vmHostname("");
    const std::string transfer_status = "FAILED";
    const std::string transfer_message = "Transfer has been forced-killed because it was stalled";
    const std::string status = "FAILED";
    double diff = 0;
    SafeConnection pooledConnection;
    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                {
                    return;
                }

            struct message_sanity msg;
            msg.forceFailTransfers = true;
            CleanUpSanityChecks temp(this, pooledConnection, msg);
            if(!temp.getCleanUpSanityCheck())
                {
                    conn->releasePooledConnection(pooledConnection);
                    return;
                }

            s = conn->createStatement(query, tag, pooledConnection);
            r = conn->createResultset(s, pooledConnection);
            while (r->next())
                {
                    job_id = r->getString(1);
                    file_id = r->getInt(2);
                    start_time = conv->toTimeT(r->getTimestamp(3));
                    pid = r->getInt(4);
                    internalParams = r->getString(5);
                    timeout = extractTimeout(internalParams);
                    time_t lifetime = std::time(NULL);
                    vmHostname = r->getString(6);
                    reuseFlag = r->getString(7);
                    if (reuseFlag.compare("Y") == 0)
                        {
                            s1 = conn->createStatement(query1, tag1, pooledConnection);
                            s1->setString(1,job_id);
                            r1 = conn->createResultset(s1, pooledConnection);
                            if(r1->next())
                                {
                                    countFiles = r1->getInt(1);
                                }
                            conn->destroyResultset(s1, r1);
                            conn->destroyStatement(s1, tag1, pooledConnection);
                            terminateTime = 360 * countFiles;
                        }
                    else
                        {
                            terminateTime = (timeout + 3600);
                        }
                    diff = difftime(lifetime, start_time);
                    if (diff > terminateTime)
                        {
                            if(vmHostname.compare(ftsHostName)==0)
                                {
                                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Killing pid:" << pid << ", jobid:" << job_id << ", fileid:" << file_id << " because it was stalled" << commit;
                                    kill(pid, SIGUSR1);
                                }
                            collectJobs.insert(std::make_pair<int, std::string > (file_id, job_id));
                            updateFileTransferStatus(0.0, job_id, file_id, transfer_status, transfer_message, pid, 0, 0);
                            updateJobTransferStatus(file_id, job_id, status);
                        }
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::setAllowed(const std::string & job_id, int file_id, const std::string & source_se, const std::string & dest, int nostreams, int timeout, int buffersize)
{

    const std::string tag4 = "setAllowed";
    const std::string tag5 = "setAllowed111";
    const std::string tag6 = "setAllowed222";
    std::string query_stmt_throuput4 = "update t_optimize set file_id=1 where nostreams=:1 and buffer=:2 and timeout=:3 and source_se=:4 and dest_se=:5";
    std::string query_stmt_throuput5 = "update t_file set INTERNAL_FILE_PARAMS=:1 where file_id=:2 and job_id=:3";
    std::string query_stmt_throuput6 = "update t_file set INTERNAL_FILE_PARAMS=:1 where job_id=:2";
    SafeStatement s4;
    SafeStatement s5;
    SafeStatement s6;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s4 = conn->createStatement(query_stmt_throuput4, tag4, pooledConnection);
            s4->setInt(1, nostreams);
            s4->setInt(2, buffersize);
            s4->setInt(3, timeout);
            s4->setString(4, source_se);
            s4->setString(5, dest);
            if (s4->executeUpdate() != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s4, tag4, pooledConnection);

            std::stringstream params;
            params << "nostreams:" << nostreams << ",timeout:" << timeout << ",buffersize:" << buffersize;
            if (file_id != -1)
                {
                    s5 = conn->createStatement(query_stmt_throuput5, tag5, pooledConnection);
                    s5->setString(1, params.str());
                    s5->setInt(2, file_id);
                    s5->setString(3, job_id);
                    if (s5->executeUpdate() != 0)
                        conn->commit(pooledConnection);
                    conn->destroyStatement(s5, tag5, pooledConnection);
                }
            else
                {
                    s6 = conn->createStatement(query_stmt_throuput6, tag6, pooledConnection);
                    s6->setString(1, params.str());
                    s6->setString(2, job_id);
                    if (s6->executeUpdate() != 0)
                        conn->commit(pooledConnection);
                    conn->destroyStatement(s6, tag6, pooledConnection);
                }
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if (s4)
                conn->destroyStatement(s4, tag4, pooledConnection);
            if (s5)
                conn->destroyStatement(s5, tag5, pooledConnection);
            if (s6)
                conn->destroyStatement(s6, tag6, pooledConnection);


            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s4)
                conn->destroyStatement(s4, tag4, pooledConnection);
            if (s5)
                conn->destroyStatement(s5, tag5, pooledConnection);
            if (s6)
                conn->destroyStatement(s6, tag6, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

bool OracleAPI::terminateReuseProcess(const std::string & jobId)
{
    const std::string tag = "terminateReuseProcess";
    const std::string tag1 = "terminateReuseProcess11";
    std::string query = "select REUSE_JOB from t_job where job_id=:1 and REUSE_JOB is not null";
    std::string update = "update t_file set file_state='FAILED' where job_id=:1 and file_state != 'FINISHED' ";
    SafeStatement s;
    SafeStatement s1;
    SafeResultSet r;
    unsigned int updated = 0;
    bool ok = true;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                {
                    ok = false;
                    return ok;
                }

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, jobId);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    s1 = conn->createStatement(update, tag1, pooledConnection);
                    s1->setString(1, jobId);
                    updated += s1->executeUpdate();
                    conn->destroyStatement(s1, tag1, pooledConnection);
                }
            if (updated != 0)
                conn->commit(pooledConnection);
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            if (s1)
                conn->destroyStatement(s1, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
            ok = false;
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            if (s1)
                conn->destroyStatement(s1, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
            ok = false;
        }
    conn->releasePooledConnection(pooledConnection);
    return ok;
}

void OracleAPI::setPid(const std::string & jobId, int fileId, int pid)
{
    const std::string tag = "setPid";
    std::string query = "update t_file set pid=:1 where job_id=:2 and file_id=:3";
    SafeStatement s;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, pid);
            s->setString(2, jobId);
            s->setInt(3, fileId);
            if (s->executeUpdate() != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::setPidV(int pid, std::map<int, std::string>& pids)
{
    const std::string tag = "setPidV";
    std::string query = "update t_file set pid=:1 where job_id=:2 and file_id=:3";
    SafeStatement s;
    std::map<int, std::string>::const_iterator iter;
    unsigned int updated = 0;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            for (iter = pids.begin(); iter != pids.end(); ++iter)
                {
                    s->setInt(1, pid);
                    s->setString(2, (*iter).second);
                    s->setInt(3, (*iter).first);
                    updated += s->executeUpdate();
                }
            if (updated != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);


            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);


            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::revertToSubmitted()
{
    const std::string tag1 = "revertToSubmitted1";
    const std::string tag2 = "revertToSubmitted2";
    const std::string tag3 = "revertToSubmitted3";
    const std::string tag4 = "revertToSubmitted4";

    std::string query1 = " update t_file set transferhost='', file_state='SUBMITTED', reason='' where file_state='READY' and FINISH_TIME is NULL and JOB_FINISHED is NULL and file_id=:1";

    std::string query2 = " select t_file.start_time, t_file.file_id, t_file.job_id, t_job.REUSE_JOB from t_file,t_job where t_file.file_state"
                         "='READY' and t_file.FINISH_TIME is NULL "
                         " and t_file.JOB_FINISHED is NULL and t_file.job_id=t_job.job_id";


    std::string query3 = " SELECT COUNT(*) FROM t_file WHERE job_id = :1";

    std::string query4 = " UPDATE t_job SET job_state = 'SUBMITTED' where job_id = :1";

    SafeStatement s1;
    SafeStatement s2;
    SafeResultSet r2;
    SafeStatement s3;
    SafeResultSet r3;
    SafeStatement s4;

    double diff = 0;
    int file_id = 0;
    std::string job_id("");
    std::string reuseFlag("");
    time_t start_time;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            struct message_sanity msg;
            msg.revertToSubmitted = true;
            CleanUpSanityChecks temp(this, pooledConnection, msg);
            if(!temp.getCleanUpSanityCheck())
                {
                    conn->releasePooledConnection(pooledConnection);
                    return;
                }

            s2 = conn->createStatement(query2, tag2, pooledConnection);
            r2 = conn->createResultset(s2, pooledConnection);
            while (r2->next())
                {
                    start_time = conv->toTimeT(r2->getTimestamp(1));
                    file_id = r2->getInt(2);
                    job_id = r2->getString(3);
                    reuseFlag = r2->getString(4);
                    time_t current_time = std::time(NULL);
                    diff = difftime(current_time, start_time);
                    bool alive = ThreadSafeList::get_instance().isAlive(file_id);
                    if (diff > 200 && reuseFlag != "Y"  && !alive)
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "The transfer with file id " << file_id << " seems to be stalled, restart it" << commit;
                            s1 = conn->createStatement(query1, tag1, pooledConnection);
                            s1->setInt(1, file_id);
                            s1->executeUpdate();
                            conn->commit(pooledConnection);
                            conn->destroyStatement(s1, tag1, pooledConnection);
                        }
                    else
                        {
                            int count = 0;
                            int terminateTime = 0;
                            s3 = conn->createStatement(query3, tag3, pooledConnection);
                            s3->setString(1, job_id);
                            r3 = conn->createResultset(s3, pooledConnection);
                            while (r3->next())
                                {
                                    count = r3->getInt(1);
                                }
                            conn->destroyResultset(s3, r3);
                            conn->destroyStatement(s3, tag3, pooledConnection);

                            if(count > 0)
                                terminateTime = count * 360;

                            if (diff > terminateTime && reuseFlag == "Y")
                                {
                                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "The transfer with file id(reused) " << file_id << " seems to be stalled, restart it" << commit;
                                    s4 = conn->createStatement(query4, tag4, pooledConnection);
                                    s4->setString(1, job_id);
                                    s4->executeUpdate();
                                    conn->commit(pooledConnection);
                                    conn->destroyStatement(s4, tag4, pooledConnection);

                                    s1 = conn->createStatement(query1, tag1, pooledConnection);
                                    s1->setInt(1, file_id);
                                    s1->executeUpdate();
                                    conn->commit(pooledConnection);
                                    conn->destroyStatement(s1, tag1, pooledConnection);
                                }
                        }
                }
            conn->destroyResultset(s2, r2);
            conn->destroyStatement(s2, tag2, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if(s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if(s2 && r2)
                conn->destroyResultset(s2, r2);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            if(s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            if(s4)
                conn->destroyStatement(s4, tag4, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if(s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if(s2 && r2)
                conn->destroyResultset(s2, r2);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            if(s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            if(s4)
                conn->destroyStatement(s4, tag4, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::backup()
{
    std::string query1 = "insert into t_job_backup select * from t_job where job_state IN ('FINISHED', 'FAILED', 'CANCELED', 'FINISHEDDIRTY') and job_finished < (systimestamp - interval '4' DAY )";
    std::string query2 = "insert into t_file_backup select * from t_file  where job_id IN (select job_id from t_job_backup)";
    std::string query3 = "delete from t_file where file_id in (select file_id from t_file_backup)";
    std::string query4 = "delete from t_job where job_id in (select job_id from t_job_backup)";
    SafeStatement s1;
    SafeStatement s2;
    SafeStatement s3;
    SafeStatement s4;
    SafeConnection pooledConnection;
    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            struct message_sanity msg;
            msg.cleanUpRecords = true;
            CleanUpSanityChecks temp(this, pooledConnection, msg);
            if(!temp.getCleanUpSanityCheck())
                {
                    conn->releasePooledConnection(pooledConnection);
                    return;
                }

            s1 = conn->createStatement(query1, "", pooledConnection);
            s1->executeUpdate();
            conn->commit(pooledConnection);
            s2 = conn->createStatement(query2, "", pooledConnection);
            s2->executeUpdate();
            conn->commit(pooledConnection);
            s3 = conn->createStatement(query3, "", pooledConnection);
            s3->executeUpdate();
            conn->commit(pooledConnection);
            s4 = conn->createStatement(query4, "", pooledConnection);
            s4->executeUpdate();
            conn->commit(pooledConnection);

            conn->destroyStatement(s1, "", pooledConnection);
            conn->destroyStatement(s2, "", pooledConnection);
            conn->destroyStatement(s3, "", pooledConnection);
            conn->destroyStatement(s4, "", pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if(s1)
                conn->destroyStatement(s1, "", pooledConnection);
            if(s2)
                conn->destroyStatement(s2, "", pooledConnection);
            if(s3)
                conn->destroyStatement(s3, "", pooledConnection);
            if(s4)
                conn->destroyStatement(s2, "", pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if(s1)
                conn->destroyStatement(s1, "", pooledConnection);
            if(s2)
                conn->destroyStatement(s2, "", pooledConnection);
            if(s3)
                conn->destroyStatement(s3, "", pooledConnection);
            if(s4)
                conn->destroyStatement(s2, "", pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::forkFailedRevertState(const std::string & jobId, int fileId)
{
    const std::string tag = "forkFailedRevertState";
    std::string query = "update t_file set file_state='SUBMITTED' where file_id=:1 and job_id=:2 and file_state not in ('FINISHED','FAILED','CANCELED')";
    SafeStatement stmt;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            stmt = conn->createStatement(query, tag, pooledConnection);
            stmt->setInt(1, fileId);
            stmt->setString(2, jobId);
            if (stmt->executeUpdate() != 0)
                {
                    conn->commit(pooledConnection);
                }
            conn->destroyStatement(stmt, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(stmt)
                conn->destroyStatement(stmt, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(stmt)
                conn->destroyStatement(stmt, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::forkFailedRevertStateV(std::map<int, std::string>& pids)
{
    const std::string tag = "forkFailedRevertStateV";
    std::string query = "update t_file set file_state='SUBMITTED' where file_id=:1 and job_id=:2 and file_state not in ('FINISHED','FAILED','CANCELED')";
    SafeStatement s;
    std::map<int, std::string>::const_iterator iter;
    unsigned int updated = 0;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            for (iter = pids.begin(); iter != pids.end(); ++iter)
                {
                    s->setInt(1, (*iter).first);
                    s->setString(2, (*iter).second);
                    updated += s->executeUpdate();
                }
            if (updated != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

bool OracleAPI::retryFromDead(std::vector<struct message_updater>& messages)
{
    std::vector<struct message_updater>::const_iterator iter;
    bool isUpdated = true;
    const std::string transfer_status = "FAILED";
    const std::string transfer_message = "no FTS server had updated the transfer status the last 300 seconds, probably stalled in " + ftsHostName;
    const std::string status = "FAILED";
    SafeStatement s;
    SafeResultSet r;
    std::string query = "select file_id from t_file where job_id=:1 and file_id=:2 and file_state='ACTIVE' and transferhost=:3 ";
    const std::string tag = "retryFormDead";
    SafeConnection pooledConnection;
    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                {
                    return isUpdated;
                }

            s = conn->createStatement(query, tag, pooledConnection);
            for (iter = messages.begin(); iter != messages.end(); ++iter)
                {
                    s->setString(1, (*iter).job_id);
                    s->setInt(2, (*iter).file_id);
                    s->setString(3, ftsHostName);
                    r = conn->createResultset(s, pooledConnection);
                    if(r->next())
                        {
                            updateFileTransferStatus(0.0, (*iter).job_id, (*iter).file_id, transfer_status, transfer_message, (*iter).process_id, 0, 0);
                            updateJobTransferStatus((*iter).file_id, (*iter).job_id, status);
                        }
                    conn->destroyResultset(s, r);
                }

            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {
            isUpdated = false;

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            isUpdated = false;

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return isUpdated;
}

void OracleAPI::blacklistSe(std::string se, std::string vo, std::string status, int timeout, std::string msg, std::string adm_dn)
{

    std::string select = "SELECT COUNT(*) FROM t_bad_ses WHERE se = :1";
    std::string insert = "INSERT INTO t_bad_ses (se, message, addition_time, admin_dn, vo, status, wait_timeout) VALUES (:1, :2, :3, :4, :5, :6, :7)";
    std::string update = "UPDATE t_bad_ses SET addition_time = :1, admin_dn = :2, vo = :3, status = :4, wait_timeout = :5 WHERE se = :6 ";

    std::string selectTag = "blacklistSeSelect";
    std::string updateTag = "blacklistSeUpdate";
    std::string insertTag = "blacklistSeInsert";

    SafeStatement s1;
    SafeResultSet r1;
    SafeStatement s2;
    SafeStatement s3;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            time_t timed = time(NULL);

            s1 = conn->createStatement(select, selectTag, pooledConnection);
            s1->setString(1, se);
            r1 = conn->createResultset(s1, pooledConnection);

            int count = 0;
            while(r1->next())
                {
                    count = r1->getInt(1);
                }

            conn->destroyResultset(s1, r1);
            conn->destroyStatement(s1, selectTag, pooledConnection);

            if (count)
                {
                    s2 = conn->createStatement(update, updateTag, pooledConnection);
                    s2->setTimestamp(1, conv->toTimestamp(timed, conn->getEnv()));
                    s2->setString(2, adm_dn);
                    s2->setString(3, vo);
                    s2->setString(4, status);
                    s2->setInt(5, timeout);
                    s2->setString(6, se);
                    s2->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s2, updateTag, pooledConnection);
                }
            else
                {
                    s3 = conn->createStatement(insert, insertTag, pooledConnection);
                    s3->setString(1, se);
                    s3->setString(2, msg);
                    s3->setTimestamp(3, conv->toTimestamp(timed, conn->getEnv()));
                    s3->setString(4, adm_dn);
                    s3->setString(5, vo);
                    s3->setString(6, status);
                    s3->setInt(7, timeout);
                    s3->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s3, selectTag, pooledConnection);
                }

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, selectTag, pooledConnection);

            if (s2)
                conn->destroyStatement(s2, updateTag, pooledConnection);

            if (s3)
                conn->destroyStatement(s3, updateTag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, selectTag, pooledConnection);

            if (s2)
                conn->destroyStatement(s2, updateTag, pooledConnection);

            if (s3)
                conn->destroyStatement(s3, updateTag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::blacklistDn(std::string dn, std::string msg, std::string adm_dn)
{
    std::string insert = "INSERT INTO t_bad_dns (dn, message, addition_time, admin_dn) "
                         "  SELECT :1, :2, :3, :4 FROM DUAL "
                         "  WHERE NOT EXISTS (SELECT dn FROM t_bad_dns WHERE dn = :1";

    std::string insertTag = "blacklistDnInsert";

    SafeStatement s1;

    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            time_t timed = time(NULL);

            s1 = conn->createStatement(insert, insertTag, pooledConnection);
            s1->setString(1, dn);
            s1->setString(2, msg);
            s1->setTimestamp(3, conv->toTimestamp(timed, conn->getEnv()));
            s1->setString(4, adm_dn);
            s1->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s1, insertTag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if (s1)
                conn->destroyStatement(s1, insertTag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s1)
                conn->destroyStatement(s1, insertTag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::unblacklistSe(std::string se)
{

    std::string query = "DELETE FROM t_bad_ses WHERE se = :1";
    std::string tag = "unblacklistSe";

    std::string query2 =
        " UPDATE t_file f "
        " SET f.wait_timestamp = NULL, f.wait_timeout = NULL "
        " WHERE (f.source_se = :1 OR f.dest_se = :2) "
        "	AND f.file_state IN ('ACTIVE', 'READY', 'SUBMITTED') "
        "	AND NOT EXISTS ( "
        "		SELECT NULL "
        "		FROM t_bad_dns, t_job j "
        "		WHERE j.job_id = f.job_id "
        "			AND dn = j.user_dn AND status = 'WAIT' "
        "	) "
        "	AND NOT EXISTS ( "
        "		SELECT NULL "
        "		FROM t_bad_ses, t_job j "
        "		WHERE (se = f.source_se OR se = f.dest_se) AND (status = 'WAIT' OR status = 'WAIT_AS') "
        "	)"
        ;

    std::string tag2 = "unblacklistSeUpdate";

    SafeStatement s;
    SafeStatement s2;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, se);
            s->executeUpdate();
            conn->commit(pooledConnection);

            s2 = conn->createStatement(query2, tag2, pooledConnection);
            s2->setString(1, se);
            s2->setString(2, se);
            s2->executeUpdate();

            conn->commit(pooledConnection);

            conn->destroyStatement(s, tag, pooledConnection);
            conn->destroyStatement(s2, tag2, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::unblacklistDn(std::string dn)
{

    std::string query = "DELETE FROM t_bad_dns WHERE dn = :1";
    std::string tag = "unblacklistDn";

    std::string query2 =
        " UPDATE t_file f "
        " SET f.wait_timestamp = NULL, f.wait_timeout = NULL "
        " WHERE f.file_state IN ('ACTIVE', 'READY', 'SUBMITTED') "
        "	AND NOT EXISTS ( "
        "		SELECT NULL "
        "		FROM t_bad_ses, t_job j "
        "		WHERE (se = f.source_se OR se = f.dest_se) AND status = 'WAIT' "
        "	) "
        "	AND EXISTS ( "
        "		SELECT NULL "
        "		FROM t_job j "
        "		WHERE j.job_id = f.job_id "
        "			AND j.user_dn = :1 "
        "	)";

    std::string tag2 = "unblacklistDnUpdate";

    SafeStatement s;
    SafeStatement s2;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, dn);
            s->executeUpdate();

            s2 = conn->createStatement(query2, tag2, pooledConnection);
            s2->setString(1, dn);
            s2->executeUpdate();

            conn->commit(pooledConnection);

            conn->destroyStatement(s, tag, pooledConnection);
            conn->destroyStatement(s2, tag2, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

bool OracleAPI::isSeBlacklisted(std::string se, std::string vo)
{

    std::string tag = "isSeBlacklisted";
    std::string stmt = "SELECT * FROM t_bad_ses WHERE se = :1 AND (vo IS NULL OR vo='' OR vo = :2)";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;
    bool ret = false;


    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(stmt, tag, pooledConnection);
            s->setString(1, se);
            s->setString(2, vo);
            r = conn->createResultset(s, pooledConnection);

            ret = r->next();

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

bool OracleAPI::allowSubmitForBlacklistedSe(std::string se)
{
    std::string tag = "allowSubmitForBlacklistedSe";
    std::string stmt = "SELECT * FROM t_bad_ses WHERE se = :1 AND status = 'WAIT_AS'";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;
    bool ret = false;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(stmt, tag, pooledConnection);
            s->setString(1, se);
            r = conn->createResultset(s, pooledConnection);

            ret = r->next();

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);

    return ret;
}

boost::optional<int> OracleAPI::getTimeoutForSe(std::string se)
{
    std::string tag = "getTimeoutForSe";
    std::string stmt = " SELECT wait_timeout FROM t_bad_ses WHERE se = :1 ";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;
    boost::optional<int> ret;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(stmt, tag, pooledConnection);
            s->setString(1, se);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    if (!r->isNull(1)) ret = r->getInt(1);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);

    return ret;
}

bool OracleAPI::isDnBlacklisted(std::string dn)
{

    std::string tag = "isDnBlacklisted";
    std::string stmt = "SELECT * FROM t_bad_dns WHERE dn = :1";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;
    bool ret = false;


    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(stmt, tag, pooledConnection);
            s->setString(1, dn);
            r = conn->createResultset(s, pooledConnection);

            ret = r->next();

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {


            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);


            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknoen exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

/********* section for the new config API **********/
bool OracleAPI::isFileReadyState(int fileID)
{
    const std::string tag = "isFileReadyState";
    bool ready = false;
    std::string query = "select file_state from t_file where file_id=:1";
    std::string hostname("");
    std::string tag1 = "transferHost2";
    std::string query1 = "select transferHost from t_file where file_id=:1";
    SafeStatement s1;
    SafeResultSet r1;


    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return false;

            //check ready
            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, fileID);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    std::string state = r->getString(1);
                    if (state.compare("READY") == 0)
                        ready = true;
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

            //check hostname
            s1 = conn->createStatement(query1, tag1, pooledConnection);
            s1->setInt(1, fileID);
            r1 = conn->createResultset(s1, pooledConnection);
            if (r1->next())
                {
                    hostname = r1->getString(1);
                }
            conn->destroyResultset(s1, r1);
            conn->destroyStatement(s1, tag1, pooledConnection);

            ready = (hostname == ftsHostName);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ready;
}

bool OracleAPI::checkGroupExists(const std::string & groupName)
{
    const std::string tag = "checkGroupExists";
    std::string query = "select groupName from t_group_members where groupName=:1";
    SafeStatement s;
    SafeResultSet r;
    bool groupExist = false;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return false;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, groupName);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    groupExist = true;
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);


            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return groupExist;
}

//t_group_members

void OracleAPI::getGroupMembers(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    const std::string tag = "getGroupMembers";
    std::string query = "select member from t_group_members where groupName=:1";
    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, groupName);
            r = conn->createResultset(s, pooledConnection);
            while (r->next())
                {
                    std::string member = r->getString(1);
                    groupMembers.push_back(member);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            if (conn)
                {
                    conn->rollback(pooledConnection);

                    if(s && r)
                        conn->destroyResultset(s, r);
                    if (s)
                        conn->destroyStatement(s, tag, pooledConnection);
                }

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

std::string OracleAPI::getGroupForSe(const std::string se)
{
    const std::string tag = "getGroupForSe";
    std::string query = "select groupName from t_group_members where member=:1";
    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;


    std::string ret;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, se);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    ret = r->getString(1);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);


            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}


void OracleAPI::addMemberToGroup(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    std::string tag="addMemberToGroup";
    std::string query = "insert into t_group_members(member, groupName) values(:1, :2)";
    SafeStatement s;
    std::vector<std::string>::const_iterator iter;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            for (iter = groupMembers.begin(); iter != groupMembers.end(); ++iter)
                {
                    s->setString(1, std::string(*iter));
                    s->setString(2, groupName);
                    s->executeUpdate();
                }
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::deleteMembersFromGroup(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    std::string tag = "deleteMembersFromGroup";
    std::string query = "delete from t_group_members where groupName=:1 and member=:2";
    SafeStatement s;
    std::vector<std::string>::const_iterator iter;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            for (iter = groupMembers.begin(); iter != groupMembers.end(); ++iter)
                {
                    s->setString(1, groupName);
                    s->setString(2, std::string(*iter));
                    s->executeUpdate();
                }
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);


            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::addLinkConfig(LinkConfig* cfg)
{
    const std::string tag = "addLinkConfig";
    std::string query =
        "insert into t_link_config("
        "	source,"
        "	destination,"
        "	state,"
        "	symbolicName,"
        "	NOSTREAMS,"
        "	tcp_buffer_size,"
        "	URLCOPY_TX_TO,"
        "	no_tx_activity_to,"
        "	auto_tuning"
        ") values(:1,:2,:3,:4,:5,:6,:7,:8,:9)";

    SafeStatement s;
    SafeConnection pooledConnection;
    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, cfg->source);
            s->setString(2, cfg->destination);
            s->setString(3, cfg->state);
            s->setString(4, cfg->symbolic_name);
            s->setInt(5, cfg->NOSTREAMS);
            s->setInt(6, cfg->TCP_BUFFER_SIZE);
            s->setInt(7, cfg->URLCOPY_TX_TO);
            s->setInt(8, cfg->NO_TX_ACTIVITY_TO);
            s->setString(9, cfg->auto_tuning);
            if (s->executeUpdate() != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::updateLinkConfig(LinkConfig* cfg)
{
    std::string tag = "updateLinkConfig";
    std::string query =
        "update t_link_config "
        "set state=:1,symbolicName=:2,NOSTREAMS=:3,tcp_buffer_size=:4,URLCOPY_TX_TO=:5,no_tx_activity_to=:6, auto_tuning=:7 "
        "where source=:8 and destination=:9";
    SafeStatement s;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, cfg->state);
            s->setString(2, cfg->symbolic_name);
            s->setInt(3, cfg->NOSTREAMS);
            s->setInt(4, cfg->TCP_BUFFER_SIZE);
            s->setInt(5, cfg->URLCOPY_TX_TO);
            s->setInt(6, cfg->NO_TX_ACTIVITY_TO);
            s->setString(7, cfg->auto_tuning);
            s->setString(8, cfg->source);
            s->setString(9, cfg->destination);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::deleteLinkConfig(std::string source, std::string destination)
{
    const std::string tag = "deleteLinkConfig";
    std::string query = "delete from t_link_config where source=:1 and destination=:2";
    SafeStatement s;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, source);
            s->setString(2, destination);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

LinkConfig* OracleAPI::getLinkConfig(std::string source, std::string destination)
{
    std::string tag = "getLinkConfig";
    std::string query =
        "select source, destination, state, symbolicName, nostreams, tcp_buffer_size, urlcopy_tx_to, no_tx_activity_to, auto_tuning "
        "from t_link_config where source=:1 and destination=:2";
    SafeStatement s;
    SafeResultSet r;
    LinkConfig* cfg = NULL;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return NULL;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, source);
            s->setString(2, destination);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    cfg = new LinkConfig();
                    cfg->source = r->getString(1);
                    cfg->destination = r->getString(2);
                    cfg->state = r->getString(3);
                    cfg->symbolic_name = r->getString(4);
                    cfg->NOSTREAMS = r->getInt(5);
                    cfg->TCP_BUFFER_SIZE = r->getInt(6);
                    cfg->URLCOPY_TX_TO = r->getInt(7);
                    cfg->NO_TX_ACTIVITY_TO = r->getInt(8);
                    cfg->auto_tuning = r->getString(9);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);


            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return cfg;
}

bool OracleAPI::isThereLinkConfig(std::string source, std::string destination)
{

    std::string tag = "isThereLinkConfig";
    std::string query =
        "select count(*) "
        "from t_link_config "
        "where state='on' and source=:1 and destination=:2 ";
    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;


    bool ret = false;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return NULL;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, source);
            s->setString(2, destination);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    ret = r->getInt(1) > 0;
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

std::pair<std::string, std::string>* OracleAPI::getSourceAndDestination(std::string symbolic_name)
{

    std::string tag = "getSourceAndDestination";
    std::string query =
        "select source,destination "
        "from t_link_config where symbolicName=:1";
    SafeStatement s;
    SafeResultSet r;
    std::pair<std::string, std::string>* ret = NULL;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return NULL;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, symbolic_name);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    ret = new std::pair<std::string, std::string>();
                    ret->first = r->getString(1);
                    ret->second = r->getString(2);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

bool OracleAPI::isGrInPair(std::string group)
{

    std::string tag = "isGrInPair";
    std::string query =
        "select * from t_link_config "
        "where ((source=:1 and destination<>'*') or (source<>'*' and destination=:1))";
    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;


    bool ret = false;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return NULL;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, group);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    ret = true;
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

bool OracleAPI::isShareOnly(std::string se)
{

    std::string tag = "isShareOnly";
    std::string query =
        "select * from t_link_config "
        "where source=:1 and destination = '*' and auto_tuning = 'all' ";
    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;


    bool ret = false;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return NULL;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, se);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    ret = true;
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

void OracleAPI::addShareConfig(ShareConfig* cfg)
{
    const std::string tag = "addShareConfig";
    std::string query =
        "insert into t_share_config("
        "	source,"
        "	destination,"
        "	vo,"
        "	active"
        ") values(:1,:2,:3,:4)";

    SafeStatement s;
    SafeConnection pooledConnection;
    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, cfg->source);
            s->setString(2, cfg->destination);
            s->setString(3, cfg->vo);
            s->setInt(4, cfg->active_transfers);
            if (s->executeUpdate() != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}
void OracleAPI::updateShareConfig(ShareConfig* cfg)
{
    std::string tag = "updateShareConfig";
    std::string query =
        "update t_share_config "
        "set active=:1 "
        "where source=:2 and destination=:3 and vo=:4";
    SafeStatement s;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, cfg->active_transfers);
            s->setString(2, cfg->source);
            s->setString(3, cfg->destination);
            s->setString(4, cfg->vo);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::deleteShareConfig(std::string source, std::string destination, std::string vo)
{
    const std::string tag = "deleteShareConfig";
    std::string query = "delete from t_share_config where source=:1 and destination=:2 and vo=:3";
    SafeStatement s;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, source);
            s->setString(2, destination);
            s->setString(3, vo);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::deleteShareConfig(std::string source, std::string destination)
{
    const std::string tag = "deleteShareConfig2";
    std::string query = "delete from t_share_config where source=:1 and destination=:2";
    SafeStatement s;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, source);
            s->setString(2, destination);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

ShareConfig* OracleAPI::getShareConfig(std::string source, std::string destination, std::string vo)
{
    std::string tag = "getShareConfig";
    std::string query =
        "select source,destination,vo,active "
        "from t_share_config where source=:1 and destination=:2 and vo=:3";
    SafeStatement s;
    SafeResultSet r;
    ShareConfig* cfg = NULL;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return NULL;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, source);
            s->setString(2, destination);
            s->setString(3, vo);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    cfg = new ShareConfig();
                    cfg->source = r->getString(1);
                    cfg->destination = r->getString(2);
                    cfg->vo = r->getString(3);
                    cfg->active_transfers = r->getInt(4);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return cfg;
}

std::vector<ShareConfig*> OracleAPI::getShareConfig(std::string source, std::string destination)
{
    std::string tag = "getShareConfig2";
    std::string query =
        "select source,destination,vo,active "
        "from t_share_config where source=:1 and destination=:2";
    SafeStatement s;
    SafeResultSet r;
    std::vector<ShareConfig*> ret;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, source);
            s->setString(2, destination);
            r = conn->createResultset(s, pooledConnection);
            while (r->next())
                {
                    ShareConfig* cfg = new ShareConfig();
                    cfg->source = r->getString(1);
                    cfg->destination = r->getString(2);
                    cfg->vo = r->getString(3);
                    cfg->active_transfers = r->getInt(4);
                    ret.push_back(cfg);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

void OracleAPI::submitHost(const std::string & jobId)
{

    std::string tag = "submitHost";
    std::string query = "update t_job set submit_host=:1 where job_id=:2";
    SafeStatement s;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, ftsHostName);
            s->setString(2, jobId);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

std::string OracleAPI::transferHost(int fileId)
{
    std::string hostname("");
    std::string tag = "transferHost";
    std::string query = "select transferHost from t_file where file_id=:1";
    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return std::string("");

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, fileId);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    hostname = r->getString(1);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return hostname;
}

/*for session reuse check only*/
bool OracleAPI::isFileReadyStateV(std::map<int, std::string>& fileIds)
{
    const std::string tag = "isFileReadyStateV";
    std::string query = "select file_state from t_file where file_id=:1";
    SafeStatement s;
    SafeResultSet r;
    std::string ready("");
    bool isReady = false;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return false;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, fileIds.begin()->first);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    ready = r->getString(1);
                    if (ready.compare("READY") == 0)
                        {
                            isReady = true;
                        }
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return isReady;
}

std::string OracleAPI::transferHostV(std::map<int, std::string>& fileIds)
{
    const std::string tag = "transferHostV";
    std::string host("");
    std::string query = "select transferHost from t_file where file_id=:1";
    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return std::string("");

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, fileIds.begin()->first);
            r = conn->createResultset(s, pooledConnection);
            if(r->next())
                {
                    host = r->getString(1);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return host;
}


bool OracleAPI::checkIfSeIsMemberOfAnotherGroup(const std::string & member)
{
    std::string tag = "checkIfSeIsMemberOfAnotherGroup";
    std::string stmt = "select groupName from t_group_members where member=:1 ";

    SafeStatement s;
    SafeResultSet r;
    bool ret = false;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(stmt, tag, pooledConnection);
            s->setString(1, member);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    ret = true;
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {


            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);


            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {


            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}


void OracleAPI::addFileShareConfig(int file_id, std::string source, std::string destination, std::string vo)
{
    const std::string tag = "addJobShareConfig";

    std::string insert = " insert into t_file_share_config (file_id, source, destination, vo) "
                         " select :1, :2, :3, :4 from dual where not exists(select file_id, source, destination, vo "
                         " from t_file_share_config WHERE file_id = :1 AND source = :2 AND destination = :3 AND vo = :4)";

    SafeStatement s;
    SafeConnection pooledConnection;
    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(insert, tag, pooledConnection);
            s->setInt(1, file_id);
            s->setString(2, source);
            s->setString(3, destination);
            s->setString(4, vo);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);

            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {
            conn->rollback(pooledConnection);

            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

//void OracleAPI::delJobShareConfig(std::string job_id) {
//
//    std::string query = "delete from t_job_share_config where job_id = :1";
//    std::string tag = "delJobShareConfig";
//
//    SafeStatement s;
//    SafeConnection pooledConnection;
//
//    try {
//	pooledConnection = conn->getPooledConnection();
//        if (!pooledConnection)
//		return;
//
//        s = conn->createStatement(query, tag, pooledConnection);
//        s->setString(1, job_id);
//        if(s->executeUpdate() != 0) conn->commit(pooledConnection);
//        conn->destroyStatement(s, tag, pooledConnection);
//
//    } catch (oracle::occi::SQLException const &e) {
//			conn->rollback(pooledConnection);
//			if(s)
//				conn->destroyStatement(s, tag, pooledConnection);
//
//		FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
//    }catch (...) {
//
//			conn->rollback(pooledConnection);
//			if(s)
//				conn->destroyStatement(s, tag, pooledConnection);
//
//		FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
//    }
//    conn->releasePooledConnection(pooledConnection);
//}
//
//std::vector< boost::tuple<std::string, std::string, std::string> > OracleAPI::getJobShareConfig(std::string job_id) {
//
//    std::string tag = "getJobShareConfig";
//    std::string query = " select source, destination, vo from t_job_share_config where job_id=:1 ";
//
//    SafeStatement s;
//    SafeResultSet r;
//    SafeConnection pooledConnection;
//    std::vector< boost::tuple<std::string, std::string, std::string> > ret;
//
//
//    try {
//
//	pooledConnection = conn->getPooledConnection();
//        if (!pooledConnection) return ret;
//
//        s = conn->createStatement(query, tag, pooledConnection);
//        s->setString(1, job_id);
//        r = conn->createResultset(s, pooledConnection);
//
//        while (r->next()) {
//        	boost::tuple<std::string, std::string, std::string> tmp;
//        	boost::get<0>(tmp) = r->getString(1);
//        	boost::get<1>(tmp) = r->getString(2);
//        	boost::get<2>(tmp) = r->getString(3);
//        	ret.push_back(tmp);
//        }
//
//        conn->destroyResultset(s, r);
//        conn->destroyStatement(s, tag, pooledConnection);
//
//    } catch (oracle::occi::SQLException const &e) {
//
//            conn->rollback(pooledConnection);
//        	if(s && r)
//        		conn->destroyResultset(s, r);
//        	if (s)
//        		conn->destroyStatement(s, tag, pooledConnection);
//
//        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
//    }catch (...) {
//
//
//            conn->rollback(pooledConnection);
//        	if(s && r)
//        		conn->destroyResultset(s, r);
//        	if (s)
//        		conn->destroyStatement(s, tag, pooledConnection);
//
//        FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
//    }
//    conn->releasePooledConnection(pooledConnection);
//    return ret;
//}
//
//unsigned int OracleAPI::countJobShareConfig(std::string job_id) {
//
//    std::string tag = "isThereJobShareConfig";
//    std::string query = " select count(*) from t_job_share_config where job_id=:1 ";
//
//    SafeStatement s;
//    SafeResultSet r;
//    SafeConnection pooledConnection;
//    unsigned int ret;
//
//
//    try {
//
//	pooledConnection = conn->getPooledConnection();
//        if (!pooledConnection) return ret;
//
//        s = conn->createStatement(query, tag, pooledConnection);
//        s->setString(1, job_id);
//        r = conn->createResultset(s, pooledConnection);
//
//        if (r->next()) {
//        	ret = r->getInt(1);
//        }
//
//        conn->destroyResultset(s, r);
//        conn->destroyStatement(s, tag, pooledConnection);
//
//    } catch (oracle::occi::SQLException const &e) {
//
//            conn->rollback(pooledConnection);
//        	if(s && r)
//        		conn->destroyResultset(s, r);
//        	if (s)
//        		conn->destroyStatement(s, tag, pooledConnection);
//
//        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
//    }catch (...) {
//
//            conn->rollback(pooledConnection);
//        	if(s && r)
//        		conn->destroyResultset(s, r);
//        	if (s)
//        		conn->destroyStatement(s, tag, pooledConnection);
//
//        FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
//    }
//    conn->releasePooledConnection(pooledConnection);
//    return ret;
//}

int OracleAPI::countActiveTransfers(std::string source, std::string destination, std::string vo)
{

    std::string tag = "countSeActiveTransfers";
    std::string query =
        "select count(*) "
        "from t_file f, t_file_share_config c "
        "where "
        "	(f.file_state = 'ACTIVE' or f.file_state = 'READY') and "
        "	f.file_id = c.file_id and "
        "	c.source = :1 and "
        "	c.destination = :2 and "
        "	c.vo = :3"
        ;

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;
    int ret = 0;


    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, source);
            s->setString(2, destination);
            s->setString(3, vo);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    ret = r->getInt(1);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {


            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

int OracleAPI::countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo)
{

    std::string tag = "countActiveOutboundTransfersUsingDefaultCfg";
    std::string query =
        "select count(*) "
        "from t_file f, t_file_share_config c "
        "where "
        "	(f.file_state = 'ACTIVE'  or f.file_state = 'READY') and "
        "	f.source_se = :1 and "
        "	f.file_id = c.file_id and "
        "	c.source = '(*)' and "
        "	c.destination = '*' and "
        "	c.vo = :2"
        ;

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;
    int ret = 0;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, se);
            s->setString(2, vo);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    ret = r->getInt(1);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

int OracleAPI::countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo)
{

    std::string tag = "countActiveInboundTransfersUsingDefaultCfg";
    std::string query =
        "select count(*) "
        "from t_file f, t_file_share_config c "
        "where "
        "	(f.file_state = 'ACTIVE' or f.file_state = 'READY') and "
        "	f.dest_se = :1 and "
        "	f.file_id = c.file_id and "
        "	c.source = '*' and "
        "	c.destination = '(*)' and "
        "	c.vo = :2"
        ;

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;
    int ret = 0;


    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, se);
            s->setString(2, vo);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    ret = r->getInt(1);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

int OracleAPI::sumUpVoShares (std::string source, std::string destination, std::set<std::string> vos)
{
    std::string tag1 = "sumUpVoShares1";
    std::string query1 =
        " SELECT vo "
        " FROM t_share_config "
        " WHERE source = :1 "
        "	AND destination = :2 "
        "	AND vo = :3 "
        ;

    std::string tag2 = "sumUpVoShares2";
    std::string query2 =
        " SELECT SUM(active) "
        " FROM t_share_config "
        " WHERE source = :1 "
        "	AND destination = :2 "
        "	AND vo IN "
        ;

    SafeStatement s1;
    SafeResultSet r1;
    SafeStatement s2;
    SafeResultSet r2;

    SafeConnection pooledConnection;

    int sum = 0;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return false;

            std::set<std::string>::iterator it = vos.begin();

            while (it != vos.end())
                {
                    std::set<std::string>::iterator remove = it;
                    it++;

                    s1 = conn->createStatement(query1, tag1, pooledConnection);
                    s1->setString(1, source);
                    s1->setString(2, destination);
                    s1->setString(3, *remove);
                    r1 = conn->createResultset(s1, pooledConnection);

                    if (!r1->next() && *remove != "public")
                        {
                            // if there is no configuration for this VO replace it with 'public'
                            vos.erase(remove);
                            vos.insert("public");
                        }

                    conn->destroyResultset(s1, r1);
                    conn->destroyStatement(s1, tag1, pooledConnection);
                }

            std::string vos_str = "(";

            for (it = vos.begin(); it != vos.end(); ++it)
                {

                    vos_str += "'" + *it + "'" + ",";
                }

            // replace the last ',' with closing ')'
            vos_str[vos_str.size() - 1] = ')';

            // append the VO list to the second query
            query2 += vos_str;
            tag2 += vos_str;

            s2 = conn->createStatement(query2, tag2, pooledConnection);
            s2->setString(1, source);
            s2->setString(2, destination);
            r2 = conn->createResultset(s2, pooledConnection);

            if (r2->next())
                {
                    sum = r2->getInt(1);
                }

            conn->destroyResultset(s2, r2);
            conn->destroyStatement(s2, tag2, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);

            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if (s2 && r2)
                conn->destroyResultset(s1, r1);
            if (s2)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if (s2 && r2)
                conn->destroyResultset(s1, r1);
            if (s2)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);

    return sum;
}


void OracleAPI::setPriority(std::string job_id, int priority)
{

    const std::string tag = "setPriority";
    std::string query =
        "UPDATE t_job "
        "SET priority =:1 "
        "WHERE job_id = :2 ";

    SafeStatement s;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, priority);
            s->setString(2, job_id);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

int OracleAPI::getRetry(const std::string & jobId)
{
    std::string tag = "getRetry";
    std::string tag1 = "getRetry1";
    std::string query =
        "select retry "
        "from t_server_config ";
    std::string query1 =
        "select retry "
        "from t_job where job_id=:1 ";

    SafeStatement s;
    SafeResultSet r;
    SafeStatement s1;
    SafeResultSet r1;
    int ret = 0;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;


            s1 = conn->createStatement(query1, tag1, pooledConnection);
            s1->setString(1, jobId);
            r1 = conn->createResultset(s1, pooledConnection);

            if (r1->next())
                {
                    ret = r1->getInt(1);
                }

            conn->destroyResultset(s1, r1);
            conn->destroyStatement(s1, tag1, pooledConnection);

            if(ret==0) //now check for global retry value
                {
                    s = conn->createStatement(query, tag, pooledConnection);
                    r = conn->createResultset(s, pooledConnection);

                    if (r->next())
                        {
                            ret = r->getInt(1);
                        }

                    conn->destroyResultset(s, r);
                    conn->destroyStatement(s, tag, pooledConnection);
                }
            else if (ret < 0)
                {
                    ret = 0;
                }

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

void OracleAPI::setRetry(int retry)
{
    const std::string tag = "setRetry";
    std::string query =
        "UPDATE t_server_config "
        "SET retry =:1 ";

    SafeStatement s;
    SafeConnection pooledConnection;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, retry);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

int OracleAPI::getRetryTimes(const std::string & jobId, int fileId)
{
    std::string tag = "getRetryTimes";
    std::string query =
        "select retry from t_file where job_id=:1 and file_id=:2 ";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;
    int ret = 0;


    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1,jobId);
            s->setInt(2,fileId);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    ret = r->getInt(1);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

int OracleAPI::getMaxTimeInQueue()
{
    std::string tag = "getMaxTimeInQueue";
    std::string query =
        "select max_time_queue "
        "from t_server_config ";

    SafeStatement s;
    SafeResultSet r;
    int ret = 0;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    ret = r->getInt(1);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;

}

void OracleAPI::setMaxTimeInQueue(int afterXHours)
{
    const std::string tag = "setMaxTimeInQueue";
    std::string query = " UPDATE t_server_config set max_time_queue=:1 ";

    SafeStatement s;

    SafeConnection pooledConnection;
    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, afterXHours);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}


bool OracleAPI::checkConnectionStatus()
{
    SafeConnection pooledConnection;

    bool alive = false;
    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                alive = false;
            else
                alive = true;
        }
    catch(...)
        {
            alive = false;
        }
    conn->releasePooledConnection(pooledConnection);
    return alive;
}


void OracleAPI::setToFailOldQueuedJobs(std::vector<std::string>& jobs)
{

    /*in hours*/
    int maxTime = getMaxTimeInQueue();
    if(maxTime==0)
        return;


    const std::string tag1 = "setToFailOldQueuedJobs111";
    const std::string tag2 = "setToFailOldQueuedJobs222";
    const std::string tag3 = "setToFailOldQueuedJobs333";

    std::string query1 = " UPDATE t_file set file_state='CANCELED', reason=:1 where job_id=:2 and file_state in ('SUBMITTED','READY') ";
    std::string query2 = " UPDATE t_job set job_state='CANCELED', reason=:1 where job_id=:2  and job_state in ('SUBMITTED','READY') ";
    std::string message = "Job has been canceled because it stayed in the queue for too long";
    std::stringstream query3;

    SafeStatement s1;
    SafeStatement s2;
    SafeStatement s3;
    SafeResultSet r;
    std::vector<std::string> jobId;
    std::vector<std::string>::const_iterator iter;
    SafeConnection pooledConnection;

    query3 << " select job_id from t_job where (SUBMIT_TIME < (SYS_EXTRACT_UTC(SYSTIMESTAMP) - interval '";
    query3 << maxTime;
    query3 << "' hour(5))) and job_state in ('SUBMITTED','READY')  ";

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            struct message_sanity msg;
            msg.setToFailOldQueuedJobs = true;
            CleanUpSanityChecks temp(this, pooledConnection, msg);
            if(!temp.getCleanUpSanityCheck())
                {
                    conn->releasePooledConnection(pooledConnection);
                    return;
                }

            s3 = conn->createStatement(query3.str(), tag3, pooledConnection);
            r = conn->createResultset(s3, pooledConnection);
            while (r->next())
                {
                    jobId.push_back(r->getString(1));
                    jobs.push_back(r->getString(1));
                }
            conn->destroyResultset(s3, r);
            conn->destroyStatement(s3, tag3, pooledConnection);

            if(!jobId.empty())
                {
                    s1 = conn->createStatement(query1, tag1, pooledConnection);
                    s2 = conn->createStatement(query2, tag2, pooledConnection);
                    for (iter = jobId.begin(); iter != jobId.end(); ++iter)
                        {
                            s1->setString(1,message);
                            s1->setString(2,*iter);
                            s1->executeUpdate();
                            s2->setString(1,message);
                            s2->setString(2,*iter);
                            s2->executeUpdate();
                        }
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s1, tag1, pooledConnection);
                    conn->destroyStatement(s2, tag2, pooledConnection);
                }
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s3 && r)
                conn->destroyResultset(s3, r);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s3 && r)
                conn->destroyResultset(s3, r);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);


            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

std::vector< std::pair<std::string, std::string> > OracleAPI::getPairsForSe(std::string se)
{
    std::vector< std::pair<std::string, std::string> > ret;

    std::string tag = "getPairsForSe";
    std::string query =
        " select source, destination "
        " from t_link_config "
        " where (source = :1 and destination <> '*') "
        "	or (source <> '*' and destination = :2) ";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, se);
            s->setString(2, se);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {
                    ret.push_back(std::make_pair(r->getString(1), r->getString(2)));
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

std::vector<std::string> OracleAPI::getAllStandAlloneCfgs()
{

    std::vector<std::string> ret;

    std::string tag = "getAllStandAlloneCfgs";
    std::string query =
        " select SOURCE "
        " from T_LINK_CONFIG "
        " where DESTINATION = '*' "
        "	and auto_tuning <> 'all' ";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {
                    ret.push_back(r->getString(1));
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

std::vector<std::string> OracleAPI::getAllShareOnlyCfgs()
{

    std::vector<std::string> ret;

    std::string tag = "getAllShareOnlyCfgs";
    std::string query =
        " select SOURCE "
        " from T_LINK_CONFIG "
        " where DESTINATION = '*' "
        "	and auto_tuning = 'all' ";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {
                    ret.push_back(r->getString(1));
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

std::vector< std::pair<std::string, std::string> > OracleAPI::getAllPairCfgs()
{


    std::vector< std::pair<std::string, std::string> > ret;
    std::string tag = "getAllPairCfgs";
    std::string query = "select SOURCE, DESTINATION from T_LINK_CONFIG where SOURCE <> '*' and DESTINATION <> '*'";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {
                    ret.push_back(
                        std::make_pair(r->getString(1), r->getString(2))
                    );
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}

int OracleAPI::activeProcessesForThisHost()
{
    std::string tag = "activeProcessesForThisHost";
    std::string query = "select count(*) from t_file where file_state in ('READY','ACTIVE') and TRANSFERHOST=:1";

    SafeStatement s;
    SafeResultSet r;
    int ret = 0;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, ftsHostName);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    ret = r->getInt(1);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;

}



std::vector< boost::tuple<std::string, std::string, int> >  OracleAPI::getVOBringonlimeMax()
{
    std::string tag = "getVOBringonlimeMax";
    std::string query = " select vo_name, host, concurrent_ops from t_stage_req";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;
    std::vector< boost::tuple<std::string, std::string, int> > ret;


    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {
                    boost::tuple<std::string, std::string, int> tmp;
                    boost::get<0>(tmp) = r->getString(1);
                    boost::get<1>(tmp) = r->getString(2);
                    boost::get<2>(tmp) = r->getInt(3);
                    ret.push_back(tmp);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {


            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}


std::vector<struct message_bringonline> OracleAPI::getBringOnlineFiles(std::string voName, std::string hostName, int maxValue)
{
    std::string tag1 = "getBringOnlineFiles1";
    std::string tag2 = "getBringOnlineFiles2";
    std::string tag3 = "getBringOnlineFiles3";
    std::string tag4 = "getBringOnlineFiles4";
    std::string tag5 = "getBringOnlineFiles5";

    std::string query1 =
        " select distinct(t_file.SOURCE_SE) from t_file, t_job where t_job.job_id = t_file.job_id "
        " and (t_job.BRING_ONLINE > 0 OR t_job.COPY_PIN_LIFETIME > 0) and t_file.file_state = 'STAGING' and t_file.STAGING_START is null and t_file.SOURCE_SURL like 'srm%' ";
    std::string query2 =
        " select SOURCE_SURL, job_id, file_id, COPY_PIN_LIFETIME, BRING_ONLINE from (select t_file.SOURCE_SURL, t_file.job_id, t_file.file_id, t_job.COPY_PIN_LIFETIME, t_job.BRING_ONLINE from t_file, t_job where t_job.job_id = t_file.job_id "
        " and (t_job.BRING_ONLINE > 0 OR t_job.COPY_PIN_LIFETIME > 0) and t_file.STAGING_START is null and t_file.file_state = 'STAGING' "
        " and t_file.source_se=:1 and t_job.vo_name=:2  and t_file.SOURCE_SURL like 'srm%' and SUBMIT_HOST=:3 ORDER BY t_file.file_id ) WHERE rownum<=:4 ";
    std::string query3 =
        " select SOURCE_SURL, job_id, file_id, COPY_PIN_LIFETIME, BRING_ONLINE from (select t_file.SOURCE_SURL, t_file.job_id, t_file.file_id, t_job.COPY_PIN_LIFETIME, t_job.BRING_ONLINE from t_file, t_job where t_job.job_id = t_file.job_id "
        " and (t_job.BRING_ONLINE > 0 OR t_job.COPY_PIN_LIFETIME > 0) and t_file.STAGING_START is null and t_file.file_state = 'STAGING' and t_file.source_se=:1 "
        " and t_file.SOURCE_SURL like 'srm%' and SUBMIT_HOST=:2  ORDER BY t_file.file_id) WHERE rownum<=:3 ";
    std::string query4 =
        " select count(*) from t_file, t_job where t_job.job_id = t_file.job_id "
        " and (t_job.BRING_ONLINE > 0 OR t_job.COPY_PIN_LIFETIME > 0) and t_file.file_state = 'STAGING' and t_file.STAGING_START is not null "
        " and t_job.vo_name=:1 and t_file.source_se=:2  and t_file.SOURCE_SURL like 'srm%' ";
    std::string query5 =
        " select count(*) from t_file, t_job where t_job.job_id = t_file.job_id "
        " and (t_job.BRING_ONLINE > 0 OR t_job.COPY_PIN_LIFETIME > 0) and t_file.file_state = 'STAGING' and t_file.STAGING_START is not null and t_file.source_se=:1 "
        " and t_file.SOURCE_SURL like 'srm%' ";


    SafeStatement s1;
    SafeResultSet r1;
    SafeStatement s2;
    SafeResultSet r2;
    SafeStatement s3;
    SafeResultSet r3;
    SafeStatement s4;
    SafeResultSet r4;
    SafeStatement s5;
    SafeResultSet r5;

    std::vector<struct message_bringonline> ret;
    SafeConnection pooledConnection;
    unsigned int currentStagingFilesConfig = 0;
    unsigned int currentStagingFilesNoConfig = 0;
    unsigned int maxConfig = 0;
    unsigned int maxNoConfig = 0;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return ret;

            if(voName.length() > 0)
                {
                    s4 = conn->createStatement(query4, tag4, pooledConnection);
                    s4->setString(1, voName);
                    s4->setString(2, hostName);
                    r4 = conn->createResultset(s4, pooledConnection);
                    if(r4->next())
                        {
                            currentStagingFilesConfig = r4->getInt(1);
                        }
                    conn->destroyResultset(s4, r4);
                    conn->destroyStatement(s4, tag4, pooledConnection);

                    if(currentStagingFilesConfig > 0 )
                        {
                            maxConfig = maxValue - currentStagingFilesConfig;
                        }
                    else
                        {
                            maxConfig = maxValue;
                        }

                    s2 = conn->createStatement(query2, tag2, pooledConnection);
                    s2->setString(1, hostName);
                    s2->setString(2, voName);
                    s2->setString(3,ftsHostName);
                    s2->setInt(4, maxConfig);
                    r2 = conn->createResultset(s2, pooledConnection);
                    while (r2->next())
                        {
                            struct message_bringonline msg;
                            msg.url = r2->getString(1);
                            msg.job_id = r2->getString(2);
                            msg.file_id = r2->getInt(3);
                            msg.pinlifetime = r2->getInt(4);
                            msg.bringonlineTimeout = r2->getInt(5);
                            ret.push_back(msg);
                            bringOnlineReportStatus("STARTED", "", msg);
                        }
                    conn->destroyResultset(s2, r2);
                    conn->destroyStatement(s2, tag2, pooledConnection);
                }
            else
                {
                    s1 = conn->createStatement(query1, tag1, pooledConnection);
                    r1 = conn->createResultset(s1, pooledConnection);
                    s5 = conn->createStatement(query5, tag5, pooledConnection);
                    s3 = conn->createStatement(query3, tag3, pooledConnection);
                    while (r1->next())
                        {
                            std::string hostV = r1->getString(1);
                            s5->setString(1, hostV);
                            r5 = conn->createResultset(s5, pooledConnection);
                            if(r5->next())
                                {
                                    currentStagingFilesNoConfig = r5->getInt(1);
                                }
                            conn->destroyResultset(s5, r5);

                            if(currentStagingFilesNoConfig > 0 )
                                {
                                    maxNoConfig = maxValue - currentStagingFilesNoConfig;
                                }
                            else
                                {
                                    maxNoConfig = maxValue;
                                }

                            s3->setString(1, hostV);
                            s3->setString(2,ftsHostName);
                            s3->setInt(3, maxNoConfig);
                            r3 = conn->createResultset(s3, pooledConnection);
                            while (r3->next())
                                {
                                    struct message_bringonline msg;
                                    msg.url = r3->getString(1);
                                    msg.job_id = r3->getString(2);
                                    msg.file_id = r3->getInt(3);
                                    msg.pinlifetime = r3->getInt(4);
                                    msg.bringonlineTimeout = r3->getInt(5);
                                    ret.push_back(msg);
                                    bringOnlineReportStatus("STARTED", "", msg);
                                }
                            conn->destroyResultset(s3, r3);
                        }
                    conn->destroyStatement(s5, tag5, pooledConnection);
                    conn->destroyResultset(s1, r1);
                    conn->destroyStatement(s1, tag1, pooledConnection);
                    conn->destroyStatement(s3, tag3, pooledConnection);
                }
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);
            if(s4 && r4)
                conn->destroyResultset(s4, r4);
            if (s4)
                conn->destroyStatement(s4, tag4, pooledConnection);
            if(s5 && r5)
                conn->destroyResultset(s5, r5);
            if (s5)
                conn->destroyStatement(s5, tag5, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);
            if(s4 && r4)
                conn->destroyResultset(s4, r4);
            if (s4)
                conn->destroyStatement(s4, tag4, pooledConnection);
            if(s5 && r5)
                conn->destroyResultset(s5, r5);
            if (s5)
                conn->destroyStatement(s5, tag5, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}


void OracleAPI::bringOnlineReportStatus(const std::string & state, const std::string & message, struct message_bringonline msg)
{

    const std::string tag1 = "bringOnlineReportStatus1";
    const std::string tag2 = "bringOnlineReportStatus2";
    const std::string tag3 = "bringOnlineReportStatus3";
    const std::string tag4 = "bringOnlineReportStatus4";

    std::string query1 = " UPDATE t_file set STAGING_START=:1, transferhost=:2 where job_id=:3 and file_id=:4 and file_state='STAGING' ";
    std::string query2 = " UPDATE t_file set STAGING_FINISHED=:1, reason=:2, file_state=:3 where job_id=:4 and file_id=:5 and file_state='STAGING'";
    std::string query3 = " select reuse_job from t_job where job_id=:1 ";
    std::string query4 = " select count(*) from t_file where job_id=:1 and file_state='STAGING' ";

    SafeStatement s1;
    SafeStatement s2;

    SafeStatement s3;
    SafeResultSet r3;
    SafeStatement s4;
    SafeResultSet r4;

    time_t timed = time(NULL);
    std::string reuse("");
    int countTr = 0;

    SafeConnection pooledConnection;
    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s3 = conn->createStatement(query3, tag3, pooledConnection);
            s3->setString(1, msg.job_id);
            r3 = conn->createResultset(s3, pooledConnection);
            if (r3->next())
                {
                    reuse = r3->getString(1);
                }
            conn->destroyResultset(s3, r3);
            conn->destroyStatement(s3, tag3, pooledConnection);

            if(state=="STARTED")
                {
                    s1 = conn->createStatement(query1, tag1, pooledConnection);
                    s1->setTimestamp(1, conv->toTimestamp(timed, conn->getEnv()));
                    s1->setString(2, ftsHostName);
                    s1->setString(3, msg.job_id);
                    s1->setInt(4, msg.file_id);
                    s1->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s1, tag1, pooledConnection);
                    conn->releasePooledConnection(pooledConnection);
                }
            else if(state=="FINISHED")
                {
                    s2 = conn->createStatement(query2, tag2, pooledConnection);
                    s2->setTimestamp(1, conv->toTimestamp(timed, conn->getEnv()));
                    s2->setString(2, "");
                    s2->setString(3, "SUBMITTED");
                    s2->setString(4, msg.job_id);
                    s2->setInt(5, msg.file_id);
                    s2->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s2, tag2, pooledConnection);

                    if(reuse == "Y")
                        {
                            s4 = conn->createStatement(query4, tag4, pooledConnection);
                            s4->setString(1, msg.job_id);
                            r4 = conn->createResultset(s4, pooledConnection);
                            if (r4->next())
                                {
                                    countTr = r4->getInt(1);
                                }
                            conn->destroyResultset(s4, r4);
                            conn->destroyStatement(s4, tag4, pooledConnection);

                            if(countTr == 0)
                                {
                                    conn->releasePooledConnection(pooledConnection);
                                    updateJobTransferStatus(0, msg.job_id, "SUBMITTED");
                                }
                            else
                                {
                                    conn->releasePooledConnection(pooledConnection);
                                }
                        }
                    else
                        {
                            conn->releasePooledConnection(pooledConnection);
                            updateJobTransferStatus(0, msg.job_id, "SUBMITTED");
                        }
                }
            else if(state=="FAILED")
                {
                    s2 = conn->createStatement(query2, tag2, pooledConnection);
                    s2->setTimestamp(1, conv->toTimestamp(timed, conn->getEnv()));
                    s2->setString(2, message);
                    s2->setString(3, "FAILED");
                    s2->setString(4, msg.job_id);
                    s2->setInt(5, msg.file_id);
                    s2->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s2, tag2, pooledConnection);

                    if(reuse == "Y")
                        {
                            s4 = conn->createStatement(query4, tag4, pooledConnection);
                            s4->setString(1, msg.job_id);
                            r4 = conn->createResultset(s4, pooledConnection);
                            if (r4->next())
                                {
                                    countTr = r4->getInt(1);
                                }
                            conn->destroyResultset(s4, r4);
                            conn->destroyStatement(s4, tag4, pooledConnection);

                            if(countTr == 0)
                                {
                                    conn->releasePooledConnection(pooledConnection);
                                    updateJobTransferStatus(0, msg.job_id, "FAILED");
                                }
                            else
                                {
                                    conn->releasePooledConnection(pooledConnection);
                                }
                        }
                    else
                        {
                            conn->releasePooledConnection(pooledConnection);
                            updateJobTransferStatus(0, msg.job_id, "FAILED");
                        }

                }
            else
                {
                    conn->releasePooledConnection(pooledConnection);
                }

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s1)
                conn->destroyStatement(s1, tag1, pooledConnection);
            if(s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            if(s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            if(s4 && r4)
                conn->destroyResultset(s4, r4);
            if (s4)
                conn->destroyStatement(s4, tag4, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
            conn->releasePooledConnection(pooledConnection);
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s1)
                conn->destroyStatement(s1, tag1, pooledConnection);
            if(s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            if(s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            if(s4 && r4)
                conn->destroyResultset(s4, r4);
            if (s4)
                conn->destroyStatement(s4, tag4, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
            conn->releasePooledConnection(pooledConnection);
        }

}


void OracleAPI::addToken(const std::string & job_id, int file_id, const std::string & token)
{
    const std::string tag = "addToken";
    std::string query = " UPDATE t_file set bringonline_token=:1 where job_id=:2 and file_id=:3 and file_state='STAGING' ";

    SafeStatement s;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, token);
            s->setString(2, job_id);
            s->setInt(3, file_id);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::getCredentials(std::string & vo_name, const std::string & job_id, int, std::string & dn, std::string & dlg_id)
{
    std::string tag = "getCredentials";
    std::string query = "select USER_DN, CRED_ID, VO_NAME  from t_job where job_id=:1";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, job_id);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    dn = r->getString(1);
                    dlg_id = r->getString(2);
                    vo_name = r->getString(3);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::setMaxStageOp(const std::string& se, const std::string& vo, int val)
{

    std::string selectTag = "setMaxStageOp";
    std::string updateTag = "setMaxStageOpUpdate";
    std::string insertTag = "setMaxStageOpInsert";

    std::string select =
        " SELECT COUNT(*) "
        " FROM t_stage_req "
        " WHERE vo_name = :1 AND host = :2 "
        ;

    SafeStatement s1, s2, s3;
    SafeResultSet r1;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            s1 = conn->createStatement(select, selectTag, pooledConnection);
            s1->setString(1, vo);
            s1->setString(2, se);
            r1 = conn->createResultset(s1, pooledConnection);

            int exist = 0;

            if (r1->next())
                {
                    exist = r1->getInt(1);
                }

            conn->destroyResultset(s1, r1);
            conn->destroyStatement(s1, selectTag, pooledConnection);

            // if the record already exist ...
            if (exist)
                {
                    // update
                    std::string update =
                        " UPDATE t_stage_req "
                        " SET concurrent_ops = :1 "
                        " WHERE vo_name = :2 AND host = :3 "
                        ;

                    s2 = conn->createStatement(update, updateTag, pooledConnection);
                    s2->setInt(1, val);
                    s2->setString(2, vo);
                    s2->setString(3, se);
                    s2->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s2, updateTag, pooledConnection);

                }
            else
                {
                    // otherwise insert
                    std::string insert =
                        " INSERT "
                        " INTO t_stage_req (host, vo_name, concurrent_ops) "
                        " VALUES (:1, :2, :3)"
                        ;

                    s3 = conn->createStatement(insert, insertTag, pooledConnection);
                    s3->setString(1, se);
                    s3->setString(2, vo);
                    s3->setInt(3, val);
                    s3->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s3, insertTag, pooledConnection);
                }

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, selectTag, pooledConnection);
            if (s2)
                conn->destroyStatement(s2, updateTag, pooledConnection);
            if (s3)
                conn->destroyStatement(s3, insertTag, pooledConnection);

            throw Err_Custom(e.what());

        }
    catch (...)
        {


            conn->rollback(pooledConnection);
            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, selectTag, pooledConnection);
            if (s2)
                conn->destroyStatement(s2, updateTag, pooledConnection);
            if (s3)
                conn->destroyStatement(s3, insertTag, pooledConnection);

            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

double OracleAPI::getSuccessRate(std::string source, std::string destination)
{
    const std::string tag = "getSuccessRate";
    std::string query =
        "SELECT file_state "
        "FROM t_file "
        "WHERE "
        "      t_file.source_se = :1 AND t_file.dest_se = :2 AND "
        "      file_state IN ('FAILED','FINISHED') AND "
        "      (t_file.FINISH_TIME > (SYS_EXTRACT_UTC(SYSTIMESTAMP) - interval '1' hour))"
        ;

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    double nFailedLastHour = 0, nFinishedLastHour = 0;
    double ratioSuccessFailure = 0;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return ratioSuccessFailure;

            s = conn->createStatement(query, tag, pooledConnection);

            // file state: FAILED and FINISHED (in last hour)
            s->setString(1, source);
            s->setString(2, destination);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {
                    std::string state = r->getString(1);
                    if      (state.compare("FAILED") == 0)   ++nFailedLastHour;
                    else if (state.compare("FINISHED") == 0) ++nFinishedLastHour;
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

            if(nFinishedLastHour > 0)
                {
                    ratioSuccessFailure = nFinishedLastHour/(nFinishedLastHour + nFailedLastHour) * (100.0/1.0);
                }
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch(...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);

    return ratioSuccessFailure;
}

double OracleAPI::getAvgThroughput(std::string source, std::string destination)
{
    const std::string tag = "getAvgThroughput";
    std::string query =
        " select ROUND(AVG(throughput),2) AS Average  from t_file where source_se = :1 and dest_se = :2 "
        " and file_state = 'FINISHED' and throughput is not NULL and throughput != 0 "
        " and (t_file.FINISH_TIME > (SYS_EXTRACT_UTC(SYSTIMESTAMP) - interval '1' hour))"
        ;


    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    double avgThr = 0;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return avgThr;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, source);
            s->setString(2, destination);
            r = conn->createResultset(s, pooledConnection);
            if (r->next())
                {
                    avgThr = r->getDouble(1);
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch(...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);

    return avgThr;
}


void OracleAPI::updateProtocol(const std::string& jobId, int fileId, int nostreams, int timeout, int buffersize, double filesize)
{
    const std::string tag = "updateProtocol1";
    std::string query = " UPDATE t_file set INTERNAL_FILE_PARAMS=:1, FILESIZE=:2 where job_id=:3 and file_id=:4 ";

    SafeStatement s;
    SafeConnection pooledConnection;
    std::stringstream internalParams;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            internalParams << "nostreams:" << nostreams << ",timeout:" << timeout << ",buffersize:" << buffersize;
            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, internalParams.str());
            s->setDouble(2, filesize);
            s->setString(3, jobId);
            s->setInt(4, fileId);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}


void OracleAPI::cancelFilesInTheQueue(const std::string& se, const std::string& vo, std::set<std::string>& jobs)
{

    std::string tag1 = "cancelJobsInTheQueue11";
    std::string tag2 = "cancelJobsInTheQueue12";
    std::string tag3 = "cancelJobsInTheQueue13";
    std::string tag4 = "cancelJobsInTheQueue14";

    std::string query1 =
        " SELECT file_id, job_id, file_index "
        " FROM t_file "
        " WHERE (source_se = :1 OR dest_se = :2) "
        "	AND file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
        ;
    std::string query2 =
        " SELECT f.file_id, f.job_id, f.file_index "
        " FROM t_file f, t_job j "
        " WHERE (f.source_se = :1 OR f.dest_se = :2) "
        "	AND j.vo_name = :3 "
        " 	AND f.file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
        "	AND f.job_id = j.job_id "
        ;

    std::string query3 =
        " UPDATE t_file "
        " SET file_state = 'CANCELED' "
        " WHERE file_id = :1"
        ;

    std::string query4 =
        " UPDATE t_file "
        " SET file_state = 'SUBMITTED' "
        " WHERE file_state = 'NOT_USED' "
        "	AND job_id = :1 "
        "	AND file_index = :2 "
        "	AND NOT EXISTS ( "
        "		SELECT NULL "
        "		FROM t_file "
        "		WHERE job_id = :3 "
        "			AND file_index = :4 "
        "			AND file_state <> 'NOT_USED' "
        "			AND file_state <> 'FAILED' "
        "			AND file_state <> 'CANCELED' "
        "	) "
        ;

    SafeStatement s1;
    SafeResultSet r1;
    SafeStatement s2;
    SafeResultSet r2;
    SafeStatement s3;
    SafeStatement s4;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            if (vo.empty())
                {

                    s1 = conn->createStatement(query1, tag1, pooledConnection);
                    s1->setString(1, se);
                    s1->setString(2, se);
                    r1 = conn->createResultset(s1, pooledConnection);

                    while (r1->next())
                        {

                            int fileId = r1->getInt(1);
                            std::string jobId = r1->getString(2);
                            int fileIndex = r1->getInt(3);

                            jobs.insert(
                                jobId
                            );

                            s3 = conn->createStatement(query3, tag3, pooledConnection);
                            s3->setInt(1, fileId);
                            s3->executeUpdate();
                            conn->destroyStatement(s3, tag3, pooledConnection);

                            s4 = conn->createStatement(query4, tag4, pooledConnection);
                            s4->setString(1, jobId);
                            s4->setInt(2, fileIndex);
                            s4->setString(3, jobId);
                            s4->setInt(4, fileIndex);
                            s4->executeUpdate();
                            conn->destroyStatement(s4, tag4, pooledConnection);
                        }

                    conn->commit(pooledConnection);
                    conn->destroyResultset(s1, r1);
                    conn->destroyStatement(s1, tag1, pooledConnection);

                }
            else
                {

                    s2 = conn->createStatement(query2, tag2, pooledConnection);
                    s2->setString(1, se);
                    s2->setString(2, se);
                    s2->setString(3, vo);
                    r2 = conn->createResultset(s2, pooledConnection);

                    while (r2->next())
                        {

                            int fileId = r2->getInt(1);
                            std::string jobId = r2->getString(2);
                            int fileIndex = r2->getInt(3);

                            jobs.insert(
                                jobId
                            );

                            s3 = conn->createStatement(query3, tag3, pooledConnection);
                            s3->setInt(1, fileId);
                            s3->executeUpdate();
                            conn->destroyStatement(s3, tag3, pooledConnection);

                            s4 = conn->createStatement(query4, tag4, pooledConnection);
                            s4->setString(1, jobId);
                            s4->setInt(2, fileIndex);
                            s4->setString(3, jobId);
                            s4->setInt(4, fileIndex);
                            s4->executeUpdate();
                            conn->destroyStatement(s4, tag4, pooledConnection);
                        }

                    conn->commit(pooledConnection);
                    conn->destroyResultset(s2, r2);
                    conn->destroyStatement(s2, tag2, pooledConnection);
                }

            conn->releasePooledConnection(pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if(s2 && r2)
                conn->destroyResultset(s2, r2);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            if (s4)
                conn->destroyStatement(s3, tag3, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if(s2 && r2)
                conn->destroyResultset(s2, r2);
            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            if (s3)
                conn->destroyStatement(s3, tag3, pooledConnection);

            if (s4)
                conn->destroyStatement(s3, tag3, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

    std::set<std::string>::iterator job_it;
    for (job_it = jobs.begin(); job_it != jobs.end(); job_it++)
        {
            updateJobTransferStatus(int(), *job_it, string());
        }
}

void OracleAPI::cancelJobsInTheQueue(const std::string& dn, std::vector<std::string>& jobs)
{

    std::string tag = "cancelJobsInTheQueue2";
    std::string query =
        " SELECT job_id "
        " FROM t_job "
        " WHERE user_dn = :dn "
        "	AND job_state IN ('ACTIVE', 'READY', 'SUBMITTED')"
        ;


    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, dn);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {
                    jobs.push_back(
                        r->getString(1)
                    );
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            cancelJob(jobs);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

}


void OracleAPI::transferLogFile(const std::string& filePath, const std::string& jobId, int fileId, bool debug)
{
    std::string tag = "transferLogFile";
    std::string query = "update t_file set T_LOG_FILE=:1, T_LOG_FILE_DEBUG=:2 where job_id=:3 and file_id=:4";
    SafeStatement s;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, filePath);
            s->setInt(2, debug);
            s->setString(3, jobId);
            s->setInt(4, fileId);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}


struct message_state OracleAPI::getStateOfTransfer(const std::string& jobId, int fileId)
{
    std::string tag = "getStateOfTransfer";
    std::string tag1 = "getStateOfTransfer1";

    std::string query =
        " select t_job.job_id, t_job.job_state, t_job.vo_name, t_job.job_metadata, t_job.retry, t_file.file_id, t_file.file_state, t_file.retry, "
        " t_file.file_metadata, t_file.source_se, t_file.dest_se "
        " from t_file, t_job "
        " where t_job.job_id = t_file.job_id "
        " and t_job.job_id=:1 and t_file.file_id=:2 ";

    std::string query1 = "select retry from t_server_config";


    SafeStatement s;
    SafeResultSet r;
    SafeStatement s1;
    SafeResultSet r1;
    SafeConnection pooledConnection;
    struct message_state ret;

    try
        {

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return ret;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, jobId);
            s->setInt(2, fileId);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    ret.job_id = r->getString(1);
                    ret.job_state = r->getString(2);
                    ret.vo_name = r->getString(3);
                    ret.job_metadata = r->getString(4);
                    ret.retry_max = r->getInt(5);
                    ret.file_id = r->getInt(6);
                    ret.file_state = r->getString(7);
                    ret.retry_counter = r->getInt(8);
                    ret.file_metadata = r->getString(9);
                    ret.source_se = r->getString(10);
                    ret.dest_se = r->getString(11);
                    ret.timestamp = _getTrTimestampUTC();
                }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

            if(ret.retry_max == 0)
                {
                    s1 = conn->createStatement(query1, tag1, pooledConnection);
                    r1 = conn->createResultset(s1, pooledConnection);

                    if (r1->next())
                        {
                            ret.retry_max = r1->getInt(1);
                        }
                    conn->destroyResultset(s1, r1);
                    conn->destroyStatement(s1, tag1, pooledConnection);
                }

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
    return ret;
}


void OracleAPI::getFilesForJob(const std::string& jobId, std::vector<int>& files)
{
    std::string tag = "getFilesForJob";
    std::string query = " select file_id from t_file where job_id=:1 ";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, jobId);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {
                    files.push_back(r->getInt(1));
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {


            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}


void OracleAPI::getFilesForJobInCancelState(const std::string& jobId, std::vector<int>& files)
{
    std::string tag = "getFilesForJobInCancelState";
    std::string query = " select file_id from t_file where job_id=:1 and file_state='CANCELED' ";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, jobId);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {
                    files.push_back(r->getInt(1));
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {


            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::setFilesToWaiting(const std::string& se, const std::string& vo, int timeout)
{

    std::string tag1 = "setFilesToWaiting";
    std::string tag2 = "setFilesToWaitingWithVo";

    std::string query1 =
        " UPDATE t_file "
        " SET wait_timestamp = :1, wait_timeout = :2 "
        " WHERE (source_se = :3 OR dest_se = :4) "
        "	AND file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
        "	AND (wait_timestamp IS NULL OR wait_timeout IS NULL) "
        ;

    std::string query2 =
        " UPDATE t_file f "
        " SET f.wait_timestamp = :1, f.wait_timeout = :2 "
        " WHERE (f.source_se = :3 OR f.dest_se = :4) "
        "	AND f.file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
        "	AND (f.wait_timestamp IS NULL OR f.wait_timeout IS NULL) "
        "	AND EXISTS ( "
        "		SELECT NULL "
        "		FROM t_job j "
        "		WHERE j.job_id = f.job_id "
        "			AND j.vo_name = :5 "
        "	) "
        ;

    SafeStatement s1;
    SafeStatement s2;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            time_t timestamp = time(0);

            if (vo.empty())
                {

                    s1 = conn->createStatement(query1, tag1, pooledConnection);
                    s1->setTimestamp(1, conv->toTimestamp(timestamp, conn->getEnv()));
                    s1->setInt(2, timeout);
                    s1->setString(3, se);
                    s1->setString(4, se);
                    s1->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s1, tag1, pooledConnection);

                }
            else
                {

                    s2 = conn->createStatement(query2, tag2, pooledConnection);
                    s2->setTimestamp(1, conv->toTimestamp(timestamp, conn->getEnv()));
                    s2->setInt(2, timeout);
                    s2->setString(3, se);
                    s2->setString(4, se);
                    s2->setString(5, vo);
                    s2->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s2, tag2, pooledConnection);
                }


        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s1)
                conn->destroyStatement(s1, tag1, pooledConnection);

            if (s2)
                conn->destroyStatement(s2, tag2, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);

}

void OracleAPI::setFilesToWaiting(const std::string& dn, int timeout)
{

    std::string tag = "setFilesToWaitingDn";

    std::string query =
        " UPDATE t_file f "
        " SET f.wait_timestamp = :1, f.wait_timeout = :2 "
        " WHERE f.file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
        "	AND (f.wait_timestamp IS NULL OR f.wait_timeout IS NULL) "
        "	AND EXISTS( "
        "		SELECT NULL "
        "		FROM t_job j "
        "		WHERE j.job_id = f.job_id "
        "			AND j.user_dn = :3 "
        "	)"
        ;

    SafeStatement s;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            time_t timestamp = time(0);

            s = conn->createStatement(query, tag, pooledConnection);
            s->setTimestamp(1, conv->toTimestamp(timestamp, conn->getEnv()));
            s->setInt(2, timeout);
            s->setString(3, dn);
            s->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::cancelWaitingFiles(std::set<std::string>& jobs)
{

    std::string tag = "cancelWaitingFiles";
    std::string query =
        " SELECT file_id, job_id "
        " FROM t_file "
        " WHERE wait_timeout <> 0 "
        "	AND (CAST(CURRENT_TIMESTAMP(3) AS DATE) - CAST(wait_timestamp AS DATE)) * 86400 > wait_timeout "
        "	AND file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
        ;

    std::string tag2 = "cancelWaitingFilesUpdate";
    std::string update =
        " UPDATE t_file "
        " SET file_state = 'CANCELED' "
        " WHERE file_id = :1 "
        ;

    SafeStatement s;
    SafeResultSet r;
    SafeStatement s2;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            struct message_sanity msg;
            msg.cancelWaitingFiles = true;
            CleanUpSanityChecks temp(this, pooledConnection, msg);
            if(!temp.getCleanUpSanityCheck())
                {
                    conn->releasePooledConnection(pooledConnection);
                    return;
                }

            s = conn->createStatement(query, tag, pooledConnection);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {

                    jobs.insert(r->getString(2));

                    s2 = conn->createStatement(update, tag2, pooledConnection);
                    s2->setInt(1, r->getInt(1));
                    s2->executeUpdate();
                }

            conn->commit(pooledConnection);

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
            conn->destroyStatement(s2, tag2, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            if (s2)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            if (s2)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);

    std::set<std::string>::iterator job_it;
    for (job_it = jobs.begin(); job_it != jobs.end(); job_it++)
        {
            updateJobTransferStatus(int(), *job_it, string());
        }
}

void OracleAPI::revertNotUsedFiles()
{
    std::string tag = "revertNotUsedFiles";
    std::string update =
        " UPDATE t_file f1 "
        " SET file_state = 'SUBMITTED' "
        " WHERE file_state = 'NOT_USED' "
        "	AND NOT EXISTS ( "
        "		SELECT NULL "
        "		FROM t_file f2 "
        "		WHERE f2.job_id = f1.job_id "
        "			and f2.file_index = f1.file_index "
        "			and f2.file_state <> 'NOT_USED' "
        "			and f2.file_state <> 'CANCELED' "
        "			and f2.file_state <> 'FAILED' "
        "	) "
        ;

    SafeStatement s;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            struct message_sanity msg;
            msg.revertNotUsedFiles = true;
            CleanUpSanityChecks temp(this, pooledConnection, msg);
            if(!temp.getCleanUpSanityCheck())
                {
                    conn->releasePooledConnection(pooledConnection);
                    return;
                }

            s = conn->createStatement(update, tag, pooledConnection);
            s->executeUpdate();

            conn->commit(pooledConnection);

            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::checkSanityState()
{
    std::string tag[] = {"sanity1", "sanity2", "sanity3", "sanity4", "sanity5", "sanity6", "sanity7", "sanity8", "sanity9", "sanity10", "sanity11", "sanity12","sanity13","sanity14"};
    SafeStatement s[9];
    SafeResultSet r[9];
    SafeConnection pooledConnection;
    std::string sql[] =
    {
        "SELECT job_id from t_job where job_state in ('ACTIVE','READY','SUBMITTED','STAGING') ",
        "SELECT COUNT(DISTINCT file_index) FROM t_file where job_id=:1 ",
        "UPDATE t_job SET job_state = 'CANCELED', job_finished = :1, finish_time = :2, reason = :3 WHERE job_id = :4 ",
        "UPDATE t_job SET job_state = 'FINISHED', job_finished = :1, finish_time = :2 WHERE job_id = :3",
        "UPDATE t_job SET job_state = 'FAILED', job_finished = :1, finish_time = :2, reason = :3 WHERE job_id = :4",
        "UPDATE t_job SET job_state = 'FINISHEDDIRTY', job_finished = :1, finish_time = :2, reason = :3 WHERE job_id = :4",
        "SELECT job_id from t_job where job_state in ('FINISHED','FAILED','FINISHEDDIRTY') ",
        "SELECT COUNT(*) FROM t_file where job_id=:1 AND file_state in ('ACTIVE','READY','SUBMITTED','STAGING')",
        "UPDATE t_job SET job_state = 'ACTIVE', job_finished = NULL, finish_time = NULL, reason = NULL WHERE job_id = :1"
    };

    std::vector<std::string> ret;
    std::vector<std::string> retRevert;
    unsigned int numberOfFiles = 0;
    unsigned int terminalState = 0;
    unsigned int allFinished = 0;
    unsigned int allFailed = 0;
    unsigned int allCanceled = 0;
    unsigned int numberOfFilesRevert = 0;
    std::string canceledMessage = "Transfer canceled by the user";
    std::string failed = "One or more files failed. Please have a look at the details for more information";
    time_t timed = time(NULL);

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            struct message_sanity msg;
            msg.checkSanityState = true;
            CleanUpSanityChecks temp(this, pooledConnection, msg);
            if(!temp.getCleanUpSanityCheck())
                {
                    conn->releasePooledConnection(pooledConnection);
                    return;
                }

            s[0] = conn->createStatement(sql[0], tag[0], pooledConnection);
            r[0] = conn->createResultset(s[0], pooledConnection);
            while (r[0]->next())
                {
                    ret.push_back(r[0]->getString(1));
                }
            conn->destroyResultset(s[0], r[0]);
            conn->destroyStatement(s[0], tag[0], pooledConnection);

            s[1] = conn->createStatement(sql[1], tag[1], pooledConnection);
            s[2] = conn->createStatement(sql[2], tag[2], pooledConnection);
            s[3] = conn->createStatement(sql[3], tag[3], pooledConnection);
            s[4] = conn->createStatement(sql[4], tag[4], pooledConnection);
            s[5] = conn->createStatement(sql[5], tag[5], pooledConnection);

            vector< std::string >::const_iterator it;
            for (it = ret.begin(); it != ret.end(); ++it)
                {

                    s[1]->setString(1, *it);
                    r[1] = conn->createResultset(s[1], pooledConnection);
                    while (r[1]->next())
                        {
                            numberOfFiles = r[1]->getInt(1);
                        }
                    conn->destroyResultset(s[1], r[1]);

                    if(numberOfFiles > 0)
                        {
                            countFileInTerminalStates(pooledConnection, *it, allFinished, allCanceled, allFailed);
                            terminalState = allFinished + allCanceled + allFailed;

                            if(numberOfFiles == terminalState)  /* all files terminal state but job in ('ACTIVE','READY','SUBMITTED','STAGING') */
                                {
                                    if(allCanceled > 0)
                                        {
                                            s[2]->setTimestamp(1, conv->toTimestamp(timed, conn->getEnv()));
                                            s[2]->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv()));
                                            s[2]->setString(3,canceledMessage);
                                            s[2]->setString(4, *it);
                                            s[2]->executeUpdate();
                                            conn->commit(pooledConnection);
                                        }
                                    else
                                        {
                                            if(numberOfFiles == allFinished)  /*all files finished*/
                                                {
                                                    s[3]->setTimestamp(1, conv->toTimestamp(timed, conn->getEnv()));
                                                    s[3]->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv()));
                                                    s[3]->setString(3, *it);
                                                    s[3]->executeUpdate();
                                                    conn->commit(pooledConnection);
                                                }
                                            else
                                                {
                                                    if(numberOfFiles == allFailed)
                                                        {
                                                            s[4]->setTimestamp(1, conv->toTimestamp(timed, conn->getEnv()));
                                                            s[4]->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv()));
                                                            s[4]->setString(3,canceledMessage);
                                                            s[4]->setString(4, *it);
                                                            s[4]->executeUpdate();
                                                            conn->commit(pooledConnection);
                                                        }
                                                    else
                                                        {
                                                            s[5]->setTimestamp(1, conv->toTimestamp(timed, conn->getEnv()));
                                                            s[5]->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv()));
                                                            s[5]->setString(3,failed);
                                                            s[5]->setString(4, *it);
                                                            s[5]->executeUpdate();
                                                            conn->commit(pooledConnection);
                                                        }
                                                }
                                        }
                                }

                        }
                    //reset
                    numberOfFiles = 0;
                }

            conn->destroyStatement(s[1], tag[1], pooledConnection);
            conn->destroyStatement(s[2], tag[2], pooledConnection);
            conn->destroyStatement(s[3], tag[3], pooledConnection);
            conn->destroyStatement(s[4], tag[4], pooledConnection);
            conn->destroyStatement(s[5], tag[5], pooledConnection);

            s[6] = conn->createStatement(sql[6], tag[6], pooledConnection);
            r[6] = conn->createResultset(s[6], pooledConnection);
            while (r[6]->next())
                {
                    retRevert.push_back(r[6]->getString(1));
                }
            conn->destroyResultset(s[6], r[6]);
            conn->destroyStatement(s[6], tag[6], pooledConnection);
            s[7] = conn->createStatement(sql[7], tag[7], pooledConnection);
            s[8] = conn->createStatement(sql[8], tag[8], pooledConnection);

            vector< std::string >::const_iterator itRevert;
            for (itRevert = retRevert.begin(); itRevert != retRevert.end(); ++itRevert)
                {
                    s[7]->setString(1, *itRevert);
                    r[7] = conn->createResultset(s[7], pooledConnection);
                    while (r[7]->next())
                        {
                            numberOfFilesRevert = r[7]->getInt(1);
                        }
                    conn->destroyResultset(s[7], r[7]);

                    if(numberOfFilesRevert > 0)
                        {
                            s[8]->setString(1, *itRevert);
                            s[8]->executeUpdate();
                            conn->commit(pooledConnection);
                        }

                    //reset
                    numberOfFilesRevert = 0;
                }

            conn->destroyStatement(s[7], tag[7], pooledConnection);
            conn->destroyStatement(s[8], tag[8], pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            for(int index=0; index<9; index++)
                {
                    if(r[index] && s[index])
                        conn->destroyResultset(s[index], r[index]);
                }
            for(int index2=0; index2<9; index2++)
                {
                    if (s[index2])
                        conn->destroyStatement(s[index2], tag[index2], pooledConnection);
                }
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);

            for(int index=0; index<9; index++)
                {
                    if(r[index] && s[index])
                        conn->destroyResultset(s[index], r[index]);
                }
            for(int index2=0; index2<9; index2++)
                {
                    if (s[index2])
                        conn->destroyStatement(s[index2], tag[index2], pooledConnection);
                }
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

    ret.clear();
    retRevert.clear();
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::countFileInTerminalStates(SafeConnection& pooledConnection, std::string jobId,
        unsigned int& finished, unsigned int& canceled, unsigned int& failed)
{
    if (!pooledConnection) return;

    std::string queryFinished =
        " select count(*)  "
        " from t_file "
        " where job_id = :1 "
        "	and  file_state = 'FINISHED' "
        ;

    std::string tagFinished = "countFileInTerminalStatesFinished";

    std::string queryFailed =
        " select count(distinct f1.file_index) "
        " from t_file f1 "
        " where job_id = :1 "
        "	and f1.file_state = 'FAILED' "
        "	and NOT EXISTS ( "
        "		select null "
        "		from t_file f2 "
        "		where job_id = :2 "
        "			and f2.file_index = f1.file_index "
        "			and f2.file_state NOT IN ('CANCELED', 'FAILED') "
        " 	) "
        ;

    std::string tagFailed = "countFileInTerminalStatesFailed";

    std::string queryCanceled =
        " select count(distinct f1.file_index) "
        " from t_file f1 "
        " where job_id = :1 "
        "	and NOT EXISTS ( "
        "		select null "
        "		from t_file f2 "
        "		where job_id = :2 "
        "			and f2.file_index = f1.file_index "
        "			and f2.file_state <> 'CANCELED' "
        " 	) "
        ;

    std::string tagCanceled = "countFileInTerminalStatesCanceled";

    SafeStatement s1;
    SafeResultSet r1;
    SafeStatement s2;
    SafeResultSet r2;
    SafeStatement s3;
    SafeResultSet r3;

    try
        {
            s1 = conn->createStatement(queryFinished, tagFinished, pooledConnection);
            s1->setString(1, jobId);
            r1 = conn->createResultset(s1, pooledConnection);

            if (r1->next())
                {
                    finished = r1->getInt(1);
                }

            conn->destroyResultset(s1, r1);
            conn->destroyStatement(s1, tagFinished, pooledConnection);


            s2 = conn->createStatement(queryFailed, tagFailed, pooledConnection);
            s2->setString(1, jobId);
            s2->setString(2, jobId);
            r2 = conn->createResultset(s2, pooledConnection);

            if (r2->next())
                {
                    failed = r2->getInt(1);
                }

            conn->destroyResultset(s2, r2);
            conn->destroyStatement(s2, tagFailed, pooledConnection);


            s3 = conn->createStatement(queryCanceled, tagCanceled, pooledConnection);
            s3->setString(1, jobId);
            s3->setString(2, jobId);
            r3 = conn->createResultset(s3, pooledConnection);

            if (r3->next())
                {
                    canceled = r3->getInt(1);
                }

            conn->destroyResultset(s3, r3);
            conn->destroyStatement(s3, tagCanceled, pooledConnection);


        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tagFinished, pooledConnection);

            if(s2 && r2)
                conn->destroyResultset(s2, r2);
            if (s2)
                conn->destroyStatement(s2, tagFailed, pooledConnection);

            if(s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tagCanceled, pooledConnection);

            throw Err_Custom(e.what());
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tagFinished, pooledConnection);

            if(s2 && r2)
                conn->destroyResultset(s2, r2);
            if (s2)
                conn->destroyStatement(s2, tagFailed, pooledConnection);

            if(s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tagCanceled, pooledConnection);

            throw Err_Custom("Unknown oracle exception");
        }
}

void OracleAPI::getFilesForNewSeCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out)
{
    std::string tag = "getFilesForNewSeCfg";
    std::string query =
        " select file_id "
        " from t_file, t_job "
        " where t_file.source_se like :1 "
        "	and t_file.dest_se like :2 "
        "	and t_file.job_id = t_job.job_id "
        "	and t_job.vo_name = :3 "
        "	and t_file.file_state in ('READY', 'ACTIVE') "
        ;

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, source == "*" ? "%" : source);
            s->setString(2, destination == "*" ? "%" : destination);
            s->setString(3, vo);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {
                    out.push_back(r->getInt(1));
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {


            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::getFilesForNewGrCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out)
{
    std::string tag = "getFilesForNewGrCfg";
    std::string query;
    query +=
        " select file_id "
        " from t_file, t_job "
        " where t_file.job_id = t_job.job_id "
        "	and t_job.vo_name = :1 "
        "	and t_file.file_state in ('READY', 'ACTIVE')";
    if (source != "*")
        {
            query +=
                "	and t_file.source_se in ( "
                "		select member "
                "		from t_group_members "
                "		where groupName = :2 "
                "	) ";
            tag += "1";
        }
    if (destination != "*")
        {
            query +=
                "	and t_file.dest_se in ( "
                "		select member "
                "		from t_group_members "
                "		where groupName = :3 "
                "	) ";
            tag += "2";
        }

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            s = conn->createStatement(query, tag, pooledConnection);

            int index = 1;

            s->setString(index++, vo);
            if (source != "*") s->setString(index++, source);
            if (destination != "*") s->setString(index, destination);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {
                    out.push_back(r->getInt(1));
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {


            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::delFileShareConfig(int file_id, std::string source, std::string destination, std::string vo)
{

    const std::string tag = "delFileShareConfig";
    std::string query =
        " delete from t_file_share_config  "
        " where file_id = :1 "
        "	and source = :2 "
        "	and destination = :3 "
        "	and vo = :4 "
        ;

    SafeStatement s;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, file_id);
            s->setString(2, source);
            s->setString(3, destination);
            s->setString(4, vo);
            if (s->executeUpdate() != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::delFileShareConfig(std::string group, std::string se)
{

    const std::string tag = "delFileShareConfig2";
    std::string query =
        " delete from t_file_share_config  "
        " where (source = :1 or destination = :2) "
        "	and file_id IN ( "
        "		select file_id "
        "		from t_file "
        "		where (source_se = :3 or dest_se = :4) "
        "			and file_state in ('READY', 'ACTIVE')"
        "	) "
        ;

    SafeStatement s;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setString(1, group);
            s->setString(2, group);
            s->setString(3, se);
            s->setString(4, se);
            if (s->executeUpdate() != 0)
                conn->commit(pooledConnection);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom(e.what());
        }
    catch (...)
        {

            conn->rollback(pooledConnection);
            if(s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);
            throw Err_Custom("Unknown exception");
        }
    conn->releasePooledConnection(pooledConnection);
}

bool OracleAPI::hasStandAloneSeCfgAssigned(int file_id, std::string vo)
{
    std::string tag = "hasStandAloneSeCfgAssigned";
    std::string query =
        " select count(*) "
        " from t_file_share_config fc "
        " where fc.file_id = :1 "
        "	and fc.vo = :2 "
        "	and ((fc.source <> '(*)' and fc.destination = '*') or (fc.source = '*' and fc.destination <> '(*)')) "
        "	and not exists ( "
        "		select null "
        "		from t_group_members g "
        "		where (g.groupName = fc.source or g.groupName = fc.destination) "
        "	) "
        ;

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    int count = 0;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return count > 0;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, file_id);
            s->setString(2, vo);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    count = r->getInt(1);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);

    return count > 0;
}

bool OracleAPI::hasPairSeCfgAssigned(int file_id, std::string vo)
{
    std::string tag = "hasPairSeCfgAssigned";
    std::string query =
        " select count(*) "
        " from t_file_share_config fc "
        " where fc.file_id = :1 "
        "	and fc.vo = :2 "
        "	and (fc.source <> '(*)' and fc.source <> '*' and fc.destination <> '*' and fc.destination <> '(*)') "
        "	and not exists ( "
        "		select null "
        "		from t_group_members g "
        "		where (g.groupName = fc.source or g.groupName = fc.destination) "
        "	) "
        ;

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    int count = 0;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return count > 0;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, file_id);
            s->setString(2, vo);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    count = r->getInt(1);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);

    return count > 0;
}

bool OracleAPI::hasStandAloneGrCfgAssigned(int file_id, std::string vo)
{
    std::string tag = "hasStandAloneGrCfgAssigned";
    std::string query =
        " select count(*) "
        " from t_file_share_config fc "
        " where fc.file_id = :1 "
        "	and fc.vo = :2 "
        "	and ((fc.source <> '(*)' and fc.destination = '*') or (fc.source = '*' and fc.destination <> '(*)')) "
        "	and exists ( "
        "		select null "
        "		from t_group_members g "
        "		where (g.groupName = fc.source or g.groupName = fc.destination) "
        "	) "
        ;

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    int count = 0;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return count > 0;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, file_id);
            s->setString(2, vo);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    count = r->getInt(1);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);

    return count > 0;
}

bool OracleAPI::hasPairGrCfgAssigned(int file_id, std::string vo)
{
    std::string tag = "hasPairGrCfgAssigned";
    std::string query =
        " select count(*) "
        " from t_file_share_config fc "
        " where fc.file_id = :1 "
        "	and fc.vo = :2 "
        "	and (fc.source <> '(*)' and fc.source <> '*' and fc.destination <> '*' and fc.destination <> '(*)') "
        "	and exists ( "
        "		select null "
        "		from t_group_members g "
        "		where (g.groupName = fc.source or g.groupName = fc.destination) "
        "	) "
        ;

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    int count = 0;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return count > 0;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, file_id);
            s->setString(2, vo);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    count = r->getInt(1);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);

    return count > 0;
}


void OracleAPI::checkSchemaLoaded()
{
    std::string tag = "checkSchemaLoaded";
    std::string query =
        " select count(*) "
        " from t_debug ";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    int count = 0;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            s = conn->createStatement(query, tag, pooledConnection);
            r = conn->createResultset(s, pooledConnection);

            if (r->next())
                {
                    count = r->getInt(1);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);

            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);

            throw Err_Custom(std::string(__func__) + ": Caught exception, check if db schema has been created");
        }
    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::storeProfiling(const fts3::ProfilingSubsystem* prof)
{
    std::string resetTag = "storeProfilingReset";
    std::string reset = "UPDATE t_profiling_snapshot SET "
                        "    cnt = 0, exceptions = 0, total = 0, average = 0";
    SafeStatement resetStmt;

    std::string updateTag = "storeProfilingUpdate";
    std::string update = "MERGE INTO t_profiling_snapshot p USING dual ON (scope = :1) "
                         "WHEN NOT MATCHED THEN INSERT (scope, cnt, exceptions, total, average)"
                         "      VALUES (:2, :3, :4, :5, :6)"
                         "WHEN MATCHED THEN UPDATE SET  "
                         "    cnt = :7, exceptions = :8, total = :9, average = :10";

    SafeStatement updateStmt;

    std::string beatTag = "storeProfilingBeat";
    std::string beat    = "UPDATE t_profiling_info SET "
                          "    updated = :1, period = :2";
    SafeStatement beatStmt;

    std::string insertBeatTag = "storeProfilingBeatInsert";
    std::string insertBeat = "INSERT INTO t_profiling_info (updated, period) "
                             "VALUES (:1, :2)";
    SafeStatement insertBeatStmt;


    SafeConnection pooledConn;

    try
        {
            pooledConn = conn->getPooledConnection();
            if (!pooledConn) return;

            // Reset values
            resetStmt = conn->createStatement(reset, resetTag, pooledConn);
            resetStmt->executeUpdate();
            conn->destroyStatement(resetStmt, resetTag, pooledConn);

            // Update values
            updateStmt = conn->createStatement(update, updateTag, pooledConn);

            std::map<std::string, fts3::Profile>::const_iterator i;
            for (i = prof->profiles.begin(); i != prof->profiles.end(); ++i)
                {
                    updateStmt->setString(1, i->first);
                    updateStmt->setString(2, i->first);
                    updateStmt->setInt(3, static_cast<int>(i->second.nCalled));
                    updateStmt->setInt(4, static_cast<int>(i->second.nExceptions));
                    updateStmt->setNumber(5, i->second.totalTime);
                    updateStmt->setNumber(6, i->second.getAverage());
                    updateStmt->setInt(7, static_cast<int>(i->second.nCalled));
                    updateStmt->setInt(8, static_cast<int>(i->second.nExceptions));
                    updateStmt->setNumber(9, i->second.totalTime);
                    updateStmt->setNumber(10, i->second.getAverage());
                    updateStmt->executeUpdate();
                }

            conn->destroyStatement(updateStmt, updateTag, pooledConn);

            // New timestamp
            beatStmt = conn->createStatement(beat, beatTag, pooledConn);
            beatStmt->setTimestamp(1, conv->toTimestamp(time(NULL), conn->getEnv()));
            beatStmt->setInt(2, prof->getInterval());
            if (beatStmt->executeUpdate() == 0)
                {
                    insertBeatStmt = conn->createStatement(insertBeat, insertBeatTag, pooledConn);
                    insertBeatStmt->setTimestamp(1, conv->toTimestamp(time(NULL), conn->getEnv()));
                    insertBeatStmt->setInt(2, prof->getInterval());
                    insertBeatStmt->executeUpdate();
                    conn->destroyStatement(insertBeatStmt, insertBeatTag, pooledConn);
                }
            conn->destroyStatement(beatStmt, beatTag, pooledConn);
            conn->commit(pooledConn);
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConn);
            if (resetStmt)
                conn->destroyStatement(resetStmt, resetTag, pooledConn);
            if (beatStmt)
                conn->destroyStatement(beatStmt, beatTag, pooledConn);
            if (insertBeatStmt)
                conn->destroyStatement(insertBeatStmt, insertBeatTag, pooledConn);
            if (updateStmt)
                conn->destroyStatement(updateStmt, updateTag, pooledConn);

            conn->releasePooledConnection(pooledConn);

            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    conn->releasePooledConnection(pooledConn);
}


void OracleAPI::setOptimizerMode(int mode)
{
    std::string select = "select count(*) from t_optimize_mode";
    std::string insert = "insert into t_optimize_mode(mode_opt) values(:1)";
    std::string update = "update t_optimize_mode set mode_opt=:1";
    std::string selectTag = "setOptimizerMode1";
    std::string insertTag = "setOptimizerMode2";
    std::string updateTag = "setOptimizerMode3";

    SafeConnection pooledConnection;
    SafeStatement s1;
    SafeResultSet r1;
    SafeStatement s2;
    SafeStatement s3;

    int _mode = 0;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            s1 = conn->createStatement(select, selectTag, pooledConnection);
            r1 = conn->createResultset(s1, pooledConnection);

            if (r1->next())
                {
                    _mode = r1->getInt(1);
                }

            conn->destroyResultset(s1, r1);
            conn->destroyStatement(s1, selectTag, pooledConnection);

            if(_mode == 0)  //issue insert
                {
                    s3 = conn->createStatement(insert, insertTag, pooledConnection);
                    s3->setInt(1, mode);
                    s3->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s3, insertTag, pooledConnection);
                }
            else   //update
                {
                    s2 = conn->createStatement(update, updateTag, pooledConnection);
                    s2->setInt(1, mode);
                    s2->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(s2, updateTag, pooledConnection);
                }
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, selectTag, pooledConnection);

            if (s2)
                conn->destroyStatement(s1, updateTag, pooledConnection);

            if (s3)
                conn->destroyStatement(s1, insertTag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);

            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, selectTag, pooledConnection);

            if (s2)
                conn->destroyStatement(s1, updateTag, pooledConnection);

            if (s3)
                conn->destroyStatement(s1, insertTag, pooledConnection);

            conn->releasePooledConnection(pooledConnection);

            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
    conn->releasePooledConnection(pooledConnection);

}

int OracleAPI::getOptimizerMode()
{
    std::string tag = "getOptimizerMode";
    std::string query = "select mode_opt from t_optimize_mode";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    int mode = 0;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return mode;

            s = conn->createStatement(query, tag, pooledConnection);
            r = conn->createResultset(s, pooledConnection);

            if (r->next() && !r->isNull(1))
                {
                    mode = r->getInt(1);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);

    return mode;
}



void OracleAPI::setRetryTransfer(const std::string & jobId, int fileId, int retry, const std::string& reason)
{
    const std::string tagsetRetryTimes = "setRetryTimes";
    const std::string querysetRetryTimes =
        "UPDATE t_file "
        "SET retry =:1 where job_id=:2 and file_id=:3 ";

    const std::string tagLogError = "setRetryTimesLog";
    const std::string queryLogError =
        "INSERT INTO t_file_retry_errors "
        "    (file_id, attempt, datetime, reason) "
        "VALUES (:1, :2, :3, :4)";

    SafeStatement ssetRetryTimes;
    SafeStatement sLogError;
    SafeConnection pooledConnection;


    try
        {
            time_t now = time(NULL);

            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection)
                return;

            ssetRetryTimes = conn->createStatement(querysetRetryTimes, tagsetRetryTimes, pooledConnection);
            ssetRetryTimes->setInt(1, retry);
            ssetRetryTimes->setString(2,jobId);
            ssetRetryTimes->setInt(3,fileId);
            ssetRetryTimes->executeUpdate();
            conn->destroyStatement(ssetRetryTimes, tagsetRetryTimes, pooledConnection);

            sLogError = conn->createStatement(queryLogError, tagLogError, pooledConnection);
            sLogError->setInt(1, fileId);
            sLogError->setInt(2, retry);
            sLogError->setTimestamp(3, conv->toTimestamp(now, conn->getEnv()));
            sLogError->setString(4, reason);
            sLogError->executeUpdate();
            conn->destroyStatement(sLogError, tagLogError, pooledConnection);

            conn->commit(pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);

            if(ssetRetryTimes)
                conn->destroyStatement(ssetRetryTimes, tagsetRetryTimes, pooledConnection);

            if(sLogError)
                conn->destroyStatement(sLogError, tagLogError, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);

            if(ssetRetryTimes)
                conn->destroyStatement(ssetRetryTimes, tagsetRetryTimes, pooledConnection);

            if(sLogError)
                conn->destroyStatement(sLogError, tagLogError, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }

    const std::string tagsetRetryTransfer = "setRetryTransfer";
    const std::string tag1setRetryTransfer1 = "setRetryTransfer1";
    const std::string tag2setRetryTransfer2 = "setRetryTransfer2";

    std::string querysetRetryTransfer =
        " UPDATE t_file set file_state='SUBMITTED' "
        "  where job_id=:1 and file_id=:2 and file_state not in ('FAILED','CANCELED') ";

    std::string query1setRetryTransfer1= "select job_id from t_job where t_job.reuse_job='Y' and job_id=:1  and job_state not in ('FAILED','CANCELED') ";

    std::string query2setRetryTransfer2= "update t_job set job_state='ACTIVE' where job_id=:1  and job_state not in ('FAILED','CANCELED') ";

    SafeStatement ssetRetryTransfer;
    SafeStatement s2setRetryTransfer2;

    SafeStatement s1setRetryTransfer1;
    SafeResultSet rsetRetryTransfer1;

    try
        {
            s1setRetryTransfer1 = conn->createStatement(query1setRetryTransfer1, tag1setRetryTransfer1, pooledConnection);
            s1setRetryTransfer1->setString(1,jobId);
            rsetRetryTransfer1 = conn->createResultset(s1setRetryTransfer1, pooledConnection);

            if (rsetRetryTransfer1->next())
                {
                    s2setRetryTransfer2 = conn->createStatement(query2setRetryTransfer2, tag2setRetryTransfer2, pooledConnection);
                    s2setRetryTransfer2->setString(1, jobId);
                    s2setRetryTransfer2->executeUpdate();
                    conn->destroyStatement(s2setRetryTransfer2, tag2setRetryTransfer2, pooledConnection);
                }

            conn->destroyResultset(s1setRetryTransfer1, rsetRetryTransfer1);
            conn->destroyStatement(s1setRetryTransfer1, tag1setRetryTransfer1, pooledConnection);

            ssetRetryTransfer = conn->createStatement(querysetRetryTransfer, tagsetRetryTransfer, pooledConnection);
            ssetRetryTransfer->setString(1, jobId);
            ssetRetryTransfer->setInt(2, fileId);
            ssetRetryTransfer->executeUpdate();
            conn->commit(pooledConnection);
            conn->destroyStatement(ssetRetryTransfer, tagsetRetryTransfer, pooledConnection);

        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);

            if(ssetRetryTransfer)
                conn->destroyStatement(ssetRetryTransfer, tagsetRetryTransfer, pooledConnection);

            if(s1setRetryTransfer1 && rsetRetryTransfer1)
                conn->destroyResultset(s1setRetryTransfer1, rsetRetryTransfer1);

            if(s1setRetryTransfer1)
                conn->destroyStatement(s1setRetryTransfer1, tag1setRetryTransfer1, pooledConnection);

            if (s2setRetryTransfer2)
                conn->destroyStatement(s2setRetryTransfer2, tag2setRetryTransfer2, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);

            if(ssetRetryTransfer)
                conn->destroyStatement(ssetRetryTransfer, tagsetRetryTransfer, pooledConnection);

            if(s1setRetryTransfer1 && rsetRetryTransfer1)
                conn->destroyResultset(s1setRetryTransfer1, rsetRetryTransfer1);

            if(s1setRetryTransfer1)
                conn->destroyStatement(s1setRetryTransfer1, tag1setRetryTransfer1, pooledConnection);

            if (s2setRetryTransfer2)
                conn->destroyStatement(s2setRetryTransfer2, tag2setRetryTransfer2, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Oracle plug-in unknown exception"));
        }

    std::string tagsetRetryTimestamp = "setRetryTimestamp";
    std::string tagsetRetryTimestamp1 = "setRetryTimestamp1";
    std::string querysetRetryTimestamp = "select RETRY_DELAY from t_job where job_id=:1";
    std::string querysetRetryTimestamp1 = "update t_file set retry_timestamp=:1 where job_id=:2 and file_id=:3";

    SafeStatement setRetryTimestamp;
    SafeStatement ssetRetryTimestamp1;
    SafeResultSet retRetryTimestamp;
    //expressed in secs
    int retry_delay = 0;
    const int default_retry_delay = 7;

    try
        {
            setRetryTimestamp = conn->createStatement(querysetRetryTimestamp, tagsetRetryTimestamp, pooledConnection);
            setRetryTimestamp->setString(1, jobId);
            retRetryTimestamp = conn->createResultset(setRetryTimestamp, pooledConnection);

            if (retRetryTimestamp->next())
                {
                    retry_delay = retRetryTimestamp->getInt(1);
                }

            conn->destroyResultset(setRetryTimestamp, retRetryTimestamp);
            conn->destroyStatement(setRetryTimestamp, tagsetRetryTimestamp, pooledConnection);

            if(retry_delay > 0)
                {
                    time_t now = time(0);
                    time_t now_plus_seconds = now + retry_delay;
                    ssetRetryTimestamp1 = conn->createStatement(querysetRetryTimestamp1, tagsetRetryTimestamp1, pooledConnection);
                    ssetRetryTimestamp1->setTimestamp(1, conv->toTimestamp(now_plus_seconds, conn->getEnv()));
                    ssetRetryTimestamp1->setString(2, jobId);
                    ssetRetryTimestamp1->setInt(3, fileId);
                    ssetRetryTimestamp1->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(ssetRetryTimestamp1, tagsetRetryTimestamp1, pooledConnection);
                }
            else
                {
                    time_t now = time(0);
                    time_t now_plus_seconds = now + default_retry_delay;
                    ssetRetryTimestamp1 = conn->createStatement(querysetRetryTimestamp1, tagsetRetryTimestamp1, pooledConnection);
                    ssetRetryTimestamp1->setTimestamp(1, conv->toTimestamp(now_plus_seconds, conn->getEnv()));
                    ssetRetryTimestamp1->setString(2, jobId);
                    ssetRetryTimestamp1->setInt(3, fileId);
                    ssetRetryTimestamp1->executeUpdate();
                    conn->commit(pooledConnection);
                    conn->destroyStatement(ssetRetryTimestamp1, tagsetRetryTimestamp1, pooledConnection);
                }
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);

            if(setRetryTimestamp && retRetryTimestamp)
                conn->destroyResultset(setRetryTimestamp, retRetryTimestamp);
            if (setRetryTimestamp)
                conn->destroyStatement(setRetryTimestamp, tagsetRetryTimestamp, pooledConnection);

            if (ssetRetryTimestamp1)
                conn->destroyStatement(ssetRetryTimestamp1, tagsetRetryTimestamp1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {

            conn->rollback(pooledConnection);

            if(setRetryTimestamp && retRetryTimestamp)
                conn->destroyResultset(setRetryTimestamp, retRetryTimestamp);
            if (setRetryTimestamp)
                conn->destroyStatement(setRetryTimestamp, tagsetRetryTimestamp, pooledConnection);

            if (ssetRetryTimestamp1)
                conn->destroyStatement(ssetRetryTimestamp1, tagsetRetryTimestamp1, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

    conn->releasePooledConnection(pooledConnection);
}

void OracleAPI::getTransferRetries(int fileId, std::vector<FileRetry*>& retries)
{
    const std::string tag = "getTransferRetries";
    const std::string query = "SELECT attempt, datetime, reason from t_file_retry_errors WHERE file_id = :1";

    SafeStatement s;
    SafeResultSet r;
    SafeConnection pooledConnection;

    try
        {
            pooledConnection = conn->getPooledConnection();
            if (!pooledConnection) return;

            s = conn->createStatement(query, tag, pooledConnection);
            s->setInt(1, fileId);
            r = conn->createResultset(s, pooledConnection);

            while (r->next())
                {
                    FileRetry* retry = new FileRetry();

                    retry->fileId   = fileId;
                    retry->attempt  = r->getInt(1);
                    retry->datetime = conv->toTimeT(r->getTimestamp(2));
                    retry->reason   = r->getString(3);

                    retries.push_back(retry);
                }

            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag, pooledConnection);
        }
    catch (oracle::occi::SQLException const &e)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            conn->rollback(pooledConnection);
            if(s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag, pooledConnection);

            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    conn->releasePooledConnection(pooledConnection);
}


bool OracleAPI::assignSanityRuns(SafeConnection& pooled, struct message_sanity &msg)
{
    long long rows = 0;

    try
        {
            if(msg.checkSanityState)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set checkSanityState=1, t_checkSanityState = SYS_EXTRACT_UTC(SYSTIMESTAMP) "
                                             "where checkSanityState=0"
                                             " AND (t_checkSanityState < (SYS_EXTRACT_UTC(SYSTIMESTAMP) - INTERVAL '30' minute)) ",
                                             "assignSanityRuns/checkSanityState", pooled);
                    rows = stmt->executeUpdate();
                    msg.checkSanityState = (rows > 0? true: false);
                    conn->commit(pooled);
                    return msg.checkSanityState;
                }
            else if(msg.setToFailOldQueuedJobs)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set setToFailOldQueuedJobs=1, t_setToFailOldQueuedJobs = SYS_EXTRACT_UTC(SYSTIMESTAMP) "
                                             " where setToFailOldQueuedJobs=0"
                                             " AND (t_setToFailOldQueuedJobs < (SYS_EXTRACT_UTC(SYSTIMESTAMP) - INTERVAL '15' minute)) ",
                                             "assignSanityRuns/setToFailOldQueuedJobs",
                                             pooled);
                    rows = stmt->executeUpdate();
                    msg.setToFailOldQueuedJobs = (rows > 0? true: false);
                    conn->commit(pooled);
                    return msg.setToFailOldQueuedJobs;
                }
            else if(msg.forceFailTransfers)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set forceFailTransfers=1, t_forceFailTransfers = SYS_EXTRACT_UTC(SYSTIMESTAMP) "
                                             " where forceFailTransfers=0"
                                             " AND (t_forceFailTransfers < (SYS_EXTRACT_UTC(SYSTIMESTAMP) - INTERVAL '15' minute)) ",
                                             "assignSanityRuns/forceFailTransfers",
                                             pooled);
                    rows = stmt->executeUpdate();
                    msg.forceFailTransfers = (rows > 0? true: false);
                    conn->commit(pooled);
                    return msg.forceFailTransfers;
                }
            else if(msg.revertToSubmitted)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set revertToSubmitted=1, t_revertToSubmitted = SYS_EXTRACT_UTC(SYSTIMESTAMP) "
                                             " where revertToSubmitted=0"
                                             " AND (t_revertToSubmitted < (SYS_EXTRACT_UTC(SYSTIMESTAMP) - INTERVAL '15' minute)) ",
                                             "assignSanityRuns/revertToSubmitted",
                                             pooled);
                    rows = stmt->executeUpdate();
                    msg.revertToSubmitted = (rows > 0? true: false);
                    conn->commit(pooled);
                    return msg.revertToSubmitted;
                }
            else if(msg.cancelWaitingFiles)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set cancelWaitingFiles=1, t_cancelWaitingFiles = SYS_EXTRACT_UTC(SYSTIMESTAMP) "
                                             "  where cancelWaitingFiles=0"
                                             " AND (t_cancelWaitingFiles < (SYS_EXTRACT_UTC(SYSTIMESTAMP) - INTERVAL '15' minute)) ",
                                             "assignSanityRuns/cancelWaitingFiles",
                                             pooled);
                    rows = stmt->executeUpdate();
                    msg.cancelWaitingFiles = (rows > 0? true: false);
                    conn->commit(pooled);
                    return msg.cancelWaitingFiles;
                }
            else if(msg.revertNotUsedFiles)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set revertNotUsedFiles=1, t_revertNotUsedFiles = SYS_EXTRACT_UTC(SYSTIMESTAMP) "
                                             " where revertNotUsedFiles=0"
                                             " AND (t_revertNotUsedFiles < (SYS_EXTRACT_UTC(SYSTIMESTAMP) - INTERVAL '15' minute)) ",
                                             "assignSanityRuns/revertNotUsedFiles",
                                             pooled);
                    rows = stmt->executeUpdate();
                    msg.revertNotUsedFiles = (rows > 0? true: false);
                    conn->commit(pooled);
                    return msg.revertNotUsedFiles;
                }
            else if(msg.cleanUpRecords)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set cleanUpRecords=1, t_cleanUpRecords = SYS_EXTRACT_UTC(SYSTIMESTAMP) "
                                             " where cleanUpRecords=0"
                                             " AND (t_cleanUpRecords < (SYS_EXTRACT_UTC(SYSTIMESTAMP) - INTERVAL '3' day)) ",
                                             "assignSanityRuns/cleanUpRecords",
                                             pooled);
                    rows = stmt->executeUpdate();
                    msg.cleanUpRecords = (rows > 0? true: false);
                    conn->commit(pooled);
                    return msg.cleanUpRecords;
                }
            else if(msg.msgCron)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set msgcron=1, t_msgcron = SYS_EXTRACT_UTC(SYSTIMESTAMP) "
                                             " where msgcron=0"
                                             " AND (t_msgcron < (SYS_EXTRACT_UTC(SYSTIMESTAMP) - INTERVAL '1' day)) ",
                                             "assignSanityRuns/msgcron",
                                             pooled);
                    rows = stmt->executeUpdate();
                    msg.msgCron = (rows > 0? true: false);
                    conn->commit(pooled);
                    return msg.msgCron;
                }
        }
    catch (std::exception& e)
        {
            conn->rollback(pooled);
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return false;
}


void OracleAPI::resetSanityRuns(SafeConnection& pooled, struct message_sanity &msg)
{
    try
        {
            if(msg.checkSanityState)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set checkSanityState=0 where checkSanityState=1",
                                             "resetSanityRuns/checkSanityState",
                                             pooled);
                    stmt->executeUpdate();
                }
            else if(msg.setToFailOldQueuedJobs)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set setToFailOldQueuedJobs=0 where setToFailOldQueuedJobs=1",
                                             "resetSanityRuns/setToFailOldQueuedJobs",
                                             pooled);
                    stmt->executeUpdate();
                }
            else if(msg.forceFailTransfers)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set forceFailTransfers=0 where forceFailTransfers=1",
                                             "resetSanityRuns/forceFailTransfers",
                                             pooled);
                    stmt->executeUpdate();
                }
            else if(msg.revertToSubmitted)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set revertToSubmitted=0 where revertToSubmitted=1",
                                             "resetSanityRuns/revertToSubmitted",
                                             pooled);
                    stmt->executeUpdate();
                }
            else if(msg.cancelWaitingFiles)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set cancelWaitingFiles=0 where cancelWaitingFiles=1",
                                             "resetSanityRuns/cancelWaitingFiles",
                                             pooled);
                    stmt->executeUpdate();
                }
            else if(msg.revertNotUsedFiles)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set revertNotUsedFiles=0 where revertNotUsedFiles=1",
                                             "resetSanityRuns/revertNotUsedFiles",
                                             pooled);
                    stmt->executeUpdate();
                }
            else if(msg.cleanUpRecords)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set cleanUpRecords=0 where cleanUpRecords=1",
                                             "resetSanityRuns/cleanUpRecords",
                                             pooled);
                    stmt->executeUpdate();
                }
            else if(msg.msgCron)
                {
                    SafeStatement stmt = conn->createStatement(
                                             "update t_server_sanity set msgcron=0 where msgcron=1",
                                             "resetSanityRuns/msgcron",
                                             pooled);
                    stmt->executeUpdate();
                }
            conn->commit(pooled);
        }
    catch (std::exception& e)
        {
            conn->rollback(pooled);
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
}

std::vector<boost::tuple<std::string, std::string, std::string> > OracleAPI::distinctSrcDestVO()
{
    return std::vector<boost::tuple<std::string, std::string, std::string> >();
}

// the class factories

extern "C" GenericDbIfce* create()
{
    return new OracleAPI();
}

extern "C" void destroy(GenericDbIfce* p)
{
    if (p)
        delete p;
}

extern "C" MonitoringDbIfce* create_monitoring()
{
    return new OracleMonitoring();
}

extern "C" void destroy_monitoring(MonitoringDbIfce* p)
{
    if (p)
        delete p;
}
