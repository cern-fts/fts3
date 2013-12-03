/*
 *  Copyright notice:
 *  Copyright © Members of the EMI Collaboration, 2010.
 *
 *  See www.eu-emi.eu for details on the copyright holders
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>

#include <error.h>
#include <logger.h>
#include <oracle/soci-oracle.h>
#include <signal.h>
#include <sys/param.h>
#include <unistd.h>
#include "OracleAPI.h"
#include "OracleMonitoring.h"
#include "sociConversions.h"
#include "queue_updater.h"
#include "DbUtils.h"

using namespace FTS3_COMMON_NAMESPACE;
using namespace db;


bool OracleAPI::getChangedFile (std::string source, std::string dest, double rate, double thr, double& thrStored, double retry, double& retryStored)
{
    bool returnValue = false;

    if(filesMemStore.empty())
        {
            boost::tuple<std::string, std::string, double, double, double> record(source, dest, rate, thr, retry);
            filesMemStore.push_back(record);
        }
    else
        {
            bool found = false;
            std::vector< boost::tuple<std::string, std::string, double, double, double> >::iterator itFind;
            for (itFind = filesMemStore.begin(); itFind < filesMemStore.end(); ++itFind)
                {
                    boost::tuple<std::string, std::string, double, double, double>& tupleRecord = *itFind;
                    std::string sourceLocal = boost::get<0>(tupleRecord);
                    std::string destLocal = boost::get<1>(tupleRecord);
                    if(sourceLocal == source && destLocal == dest)
                        {
                            found = true;
                            break;
                        }
                }
            if (!found)
                {
                    boost::tuple<std::string, std::string, double, double, double> record(source, dest, rate, thr, retry);
                    filesMemStore.push_back(record);
                }

            std::vector< boost::tuple<std::string, std::string, double, double, double> >::iterator it =  filesMemStore.begin();
            while (it != filesMemStore.end())
                {
                    boost::tuple<std::string, std::string, double, double, double>& tupleRecord = *it;
                    std::string sourceLocal = boost::get<0>(tupleRecord);
                    std::string destLocal = boost::get<1>(tupleRecord);
                    double rateLocal = boost::get<2>(tupleRecord);
                    double thrLocal = boost::get<3>(tupleRecord);
                    double retryThr = boost::get<4>(tupleRecord);

                    if(sourceLocal == source && destLocal == dest)
                        {
                            retryStored = retryThr;
                            thrStored = thrLocal;
                            if(rateLocal != rate || thrLocal != thr || retry != retryThr)
                                {
                                    it = filesMemStore.erase(it);
                                    boost::tuple<std::string, std::string, double, double, double> record(source, dest, rate, thr, retry);
                                    filesMemStore.push_back(record);
                                    returnValue = true;
                                    break;

                                }
                            break;
                        }
                    else
                        {
                            ++it;
                        }
                }
        }

    return returnValue;
}


OracleAPI::OracleAPI(): poolSize(10), connectionPool(NULL), hostname(getFullHostname())
{
    // Pass
}



OracleAPI::~OracleAPI()
{
    if(connectionPool)
        delete connectionPool;
}



void OracleAPI::init(std::string username, std::string password, std::string connectString, int pooledConn)
{
    std::ostringstream connParams;
    std::string host, db, port;

    try
        {
            connectionPool = new soci::connection_pool(pooledConn);

            // Build connection string
            connParams << "user=" << username << " "
                       << "password=" << password << " "
                       << "service=\"" << connectString << '"';

            std::string connStr = connParams.str();

            // Connect
            poolSize = (size_t) pooledConn;

            for (size_t i = 0; i < poolSize; ++i)
                {
                    soci::session& sql = (*connectionPool).at(i);
                    sql.open(soci::oracle, connStr);
                }
        }
    catch (std::exception& e)
        {
            if(connectionPool)
                {
                    delete connectionPool;
                    connectionPool = NULL;
                }
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            if(connectionPool)
                {
                    delete connectionPool;
                    connectionPool = NULL;
                }
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}




TransferJobs* OracleAPI::getTransferJob(std::string jobId, bool archive)
{
    soci::session sql(*connectionPool);

    std::string query;
    if (archive)
        {
            query = "SELECT t_job_backup.vo_name, t_job_backup.user_dn "
                    "FROM t_job_backup WHERE t_job_backup.job_id = :jobId";
        }
    else
        {
            query = "SELECT t_job.vo_name, t_job.user_dn "
                    "FROM t_job WHERE t_job.job_id = :jobId";
        }

    TransferJobs* job = NULL;
    try
        {
            job = new TransferJobs();

            sql << query,
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
            if(job)
                delete job;
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            if(job)
                delete job;
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return job;
}


std::map<std::string, double> OracleAPI::getActivityShareConf(soci::session& sql, std::string vo)
{

    std::map<std::string, double> ret;

    soci::indicator isNull = soci::i_ok;
    std::string activity_share_str;
    try
        {

            sql <<
                " SELECT activity_share "
                " FROM t_activity_share_config "
                " WHERE vo = :vo "
                "   AND active = 'on'",
                soci::use(vo),
                soci::into(activity_share_str, isNull)
                ;

            if (isNull == soci::i_null || activity_share_str.empty()) return ret;

            // remove the opening '[' and closing ']'
            activity_share_str = activity_share_str.substr(1, activity_share_str.size() - 2);

            // iterate over activity shares
            boost::char_separator<char> sep(",");
            boost::tokenizer< boost::char_separator<char> > tokens(activity_share_str, sep);
            boost::tokenizer< boost::char_separator<char> >::iterator it;

            static const boost::regex re("^\\s*\\{\\s*\"([a-zA-Z0-9\\.-]+)\"\\s*:\\s*(0\\.\\d+)\\s*\\}\\s*$");
            static const int ACTIVITY_NAME = 1;
            static const int ACTIVITY_SHARE = 2;

            for (it = tokens.begin(); it != tokens.end(); it++)
                {
                    // parse single activity share
                    std::string str = *it;

                    boost::smatch what;
                    boost::regex_match(str, what, re, boost::match_extra);

                    ret[what[ACTIVITY_NAME]] = boost::lexical_cast<double>(what[ACTIVITY_SHARE]);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return ret;
}

std::vector<std::string> OracleAPI::getAllActivityShareConf()
{
    soci::session sql(*connectionPool);

    std::vector<std::string> ret;

    try
        {
            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT vo "
                                             " FROM t_activity_share_config "
                                         );

            soci::rowset<soci::row>::const_iterator it;
            for (it = rs.begin(); it != rs.end(); it++)
                {
                    ret.push_back(it->get<std::string>("vo"));
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ret;
}

std::map<std::string, long long> OracleAPI::getActivitiesInQueue(soci::session& sql, std::string src, std::string dst, std::string vo)
{
    std::map<std::string, long long> ret;

    time_t now = time(NULL);
    struct tm tTime;
    gmtime_r(&now, &tTime);

    try
        {
            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT activity, COUNT(DISTINCT f.job_id, file_index) AS count "
                                             " FROM t_job j, t_file f "
                                             " WHERE j.job_id = f.job_id AND j.vo_name = f.vo_name AND f.file_state = 'SUBMITTED' AND "
                                             "	f.source_se = :source AND f.dest_se = :dest AND "
                                             "	f.vo_name = :vo_name AND "
                                             "	f.wait_timestamp IS NULL AND "
                                             "	(f.retry_timestamp is NULL OR f.retry_timestamp < :tTime) AND "
                                             "	(f.hashed_id >= :hStart AND f.hashed_id <= :hEnd) AND "
                                             "  j.job_state in ('ACTIVE','READY','SUBMITTED') AND "
                                             "  (j.reuse_job = 'N' OR j.reuse_job IS NULL) "
                                             " GROUP BY activity ",
                                             soci::use(src),
                                             soci::use(dst),
                                             soci::use(vo),
                                             soci::use(tTime),
                                             soci::use(hashSegment.start), soci::use(hashSegment.end),
                                             soci::use(vo)
                                         );

            soci::rowset<soci::row>::const_iterator it;
            for (it = rs.begin(); it != rs.end(); it++)
                {
                    if (it->get_indicator("ACTIVITY") == soci::i_null)
                        {
                            ret["default"] = it->get<long long>("COUNT");
                        }
                    else
                        {
                            std::string activityShare = it->get<std::string>("ACTIVITY");
                            long long nFiles = it->get<long long>("COUNT");
                            ret[activityShare.empty() ? "default" : activityShare] = nFiles;
                        }
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ret;
}

std::map<std::string, int> OracleAPI::getFilesNumPerActivity(soci::session& sql, std::string src, std::string dst, std::string vo, int filesNum)
{
    std::map<std::string, int> activityFilesNum;

    try
        {

            // get activity shares configuration for given VO
            std::map<std::string, double> activityShares = getActivityShareConf(sql, vo);

            // if there is no configuration no assigment can be made
            if (activityShares.empty()) return activityFilesNum;

            // get the activities in the queue
            std::map<std::string, long long> activitiesInQueue = getActivitiesInQueue(sql, src, dst, vo);

            // sum of all activity shares in the queue (needed for normalization)
            double sum = 0;

            std::map<std::string, long long>::iterator it;
            for (it = activitiesInQueue.begin(); it != activitiesInQueue.end(); it++)
                {
                    sum += activityShares[it->first];
                }

            // assign slots to activities
            for (int i = 0; i < filesNum; i++)
                {
                    // if sum <= 0 there is nothing to assign
                    if (sum <= 0) break;
                    // a random number from (0, 1)
                    double r = ((double) rand() / (RAND_MAX));
                    // interval corresponding to given activity
                    double interval = 0;

                    for (it = activitiesInQueue.begin(); it != activitiesInQueue.end(); it++)
                        {
                            // if there are no more files for this activity continue
                            if (it->second <= 0) continue;
                            // calculate the interval
                            interval += activityShares[it->first] / sum;
                            // if the slot has been assigned to the given activity ...
                            if (r < interval)
                                {
                                    ++activityFilesNum[it->first];
                                    --it->second;
                                    // if there are no more files for the given ativity remove it from the sum
                                    if (it->second == 0) sum -= activityShares[it->first];
                                    break;
                                }
                        }
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return activityFilesNum;
}


void OracleAPI::getByJobId(std::map< std::string, std::list<TransferFiles*> >& files)
{
    soci::session sql(*connectionPool);

    time_t now = time(NULL);
    struct tm tTime;
    gmtime_r(&now, &tTime);
    std::vector< boost::tuple<std::string, std::string, std::string> > distinct;
    distinct.reserve(1500); //approximation

    try
        {
            int mode = getOptimizerMode(sql);
            int defaultFilesNum = 0;
            if(mode==1)
                {
                    defaultFilesNum = mode_1[3];
                }
            else if(mode==2)
                {
                    defaultFilesNum = mode_2[3];
                }
            else if(mode==3)
                {
                    defaultFilesNum = mode_3[3];
                }
            else
                {
                    defaultFilesNum = mode_1[3];
                }

            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT DISTINCT source_se, dest_se, vo_name "
                                             " FROM t_file "
                                             " WHERE "
                                             "      file_state = 'SUBMITTED' AND "
                                             "      (hashed_id >= :hStart AND hashed_id <= :hEnd) ",
                                             soci::use(hashSegment.start), soci::use(hashSegment.end)
                                         );
            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    soci::row const& r = *i;
                    distinct.push_back(
                        boost::tuple< std::string, std::string, std::string>(
                            r.get<std::string>("SOURCE_SE",""),
                            r.get<std::string>("DEST_SE",""),
                            r.get<std::string>("VO_NAME","")
                        )

                    );
                }

            if(distinct.empty())
                return;

            // Iterate through pairs, getting jobs IF the VO has not run out of credits
            // AND there are pending file transfers within the job
            std::vector< boost::tuple<std::string, std::string, std::string> >::iterator it;
            for (it = distinct.begin(); it != distinct.end(); ++it)
                {
                    boost::tuple<std::string, std::string, std::string>& triplet = *it;
                    int count = 0;
                    bool manualConfigExists = false;
                    int filesNum = defaultFilesNum;

                    sql << "SELECT COUNT(*) FROM t_link_config WHERE (source = :source OR source = '*') AND (destination = :dest OR destination = '*')",
                        soci::use(boost::get<0>(triplet)),soci::use(boost::get<1>(triplet)),soci::into(count);
                    if(count > 0)
                        manualConfigExists = true;

                    if(!manualConfigExists)
                        {
                            sql << "SELECT COUNT(*) FROM t_group_members WHERE (member=:source OR member=:dest)",
                                soci::use(boost::get<0>(triplet)),soci::use(boost::get<1>(triplet)),soci::into(count);
                            if(count > 0)
                                manualConfigExists = true;
                        }

                    if(!manualConfigExists)
                        {
                            int limit = 0;
                            int maxActive = 0;
                            soci::indicator isNull = soci::i_ok;

                            sql << " select count(*) from t_file where source_se=:source_se and dest_se=:dest_se and file_state in ('READY','ACTIVE') ",
                                soci::use(boost::get<0>(triplet)),
                                soci::use(boost::get<1>(triplet)),
                                soci::into(limit);

                            sql << "select active from t_optimize_active where source_se=:source_se and dest_se=:dest_se",
                                soci::use(boost::get<0>(triplet)),
                                soci::use(boost::get<1>(triplet)),
                                soci::into(maxActive, isNull);

                            /* need to check whether a manual config exists for source_se or dest_se so as not to limit the files */
                            if (isNull != soci::i_null && maxActive > 0)
                                {
                                    filesNum = (maxActive - limit);
                                    if(filesNum <=0 )
                                        continue;
                                }
                        }

                    std::map<std::string, int> activityFilesNum =
                        getFilesNumPerActivity(sql, boost::get<0>(triplet), boost::get<1>(triplet), boost::get<2>(triplet), filesNum);

                    if (activityFilesNum.empty())
                        {
                            soci::rowset<TransferFiles> rs = (
                                                                 sql.prepare <<
                                                                 " SELECT * FROM (SELECT "
                                                                 "       rownum as rn, f.file_state, f.source_surl, f.dest_surl, f.job_id, j.vo_name, "
                                                                 "       f.file_id, j.overwrite_flag, j.user_dn, j.cred_id, "
                                                                 "       f.checksum, j.checksum_method, j.source_space_token, "
                                                                 "       j.space_token, j.copy_pin_lifetime, j.bring_online, "
                                                                 "       f.user_filesize, f.file_metadata, j.job_metadata, f.file_index, f.bringonline_token, "
                                                                 "       f.source_se, f.dest_se, f.selection_strategy, j.internal_job_params  "
                                                                 " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) WHERE  "
                                                                 " j.vo_name = f.vo_name AND f.file_state = 'SUBMITTED' AND  "
                                                                 "    f.source_se = :source AND f.dest_se = :dest AND "
                                                                 "    f.vo_name = :vo_name AND "
                                                                 "    f.wait_timestamp IS NULL AND "
                                                                 "    (f.retry_timestamp is NULL OR f.retry_timestamp < :tTime) AND "
                                                                 "    (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd) AND "
                                                                 "    j.job_state in ('ACTIVE','READY','SUBMITTED') AND "
                                                                 "    (j.reuse_job = 'N' OR j.reuse_job IS NULL) AND j.vo_name=:vo_name "
                                                                 "     ORDER BY j.priority DESC, j.submit_time) "
                                                                 " WHERE rn < :filesNum ",
                                                                 soci::use(boost::get<0>(triplet)),
                                                                 soci::use(boost::get<1>(triplet)),
                                                                 soci::use(boost::get<2>(triplet)),
                                                                 soci::use(tTime),
                                                                 soci::use(hashSegment.start), soci::use(hashSegment.end),
                                                                 soci::use(boost::get<2>(triplet)),
                                                                 soci::use(filesNum)
                                                             );

                            for (soci::rowset<TransferFiles>::const_iterator ti = rs.begin(); ti != rs.end(); ++ti)
                                {
                                    TransferFiles const& tfile = *ti;
                                    files[tfile.VO_NAME].push_back(new TransferFiles(tfile));
                                }
                        }
                    else
                        {
                            std::map<std::string, int>::iterator it_act;

                            for (it_act = activityFilesNum.begin(); it_act != activityFilesNum.end(); ++it_act)
                                {
                                    if (it_act->second == 0) continue;

                                    std::string select =
                                        " SELECT * FROM ("
                                        "       SELECT rownum as rn, f.file_state, f.source_surl, f.dest_surl, f.job_id, j.vo_name, "
                                        "       f.file_id, j.overwrite_flag, j.user_dn, j.cred_id, "
                                        "       f.checksum, j.checksum_method, j.source_space_token, "
                                        "       j.space_token, j.copy_pin_lifetime, j.bring_online, "
                                        "       f.user_filesize, f.file_metadata, j.job_metadata, f.file_index, f.bringonline_token, "
                                        "       f.source_se, f.dest_se, f.selection_strategy, j.internal_job_params  "
                                        " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) WHERE "
                                        " j.vo_name = f.vo_name AND f.file_state = 'SUBMITTED' AND  "
                                        "    f.source_se = :source AND f.dest_se = :dest AND "
                                        "    f.vo_name = :vo_name AND ";
                                    select +=
                                        it_act->first == "default" ?
                                        "     (f.activity = :activity OR f.activity = '' OR f.activity IS NULL) AND "
                                        :
                                        "     f.activity = :activity AND ";
                                    select +=
                                        "    f.wait_timestamp IS NULL AND "
                                        "    (f.retry_timestamp is NULL OR f.retry_timestamp < :tTime) AND "
                                        "    (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd) AND "
                                        "    j.job_state in ('ACTIVE','READY','SUBMITTED') AND "
                                        "    (j.reuse_job = 'N' OR j.reuse_job IS NULL) AND j.vo_name=:vo_name "
                                        "    ORDER BY j.priority DESC, j.submit_time)"
                                        " WHERE rn <= :filesNum"
                                        ;


                                    soci::rowset<TransferFiles> rs = (
                                                                         sql.prepare <<
                                                                         select,
                                                                         soci::use(boost::get<0>(triplet)),
                                                                         soci::use(boost::get<1>(triplet)),
                                                                         soci::use(boost::get<2>(triplet)),
                                                                         soci::use(it_act->first),
                                                                         soci::use(tTime),
                                                                         soci::use(hashSegment.start), soci::use(hashSegment.end),
                                                                         soci::use(boost::get<2>(triplet)),
                                                                         soci::use(it_act->second)
                                                                     );

                                    for (soci::rowset<TransferFiles>::const_iterator ti = rs.begin(); ti != rs.end(); ++ti)
                                        {
                                            TransferFiles const& tfile = *ti;
                                            files[tfile.VO_NAME].push_back(new TransferFiles(tfile));
                                        }
                                }
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
    catch (...)
        {
            for (std::map< std::string, std::list<TransferFiles*> >::iterator i = files.begin(); i != files.end(); ++i)
                {
                    std::list<TransferFiles*>& l = i->second;
                    for (std::list<TransferFiles*>::iterator it = l.begin(); it != l.end(); ++it)
                        {
                            if(*it)
                                delete *it;
                        }
                    l.clear();
                }
            files.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
}




void OracleAPI::setFilesToNotUsed(std::string jobId, int fileIndex, std::vector<int>& files)
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
                "   job_id = :jobId AND "
                "   file_index = :fileIndex "
                "   AND file_state = 'SUBMITTED' "
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
                                                     "  AND file_index = :fileIndex "
                                                     "  AND file_state = 'NOT_USED' ",
                                                     soci::use(jobId),
                                                     soci::use(fileIndex)
                                                 );

                    soci::rowset<soci::row>::const_iterator it;
                    for (it = rs.begin(); it != rs.end(); ++it)
                        {
                            files.push_back(
                                static_cast<int>(it->get<long long>("FILE_ID"))
                            );
                        }
                }


        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::useFileReplica(soci::session& sql, std::string jobId, int fileId)
{
    try
        {
            soci::indicator ind = soci::i_ok;
            int fileIndex=0;

            sql <<
                " SELECT file_index "
                " FROM t_file "
                " WHERE file_id = :fileId AND file_state = 'NOT_USED' ",
                soci::use(fileId),
                soci::into(fileIndex, ind)
                ;

            // make sure it's not NULL
            if (ind == soci::i_ok)
                {
                    sql.begin();
                    sql <<
                        " UPDATE t_file "
                        " SET file_state = 'SUBMITTED' "
                        " WHERE job_id = :jobId "
                        "   AND file_index = :fileIndex "
                        "   AND file_state = 'NOT_USED'",
                        soci::use(jobId),
                        soci::use(fileIndex);
                    sql.commit();
                }
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

unsigned int OracleAPI::updateFileStatus(TransferFiles* file, const std::string status)
{
    soci::session sql(*connectionPool);

    unsigned int updated = 0;


    try
        {
            sql.begin();

            int countSame = 0;

            sql << "select count(*) from t_file where file_state in ('READY','ACTIVE') and dest_surl=:destUrl and vo_name=:vo_name and dest_se=:dest_se ",
                soci::use(file->DEST_SURL),
                soci::use(file->VO_NAME),
                soci::use(file->DEST_SE),
                soci::into(countSame);

            if(countSame > 0)
                {
                    return 0;
                }

            soci::statement stmt(sql);

            stmt.exchange(soci::use(status, "state"));
            stmt.exchange(soci::use(file->FILE_ID, "fileId"));
            stmt.exchange(soci::use(hostname, "hostname"));
            stmt.alloc();
            stmt.prepare("UPDATE t_file SET "
                         "    file_state = :state, start_time = sys_extract_utc(systimestamp), transferHost = :hostname "
                         "WHERE file_id = :fileId AND file_state = 'SUBMITTED'");
            stmt.define_and_bind();
            stmt.execute(true);

            updated = (unsigned int) stmt.get_affected_rows();
            if (updated > 0)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return updated;
}


void OracleAPI::getByJobIdReuse(std::vector<TransferJobs*>& jobs, std::map< std::string, std::list<TransferFiles*> >& files)
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
                                                         "SELECT "
                                                         "       f.file_state, f.source_surl, f.dest_surl, f.job_id, j.vo_name, "
                                                         "       f.file_id, j.overwrite_flag, j.user_dn, j.cred_id, "
                                                         "       f.checksum, j.checksum_method, j.source_space_token, "
                                                         "       j.space_token, j.copy_pin_lifetime, j.bring_online, "
                                                         "       f.user_filesize, f.file_metadata, j.job_metadata, f.file_index, f.bringonline_token, "
                                                         "       f.source_se, f.dest_se, f.selection_strategy, j.internal_job_params  "
                                                         "FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                                         "WHERE f.job_id = :jobId AND"
                                                         "    f.file_state = 'SUBMITTED' AND "
                                                         "    f.job_finished IS NULL AND "
                                                         "    f.wait_timestamp IS NULL AND "
                                                         "    (f.retry_timestamp is NULL OR f.retry_timestamp < :tTime) AND "
                                                         "    (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd) ",
                                                         soci::use(jobId),
                                                         soci::use(tTime),
                                                         soci::use(hashSegment.start), soci::use(hashSegment.end)
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
    catch (...)
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
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}




void OracleAPI::submitPhysical(const std::string & jobId, std::list<job_element_tupple> src_dest_pair,
                               const std::string & DN, const std::string & cred,
                               const std::string & voName, const std::string & myProxyServer, const std::string & delegationID,
                               const std::string & sourceSe, const std::string & destinationSe,
                               const JobParameterHandler & params)
{
    const int         bringOnline      = params.get<int>(JobParameterHandler::BRING_ONLINE);
    const char        checksumMethod   = params.get(JobParameterHandler::CHECKSUM_METHOD)[0];
    const int         copyPinLifeTime  = params.get<int>(JobParameterHandler::COPY_PIN_LIFETIME);
    const std::string failNearLine     = params.get(JobParameterHandler::FAIL_NEARLINE);
    const std::string metadata         = params.get(JobParameterHandler::JOB_METADATA);
    const std::string overwrite        = params.get(JobParameterHandler::OVERWRITEFLAG);
    const std::string paramFTP         = params.get(JobParameterHandler::GRIDFTP);
    const int         retry            = params.get<int>(JobParameterHandler::RETRY);
    const int         retryDelay       = params.get<int>(JobParameterHandler::RETRY_DELAY);
    const std::string reuse            = params.get(JobParameterHandler::REUSE);
    const std::string hop              = params.get(JobParameterHandler::MULTIHOP);
    const std::string sourceSpaceToken = params.get(JobParameterHandler::SPACETOKEN_SOURCE);
    const std::string spaceToken       = params.get(JobParameterHandler::SPACETOKEN);
    const std::string nostreams		   = params.get(JobParameterHandler::NOSTREAMS);
    const std::string buffSize		   = params.get(JobParameterHandler::BUFFER_SIZE);
    const std::string timeout		   = params.get(JobParameterHandler::TIMEOUT);

    std::string reuseFlag = "N";
    if (reuse == "Y")
        reuseFlag = "Y";
    else if (hop == "Y")
        reuseFlag = "H";

    const std::string initialState = bringOnline > 0 || copyPinLifeTime > 0 ? "STAGING" : "SUBMITTED";
    const int priority = 3;
    std::string jobParams;

    // create the internal params string
    // if numbre of streams was specified ...
    if (!nostreams.empty()) jobParams = "nostreams:" + nostreams;
    // if timeout was specified ...
    if (!timeout.empty())
        {
            if (!jobParams.empty()) jobParams += ",";
            jobParams += "timeout:" + timeout;
        }
    // if buffer size was specified ...
    if (!buffSize.empty())
        {
            if (!jobParams.empty()) jobParams += ",";
            jobParams += "buffersize:" + buffSize;
        }


    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            soci::indicator reuseFlagIndicator = soci::i_ok;
            if (reuseFlag.empty())
                reuseFlagIndicator = soci::i_null;
            // Insert job
            soci::statement insertJob = (
                                            sql.prepare << "INSERT INTO t_job (job_id, job_state, job_params, user_dn, user_cred, priority,       "
                                            "                   vo_name, submit_time, internal_job_params, submit_host, cred_id,   "
                                            "                   myproxy_server, space_token, overwrite_flag, source_space_token,   "
                                            "                   copy_pin_lifetime, fail_nearline, checksum_method, "
                                            "                   reuse_job, bring_online, retry, retry_delay, job_metadata,        "
                                            "                  source_se, dest_se)                                                "
                                            "VALUES (:jobId, :jobState, :jobParams, :userDn, :userCred, :priority,                 "
                                            "        :voName, sys_extract_utc(systimestamp), :internalParams, :submitHost, :credId,              "
                                            "        :myproxyServer, :spaceToken, :overwriteFlag, :sourceSpaceToken,               "
                                            "        :copyPinLifetime, :failNearline, :checksumMethod,             "
                                            "        :reuseJob, :bring_online, :retry, :retryDelay, :job_metadata,                "
                                            "       :sourceSe, :destSe)                                                           ",
                                            soci::use(jobId), soci::use(initialState), soci::use(paramFTP), soci::use(DN), soci::use(cred), soci::use(priority),
                                            soci::use(voName), soci::use(jobParams), soci::use(hostname), soci::use(delegationID),
                                            soci::use(myProxyServer), soci::use(spaceToken), soci::use(overwrite), soci::use(sourceSpaceToken),
                                            soci::use(copyPinLifeTime), soci::use(failNearLine), soci::use(checksumMethod),
                                            soci::use(reuseFlag, reuseFlagIndicator), soci::use(bringOnline),
                                            soci::use(retry), soci::use(retryDelay), soci::use(metadata),
                                            soci::use(sourceSe), soci::use(destinationSe));

            insertJob.execute(true);

            // Insert src/dest pair
            std::string sourceSurl, destSurl, checksum, metadata, selectionStrategy, sourceSe, destSe, activity;
            double filesize = 0.0;
            int fileIndex = 0, timeout = 0;
            int newFileId;
            soci::statement pairStmt = (
                                           sql.prepare <<
                                           "INSERT INTO t_file (vo_name, job_id, file_state, source_surl, dest_surl, checksum, user_filesize, file_metadata, selection_strategy, file_index, source_se, dest_se, activity) "
                                           "VALUES (:voName, :jobId, :fileState, :sourceSurl, :destSurl, :checksum, :filesize, :metadata, :ss, :fileIndex, :source_se, :dest_se, :activity) ",
                                           soci::use(voName),
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
                                           soci::use(destSe),
                                           soci::use(activity)
                                       );

            soci::statement pairStmtSeBlaklisted = (
                    sql.prepare <<
                    "INSERT INTO t_file (vo_name, job_id, file_state, source_surl, dest_surl, checksum, user_filesize, file_metadata, selection_strategy, file_index, source_se, dest_se, wait_timestamp, wait_timeout, activity) "
                    "VALUES (:voName, :jobId, :fileState, :sourceSurl, :destSurl, :checksum, :filesize, :metadata, :ss, :fileIndex, :source_se, :dest_se, sys_extract_utc(systimestamp), :timeout, :activity) ",
                    soci::use(voName),
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
                    soci::use(destSe),
                    soci::use(timeout),
                    soci::use(activity)
                                                   );

            // When reuse is enabled, we hash the job id instead of the file ID
            // This guarantees that the whole set belong to the same machine, but keeping
            // the load balance between hosts
            soci::statement updateHashedId = (sql.prepare <<
                                              "UPDATE t_file SET hashed_id = ora_hash(file_id, 65535) WHERE file_id = :fileId",
                                              soci::use(newFileId)
                                             );

            soci::statement updateHashedIdWithJobId = (sql.prepare <<
                    "UPDATE t_file SET hashed_id = ora_hash(job_id, 35535) WHERE file_id = :fileId",
                    soci::use(newFileId)
                                                      );

            std::list<job_element_tupple>::const_iterator iter;
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
                    activity = iter->activity;

                    sql << " MERGE INTO t_optimize_active USING "
                        "    (SELECT :sourceSe as source, :destSe as dest FROM dual) Pair "
                        " ON (t_optimize_active.source_se = Pair.source AND t_optimize_active.dest_se = Pair.dest) "
                        " WHEN NOT MATCHED THEN INSERT (source_se, dest_se) VALUES (Pair.source, Pair.dest)",
                        soci::use(sourceSe), soci::use(destSe);

                    if (iter->wait_timeout.is_initialized())
                        {
                            timeout = *iter->wait_timeout;
                            pairStmtSeBlaklisted.execute(true);
                        }
                    else
                        {
                            pairStmt.execute(true);
                        }

                    // Last id
                    sql << "SELECT file_file_id_seq.currval FROM dual",
                        soci::into(newFileId);

                    // Update hash
                    if (reuseFlag == "N")
                        updateHashedId.execute(true);
                    else
                        updateHashedIdWithJobId.execute(true);
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}




void OracleAPI::getTransferJobStatus(std::string requestID, bool archive, std::vector<JobStatus*>& jobs)
{
    soci::session sql(*connectionPool);

    std::string fileCountQuery;
    std::string statusQuery;

    if (archive)
        {
            fileCountQuery = "SELECT COUNT(DISTINCT file_index) FROM t_file_backup WHERE t_file_backup.job_id = :jobId";
            statusQuery = "SELECT t_job_backup.job_id, t_job_backup.job_state, t_file_backup.file_state, "
                          "    t_job_backup.user_dn, t_job_backup.reason, t_job_backup.submit_time, t_job_backup.priority, "
                          "    t_job_backup.vo_name, t_file_backup.file_index "
                          "FROM t_job_backup, t_file_backup "
                          "WHERE t_file_backup.job_id = :jobId and t_file_backup.job_id= t_job_backup.job_id";
        }
    else
        {
            fileCountQuery = "SELECT COUNT(DISTINCT file_index) FROM t_file WHERE t_file.job_id = :jobId";
            statusQuery = "SELECT t_job.job_id, t_job.job_state, t_file.file_state, "
                          "    t_job.user_dn, t_job.reason, t_job.submit_time, t_job.priority, "
                          "    t_job.vo_name, t_file.file_index "
                          "FROM t_job, t_file "
                          "WHERE t_file.job_id = :jobId and t_file.job_id=t_job.job_id";
        }

    try
        {
            long long numFiles = 0;
            sql << fileCountQuery, soci::use(requestID), soci::into(numFiles);

            soci::rowset<JobStatus> rs = (
                                             sql.prepare << statusQuery,
                                             soci::use(requestID, "jobId"));

            for (soci::rowset<JobStatus>::iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    JobStatus& job = *i;
                    job.numFiles = numFiles;
                    jobs.push_back(new JobStatus(job));
                }
        }
    catch (std::exception& e)
        {
            std::vector< JobStatus* >::iterator it;
            for (it = jobs.begin(); it != jobs.end(); ++it)
                {
                    if(*it)
                        delete (*it);
                }
            jobs.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    catch (...)
        {
            std::vector< JobStatus* >::iterator it;
            for (it = jobs.begin(); it != jobs.end(); ++it)
                {
                    if(*it)
                        delete (*it);
                }
            jobs.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



/*
 * Return a list of jobs based on the status requested
 * std::vector<JobStatus*> jobs: the caller will deallocate memory JobStatus instances and clear the vector
 * std::vector<std::string> inGivenStates: order doesn't really matter, more than one states supported
 */
void OracleAPI::listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates, std::string restrictToClientDN, std::string forDN, std::string VOname)
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
            std::vector< JobStatus* >::iterator it;
            for (it = jobs.begin(); it != jobs.end(); ++it)
                {
                    if(*it)
                        delete (*it);
                }
            jobs.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    catch (...)
        {
            std::vector< JobStatus* >::iterator it;
            for (it = jobs.begin(); it != jobs.end(); ++it)
                {
                    if(*it)
                        delete (*it);
                }
            jobs.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::getTransferFileStatus(std::string requestID, bool archive,
                                      unsigned offset, unsigned limit, std::vector<FileTransferStatus*>& files)
{
    soci::session sql(*connectionPool);

    try
        {
            std::string query;

            if (archive)
                {
                    query = "SELECT * FROM (SELECT rownum as rn, t_file_backup.file_id, t_file_backup.source_surl, t_file_backup.dest_surl, t_file_backup.file_state, "
                            "       t_file_backup.reason, t_file_backup.start_time, "
                            "       t_file_backup.finish_time, t_file_backup.retry "
                            "FROM t_file_backup WHERE t_file_backup.job_id = :jobId ";
                }
            else
                {
                    query = "SELECT * FROM (SELECT rownum as rn, t_file.file_id, t_file.source_surl, t_file.dest_surl, t_file.file_state, "
                            "       t_file.reason, t_file.start_time, t_file.finish_time, t_file.retry "
                            "FROM t_file WHERE t_file.job_id = :jobId ";
                }

            if (limit)
                query += ") WHERE rn >= :offset AND rn <= :offset + :limit";
            else
                query += ") WHERE rn >= :offset";


            FileTransferStatus transfer;
            soci::statement stmt(sql);
            stmt.exchange(soci::into(transfer));
            stmt.exchange(soci::use(requestID, "jobId"));
            stmt.exchange(soci::use(offset, "offset"));
            if (limit)
                stmt.exchange(soci::use(limit, "limit"));

            stmt.alloc();
            stmt.prepare(query);
            stmt.define_and_bind();

            if (stmt.execute(true))
                {
                    do
                        {
                            files.push_back(new FileTransferStatus(transfer));
                        }
                    while (stmt.fetch());
                }
        }
    catch (std::exception& e)
        {
            std::vector< FileTransferStatus* >::iterator it;
            for (it = files.begin(); it != files.end(); ++it)
                {
                    if(*it)
                        delete (*it);
                }
            files.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    catch (...)
        {
            std::vector< FileTransferStatus* >::iterator it;
            for (it = files.begin(); it != files.end(); ++it)
                {
                    if(*it)
                        delete (*it);
                }
            files.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::getSe(Se* &se, std::string seName)
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
            if(se)
                delete se;
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    catch (...)
        {
            if(se)
                delete se;
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}


bool OracleAPI::updateFileTransferStatus(double throughputIn, std::string job_id, int file_id, std::string transfer_status, std::string transfer_message,
        int process_id, double filesize, double duration)
{

    soci::session sql(*connectionPool);
    try
        {
            updateFileTransferStatusInternal(sql, throughputIn, job_id, file_id, transfer_status, transfer_message, process_id, filesize, duration);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return true;
}


bool OracleAPI::updateFileTransferStatusInternal(soci::session& sql, double throughputIn, std::string job_id, int file_id, std::string transfer_status, std::string transfer_message,
        int process_id, double filesize, double duration)
{
    bool ok = true;

    try
        {
            double throughput = 0.0;

            bool staging = false;

            time_t now = time(NULL);
            struct tm tTime;
            gmtime_r(&now, &tTime);


            sql.begin();

            // query for the file state in DB
            soci::rowset<soci::row> rs = (
                                             sql.prepare << "SELECT file_state FROM t_file WHERE file_id=:fileId and job_id=:jobId",
                                             soci::use(file_id),
                                             soci::use(job_id)
                                         );


            // check if the state is STAGING, there should be just one row
            std::string st;
            soci::rowset<soci::row>::const_iterator it = rs.begin();
            if (it != rs.end())
                {
                    soci::row const& r = *it;
                    std::string st = r.get<std::string>("FILE_STATE");
                    staging = (st == "STAGING");
                }

            if(st == "ACTIVE" && (transfer_status != "FAILED" ||  transfer_status != "FINISHED" ||  transfer_status != "CANCELED"))
                return true;

            soci::statement stmt(sql);
            std::ostringstream query;

            query << "UPDATE t_file SET "
                  "    file_state = :state, reason = :reason";
            stmt.exchange(soci::use(transfer_status, "state"));
            stmt.exchange(soci::use(transfer_message, "reason"));

            if (transfer_status == "FINISHED" || transfer_status == "FAILED" || transfer_status == "CANCELED")
                {
                    query << ", FINISH_TIME = :time1";
                    query << ", JOB_FINISHED = :time1";
                    stmt.exchange(soci::use(tTime, "time1"));
                }
            if (transfer_status == "ACTIVE")
                {
                    query << ", START_TIME = :time1";
                    stmt.exchange(soci::use(tTime, "time1"));
                }

            if (transfer_status == "STAGING")
                {
                    if (staging)
                        {
                            query << ", STAGING_FINISHED = :time1";
                            stmt.exchange(soci::use(tTime, "time1"));
                        }
                    else
                        {
                            query << ", STAGING_START = :time1";
                            stmt.exchange(soci::use(tTime, "time1"));
                        }
                }

            if (filesize > 0 && duration > 0 && transfer_status == "FINISHED")
                {
                    if(throughputIn != 0.0)
                        {
                            throughput = convertKbToMb(throughputIn);
                        }
                    else
                        {
                            throughput = convertBtoM(filesize, duration);
                        }
                }
            else if (filesize > 0 && duration <= 0 && transfer_status == "FINISHED")
                {
                    if(throughputIn != 0.0)
                        {
                            throughput = convertKbToMb(throughputIn);
                        }
                    else
                        {
                            throughput = convertBtoM(filesize, 1);
                        }
                }
            else
                {
                    throughput = 0.0;
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

            if(transfer_status == "FAILED")
                useFileReplica(sql, job_id, file_id);

        }
    catch (std::exception& e)
        {
            ok = false;
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return ok;
}

bool OracleAPI::updateJobTransferStatus(std::string job_id, const std::string status)
{

    soci::session sql(*connectionPool);

    try
        {
            updateJobTransferStatusInternal(sql, job_id, status);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return true;
}



bool OracleAPI::updateJobTransferStatusInternal(soci::session& sql, std::string job_id, const std::string status)
{
    bool ok = true;

    try
        {
            int numberOfFilesInJob = 0;
            int numberOfFilesCanceled = 0;
            int numberOfFilesFinished = 0;
            int numberOfFilesFailed = 0;

            int numberOfFilesNotCanceled = 0;
            int numberOfFilesNotCanceledNorFailed = 0;

            std::string currentState("");
            std::string reuseFlag;

            // prevent multiple updates of the same state for the same job
            sql <<
                " SELECT job_state, reuse_job from t_job  "
                " WHERE job_id = :job_id ",
                soci::use(job_id),
                soci::into(currentState),
                soci::into(reuseFlag)
                ;

            if(currentState == status)
                return true;

            // total number of file in the job
            sql <<
                " SELECT COUNT(DISTINCT file_index) "
                " FROM t_file "
                " WHERE job_id = :job_id ",
                soci::use(job_id),
                soci::into(numberOfFilesInJob)
                ;

            // number of files that were not canceled
            sql <<
                " SELECT COUNT(DISTINCT file_index) "
                " FROM t_file "
                " WHERE job_id = :jobId "
                "   AND file_state <> 'CANCELED' ", // all the replicas have to be in CANCELED state in order to count a file as canceled
                soci::use(job_id),                  // so if just one replica is in a different state it is enoght to count it as not canceled
                soci::into(numberOfFilesNotCanceled)
                ;

            // number of files that were canceled
            numberOfFilesCanceled = numberOfFilesInJob - numberOfFilesNotCanceled;

            // number of files that were finished
            sql <<
                " SELECT COUNT(DISTINCT file_index) "
                " FROM t_file "
                " WHERE job_id = :jobId "
                "   AND file_state = 'FINISHED' ", // at least one replica has to be in FINISH state in order to count the file as finished
                soci::use(job_id),
                soci::into(numberOfFilesFinished)
                ;

            // number of files that were not canceled nor failed
            sql <<
                " SELECT COUNT(DISTINCT file_index) "
                " FROM t_file "
                " WHERE job_id = :jobId "
                "   AND file_state <> 'CANCELED' " // for not canceled files see above
                "   AND file_state <> 'FAILED' ",  // all the replicas have to be either in CANCELED or FAILED state in order to count
                soci::use(job_id),                 // a files as failed so if just one replica is not in CANCEL neither in FAILED state
                soci::into(numberOfFilesNotCanceledNorFailed) //it is enought to count it as not canceld nor failed
                ;

            // number of files that failed
            numberOfFilesFailed = numberOfFilesInJob - numberOfFilesNotCanceledNorFailed - numberOfFilesCanceled;

            // agregated number of files in terminal states (FINISHED, FAILED and CANCELED)
            int numberOfFilesTerminal = numberOfFilesCanceled + numberOfFilesFailed + numberOfFilesFinished;

            bool jobFinished = (numberOfFilesInJob == numberOfFilesTerminal);

            sql.begin();
            if (jobFinished)
                {
                    std::string state;
                    std::string reason = "One or more files failed. Please have a look at the details for more information";
                    if (numberOfFilesFinished > 0 && numberOfFilesFailed > 0)
                        {
                            if (reuseFlag == "H")
                                state = "FAILED";
                            else
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
                        "    job_state = :state, job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), "
                        "    reason = :reason "
                        "WHERE job_id = :jobId AND "
                        "      job_state NOT IN ('FINISHEDDIRTY','CANCELED','FINISHED','FAILED')",
                        soci::use(state, "state"), soci::use(reason, "reason"),
                        soci::use(job_id, "jobId");

                    // And file finish timestamp
                    sql << "UPDATE t_file SET job_finished = sys_extract_utc(systimestamp) WHERE job_id = :jobId ",
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ok;
}



void OracleAPI::updateFileTransferProgressVector(std::vector<struct message_updater>& messages)
{

    soci::session sql(*connectionPool);
    double throughput = 0.0;
    int file_id = 0;

    try
        {
            soci::statement stmt = (sql.prepare << "UPDATE t_file SET throughput = :throughput WHERE file_id = :fileId ",
                                    soci::use(throughput), soci::use(file_id));

            sql.begin();

            std::vector<struct message_updater>::iterator iter;
            for (iter = messages.begin(); iter != messages.end(); ++iter)
                {
                    if (iter->msg_errno == 0)
                        {
                            if((*iter).throughput != 0)
                                {
                                    throughput = convertKbToMb((*iter).throughput);
                                    file_id = (*iter).file_id;
                                    stmt.execute(true);
                                }
                        }
                    else
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to read a stall message: "
                                                           << iter->msg_error_reason << commit;
                        }
                }

            sql.commit();

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }


}

void OracleAPI::cancelJob(std::vector<std::string>& requestIDs)
{
    soci::session sql(*connectionPool);

    try
        {
            const std::string reason = "Job canceled by the user";

            sql.begin();

            for (std::vector<std::string>::const_iterator i = requestIDs.begin(); i != requestIDs.end(); ++i)
                {
                    // Cancel job
                    sql << "UPDATE t_job SET job_state = 'CANCELED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), cancel_job='Y' "
                        "                 ,reason = :reason "
                        "WHERE job_id = :jobId AND job_state NOT IN ('CANCELED','FINISHEDDIRTY', 'FINISHED', 'FAILED')",
                        soci::use(reason, "reason"), soci::use(*i, "jobId");


                    soci::rowset<soci::row> rs = (sql.prepare << "SELECT f.pid FROM t_file f "
                                                  "WHERE  "
                                                  "      f.FILE_STATE IN ('ACTIVE','READY') AND "
                                                  "      f.PID IS NOT NULL AND "
                                                  "      f.job_id=:job_id ", soci::use(*i, "job_id"));


                    for (soci::rowset<soci::row>::const_iterator i2 = rs.begin(); i2 != rs.end(); ++i2)
                        {
                            soci::row const& row = *i2;
                            int pid = static_cast<int>(row.get<long long>("PID"));
                            kill(pid, SIGTERM);
                        }

                    // Cancel files
                    sql << "UPDATE t_file SET file_state = 'CANCELED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), "
                        "                  reason = :reason "
                        "WHERE job_id = :jobId AND file_state NOT IN ('CANCELED','FINISHED','FAILED')",
                        soci::use(reason, "reason"), soci::use(*i, "jobId");
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



/*t_credential API*/
void OracleAPI::insertGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::updateGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::statement stmt(sql);

            stmt.exchange(soci::use(dlg_id, "dlgId"));
            stmt.exchange(soci::use(dn, "dn"));
            stmt.exchange(soci::use(cert_request, "certRequest"));
            stmt.exchange(soci::use(priv_key, "privKey"));
            stmt.exchange(soci::use(voms_attrs, "vomsAttrs"));

            stmt.alloc();

            stmt.prepare("UPDATE t_credential_cache SET "
                         "    dlg_id = :dlgId, dn = :dn, " // Workaround for ORA-24816
                         "    cert_request = :certRequest, "
                         "    priv_key = :privKey, "
                         "    voms_attrs = :vomsAttrs "
                         "WHERE dlg_id = :dlgId AND dn=:dn");

            stmt.define_and_bind();

            sql.begin();
            stmt.execute(true);
            if (stmt.get_affected_rows() == 0)
                {
                    std::ostringstream msg;
                    msg << "No entries updated in t_credential_cache! "
                        << dn << " (" << dlg_id << ")";
                    throw Err_Custom(msg.str());
                }
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



CredCache* OracleAPI::findGrDPStorageCacheElement(std::string delegationID, std::string dn)
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
            if(cred)
                delete cred;
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    catch (...)
        {
            if(cred)
                delete cred;
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return cred;
}



void OracleAPI::deleteGrDPStorageCacheElement(std::string delegationID, std::string dn)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::insertGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::updateGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time)
{
    soci::session sql(*connectionPool);


    try
        {
            sql.begin();
            struct tm tTime;
            gmtime_r(&termination_time, &tTime);

            sql << "UPDATE t_credential SET "
                "    dlg_id = :dlgId, dn = :dn, " // Workaround for ORA-24816
                "    proxy = :proxy, "
                "    voms_attrs = :vomsAttrs, "
                "    termination_time = :terminationTime "
                "WHERE dlg_id = :dlgId AND dn = :dn",
                soci::use(proxy, "proxy"), soci::use(voms_attrs, "vomsAttrs"),
                soci::use(tTime, "terminationTime"), soci::use(dlg_id, "dlgId"),
                soci::use(dn, "dn");

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



Cred* OracleAPI::findGrDPStorageElement(std::string delegationID, std::string dn)
{
    Cred* cred = NULL;
    soci::session sql(*connectionPool);

    try
        {
            cred = new Cred();
            sql << "SELECT dlg_id, dn, voms_attrs, proxy, termination_time AS TERMINATION_TIME "
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
            if(cred)
                delete cred;
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    catch (...)
        {
            if(cred)
                delete cred;
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return cred;
}



void OracleAPI::deleteGrDPStorageElement(std::string delegationID, std::string dn)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



bool OracleAPI::getDebugMode(std::string source_hostname, std::string destin_hostname)
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return isDebug;
}



void OracleAPI::setDebugMode(std::string source_hostname, std::string destin_hostname, std::string mode)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::getSubmittedJobsReuse(std::vector<TransferJobs*>& jobs, const std::string & vos)
{
    soci::session sql(*connectionPool);

    try
        {
            std::string query;
            query = "SELECT * FROM (SELECT "
                    "   job_id, "
                    "   job_state, "
                    "   vo_name,  "
                    "   priority,  "
                    "   source_se, "
                    "   dest_se,  "
                    "   agent_dn, "
                    "   submit_host, "
                    "   user_dn, "
                    "   user_cred, "
                    "   cred_id,  "
                    "   space_token, "
                    "   storage_class,  "
                    "   job_params, "
                    "   overwrite_flag, "
                    "   source_space_token, "
                    "   source_token_description,"
                    "   copy_pin_lifetime, "
                    "   checksum_method, "
                    "   bring_online, "
                    "   reuse_job, "
                    "   submit_time "
                    "FROM t_job WHERE "
                    "    job_state = 'SUBMITTED' AND job_finished IS NULL AND "
                    "    cancel_job IS NULL AND "
                    "    (reuse_job IS NOT NULL and reuse_job != 'N') ";

            if (vos != "*")
                {
                    query += " AND t_job.vo_name IN " + vos + " ";
                }

            query += " ORDER BY priority DESC, SYS_EXTRACT_UTC(submit_time) "
                     ") WHERE ROWNUM <= 1";

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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}




void OracleAPI::auditConfiguration(const std::string & dn, const std::string & config, const std::string & action)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql << "INSERT INTO t_config_audit (datetime, dn, config, action ) VALUES "
                "                           (sys_extract_utc(systimestamp), :dn, :config, :action)",
                soci::use(dn), soci::use(config), soci::use(action);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



/*custom optimization stuff*/

void OracleAPI::fetchOptimizationConfig2(OptimizerSample* ops, const std::string & /*source_hostname*/, const std::string & /*destin_hostname*/)
{
    ops->streamsperfile = DEFAULT_NOSTREAMS;
    ops->timeout = MID_TIMEOUT;
    ops->bufsize = DEFAULT_BUFFSIZE;
}

void OracleAPI::recordOptimizerUpdate(soci::session& sql, int active, double filesize,
                                      double throughput, int nostreams, int timeout, int buffersize,
                                      std::string source_hostname, std::string destin_hostname)
{
    try
        {
            sql.begin();
            sql << "INSERT INTO t_optimizer_evolution "
                " (datetime, source_se, dest_se, nostreams, timeout, active, throughput, buffer, filesize) "
                " VALUES "
                " (sys_extract_utc(systimestamp), :source, :dest, :nostreams, :timeout, :active, :throughput, :buffer, :filesize)",
                soci::use(source_hostname), soci::use(destin_hostname), soci::use(nostreams),
                soci::use(timeout), soci::use(active), soci::use(throughput), soci::use(buffersize), soci::use(filesize);
            sql.commit();
        }
    catch (...)
        {
            sql.rollback();
        }
}

bool OracleAPI::updateOptimizer(double throughputIn, int, double filesize, double timeInSecs, int nostreams,
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
                "   AND t_file.dest_se = :dest_se "
                "   AND file_state = 'ACTIVE' ",
                soci::use(source_hostname),
                soci::use(destin_hostname),
                soci::into(active);

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
                " SET filesize = :fsize, throughput = :throughput, active = :active, datetime = sys_extract_utc(systimestamp), timeout= :timeout "
                " WHERE nostreams = :nstreams "
                "   AND buffer = :buffer "
                "   AND source_se = :source_se "
                "   AND dest_se = :dest_se "
                "   AND (throughput IS NULL OR throughput<=:throughput) "
                "   AND (active<=:active OR active IS NULL)"
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
                                  " SET datetime = sys_extract_utc(systimestamp) "
                                  " WHERE nostreams = :nstreams "
                                  " AND buffer = :buffer "
                                  " AND source_se = :source_se "
                                  " AND dest_se = :dest_se "
                                  "     AND (active <= :active OR active IS NULL)");

                    stmt2.define_and_bind();
                    stmt2.execute(true);
                    affected_rows += stmt2.get_affected_rows();
                    sql.commit();
                }

            // Historical data
            if (affected_rows)
                recordOptimizerUpdate(sql, active, filesize, throughput, nostreams,
                                      timeout, buffersize,
                                      source_hostname, destin_hostname);

        }
    catch (std::exception& e)
        {
            ok = false;
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return ok;
}

void OracleAPI::initOptimizer(const std::string & source_hostname, const std::string & destin_hostname, int)
{
    soci::session sql(*connectionPool);

    try
        {
            unsigned foundRecords = 0;



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
                    sql.begin();

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

                    sql.commit();
                }

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



bool OracleAPI::isCredentialExpired(const std::string & dlg_id, const std::string & dn)
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
                    time_t now = getUTC(0);
                    expired = (difftime(termTimestamp, now) <= 0);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return !expired;
}

bool OracleAPI::isTrAllowed2(const std::string & source_hostname, const std::string & destin_hostname)
{
    soci::session sql(*connectionPool);

    int maxActive = 0;
    int active = 0;
    bool allowed = false;
    soci::indicator isNull = soci::i_ok;

    try
        {
            int mode = getOptimizerMode(sql);
            int lowDefault = 3, highDefault = 5, jobsNum = 3;
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


            soci::statement stmt1 = (
                                        sql.prepare << "SELECT active FROM t_optimize_active "
                                        "WHERE source_se = :source AND dest_se = :dest_se ",
                                        soci::use(source_hostname),soci::use(destin_hostname), soci::into(maxActive, isNull));
            stmt1.execute(true);

            soci::statement stmt2 = (
                                        sql.prepare << "SELECT count(*) FROM t_file "
                                        "WHERE source_se = :source AND dest_se = :dest_se and file_state in ('READY','ACTIVE') ",
                                        soci::use(source_hostname),soci::use(destin_hostname), soci::into(active));
            stmt2.execute(true);

            if (isNull != soci::i_null)
                {
                    if(active < maxActive)
                        allowed = true;
                }

            if(active < highDefault)
                {
                    allowed = true;
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return allowed;
}



bool OracleAPI::isTrAllowed(const std::string & /*source_hostname1*/, const std::string & /*destin_hostname1*/)
{
    soci::session sql(*connectionPool);

    int allowed = false;

    try
        {
            int mode = getOptimizerMode(sql);
            int lowDefault = 3, highDefault = 5, jobsNum = 3;
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

            soci::rowset<soci::row> rs = ( sql.prepare << " select  distinct o.source_se, o.dest_se from t_optimize_active o INNER JOIN "
                                           " t_file f ON (o.source_se = f.source_se) where o.dest_se=f.dest_se and "
                                           " f.file_state='SUBMITTED'");

            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    std::string source_hostname = i->get<std::string>("SOURCE_SE");
                    std::string destin_hostname = i->get<std::string>("DEST_SE");

                    double nFailedLastHour=0.0, nFinishedLastHour=0.0;
                    double throughput=0.0;
                    double filesize = 0.0;
                    double totalSize = 0.0;
                    double retry = 0.0;   //latest from db
                    double retryStored = 0.0; //stored in mem
                    double thrStored = 0.0; //stored in mem
                    int active = 0;
                    int maxActive = 0;
                    std::stringstream message;
                    soci::indicator isNullRetry = soci::i_ok;
                    soci::indicator isNullMaxActive = soci::i_ok;

                    // Weighted average for the 5 newest transfers
                    soci::rowset<soci::row> rsSizeAndThroughput = (sql.prepare <<
                            " SELECT * FROM ("
                            " SELECT rownum as rn, filesize, throughput "
                            " FROM t_file "
                            " WHERE source_se = :source AND dest_se = :dest AND "
                            "       file_state IN ('ACTIVE','FINISHED') AND throughput > 0 AND "
                            "       filesize > 0  AND "
                            "       (start_time >= (sys_extract_utc(systimestamp) - interval '5' minute) OR "
                            "        job_finished >= (sys_extract_utc(systimestamp) - interval '5' minute)) "
                            " ORDER BY job_finished DESC)"
                            " WHERE rn <= 5 ",
                            soci::use(source_hostname),soci::use(destin_hostname));

                    for (soci::rowset<soci::row>::const_iterator j = rsSizeAndThroughput.begin();
                            j != rsSizeAndThroughput.end(); ++j)
                        {
                            filesize    = static_cast<double>(j->get<long long>("FILESIZE", 0));
                            throughput += (j->get<double>("THROUGHPUT", 0.0) * filesize);
                            totalSize  += filesize;
                        }
                    if (totalSize > 0)
                        throughput /= totalSize;


                    // Ratio of success
                    soci::rowset<std::string> rs = (sql.prepare << "SELECT file_state FROM t_file "
                                                    "WHERE "
                                                    "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                                                    "      (t_file.job_finished > (sys_extract_utc(systimestamp) - interval '5' minute)) AND "
                                                    "      file_state IN ('FAILED','FINISHED') ",
                                                    soci::use(source_hostname), soci::use(destin_hostname));

                    for (soci::rowset<std::string>::const_iterator i = rs.begin();
                            i != rs.end(); ++i)
                        {
                            if      (i->compare("FAILED") == 0)   nFailedLastHour+=1.0;
                            else if (i->compare("FINISHED") == 0) ++nFinishedLastHour+=1.0;
                        }

                    double ratioSuccessFailure = 0.0;
                    if(nFinishedLastHour > 0)
                        {
                            ratioSuccessFailure = nFinishedLastHour/(nFinishedLastHour + nFailedLastHour) * (100.0/1.0);
                        }

                    if(ratioSuccessFailure == 100)
                        highDefault = mode_2[1];
                    else
                        highDefault = mode_1[1];

                    // Active transfers
                    soci::statement stmt7 = (
                                                sql.prepare << "SELECT count(*) FROM t_file "
                                                "WHERE source_se = :source AND dest_se = :dest_se and file_state in ('READY','ACTIVE') ",
                                                soci::use(source_hostname),soci::use(destin_hostname), soci::into(active));
                    stmt7.execute(true);

                    // Max active transfers
                    soci::statement stmt8 = (
                                                sql.prepare << "SELECT active FROM t_optimize_active "
                                                "WHERE source_se = :source AND dest_se = :dest_se ",
                                                soci::use(source_hostname),soci::use(destin_hostname), soci::into(maxActive, isNullMaxActive));
                    stmt8.execute(true);


                    sql << "select * from (SELECT rownum as rn, sum(retry) from t_file WHERE source_se = :source AND dest_se = :dest_se and "
                        "file_state in ('ACTIVE','SUBMITTED') order by start_time) WHERE rn <= 50 ",
                        soci::use(source_hostname),soci::use(destin_hostname), soci::into(retry, isNullRetry);

                    if (isNullRetry == soci::i_null)
                        retry = 0;

                    //only apply the logic below if any of these values changes
                    bool changed = getChangedFile (source_hostname, destin_hostname, ratioSuccessFailure, throughput, thrStored, retry, retryStored);

                    if(changed)
                        {
                            sql.begin();

                            if(ratioSuccessFailure == 100 && throughput != 0 && thrStored !=0 && throughput > thrStored && retry <= retryStored)
                                {
                                    active = maxActive + 1;

                                    message << "Increasing active by 1, previously "
                                            << maxActive
                                            << " now max active "
                                            << active
                                            << " for link "
                                            << source_hostname
                                            << " -> "
                                            << destin_hostname
                                            << " because success rate is "
                                            << ratioSuccessFailure
                                            << "% and current throughput "
                                            << throughput
                                            << " is higher than previous "
                                            << thrStored;

                                    sql << "update t_optimize_active set datetime=sys_extract_utc(systimestamp), message=:message, active=:active where source_se=:source and dest_se=:dest ",
                                        soci::use(message.str()), soci::use(active), soci::use(source_hostname), soci::use(destin_hostname);
                                }
                            else if(ratioSuccessFailure == 100 && throughput != 0 && thrStored !=0 && throughput == thrStored && retry <= retryStored)
                                {
                                    if(mode==2 || mode==3)
                                        active = maxActive + 1;
                                    else
                                        active = maxActive;

                                    message << "Success rate is "
                                            << ratioSuccessFailure
                                            << "% for link "
                                            << source_hostname
                                            << " -> "
                                            << destin_hostname
                                            << " and current throughput is "
                                            << throughput
                                            << " equal to previous "
                                            << thrStored
                                            << " so max active remains "
                                            << maxActive;

                                    sql << "update t_optimize_active set datetime=sys_extract_utc(systimestamp), message=:message, active=:active where source_se=:source and dest_se=:dest ",
                                        soci::use(message.str()), soci::use(active), soci::use(source_hostname), soci::use(destin_hostname);
                                }
                            else if(ratioSuccessFailure == 100 && throughput != 0 && thrStored !=0 && (throughput < thrStored || retry > retryStored))
                                {
                                    if(active < highDefault || maxActive < highDefault)
                                        active = highDefault;
                                    else
                                        active = maxActive - 1;

                                    message << "Success rate is "
                                            << ratioSuccessFailure
                                            << "% for link "
                                            << source_hostname
                                            << " -> "
                                            << destin_hostname
                                            << " and current throughput "
                                            << throughput
                                            << " is less than previous sample "
                                            << thrStored
                                            << " so max active is decreased by 2 and now is "
                                            << active;

                                    sql << "update t_optimize_active set datetime=sys_extract_utc(systimestamp), message=:message, active=:active where source_se=:source and dest_se=:dest ",
                                        soci::use(message.str()), soci::use(active), soci::use(source_hostname), soci::use(destin_hostname);
                                }
                            else if (ratioSuccessFailure < 100 || retry > retryStored)
                                {
                                    if(active < highDefault || maxActive < highDefault)
                                        active = highDefault;
                                    else
                                        active = maxActive - 2;

                                    message << "Success rate is "
                                            << ratioSuccessFailure
                                            << "% for link "
                                            << source_hostname
                                            << " -> "
                                            << destin_hostname
                                            << " and current throughput is "
                                            << throughput
                                            << " while previous was "
                                            << thrStored
                                            << " so max active is decreased by 2 and now is "
                                            << active;

                                    sql << "update t_optimize_active set datetime=sys_extract_utc(systimestamp), message=:message, active=:active where source_se=:source and dest_se=:dest ",
                                        soci::use(message.str()), soci::use(active), soci::use(source_hostname), soci::use(destin_hostname);
                                }
                            else if(active == 0 || isNullMaxActive == soci::i_null )
                                {
                                    active = highDefault;

                                    message << "Number of active for link "
                                            << source_hostname
                                            << " -> "
                                            << destin_hostname
                                            << " is 0 and there is no max active yet into the db (no samples) "
                                            << " so max active now is the default "
                                            << active;

                                    sql << "update t_optimize_active set datetime=sys_extract_utc(systimestamp), message=:message, active=:active where source_se=:source and dest_se=:dest ",
                                        soci::use(message.str()), soci::use(active), soci::use(source_hostname), soci::use(destin_hostname);
                                }
                            else
                                {
                                    if(active < highDefault || maxActive < highDefault)
                                        active = highDefault;
                                    else
                                        active = maxActive;

                                    message << "Number of active for link "
                                            << source_hostname
                                            << " -> "
                                            << destin_hostname
                                            << " is "
                                            << active
                                            <<  " success rate is "
                                            << ratioSuccessFailure
                                            << "% current throughput is "
                                            << throughput
                                            << " and previous throughput is "
                                            << thrStored;

                                    sql << "update t_optimize_active set datetime=sys_extract_utc(systimestamp), message=:message, active=:active where source_se=:source and dest_se=:dest ",
                                        soci::use(message.str()), soci::use(active), soci::use(source_hostname), soci::use(destin_hostname);
                                }

                            sql.commit();
                        }
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return allowed;
}


int OracleAPI::getSeOut(const std::string & source, const std::set<std::string> & destination)
{
    soci::session sql(*connectionPool);

    // total number of allowed active for the source (both currently in use and free credits)
    int ret = 0;

    try
        {
            int nActiveSource=0;

            std::set<std::string>::iterator it;

            std::string source_hostname = source;

            sql << "SELECT COUNT(*) FROM t_file "
                "WHERE t_file.source_se = :source AND t_file.file_state in ('READY','ACTIVE')  ",
                soci::use(source_hostname), soci::into(nActiveSource);

            ret += nActiveSource;

            for (it = destination.begin(); it != destination.end(); ++it)
                {
                    std::string destin_hostname = *it;
                    ret += getCredits(source_hostname, destin_hostname);
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ret;
}

int OracleAPI::getSeIn(const std::set<std::string> & source, const std::string & destination)
{
    soci::session sql(*connectionPool);

    // total number of allowed active for the source (both currently in use and free credits)
    int ret = 0;

    try
        {
            int nActiveDest=0;

            std::set<std::string>::iterator it;

            std::string destin_hostname = destination;

            sql << "SELECT COUNT(*) FROM t_file "
                "WHERE t_file.dest_se = :dst AND t_file.file_state in ('READY','ACTIVE') ",
                soci::use(destin_hostname), soci::into(nActiveDest);

            ret += nActiveDest;

            for (it = source.begin(); it != source.end(); ++it)
                {
                    std::string source_hostname = *it;
                    ret += getCredits(source_hostname, destin_hostname);
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ret;
}

int OracleAPI::getCredits(const std::string & source_hostname, const std::string & destin_hostname)
{
    soci::session sql(*connectionPool);
    int freeCredits = 0;
    int limit = 0;
    int maxActive = 0;
    soci::indicator isNull = soci::i_ok;

    try
        {
            sql << " select count(*) from t_file where source_se=:source_se and dest_se=:dest_se and file_state in ('READY','ACTIVE') ",
                soci::use(source_hostname),
                soci::use(destin_hostname),
                soci::into(limit);

            sql << "select active from t_optimize_active where source_se=:source_se and dest_se=:dest_se",
                soci::use(source_hostname),
                soci::use(destin_hostname),
                soci::into(maxActive, isNull);

            if (isNull != soci::i_null)
                freeCredits = maxActive - limit;

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return freeCredits;
}



void OracleAPI::setAllowedNoOptimize(const std::string & job_id, int file_id, const std::string & params)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::forceFailTransfers(std::map<int, std::string>& collectJobs)
{
    soci::session sql(*connectionPool);

    try
        {
            struct message_sanity msg;
            msg.forceFailTransfers = true;
            CleanUpSanityChecks temp(this, sql, msg);
            if(!temp.getCleanUpSanityCheck())
                return;


            std::string jobId, params, tHost,reuse;
            int fileId=0, pid=0, timeout=0;
            struct tm startTimeSt;
            time_t now2 = getUTC(0);
            time_t startTime;
            double diff = 0.0;
            soci::indicator isNull = soci::i_ok;

            soci::statement stmt = (
                                       sql.prepare <<
                                       " SELECT f.job_id, f.file_id, f.start_time, f.pid, f.internal_file_params, "
                                       " f.transferHost, j.reuse_job "
                                       " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                       " WHERE f.file_state='ACTIVE' AND f.pid IS NOT NULL and f.job_finished is NULL "
                                       " and f.internal_file_params is not null and f.transferHost is not null",
                                       soci::into(jobId), soci::into(fileId), soci::into(startTimeSt),
                                       soci::into(pid), soci::into(params), soci::into(tHost), soci::into(reuse, isNull)
                                   );



            if (stmt.execute(true))
                {

                    do
                        {
                            startTime = timegm(&startTimeSt); //from db
                            timeout = extractTimeout(params);

                            int terminateTime = timeout + 1000;

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
                                    updateFileTransferStatusInternal(sql, 0.0, jobId, fileId,
                                                                     "FAILED", "Transfer has been forced-killed because it was stalled",
                                                                     pid, 0, 0);
                                    updateJobTransferStatusInternal(sql, jobId, "FAILED");
                                }

                        }
                    while (stmt.fetch());
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::setAllowed(const std::string & job_id, int file_id, const std::string & /*source_se*/, const std::string & /*dest*/,
                           int nostreams, int timeout, int buffersize)
{
    soci::session sql(*connectionPool);
    std::stringstream params;

    try
        {
            sql.begin();

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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



bool OracleAPI::terminateReuseProcess(const std::string & jobId)
{
    bool ok = true;
    soci::session sql(*connectionPool);

    try
        {
            std::string reuse;
            sql << "SELECT reuse_job FROM t_job WHERE job_id = :jobId AND reuse_job IS NOT NULL",
                soci::use(jobId), soci::into(reuse);

            sql.begin();
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return ok;
}



void OracleAPI::setPid(const std::string & /*jobId*/, int fileId, int pid)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql << "UPDATE t_file SET pid = :pid WHERE file_id = :fileId",
                soci::use(pid), soci::use(fileId);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::setPidV(int pid, std::map<int, std::string>& pids)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::revertToSubmitted()
{
    soci::session sql(*connectionPool);

    try
        {
            struct message_sanity msg;
            msg.revertToSubmitted = true;
            CleanUpSanityChecks temp(this, sql, msg);
            if(!temp.getCleanUpSanityCheck())
                return;

            struct tm startTime;
            int fileId=0;
            std::string jobId, reuseJob;
            time_t now2 = getUTC(0);

            sql.begin();

            soci::indicator reuseInd = soci::i_ok;
            soci::statement readyStmt = (sql.prepare << "SELECT f.start_time, f.file_id, f.job_id, j.reuse_job "
                                         " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                         " WHERE f.file_state = 'READY' and j.job_finished is null ",
                                         soci::into(startTime),
                                         soci::into(fileId),
                                         soci::into(jobId),
                                         soci::into(reuseJob, reuseInd));

            if (readyStmt.execute(true))
                {
                    do
                        {
                            time_t startTimestamp = timegm(&startTime);
                            double diff = difftime(now2, startTimestamp);

                            if (diff > 1500 && reuseJob != "Y")
                                {
                                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "The transfer with file id " << fileId << " seems to be stalled, restart it" << commit;

                                    sql << "UPDATE t_file SET file_state = 'SUBMITTED', reason='', transferhost='',finish_time=NULL, job_finished=NULL "
                                        "WHERE file_id = :fileId AND job_id= :jobId AND file_state = 'READY' ",
                                        soci::use(fileId),soci::use(jobId);
                                }
                            else
                                {
                                    if(reuseJob == "Y")
                                        {
                                            int count = 0;
                                            int terminateTime = 0;

                                            sql << " SELECT COUNT(*) FROM t_file WHERE job_id = :jobId ", soci::use(jobId), soci::into(count);
                                            if(count > 0)
                                                terminateTime = count * 1000;

                                            if(diff > terminateTime)
                                                {
                                                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "The transfer with file id (reuse) " << fileId << " seems to be stalled, restart it" << commit;

                                                    sql << "UPDATE t_job SET job_state = 'SUBMITTED' where job_id = :jobId ", soci::use(jobId);

                                                    sql << "UPDATE t_file SET file_state = 'SUBMITTED', reason='', transferhost='' "
                                                        "WHERE file_state = 'READY' AND "
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::backup(long* nJobs, long* nFiles)
{
    soci::session sql(*connectionPool);

    *nJobs = 0;
    *nFiles = 0;

    try
        {
            struct message_sanity msg;
            msg.cleanUpRecords = true;
            CleanUpSanityChecks temp(this, sql, msg);
            if(!temp.getCleanUpSanityCheck())
                return;

            sql.begin();

            sql << "insert into t_job_backup select * from t_job where job_state IN "
                " ('FINISHED', 'FAILED', 'CANCELED', 'FINISHEDDIRTY') and job_finished < (systimestamp - interval '4' DAY )";

            sql << "insert into t_file_backup select * from t_file where job_id IN (select job_id from t_job_backup)";

            soci::statement delFilesStmt = (sql.prepare << "delete from t_file where file_id in (select file_id from t_file_backup)");
            delFilesStmt.execute(true);
            *nFiles = delFilesStmt.get_affected_rows();

            soci::statement delJobsStmt = (sql.prepare << "delete from t_job where job_id in (select job_id from t_job_backup)");
            delJobsStmt.execute(true);
            *nJobs = delJobsStmt.get_affected_rows();

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::forkFailedRevertState(const std::string & jobId, int fileId)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql << "UPDATE t_file SET file_state = 'SUBMITTED', transferhost='', start_time=NULL "
                "WHERE file_id = :fileId AND job_id = :jobId AND  transferhost= :hostname AND "
                "      file_state NOT IN ('FINISHED','FAILED','CANCELED')",
                soci::use(fileId), soci::use(jobId), soci::use(hostname);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::forkFailedRevertStateV(std::map<int, std::string>& pids)
{
    soci::session sql(*connectionPool);

    try
        {
            int fileId=0;
            std::string jobId;

            sql.begin();
            soci::statement stmt = (sql.prepare << "UPDATE t_file SET file_state = 'SUBMITTED', transferhost='', start_time=NULL "
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



bool OracleAPI::retryFromDead(std::vector<struct message_updater>& messages)
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
                                               " WHERE file_id = :fileId AND job_id = :jobId AND file_state='ACTIVE' and transferhost=:hostname ",
                                               soci::use(iter->file_id), soci::use(std::string(iter->job_id)), soci::use(hostname)
                                           );
                    if (rs.begin() != rs.end())
                        {
                            updateFileTransferStatusInternal(sql, 0.0, (*iter).job_id, (*iter).file_id, transfer_status, transfer_message, (*iter).process_id, 0, 0);
                            updateJobTransferStatusInternal(sql, (*iter).job_id, status);
                        }
                }
        }
    catch (std::exception& e)
        {
            ok = false;
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return ok;
}



void OracleAPI::blacklistSe(std::string se, std::string vo, std::string status, int timeout, std::string msg, std::string adm_dn)
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
                        "   addition_time = sys_extract_utc(systimestamp), "
                        "   admin_dn = :admin, "
                        "   vo = :vo, "
                        "   status = :status, "
                        "   wait_timeout = :timeout "
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
                        "               VALUES (:se, :message, sys_extract_utc(systimestamp), :admin, :vo, :status, :timeout)",
                        soci::use(se), soci::use(msg), soci::use(adm_dn), soci::use(vo), soci::use(status), soci::use(timeout);
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::blacklistDn(std::string dn, std::string msg, std::string adm_dn)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << " MERGE INTO t_bad_dns USING "
                "   (SELECT :dn AS dn, :message AS message, sys_extract_utc(systimestamp) as tstamp, :admin AS admin FROM dual) Blacklisted "
                " ON (t_bad_dns.dn = Blacklisted.dn) "
                " WHEN NOT MATCHED THEN INSERT (dn, message, addition_time, admin_dn) VALUES "
                "                              (BlackListed.dn, BlackListed.message, BlackListed.tstamp, BlackListed.admin)",
                soci::use(dn), soci::use(msg), soci::use(adm_dn);
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::unblacklistSe(std::string se)
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
                " UPDATE t_file f SET f.wait_timestamp = NULL, f.wait_timeout = NULL "
                " WHERE (f.source_se = :src OR f.dest_se = :dest) "
                "   AND f.file_state IN ('ACTIVE', 'READY', 'SUBMITTED') "
                "   AND NOT EXISTS ( "
                "       SELECT NULL "
                "       FROM t_bad_dns, t_job "
                "       WHERE t_job.job_id = f.job_id and t_bad_dns.dn = t_job.user_dn AND "
                "             (t_bad_dns.status = 'WAIT' OR t_bad_dns.status = 'WAIT_AS')"
                "   )",
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::unblacklistDn(std::string dn)
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
                " UPDATE (SELECT t_file.wait_timestamp AS wait_timestamp, t_file.wait_timeout AS wait_timeout "
                "         FROM t_file INNER JOIN t_job ON t_file.job_id = t_job.job_id "
                "         WHERE t_job.user_dn = :dn "
                "         AND t_file.file_state in ('ACTIVE', 'READY', 'SUBMITTED') "
                "         AND NOT EXISTS (SELECT NULL "
                "                         FROM t_bad_ses "
                "                         WHERE (se = t_file.source_se OR se = t_file.dest_se) AND "
                "                                STATUS = 'WAIT')"
                " ) SET wait_timestamp = NULL, wait_timeout = NULL",
                soci::use(dn, "dn")
                ;

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



bool OracleAPI::isSeBlacklisted(std::string se, std::string vo)
{
    soci::session sql(*connectionPool);

    bool blacklisted = false;
    try
        {
            sql << "SELECT se FROM t_bad_ses WHERE se = :se AND (vo IS NULL OR vo='' OR vo = :vo)",
                soci::use(se), soci::use(vo), soci::into(se);
            blacklisted = sql.got_data();
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return blacklisted;
}


bool OracleAPI::allowSubmitForBlacklistedSe(std::string se)
{
    soci::session sql(*connectionPool);

    bool ret = false;
    try
        {
            sql << "SELECT se FROM t_bad_ses WHERE se = :se AND status = 'WAIT_AS'", soci::use(se), soci::into(se);
            ret = sql.got_data();
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return ret;
}

void OracleAPI::allowSubmit(std::string ses, std::string vo, std::list<std::string>& notAllowed)
{
    soci::session sql(*connectionPool);

    std::string query = "SELECT se FROM t_bad_ses WHERE se IN " + ses + " AND status != 'WAIT_AS' AND (vo IS NULL OR vo='' OR vo = :vo)";

    try
        {
            soci::rowset<std::string> rs = (sql.prepare << query, soci::use(vo));

            for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    notAllowed.push_back(*i);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}


boost::optional<int> OracleAPI::getTimeoutForSe(std::string se)
{
    soci::session sql(*connectionPool);

    boost::optional<int> ret;

    try
        {
            soci::indicator isNull = soci::i_ok;
            int tmp = 0;

            sql <<
                " SELECT wait_timeout FROM t_bad_ses WHERE se = :se ",
                soci::use(se),
                soci::into(tmp, isNull)
                ;

            if (sql.got_data())
                {
                    if (isNull == soci::i_ok) ret = tmp;
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ret;
}

void OracleAPI::getTimeoutForSe(std::string ses, std::map<std::string, int>& ret)
{
    soci::session sql(*connectionPool);

    std::string query =
        " select se, wait_timeout  "
        " from t_bad_ses "
        " where se in "
        ;
    query += ses;

    try
        {
            soci::rowset<soci::row> rs = (
                                             sql.prepare << query
                                         );

            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    ret[i->get<std::string>("se")] =  i->get<int>("wait_timeout");
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}


bool OracleAPI::isDnBlacklisted(std::string dn)
{
    soci::session sql(*connectionPool);

    bool blacklisted = false;
    try
        {
            sql << "SELECT dn FROM t_bad_dns WHERE dn = :dn", soci::use(dn), soci::into(dn);
            blacklisted = sql.got_data();
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return blacklisted;
}



bool OracleAPI::isFileReadyState(int fileID)
{
    soci::session sql(*connectionPool);
    bool isReadyState = false;
    bool isReadyHost = false;
    std::string host;
    std::string state;
    soci::indicator isNull = soci::i_ok;

    try
        {
            sql.begin();

            sql << "SELECT file_state, transferHost FROM t_file WHERE file_id = :fileId",
                soci::use(fileID), soci::into(state), soci::into(host, isNull);

            isReadyState = (state == "READY");

            if (isNull != soci::i_null)
                isReadyHost = (host == hostname);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return (isReadyState && isReadyHost);
}



bool OracleAPI::checkGroupExists(const std::string & groupName)
{
    soci::session sql(*connectionPool);

    bool exists = false;
    try
        {
            std::string grp;
            sql << "SELECT groupName FROM t_group_members WHERE groupName = :groupName",
                soci::use(groupName), soci::into(grp);

            exists = sql.got_data();
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return exists;
}

//t_group_members

void OracleAPI::getGroupMembers(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::rowset<std::string> rs = (sql.prepare << "SELECT member FROM t_group_members "
                                            "WHERE groupName = :groupName",
                                            soci::use(groupName));
            for (soci::rowset<std::string>::const_iterator i = rs.begin();
                    i != rs.end(); ++i)
                groupMembers.push_back(*i);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



std::string OracleAPI::getGroupForSe(const std::string se)
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return group;
}



void OracleAPI::addMemberToGroup(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            std::string member;
            soci::statement stmt = (sql.prepare << "INSERT INTO t_group_members(member, groupName) "
                                    "                    VALUES (:member, :groupName)",
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::deleteMembersFromGroup(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            std::string member;
            soci::statement stmt = (sql.prepare << "DELETE FROM t_group_members "
                                    "WHERE groupName = :groupName AND member = :member",
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::addLinkConfig(LinkConfig* cfg)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::updateLinkConfig(LinkConfig* cfg)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::deleteLinkConfig(std::string source, std::string destination)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "DELETE FROM t_share_config WHERE source = :source AND destination = :destination",
                soci::use(source), soci::use(destination);
            sql << "DELETE FROM t_link_config WHERE source = :source AND destination = :destination",
                soci::use(source), soci::use(destination);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



LinkConfig* OracleAPI::getLinkConfig(std::string source, std::string destination)
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return lnk;
}



std::pair<std::string, std::string>* OracleAPI::getSourceAndDestination(std::string symbolic_name)
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
            if(pair)
                delete pair;
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            if(pair)
                delete pair;
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return pair;
}



bool OracleAPI::isGrInPair(std::string group)
{
    soci::session sql(*connectionPool);

    bool inPair = false;
    try
        {
            std::string symbolicName;
            sql << "SELECT symbolicName FROM t_link_config WHERE "
                "  ((source = :groupName AND destination <> '*') OR "
                "  (source <> '*' AND destination = :groupName))",
                soci::use(group, "groupName"), soci::into(symbolicName);
            inPair = sql.got_data();
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return inPair;
}



void OracleAPI::addShareConfig(ShareConfig* cfg)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::updateShareConfig(ShareConfig* cfg)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::deleteShareConfig(std::string source, std::string destination, std::string vo)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::deleteShareConfig(std::string source, std::string destination)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



ShareConfig* OracleAPI::getShareConfig(std::string source, std::string destination, std::string vo)
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
            if(cfg)
                delete cfg;
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            if(cfg)
                delete cfg;
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return cfg;
}



std::vector<ShareConfig*> OracleAPI::getShareConfig(std::string source, std::string destination)
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return cfg;
}


void OracleAPI::addActivityConfig(std::string vo, std::string shares, bool active)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            const std::string act = active ? "on" : "off";

            sql << "INSERT INTO t_activity_share_config (vo, activity_share, active) "
                "                    VALUES (:vo, :share, :active)",
                soci::use(vo),
                soci::use(shares),
                soci::use(act)
                ;

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::updateActivityConfig(std::string vo, std::string shares, bool active)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            const std::string act = active ? "on" : "off";

            sql <<
                " UPDATE t_activity_share_config "
                " SET activity_share = :share, active = :active "
                " WHERE vo = :vo",
                soci::use(shares),
                soci::use(act),
                soci::use(vo)
                ;

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::deleteActivityConfig(std::string vo)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "DELETE FROM t_activity_share_config WHERE vo = :vo ",
                soci::use(vo);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

bool OracleAPI::isActivityConfigActive(std::string vo)
{
    soci::session sql(*connectionPool);

    std::string active;

    try
        {
            sql << "SELECT active FROM t_activity_share_config WHERE vo = :vo ",
                soci::use(vo),
                soci::into(active)
                ;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return active == "on";
}

std::map< std::string, double > OracleAPI::getActivityConfig(std::string vo)
{
    soci::session sql(*connectionPool);
    try
        {
            return getActivityShareConf(sql, vo);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



/*for session reuse check only*/
bool OracleAPI::isFileReadyStateV(std::map<int, std::string>& fileIds)
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return isReady;
}


/*
    we need to check if a member already belongs to another group
    true: it is member of another group
    false: it is not member of another group
 */
bool OracleAPI::checkIfSeIsMemberOfAnotherGroup(const std::string & member)
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return isMember;
}



void OracleAPI::addFileShareConfig(int file_id, std::string source, std::string destination, std::string vo)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << " MERGE INTO t_file_share_config USING "
                "    (SELECT :fileId as fileId, :source as source, :destination as destination, :vo as vo From dual) Config "
                " ON (t_file_share_config.file_id = Config.fileId AND t_file_share_config.source = Config.source AND "
                "     t_file_share_config.destination = Config.destination AND t_file_share_config.vo = Config.vo) "
                " WHEN NOT MATCHED THEN INSERT (file_id, source, destination, vo) VALUES "
                "                              (Config.fileId, Config.source, Config.destination, Config.vo)",
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



int OracleAPI::countActiveTransfers(std::string source, std::string destination, std::string vo)
{
    soci::session sql(*connectionPool);

    int nActive = 0;
    try
        {
            sql << "SELECT COUNT(*) FROM t_file, t_file_share_config "
                "WHERE t_file.file_state in ('ACTIVE','READY')  AND "
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return nActive;
}



int OracleAPI::countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo)
{
    soci::session sql(*connectionPool);

    int nActiveOutbound = 0;
    try
        {
            sql << "SELECT COUNT(*) FROM t_file, t_file_share_config "
                "WHERE t_file.file_state in ('ACTIVE','READY')  AND "
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return nActiveOutbound;
}



int OracleAPI::countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo)
{
    soci::session sql(*connectionPool);

    int nActiveInbound = 0;
    try
        {
            sql << "SELECT COUNT(*) FROM t_file, t_file_share_config "
                "WHERE t_file.file_state in ('ACTIVE','READY') AND "
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return nActiveInbound;
}

int OracleAPI::sumUpVoShares (std::string source, std::string destination, std::set<std::string> vos)
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
                        "   AND destination = :destination "
                        "   AND vo = :vo ",
                        soci::use(source),
                        soci::use(destination),
                        soci::use(*remove)
                        ;

                    if (!sql.got_data() && *remove != "public")
                        {
                            // if there is no configuration for this VO replace it with 'public'
                            vos.erase(remove);
                            vos.insert("public");
                        }
                }

            std::string vos_str = "(";

            for (it = vos.begin(); it != vos.end(); ++it)
                {

                    vos_str += "'" + *it + "'" + ",";
                }

            // replace the last ',' with closing ')'
            vos_str[vos_str.size() - 1] = ')';

            soci::indicator isNull = soci::i_ok;

            sql <<
                " SELECT SUM(active) "
                " FROM t_share_config "
                " WHERE source = :source "
                "   AND destination = :destination "
                "   AND vo IN " + vos_str,
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return sum;
}


void OracleAPI::setPriority(std::string job_id, int priority)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}


void OracleAPI::setRetry(int retry)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



int OracleAPI::getRetry(const std::string & jobId)
{
    soci::session sql(*connectionPool);

    int nRetries = 0;
    soci::indicator isNull = soci::i_ok;

    try
        {

            sql <<
                " SELECT retry "
                " FROM t_job "
                " WHERE job_id = :jobId ",
                soci::use(jobId),
                soci::into(nRetries, isNull)
                ;

            if (isNull != soci::i_null && nRetries == 0)
                {
                    sql <<
                        " SELECT retry FROM (SELECT rownum as rn, retry "
                        "  FROM t_server_config) WHERE rn = 1",
                        soci::into(nRetries)
                        ;
                }
            else if (isNull != soci::i_null && nRetries < 0)
                {
                    nRetries = 0;
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return nRetries;
}



int OracleAPI::getRetryTimes(const std::string & jobId, int fileId)
{
    soci::session sql(*connectionPool);

    int nRetries = 0;
    soci::indicator isNull = soci::i_ok;

    try
        {
            sql << "SELECT retry FROM t_file WHERE file_id = :fileId AND job_id = :jobId ",
                soci::use(fileId), soci::use(jobId), soci::into(nRetries, isNull);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return nRetries;
}



int OracleAPI::getMaxTimeInQueue()
{
    soci::session sql(*connectionPool);

    int maxTime = 0;
    try
        {
            soci::indicator isNull = soci::i_ok;

            sql << "SELECT max_time_queue FROM (SELECT rownum as rn, max_time_queue FROM t_server_config) WHERE rn = 1",
                soci::into(maxTime, isNull);

            //just in case soci it is reseting the value to NULL
            if(isNull != soci::i_null && maxTime > 0)
                return maxTime;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return maxTime;
}



void OracleAPI::setMaxTimeInQueue(int afterXHours)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::setToFailOldQueuedJobs(std::vector<std::string>& jobs)
{
    const static std::string message = "Job has been canceled because it stayed in the queue for too long";

    int maxTime = 0;

    try
        {
            maxTime = getMaxTimeInQueue();
            if (maxTime == 0)
                return;
        }
    catch (std::exception& ex)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + ex.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }


    // Acquire the session afet calling getMaxTimeInQueue to avoid
    // deadlocks (sql acquired and getMaxTimeInQueue locked
    // waiting for a session we have)
    soci::session sql(*connectionPool);

    try
        {
            struct message_sanity msg;
            msg.setToFailOldQueuedJobs = true;
            CleanUpSanityChecks temp(this, sql, msg);
            if(!temp.getCleanUpSanityCheck())
                return;

            sql.begin();
            soci::rowset<std::string> rs = (sql.prepare << "SELECT job_id FROM t_job WHERE "
                                            "    submit_time < (sys_extract_utc(systimestamp) - interval :interval hour) AND "
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

                    jobs.push_back(*i);
                }
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}


std::vector< std::pair<std::string, std::string> > OracleAPI::getPairsForSe(std::string se)
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
                                             "  or (source <> '*' and destination = :dest) ",
                                             soci::use(se),
                                             soci::use(se)
                                         );

            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    ret.push_back(
                        make_pair(i->get<std::string>("SOURCE"), i->get<std::string>("DESTINATION"))
                    );
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ret;
}


std::vector<std::string> OracleAPI::getAllStandAlloneCfgs()
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ret;
}

std::vector< std::pair<std::string, std::string> > OracleAPI::getAllPairCfgs()
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
                        std::make_pair(row.get<std::string>("SOURCE"), row.get<std::string>("DESTINATION"))
                    );
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ret;
}

int OracleAPI::activeProcessesForThisHost()
{

    soci::session sql(*connectionPool);

    unsigned active = 0;
    try
        {
            sql << "select count(*) from t_file where TRANSFERHOST=:host AND file_state in ('READY','ACTIVE')  ", soci::use(hostname), soci::into(active);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return active;
}


std::vector< boost::tuple<std::string, std::string, int> >  OracleAPI::getVOBringonlineMax()
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
                        row.get<std::string>("VO_NAME"),
                        row.get<std::string>("HOST"),
                        row.get<int>("CONCURRENT_OPS",0)
                    );

                    ret.push_back(item);
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ret;

}

std::vector<message_bringonline> OracleAPI::getBringOnlineFiles(std::string voName, std::string hostName, int maxValue)
{

    soci::session sql(*connectionPool);

    std::vector<message_bringonline> ret;

    try
        {

            if (voName.empty())
                {

                    soci::rowset<soci::row> rs = (
                                                     sql.prepare <<
                                                     " SELECT distinct(f.source_se) "
                                                     " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                                     " WHERE "
                                                     "  (j.BRING_ONLINE > 0 OR j.COPY_PIN_LIFETIME > 0) "
                                                     "  AND f.file_state = 'STAGING' "
                                                     "  AND f.staging_start IS NULL and f.source_surl like 'srm%' and j.job_finished is null "
                                                     "  AND (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd)",
                                                     soci::use(hashSegment.start), soci::use(hashSegment.end)
                                                 );

                    for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                        {
                            soci::row const& row = *i;

                            std::string hostV = row.get<std::string>("SOURCE_SE");

                            unsigned int currentStagingFilesNoConfig = 0;

                            sql <<
                                " SELECT COUNT(*) "
                                " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                " WHERE "
                                "       (j.BRING_ONLINE > 0 OR j.COPY_PIN_LIFETIME > 0) "
                                "   AND f.file_state = 'STAGING' "
                                "   AND f.staging_start IS NOT NULL "
                                "   AND f.source_se = :hostV  and f.source_surl like 'srm%' and j.job_finished is null",
                                soci::use(hostV),
                                soci::into(currentStagingFilesNoConfig)
                                ;

                            unsigned int maxNoConfig = currentStagingFilesNoConfig > 0 ? maxValue - currentStagingFilesNoConfig : maxValue;

                            soci::rowset<soci::row> rs2 = (
                                                              sql.prepare <<
                                                              " SELECT * FROM ("
                                                              " SELECT rownum as rn, f.source_surl, f.job_id, f.file_id, j.copy_pin_lifetime, j.bring_online "
                                                              " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                                              " WHERE  "
                                                              " (j.BRING_ONLINE > 0 OR j.COPY_PIN_LIFETIME > 0) "
                                                              " AND f.staging_start IS NULL "
                                                              " AND f.file_state = 'STAGING' "
                                                              " AND f.source_se = :source_se and f.source_surl like 'srm%'   "
                                                              " AND j.job_finished is null "
                                                              " ) WHERE rn <= :limit",
                                                              soci::use(hostV),
                                                              soci::use(maxNoConfig)

                                                          );

                            for (soci::rowset<soci::row>::const_iterator i2 = rs2.begin(); i2 != rs2.end(); ++i2)
                                {
                                    soci::row const& row2 = *i2;

                                    struct message_bringonline msg;
                                    msg.url = row2.get<std::string>("SOURCE_SURL");
                                    msg.job_id = row2.get<std::string>("JOB_ID");
                                    msg.file_id = static_cast<int>(row2.get<long long>("FILE_ID"));
                                    msg.pinlifetime = static_cast<int>(row2.get<double>("COPY_PIN_LIFETIME"));
                                    msg.bringonlineTimeout = static_cast<int>(row2.get<double>("BRING_ONLINE"));

                                    ret.push_back(msg);
                                    bringOnlineReportStatus("STARTED", "", msg);
                                }
                        }
                }
            else
                {

                    unsigned currentStagingFilesConfig = 0;

                    sql <<
                        " SELECT COUNT(*) FROM t_job j INNER JOIN t_file f ON (j.job_id = f.job_id) "
                        " WHERE  "
                        "   (j.BRING_ONLINE > 0 OR j.COPY_PIN_LIFETIME > 0) "
                        "   AND f.file_state = 'STAGING' "
                        "   AND f.STAGING_START IS NOT NULL "
                        "   AND j.vo_name = :vo_name "
                        "   AND f.source_se = :source_se and f.source_surl like 'srm%'   ",
                        soci::use(voName),
                        soci::use(hostName),
                        soci::into(currentStagingFilesConfig)
                        ;

                    unsigned int maxConfig = currentStagingFilesConfig > 0 ? maxValue - currentStagingFilesConfig : maxValue;

                    soci::rowset<soci::row> rs = (
                                                     sql.prepare <<
                                                     " SELECT * FROM ("
                                                     " SELECT rownum as rn, f.source_surl, f.job_id, f.file_id, j.copy_pin_lifetime, j.bring_online "
                                                     " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                                     " WHERE  "
                                                     "  (j.BRING_ONLINE > 0 OR j.COPY_PIN_LIFETIME > 0) "
                                                     "  AND f.staging_START IS NULL "
                                                     "  AND f.file_state = 'STAGING' "
                                                     "  AND f.source_se = :source_se "
                                                     "  AND j.vo_name = :vo_name and f.source_surl like 'srm%'   "
                                                     "  AND j.job_finished is null"
                                                     ") WHERE rn <= :limit",
                                                     soci::use(hostName),
                                                     soci::use(voName),
                                                     soci::use(maxConfig)
                                                 );

                    for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                        {
                            soci::row const& row = *i;

                            struct message_bringonline msg;
                            msg.url = row.get<std::string>("SOURCE_SURL");
                            msg.job_id = row.get<std::string>("JOB_ID");
                            msg.file_id = static_cast<int>(row.get<long long>("FILE_ID"));
                            msg.pinlifetime = static_cast<int>(row.get<double>("COPY_PIN_LIFETIME"));
                            msg.bringonlineTimeout = static_cast<int>(row.get<double>("BRING_ONLINE"));

                            ret.push_back(msg);
                            bringOnlineReportStatus("STARTED", "", msg);
                        }
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ret;
}

void OracleAPI::bringOnlineReportStatus(const std::string & state, const std::string & message, struct message_bringonline msg)
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
                        " SET staging_start = sys_extract_utc(systimestamp), transferhost=:thost "
                        " WHERE job_id = :jobId "
                        "   AND file_id= :fileId "
                        "   AND file_state='STAGING'",
                        soci::use(hostname),
                        soci::use(msg.job_id),
                        soci::use(msg.file_id)
                        ;
                    sql.commit();
                }
            else
                {
                    std::string source_surl;
                    std::string dest_surl;
                    std::string dbState;
                    std::string dbReason;
                    int stage_in_only = 0;

                    sql << "select count(*) from t_file where job_id=:job_id and file_id=:file_id and source_surl=dest_surl",
                        soci::use(msg.job_id),
                        soci::use(msg.file_id),
                        soci::into(stage_in_only);

                    if(stage_in_only == 0)  //stage-in and transfer
                        {
                            dbState = state == "FINISHED" ? "SUBMITTED" : state;
                            dbReason = state == "FINISHED" ? std::string() : message;
                        }
                    else //stage-in only
                        {
                            dbState = state == "FINISHED" ? "FINISHED" : state;
                            dbReason = state == "FINISHED" ? std::string() : message;
                        }


                    sql.begin();
                    sql <<
                        " UPDATE t_file "
                        " SET staging_finished = sys_extract_utc(systimestamp), reason = :reason, file_state = :fileState "
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
                    soci::indicator isNull = soci::i_ok;
                    std::string reuse("");
                    sql << " select reuse_job from t_job where job_id=:jobId", soci::use(msg.job_id), soci::into(reuse, isNull);
                    if (isNull != soci::i_null && reuse == "Y")
                        {
                            int countTr = 0;
                            sql << " select count(*) from t_file where job_id=:jobId and file_state='STAGING' ", soci::use(msg.job_id), soci::into(countTr);
                            if(countTr == 0)
                                {
                                    if(stage_in_only == 0)
                                        {
                                            updateJobTransferStatusInternal(sql, msg.job_id, "SUBMITTED");
                                        }
                                    else
                                        {
                                            updateJobTransferStatusInternal(sql, msg.job_id, dbState);
                                        }
                                }
                        }
                    else
                        {
                            updateJobTransferStatusInternal(sql, msg.job_id, dbState);
                        }
                }
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::addToken(const std::string & job_id, int file_id, const std::string & token)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql <<
                " UPDATE t_file "
                " SET bringonline_token = :token "
                " WHERE job_id = :jobId "
                "   AND file_id = :fileId "
                "   AND file_state = 'STAGING' ",
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}


void OracleAPI::getCredentials(std::string & vo_name, const std::string & job_id, int, std::string & dn, std::string & dlg_id)
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::setMaxStageOp(const std::string& se, const std::string& vo, int val)
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::updateProtocol(const std::string& /*jobId*/, int fileId, int nostreams, int timeout, int buffersize, double filesize)
{

    std::stringstream internalParams;
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            internalParams << "nostreams:" << nostreams << ",timeout:" << timeout << ",buffersize:" << buffersize;

            sql <<
                " UPDATE t_file set INTERNAL_FILE_PARAMS=:1, FILESIZE=:2 where file_id=:fileId ",
                soci::use(internalParams.str()),
                soci::use(filesize),
                soci::use(fileId);

            sql.commit();

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

double OracleAPI::getSuccessRate(std::string source, std::string destination)
{
    soci::session sql(*connectionPool);

    double ratioSuccessFailure = 0;

    try
        {
            double nFailedLastHour=0, nFinishedLastHour=0;

            soci::rowset<std::string> rs = (
                                               sql.prepare <<
                                               "SELECT file_state FROM t_file "
                                               "WHERE "
                                               "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                                               "      (t_file.job_finished > (sys_extract_utc(systimestamp) - interval '15' minute)) AND "
                                               "      file_state IN ('FAILED','FINISHED') ",
                                               soci::use(source), soci::use(destination)
                                           )
                                           ;

            for (soci::rowset<std::string>::const_iterator i = rs.begin();
                    i != rs.end(); ++i)
                {
                    if      (i->compare("FAILED") == 0)   ++nFailedLastHour;
                    else if (i->compare("FINISHED") == 0) ++nFinishedLastHour;
                }

            if(nFinishedLastHour > 0)
                {
                    ratioSuccessFailure = nFinishedLastHour/(nFinishedLastHour + nFailedLastHour) * (100.0/1.0);
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ratioSuccessFailure;
}

double OracleAPI::getAvgThroughput(std::string source_hostname, std::string destin_hostname)
{
    soci::session sql(*connectionPool);

    double throughput=0.0;
    double filesize = 0.0;
    double totalSize = 0.0;

    try
        {
            // Weighted average for the 5 newest transfers
            soci::rowset<soci::row>  rsSizeAndThroughput = (sql.prepare <<
                    " SELECT * FROM ("
                    " SELECT rownum as rn, filesize, throughput "
                    " FROM t_file "
                    " WHERE source_se = :source AND dest_se = :dest AND "
                    "       file_state IN ('ACTIVE','FINISHED') AND throughput > 0 AND "
                    "       filesize > 0  AND "
                    "       (start_time >= (sys_extract_utc(systimestamp) - interval '5' minute) OR "
                    "        job_finished >= (sys_extract_utc(systimestamp) - interval '5' minute)) "
                    " ORDER BY job_finished DESC)"
                    " WHERE rn <= 5 ",
                    soci::use(source_hostname),soci::use(destin_hostname));

            for (soci::rowset<soci::row>::const_iterator j = rsSizeAndThroughput.begin();
                    j != rsSizeAndThroughput.end(); ++j)
                {
                    filesize    = static_cast<double>(j->get<long long>("FILESIZE", 0));
                    throughput += (j->get<double>("THROUGHPUT", 0) * filesize);
                    totalSize  += filesize;
                }
            if (totalSize > 0)
                throughput /= totalSize;


        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return throughput;
}

void OracleAPI::cancelFilesInTheQueue(const std::string& se, const std::string& vo, std::set<std::string>& jobs)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::rowset<soci::row> rs = vo.empty() ? (
                                             sql.prepare <<
                                             " SELECT file_id, job_id, file_index "
                                             " FROM t_file "
                                             " WHERE (source_se = :se OR dest_se = :se) "
                                             "  AND file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED')",
                                             soci::use(se),
                                             soci::use(se)
                                         )
                                         :
                                         (
                                             sql.prepare <<
                                             " SELECT f.file_id, f.job_id, file_index "
                                             " FROM t_file f "
                                             " WHERE  (f.source_se = :se OR f.dest_se = :se) "
                                             "  AND f.vo_name = :vo "
                                             "  AND f.file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') ",
                                             soci::use(se),
                                             soci::use(se),
                                             soci::use(vo)
                                         );

            sql.begin();

            soci::rowset<soci::row>::const_iterator it;
            for (it = rs.begin(); it != rs.end(); ++it)
                {

                    std::string jobId = it->get<std::string>("JOB_ID");
                    int fileId = static_cast<int>(it->get<long long>("FILE_ID"));

                    jobs.insert(jobId);

                    sql <<
                        " UPDATE t_file "
                        " SET file_state = 'CANCELED' "
                        " WHERE file_id = :fileId",
                        soci::use(fileId)
                        ;

                    int fileIndex = static_cast<int>(it->get<long long>("FILE_INDEX"));

                    sql <<
                        " UPDATE t_file "
                        " SET file_state = 'SUBMITTED' "
                        " WHERE file_state = 'NOT_USED' "
                        "   AND job_id = :jobId "
                        "   AND file_index = :fileIndex "
                        "   AND NOT EXISTS ( "
                        "       SELECT NULL "
                        "       FROM t_file "
                        "       WHERE job_id = :jobId "
                        "           AND file_index = :fileIndex "
                        "           AND file_state NOT IN ('NOT_USED', 'FAILED', 'CANCELED') "
                        "   ) ",
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
                    updateJobTransferStatusInternal(sql, *job_it, std::string());
                }

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::cancelJobsInTheQueue(const std::string& dn, std::vector<std::string>& jobs)
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
                                             "  AND job_state IN ('ACTIVE', 'READY', 'SUBMITTED')",
                                             soci::use(dn)
                                         );

            soci::rowset<soci::row>::const_iterator it;
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::transferLogFileVector(std::map<int, struct message_log>& messagesLog)
{
    soci::session sql(*connectionPool);

    std::string filePath;
    //soci doesn't access bool
    unsigned int debugFile = 0;
    int fileId = 0;

    try
        {
            soci::statement stmt = (sql.prepare << " update t_file set t_log_file=:filePath, t_log_file_debug=:debugFile where file_id=:fileId and file_state<>'SUBMITTED' ",
                                    soci::use(filePath),
                                    soci::use(debugFile),
                                    soci::use(fileId));

            sql.begin();

            std::map<int, struct message_log>::const_iterator iterLog;
            for (iterLog = messagesLog.begin(); iterLog != messagesLog.end(); ++iterLog)
                {
                    filePath = ((*iterLog).second).filePath;
                    fileId = ((*iterLog).second).file_id;
                    debugFile = ((*iterLog).second).debugFile;
                    stmt.execute(true);
                }

            sql.commit();

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
}


std::vector<struct message_state> OracleAPI::getStateOfTransfer(const std::string& jobId, int fileId)
{
    soci::session sql(*connectionPool);

    message_state ret;
    soci::indicator ind = soci::i_ok;
    std::vector<struct message_state> temp;
    int retry = 0;

    try
        {
            sql << "SELECT retry FROM "
                " (SELECT rownum as rn, retry FROM t_server_config) "
                "WHERE rn = 1", soci::into(retry, ind);

            soci::rowset<soci::row> rs = (fileId ==-1) ? (
                                             sql.prepare <<
                                             " SELECT "
                                             "	j.job_id, j.job_state, j.vo_name, "
                                             "	j.job_metadata, j.retry AS retry_max, f.file_id, "
                                             "	f.file_state, f.retry AS retry_counter, f.file_metadata, "
                                             "	f.source_se, f.dest_se "
                                             " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                             " WHERE "
                                             " 	j.job_id = :jobId ",
                                             soci::use(jobId)
                                         )
                                         :
                                         (
                                             sql.prepare <<
                                             " SELECT "
                                             "	j.job_id, j.job_state, j.vo_name, "
                                             "	j.job_metadata, j.retry AS retry_max, f.file_id, "
                                             "	f.file_state, f.retry AS retry_counter, f.file_metadata, "
                                             "	f.source_se, f.dest_se "
                                             " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                             " WHERE "
                                             " 	j.job_id = :jobId "
                                             "  AND f.file_id = :fileId ",
                                             soci::use(jobId),
                                             soci::use(fileId)
                                         );


            soci::rowset<soci::row>::const_iterator it;
            for (it = rs.begin(); it != rs.end(); ++it)
                {
                    ret.job_id = it->get<std::string>("JOB_ID");
                    ret.job_state = it->get<std::string>("JOB_STATE");
                    ret.vo_name = it->get<std::string>("VO_NAME");
                    ret.job_metadata = it->get<std::string>("JOB_METADATA","");
                    ret.retry_max = static_cast<int>(it->get<long long>("RETRY_MAX", 0));
                    ret.file_id = static_cast<int>(it->get<long long>("FILE_ID"));
                    ret.file_state = it->get<std::string>("FILE_STATE");
                    ret.retry_counter = static_cast<int>(it->get<double>("RETRY_COUNTER",0));
                    ret.file_metadata = it->get<std::string>("FILE_METADATA","");
                    ret.source_se = it->get<std::string>("SOURCE_SE");
                    ret.dest_se = it->get<std::string>("DEST_SE");
                    ret.timestamp = getStrUTCTimestamp();

                    if(ret.retry_max == 0)
                        {
                            ret.retry_max = retry;
                        }

                    temp.push_back(ret);
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return temp;
}

void OracleAPI::getFilesForJob(const std::string& jobId, std::vector<int>& files)
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

            soci::rowset<soci::row>::const_iterator it;
            for (it = rs.begin(); it != rs.end(); ++it)
                {
                    files.push_back(
                        static_cast<int>(it->get<long long>("FILE_ID"))
                    );
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::getFilesForJobInCancelState(const std::string& jobId, std::vector<int>& files)
{
    soci::session sql(*connectionPool);

    try
        {

            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT file_id "
                                             " FROM t_file "
                                             " WHERE job_id = :jobId "
                                             "  AND file_state = 'CANCELED' ",
                                             soci::use(jobId)
                                         );

            soci::rowset<soci::row>::const_iterator it;
            for (it = rs.begin(); it != rs.end(); ++it)
                {
                    files.push_back(
                        static_cast<int>(it->get<long long>("FILE_ID"))
                    );
                }

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}


void OracleAPI::setFilesToWaiting(const std::string& se, const std::string& vo, int timeout)
{
    soci::session sql(*connectionPool);

    try
        {

            sql.begin();

            if (vo.empty())
                {
                    sql <<
                        " UPDATE t_file "
                        " SET wait_timestamp = sys_extract_utc(systimestamp), wait_timeout = :timeout "
                        " WHERE (source_se = :src OR dest_se = :dest) "
                        "   AND file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
                        "   AND (wait_timestamp IS NULL OR wait_timeout IS NULL) ",
                        soci::use(timeout),
                        soci::use(se),
                        soci::use(se)
                        ;
                }
            else
                {
                    sql <<
                        " UPDATE t_file "
                        " SET wait_timestamp = sys_extract_utc(systimestamp), wait_timeout = :timeout "
                        " WHERE (source_se = :src OR dest_se = :dest) "
                        "   AND vo_name = : vo "
                        "   AND file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
                        "   AND (wait_timestamp IS NULL OR wait_timeout IS NULL) ",
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::setFilesToWaiting(const std::string& dn, int timeout)
{
    soci::session sql(*connectionPool);

    try
        {

            sql.begin();

            sql <<
                " UPDATE (SELECT t_file.wait_timestamp AS wait_timestamp, t_file.wait_timeout AS wait_timeout "
                "         FROM t_file INNER JOIN t_job ON t_file.job_id = t_job.job_id "
                "         WHERE t_job.user_dn = :dn AND "
                "               t_file.file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED') "
                "               AND (t_file.wait_timestamp IS NULL or t_file.wait_timeout IS NULL) "
                " ) SET wait_timestamp = sys_extract_utc(systimestamp), wait_timeout = :timeout",
                soci::use(dn), soci::use(timeout)
                ;

            sql.commit();

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::cancelWaitingFiles(std::set<std::string>& jobs)
{

    soci::session sql(*connectionPool);

    try
        {

            struct message_sanity msg;
            msg.cancelWaitingFiles = true;
            CleanUpSanityChecks temp(this, sql, msg);
            if(!temp.getCleanUpSanityCheck())
                return;

            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT file_id, job_id "
                                             " FROM t_file "
                                             " WHERE wait_timeout <> 0 "
                                             "  AND (sys_extract_utc(systimestamp) - wait_timestamp) > numtodsinterval(wait_timeout, 'second') "
                                             "  AND file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED')"
                                         );

            soci::rowset<soci::row>::iterator it;

            sql.begin();
            for (it = rs.begin(); it != rs.end(); ++it)
                {

                    jobs.insert(it->get<std::string>("JOB_ID"));

                    sql <<
                        " UPDATE t_file "
                        " SET file_state = 'CANCELED' "
                        " WHERE file_id = :fileId AND file_state IN ('ACTIVE', 'READY', 'SUBMITTED', 'NOT_USED')",
                        soci::use(static_cast<int>(it->get<long long>("FILE_ID")))
                        ;
                }

            sql.commit();

            std::set<std::string>::iterator job_it;
            for (job_it = jobs.begin(); job_it != jobs.end(); ++job_it)
                {
                    updateJobTransferStatusInternal(sql, *job_it, std::string());
                }
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::revertNotUsedFiles()
{

    soci::session sql(*connectionPool);
    std::string notUsed;

    try
        {
            struct message_sanity msg;
            msg.revertNotUsedFiles = true;
            CleanUpSanityChecks temp(this, sql, msg);
            if(!temp.getCleanUpSanityCheck())
                return;


            soci::rowset<std::string> rs = (
                                               sql.prepare <<
                                               "select distinct f.job_id from t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                               " WHERE file_state = 'NOT_USED' and j.job_finished is NULL"
                                           );
            sql.begin();

            for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    notUsed = *i;

                    sql <<
                        " UPDATE t_file f1 "
                        " SET f1.file_state = 'SUBMITTED' "
                        " WHERE f1.file_state = 'NOT_USED' "
                        "   AND NOT EXISTS ( "
                        "       SELECT NULL "
                        "       FROM ( "
                        "           SELECT job_id, file_index, file_state "
                        "           FROM t_file f2 "
                        "           WHERE f2.job_id = :notUsed AND EXISTS ( "
                        "                   SELECT NULL "
                        "                   FROM t_file f3 "
                        "                   WHERE f2.job_id = f3.job_id "
                        "                       AND f3.file_index = f2.file_index "
                        "                       AND f3.file_state = 'NOT_USED' "
                        "               ) "
                        "               AND f2.file_state <> 'NOT_USED' "
                        "               AND f2.file_state <> 'CANCELED' "
                        "               AND f2.file_state <> 'FAILED' "
                        "       ) AS t_file_tmp "
                        "       WHERE t_file_tmp.job_id = f1.job_id "
                        "           AND t_file_tmp.file_index = f1.file_index AND   "
                        "   ) ", soci::use(notUsed)
                        ;

                }
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

bool OracleAPI::isShareOnly(std::string se)
{

    soci::session sql(*connectionPool);

    bool ret = true;
    try
        {
            std::string symbolicName;
            sql <<
                " select symbolicName from t_link_config "
                " where source = :source and destination = '*' and auto_tuning = 'all'",
                soci::use(se), soci::into(symbolicName);
            ret = sql.got_data();
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ret;
}

std::vector<std::string> OracleAPI::getAllShareOnlyCfgs()
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return ret;
}

void OracleAPI::checkSanityState()
{
    //TERMINAL STATES:  "FINISHED" FAILED" "CANCELED"
    soci::session sql(*connectionPool);

    unsigned int numberOfFiles = 0;
    unsigned int terminalState = 0;
    unsigned int allFinished = 0;
    unsigned int allFailed = 0;
    unsigned int allCanceled = 0;
    unsigned int numberOfFilesRevert = 0;
    std::string canceledMessage = "Transfer canceled by the user";
    std::string failed = "One or more files failed. Please have a look at the details for more information";


    try
        {
            struct message_sanity msg;
            msg.checkSanityState = true;
            CleanUpSanityChecks temp(this, sql, msg);
            if(!temp.getCleanUpSanityCheck())
                {
                    return;
                }

            sql.begin();

            soci::rowset<std::string> rs = (
                                               sql.prepare <<
                                               " select job_id from t_job where job_finished is null "
                                           );

            for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    sql << "SELECT COUNT(DISTINCT file_index) FROM t_file where job_id=:jobId ", soci::use(*i), soci::into(numberOfFiles);

                    if(numberOfFiles > 0)
                        {
                            countFileInTerminalStates(sql, *i, allFinished, allCanceled, allFailed);
                            terminalState = allFinished + allCanceled + allFailed;

                            if(numberOfFiles == terminalState)  /* all files terminal state but job in ('ACTIVE','READY','SUBMITTED','STAGING') */
                                {
                                    if(allCanceled > 0)
                                        {

                                            sql << "UPDATE t_job SET "
                                                "    job_state = 'CANCELED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), "
                                                "    reason = :canceledMessage "
                                                "    WHERE job_id = :jobId ", soci::use(canceledMessage), soci::use(*i);

                                        }
                                    else   //non canceled, check other states: "FINISHED" and FAILED"
                                        {
                                            if(numberOfFiles == allFinished)  /*all files finished*/
                                                {

                                                    sql << "UPDATE t_job SET "
                                                        "    job_state = 'FINISHED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp) "
                                                        "    WHERE job_id = :jobId", soci::use(*i);

                                                }
                                            else
                                                {
                                                    if(numberOfFiles == allFailed)  /*all files failed*/
                                                        {

                                                            sql << "UPDATE t_job SET "
                                                                "    job_state = 'FAILED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), "
                                                                "    reason = :failed "
                                                                "    WHERE job_id = :jobId", soci::use(failed), soci::use(*i);

                                                        }
                                                    else   // otherwise it is FINISHEDDIRTY
                                                        {

                                                            sql << "UPDATE t_job SET "
                                                                "    job_state = 'FINISHEDDIRTY', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), "
                                                                "    reason = :failed "
                                                                "    WHERE job_id = :jobId", soci::use(failed), soci::use(*i);

                                                        }
                                                }
                                        }
                                }
                        }
                    //reset
                    numberOfFiles = 0;
                }
            sql.commit();

            sql.begin();

            //now check reverse sanity checks, JOB can't be FINISH,  FINISHEDDIRTY, FAILED is at least one tr is in SUBMITTED, READY, ACTIVE
            soci::rowset<std::string> rs2 = (
                                                sql.prepare <<
                                                " select job_id from t_job where job_finished IS NOT NULL "
                                            );

            for (soci::rowset<std::string>::const_iterator i2 = rs2.begin(); i2 != rs2.end(); ++i2)
                {
                    sql << "SELECT COUNT(*) FROM t_file where job_id=:jobId AND file_state in ('ACTIVE','READY','SUBMITTED','STAGING') ", soci::use(*i2), soci::into(numberOfFilesRevert);
                    if(numberOfFilesRevert > 0)
                        {

                            sql << "UPDATE t_job SET "
                                "    job_state = 'SUBMITTED', job_finished = NULL, finish_time = NULL, "
                                "    reason = NULL "
                                "    WHERE job_id = :jobId", soci::use(*i2);

                        }
                    //reset
                    numberOfFilesRevert = 0;
                }

            sql.commit();

        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::countFileInTerminalStates(soci::session& sql, std::string jobId,
        unsigned int& finished, unsigned int& canceled, unsigned int& failed)
{
    try
        {
            sql <<
                " select count(*)  "
                " from t_file "
                " where job_id = :jobId "
                "   and  file_state = 'FINISHED' ",
                soci::use(jobId),
                soci::into(finished)
                ;

            sql <<
                " select count(distinct f1.file_index) "
                " from t_file f1 "
                " where f1.job_id = :jobId "
                "   and f1.file_state = 'CANCELED' "
                "   and NOT EXISTS ( "
                "       select null "
                "       from t_file f2 "
                "       where f2.job_id = :jobId "
                "           and f1.file_index = f2.file_index "
                "           and f2.file_state <> 'CANCELED' "
                "   ) ",
                soci::use(jobId),
                soci::use(jobId),
                soci::into(canceled)
                ;

            sql <<
                " select count(distinct f1.file_index) "
                " from t_file f1 "
                " where f1.job_id = :jobId "
                "   and f1.file_state = 'FAILED' "
                "   and NOT EXISTS ( "
                "       select null "
                "       from t_file f2 "
                "       where f2.job_id = :jobId "
                "           and f1.file_index = f2.file_index "
                "           and f2.file_state NOT IN ('CANCELED', 'FAILED') "
                "   ) ",
                soci::use(jobId),
                soci::use(jobId),
                soci::into(failed)
                ;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}


void OracleAPI::getFilesForNewSeCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out)
{

    soci::session sql(*connectionPool);

    try
        {
            soci::rowset<int> rs = (
                                       sql.prepare <<
                                       " select f.file_id "
                                       " from t_file f  "
                                       " where f.source_se like :source "
                                       "    and f.dest_se like :destination "
                                       "    and f.vo_name = :vo "
                                       "    and f.file_state in ('READY', 'ACTIVE') ",
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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::getFilesForNewGrCfg(std::string source, std::string destination, std::string vo, std::vector<int>& out)
{
    soci::session sql(*connectionPool);

    std::string select;
    select +=
        " select f.file_id "
        " from t_file f "
        " where "
        "   f.vo_name = :vo "
        "   and f.file_state in ('READY', 'ACTIVE')  ";
    if (source != "*")
        select +=
            "   and f.source_se in ( "
            "       select member "
            "       from t_group_members "
            "       where groupName = :source "
            "   ) ";
    if (destination != "*")
        select +=
            "   and f.dest_se in ( "
            "       select member "
            "       from t_group_members "
            "       where groupName = :dest "
            "   ) ";

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
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}


void OracleAPI::delFileShareConfig(int file_id, std::string source, std::string destination, std::string vo)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql <<
                " delete from t_file_share_config  "
                " where file_id = :id "
                "   and source = :src "
                "   and destination = :dest "
                "   and vo = :vo",
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}


void OracleAPI::delFileShareConfig(std::string group, std::string se)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql <<
                " delete from t_file_share_config  "
                " where (source = :gr or destination = :gr) "
                "   and file_id IN ( "
                "       select file_id "
                "       from t_file "
                "       where (source_se = :se or dest_se = :se) "
                "           and file_state in ('READY', 'ACTIVE')"
                "   ) ",
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}


bool OracleAPI::hasStandAloneSeCfgAssigned(int file_id, std::string vo)
{
    soci::session sql(*connectionPool);

    int count = 0;

    try
        {
            sql <<
                " select count(*) "
                " from t_file_share_config fc "
                " where fc.file_id = :id "
                "   and fc.vo = :vo "
                "   and ((fc.source <> '(*)' and fc.destination = '*') or (fc.source = '*' and fc.destination <> '(*)')) "
                "   and not exists ( "
                "       select null "
                "       from t_group_members g "
                "       where (g.groupName = fc.source or g.groupName = fc.destination) "
                "   ) ",
                soci::use(file_id),
                soci::use(vo),
                soci::into(count)
                ;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return count > 0;
}

bool OracleAPI::hasPairSeCfgAssigned(int file_id, std::string vo)
{
    soci::session sql(*connectionPool);

    int count = 0;

    try
        {
            sql <<
                " select count(*) "
                " from t_file_share_config fc "
                " where fc.file_id = :id "
                "   and fc.vo = :vo "
                "   and (fc.source <> '(*)' and fc.source <> '*' and fc.destination <> '*' and fc.destination <> '(*)') "
                "   and not exists ( "
                "       select null "
                "       from t_group_members g "
                "       where (g.groupName = fc.source or g.groupName = fc.destination) "
                "   ) ",
                soci::use(file_id),
                soci::use(vo),
                soci::into(count)
                ;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return count > 0;
}



bool OracleAPI::hasPairGrCfgAssigned(int file_id, std::string vo)
{
    soci::session sql(*connectionPool);

    int count = 0;

    try
        {
            sql <<
                " select count(*) "
                " from t_file_share_config fc "
                " where fc.file_id = :id "
                "   and fc.vo = :vo "
                "   and (fc.source <> '(*)' and fc.source <> '*' and fc.destination <> '*' and fc.destination <> '(*)') "
                "   and exists ( "
                "       select null "
                "       from t_group_members g "
                "       where (g.groupName = fc.source or g.groupName = fc.destination) "
                "   ) ",
                soci::use(file_id),
                soci::use(vo),
                soci::into(count)
                ;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return count > 0;
}


void OracleAPI::checkSchemaLoaded()
{
    soci::session sql(*connectionPool);

    int count = 0;

    try
        {
            sql <<
                " select count(*) "
                " from t_debug",
                soci::into(count)
                ;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::storeProfiling(const fts3::ProfilingSubsystem* prof)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql << "UPDATE t_profiling_snapshot SET "
                "    cnt = 0, exceptions = 0, total = 0, average = 0";

            std::map<std::string, fts3::Profile>::const_iterator i;
            for (i = prof->profiles.begin(); i != prof->profiles.end(); ++i)
                {
                    sql << "MERGE INTO t_profiling_snapshot USING "
                        "    (SELECT :scope AS scope, :cnt as cnt, :exceptions as exceptions, :total as total, :avg as avg FROM dual) Profile"
                        " ON (t_profiling_snapshot.scope = Profile.scope) "
                        " WHEN     MATCHED THEN UPDATE SET t_profiling_snapshot.cnt = Profile.cnt,"
                        "                                  t_profiling_snapshot.exceptions = Profile.exceptions,"
                        "                                  t_profiling_snapshot.total = Profile.total,"
                        "                                  t_profiling_snapshot.average = Profile.avg "
                        " WHEN NOT MATCHED THEN INSERT (scope, cnt, exceptions, total, average) VALUES "
                        "                              (Profile.scope, Profile.cnt, Profile.exceptions, Profile.total, Profile.avg) ",
                        soci::use(i->second.nCalled, "cnt"), soci::use(i->second.nExceptions, "exceptions"),
                        soci::use(i->second.totalTime, "total"), soci::use(i->second.getAverage(), "avg"),
                        soci::use(i->first, "scope");
                }


            soci::statement update(sql);
            update.exchange(soci::use(prof->getInterval()));
            update.alloc();

            update.prepare("UPDATE t_profiling_info SET "
                           "    updated = sys_extract_utc(systimestamp), period = :period");

            update.define_and_bind();
            update.execute(true);

            if (update.get_affected_rows() == 0)
                {
                    sql << "INSERT INTO t_profiling_info (updated, period) "
                        "VALUES (sys_extract_utc(systimestamp), :period)",
                        soci::use(prof->getInterval());
                }

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

void OracleAPI::setOptimizerMode(int mode)
{

    soci::session sql(*connectionPool);
    int _mode = 0;

    try
        {
            sql << "SELECT COUNT(*) FROM t_optimize_mode", soci::into(_mode);
            if (_mode == 0)
                {
                    sql.begin();

                    sql << "INSERT INTO t_optimize_mode (mode_opt) VALUES (:mode_opt)",
                            soci::use(mode);

                    sql.commit();

                }
            else
                {
                    sql.begin();

                    sql << "UPDATE t_optimize_mode SET mode_opt = :mode_opt",
                        soci::use(mode);

                    sql.commit();
                }
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

int OracleAPI::getOptimizerMode(soci::session& sql)
{
    int mode = 0;
    soci::indicator ind = soci::i_ok;

    try
        {
            sql <<
                " SELECT mode_opt FROM"
                " (SELECT rownum as rn, mode_opt FROM t_optimize_mode)"
                " WHERE rn = 1",
                soci::into(mode, ind)
                ;
            if (ind == soci::i_ok)
                return mode;

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return mode;
}

void OracleAPI::setRetryTransfer(const std::string & jobId, int fileId, int retry, const std::string& reason)
{
    soci::session sql(*connectionPool);

    //expressed in secs, default delay
    const int default_retry_delay = 120;
    int retry_delay = 0;
    std::string reuse_job;
    soci::indicator ind = soci::i_ok;

    try
        {
            sql <<
                " select RETRY_DELAY, reuse_job  from t_job where job_id=:jobId ",
                soci::use(jobId),
                soci::into(retry_delay),
                soci::into(reuse_job, ind)
                ;

            sql.begin();

            if ( (ind == soci::i_ok) && reuse_job == "Y")
                {

                    sql << "UPDATE t_job SET "
                        "    job_state = 'ACTIVE' "
                        "WHERE job_id = :jobId AND "
                        "      job_state NOT IN ('FINISHEDDIRTY','FAILED','CANCELED','FINISHED') AND "
                        "      reuse_job = 'Y'",
                        soci::use(jobId);
                }


            if (retry_delay > 0)
                {
                    // update
                    time_t now = getUTC(retry_delay);
                    struct tm tTime;
                    gmtime_r(&now, &tTime);

                    sql << "UPDATE t_file SET retry_timestamp=:1, retry = :retry, file_state = 'SUBMITTED', start_time=NULL, transferHost=NULL, t_log_file=NULL, t_log_file_debug=NULL, throughput = 0 "
                        "WHERE  file_id = :fileId AND  job_id = :jobId AND file_state NOT IN ('FINISHED','SUBMITTED','FAILED','CANCELED')",
                        soci::use(tTime), soci::use(retry), soci::use(fileId), soci::use(jobId);
                }
            else
                {
                    // update
                    time_t now = getUTC(default_retry_delay);
                    struct tm tTime;
                    gmtime_r(&now, &tTime);

                    sql << "UPDATE t_file SET retry_timestamp=:1, retry = :retry, file_state = 'SUBMITTED', start_time=NULL, transferHost=NULL, t_log_file=NULL, t_log_file_debug=NULL, throughput = 0 "
                        "WHERE  file_id = :fileId AND  job_id = :jobId AND file_state NOT IN ('FINISHED','SUBMITTED','FAILED','CANCELED')",
                        soci::use(tTime), soci::use(retry), soci::use(fileId), soci::use(jobId);
                }

            // Keep log
            sql << "INSERT INTO t_file_retry_errors "
                "    (file_id, attempt, datetime, reason) "
                "VALUES (:fileId, :attempt, sys_extract_utc(systimestamp), :reason)",
                soci::use(fileId), soci::use(retry), soci::use(reason);

            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
}

void OracleAPI::getTransferRetries(int fileId, std::vector<FileRetry*>& retries)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::rowset<FileRetry> rs = (sql.prepare << "SELECT * FROM t_file_retry_errors WHERE file_id = :fileId",
                                          soci::use(fileId));


            for (soci::rowset<FileRetry>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    FileRetry const &retry = *i;
                    retries.push_back(new FileRetry(retry));
                }
        }
    catch (std::exception& e)
        {
            std::vector< FileRetry* >::iterator it;
            for (it = retries.begin(); it != retries.end(); ++it)
                {
                    if(*it)
                        delete (*it);
                }
            retries.clear();

            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            std::vector< FileRetry* >::iterator it;
            for (it = retries.begin(); it != retries.end(); ++it)
                {
                    if(*it)
                        delete (*it);
                }
            retries.clear();

            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

bool OracleAPI::assignSanityRuns(soci::session& sql, struct message_sanity &msg)
{

    long long rows = 0;

    try
        {
            if(msg.checkSanityState)
                {
                    sql.begin();
                    soci::statement st((sql.prepare << "update t_server_sanity set checkSanityState=1, t_checkSanityState = sys_extract_utc(systimestamp) "
                                        "where checkSanityState=0"
                                        " AND (t_checkSanityState < (sys_extract_utc(systimestamp) - INTERVAL '30' minute)) "));
                    st.execute(true);
                    rows = st.get_affected_rows();
                    msg.checkSanityState = (rows > 0? true: false);
                    sql.commit();
                    return msg.checkSanityState;
                }
            else if(msg.setToFailOldQueuedJobs)
                {
                    sql.begin();
                    soci::statement st((sql.prepare << "update t_server_sanity set setToFailOldQueuedJobs=1, t_setToFailOldQueuedJobs = sys_extract_utc(systimestamp) "
                                        " where setToFailOldQueuedJobs=0"
                                        " AND (t_setToFailOldQueuedJobs < (sys_extract_utc(systimestamp) - INTERVAL '15' minute)) "
                                       ));
                    st.execute(true);
                    rows = st.get_affected_rows();
                    msg.setToFailOldQueuedJobs = (rows > 0? true: false);
                    sql.commit();
                    return msg.setToFailOldQueuedJobs;
                }
            else if(msg.forceFailTransfers)
                {
                    sql.begin();
                    soci::statement st((sql.prepare << "update t_server_sanity set forceFailTransfers=1, t_forceFailTransfers = sys_extract_utc(systimestamp) "
                                        " where forceFailTransfers=0"
                                        " AND (t_forceFailTransfers < (sys_extract_utc(systimestamp) - INTERVAL '15' minute)) "
                                       ));
                    st.execute(true);
                    rows = st.get_affected_rows();
                    msg.forceFailTransfers = (rows > 0? true: false);
                    sql.commit();
                    return msg.forceFailTransfers;
                }
            else if(msg.revertToSubmitted)
                {
                    sql.begin();
                    soci::statement st((sql.prepare << "update t_server_sanity set revertToSubmitted=1, t_revertToSubmitted = sys_extract_utc(systimestamp) "
                                        " where revertToSubmitted=0"
                                        " AND (t_revertToSubmitted < (sys_extract_utc(systimestamp) - INTERVAL '15' minute)) "
                                       ));
                    st.execute(true);
                    rows = st.get_affected_rows();
                    msg.revertToSubmitted = (rows > 0? true: false);
                    sql.commit();
                    return msg.revertToSubmitted;
                }
            else if(msg.cancelWaitingFiles)
                {
                    sql.begin();
                    soci::statement st((sql.prepare << "update t_server_sanity set cancelWaitingFiles=1, t_cancelWaitingFiles = sys_extract_utc(systimestamp) "
                                        "  where cancelWaitingFiles=0"
                                        " AND (t_cancelWaitingFiles < (sys_extract_utc(systimestamp) - INTERVAL '15' minute)) "
                                       ));
                    st.execute(true);
                    rows = st.get_affected_rows();
                    msg.cancelWaitingFiles = (rows > 0? true: false);
                    sql.commit();
                    return msg.cancelWaitingFiles;
                }
            else if(msg.revertNotUsedFiles)
                {
                    sql.begin();
                    soci::statement st((sql.prepare << "update t_server_sanity set revertNotUsedFiles=1, t_revertNotUsedFiles = sys_extract_utc(systimestamp) "
                                        " where revertNotUsedFiles=0"
                                        " AND (t_revertNotUsedFiles < (sys_extract_utc(systimestamp) - INTERVAL '15' minute)) "
                                       ));
                    st.execute(true);
                    rows = st.get_affected_rows();
                    msg.revertNotUsedFiles = (rows > 0? true: false);
                    sql.commit();
                    return msg.revertNotUsedFiles;
                }
            else if(msg.cleanUpRecords)
                {
                    sql.begin();
                    soci::statement st((sql.prepare << "update t_server_sanity set cleanUpRecords=1, t_cleanUpRecords = sys_extract_utc(systimestamp) "
                                        " where cleanUpRecords=0"
                                        " AND (t_cleanUpRecords < (sys_extract_utc(systimestamp) - INTERVAL '3' day)) "
                                       ));
                    st.execute(true);
                    rows = st.get_affected_rows();
                    msg.cleanUpRecords = (rows > 0? true: false);
                    sql.commit();
                    return msg.cleanUpRecords;
                }
            else if(msg.msgCron)
                {
                    sql.begin();
                    soci::statement st((sql.prepare << "update t_server_sanity set msgcron=1, t_msgcron = sys_extract_utc(systimestamp) "
                                        " where msgcron=0"
                                        " AND (t_msgcron < (sys_extract_utc(systimestamp) - INTERVAL '1' day)) "
                                       ));
                    st.execute(true);
                    rows = st.get_affected_rows();
                    msg.msgCron = (rows > 0? true: false);
                    sql.commit();
                    return msg.msgCron;
                }
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return false;
}


void OracleAPI::resetSanityRuns(soci::session& sql, struct message_sanity &msg)
{
    try
        {
            sql.begin();
            if(msg.checkSanityState)
                {
                    soci::statement st((sql.prepare << "update t_server_sanity set checkSanityState=0 where (checkSanityState=1 "
                                        " OR (t_checkSanityState < (sys_extract_utc(systimestamp) - INTERVAL '45' minute)))  "));
                    st.execute(true);
                }
            else if(msg.setToFailOldQueuedJobs)
                {
                    soci::statement st((sql.prepare << "update t_server_sanity set setToFailOldQueuedJobs=0 where (setToFailOldQueuedJobs=1 "
                                        " OR (t_setToFailOldQueuedJobs < (sys_extract_utc(systimestamp) - INTERVAL '45' minute)))  "));
                    st.execute(true);
                }
            else if(msg.forceFailTransfers)
                {
                    soci::statement st((sql.prepare << "update t_server_sanity set forceFailTransfers=0 where (forceFailTransfers=1 "
                                        " OR (t_forceFailTransfers < (sys_extract_utc(systimestamp) - INTERVAL '45' minute)))  "));
                    st.execute(true);
                }
            else if(msg.revertToSubmitted)
                {
                    soci::statement st((sql.prepare << "update t_server_sanity set revertToSubmitted=0 where (revertToSubmitted=1  "
                                        " OR (t_revertToSubmitted < (sys_extract_utc(systimestamp) - INTERVAL '45' minute)))  "));
                    st.execute(true);
                }
            else if(msg.cancelWaitingFiles)
                {
                    soci::statement st((sql.prepare << "update t_server_sanity set cancelWaitingFiles=0 where (cancelWaitingFiles=1  "
                                        " OR (t_cancelWaitingFiles < (sys_extract_utc(systimestamp) - INTERVAL '45' minute)))  "));
                    st.execute(true);
                }
            else if(msg.revertNotUsedFiles)
                {
                    soci::statement st((sql.prepare << "update t_server_sanity set revertNotUsedFiles=0 where (revertNotUsedFiles=1  "
                                        " OR (t_revertNotUsedFiles < (sys_extract_utc(systimestamp) - INTERVAL '45' minute)))  "));
                    st.execute(true);
                }
            else if(msg.cleanUpRecords)
                {
                    soci::statement st((sql.prepare << "update t_server_sanity set cleanUpRecords=0 where (cleanUpRecords=1  "
                                        " OR (t_cleanUpRecords < (sys_extract_utc(systimestamp) - INTERVAL '4' day)))  "));
                    st.execute(true);
                }
            else if(msg.msgCron)
                {
                    soci::statement st((sql.prepare << "update t_server_sanity set msgcron=0 where msgcron=1"));
                    st.execute(true);
                }
            sql.commit();
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::updateHeartBeat(unsigned* index, unsigned* count, unsigned* start, unsigned* end)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            // Update beat
            sql << "MERGE INTO t_hosts USING "
                " (SELECT :hostname AS hostname FROM dual) Hostname ON (t_hosts.hostname = Hostname.hostname) "
                " WHEN     MATCHED THEN UPDATE SET t_hosts.beat = sys_extract_utc(systimestamp)"
                " WHEN NOT MATCHED THEN INSERT (hostname, beat) VALUES (Hostname.hostname, sys_extract_utc(systimestamp))",
                soci::use(hostname);

            // Total number of working instances
            sql << "SELECT COUNT(hostname) FROM t_hosts "
                "  WHERE beat >= (sys_extract_utc(systimestamp) - interval '2' minute)",
                soci::into(*count);

            // This instance index
            soci::rowset<std::string> rsHosts = (sql.prepare <<
                                                 "SELECT hostname FROM t_hosts "
                                                 "WHERE beat >= (sys_extract_utc(systimestamp) - interval '2' minute)"
                                                 "ORDER BY hostname");

            soci::rowset<std::string>::const_iterator i;
            for (*index = 0, i = rsHosts.begin(); i != rsHosts.end(); ++i, ++(*index))
                {
                    std::string& host = *i;
                    if (host == hostname)
                        break;
                }

            sql.commit();

            // Calculate start and end hash values
            unsigned segsize = 0xFFFF / *count;
            unsigned segmod  = 0xFFFF % *count;

            *start = segsize * (*index);
            *end   = segsize * (*index + 1) - 1;

            // Last one take over what is left
            if (*index == *count - 1)
                *end += segmod + 1;

            this->hashSegment.start = *start;
            this->hashSegment.end   = *end;
        }
    catch (std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}

// the class factories

extern "C" GenericDbIfce* create()
{
    return new OracleAPI;
}

extern "C" void destroy(GenericDbIfce* p)
{
    if (p)
        delete p;
}

extern "C" MonitoringDbIfce* create_monitoring()
{
    return new OracleMonitoring;
}

extern "C" void destroy_monitoring(MonitoringDbIfce* p)
{
    if (p)
        delete p;
}
