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
#include <soci/oracle/soci-oracle.h>
#include <signal.h>
#include <sys/param.h>
#include <unistd.h>
#include "OracleAPI.h"
#include "sociConversions.h"
#include "queue_updater.h"
#include "DbUtils.h"


using namespace FTS3_COMMON_NAMESPACE;
using namespace db;

static unsigned getHashedId(void)
{
    static __thread struct random_data rand_data =
    {
        NULL, NULL, NULL,
        0, 0, 0,
        NULL
    };
    static __thread char statbuf[16] = {0};

    if (rand_data.state == NULL)
        {
            initstate_r(static_cast<unsigned>(time(NULL)), statbuf, sizeof(statbuf), &rand_data);
        }

    int value = 0;
    random_r(&rand_data, &value);

    return value % UINT16_MAX;
}



bool OracleAPI::getChangedFile (std::string source, std::string dest, double rate, double& rateStored, double thr, double& thrStored, double retry, double& retryStored, int active, int& activeStored, int& throughputSamplesEqual, int& throughputSamplesStored)
{
    bool returnValue = false;

    if(thr == 0 || rate == 0 || active == 0)
        return returnValue;

    if(filesMemStore.empty())
        {
            boost::tuple<std::string, std::string, double, double, double, int, int, int> record(source, dest, rate, thr, retry, active, 0, 0);
            filesMemStore.push_back(record);
            return returnValue;
        }
    else
        {
            bool found = false;
            std::vector< boost::tuple<std::string, std::string, double, double, double, int, int, int> >::iterator itFind;
            for (itFind = filesMemStore.begin(); itFind < filesMemStore.end(); ++itFind)
                {
                    boost::tuple<std::string, std::string, double, double, double, int, int, int>& tupleRecord = *itFind;
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
                    boost::tuple<std::string, std::string, double, double, double, int, int, int> record(source, dest, rate, thr, retry, active, 0, 0);
                    filesMemStore.push_back(record);
                    return found;
                }

            std::vector< boost::tuple<std::string, std::string, double, double, double, int, int, int> >::iterator it =  filesMemStore.begin();
            while (it != filesMemStore.end())
                {
                    boost::tuple<std::string, std::string, double, double, double, int, int, int>& tupleRecord = *it;
                    std::string sourceLocal = boost::get<0>(tupleRecord);
                    std::string destLocal = boost::get<1>(tupleRecord);
                    double rateLocal = boost::get<2>(tupleRecord);
                    double thrLocal = boost::get<3>(tupleRecord);
                    double retryThr = boost::get<4>(tupleRecord);
                    int activeLocal = boost::get<5>(tupleRecord);
                    int throughputSamplesLocal = boost::get<6>(tupleRecord);
                    int throughputSamplesEqualLocal = boost::get<7>(tupleRecord);

                    if(sourceLocal == source && destLocal == dest)
                        {
                            retryStored = retryThr;
                            thrStored = thrLocal;
                            rateStored = rateLocal;
                            activeStored = activeLocal;

                            //if EMA is the same for 10min, spawn one more transfer to see how it goes!
                            if(thr == thrLocal)
                                {
                                    throughputSamplesEqualLocal += 1;
                                    throughputSamplesEqual = throughputSamplesEqualLocal;
                                    if(throughputSamplesEqualLocal == 11)
                                        throughputSamplesEqualLocal = 0;
                                }
                            else
                                {
                                    throughputSamplesEqualLocal = 0;
                                    throughputSamplesEqual = 0;
                                }

                            if(thr < thrLocal)
                                {
                                    throughputSamplesLocal += 1;
                                }
                            else if(thr >= thrLocal && throughputSamplesLocal > 0)
                                {
                                    throughputSamplesLocal -= 1;
                                }
                            else
                                {
                                    throughputSamplesLocal = 0;
                                }

                            if(throughputSamplesLocal == 3)
                                {
                                    throughputSamplesStored = throughputSamplesLocal;
                                    throughputSamplesLocal = 0;
                                }


                            if(rateLocal != rate || thrLocal != thr || retry != retryThr || throughputSamplesEqualLocal >= 0)
                                {
                                    it = filesMemStore.erase(it);
                                    boost::tuple<std::string, std::string, double, double, double, int, int, int> record(source, dest, rate, thr, retry, active, throughputSamplesLocal, throughputSamplesEqualLocal);
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


void OracleAPI::submitdelete(const std::string & jobId, const std::map<std::string,std::string>& urlsHost,
                             const std::string & userDN, const std::string & voName, const std::string & credID)
{
    if (urlsHost.empty()) return;

    const std::string initialState = "DELETE";
    std::ostringstream pairStmt;

    soci::session sql(*connectionPool);

    // first check if the job conserns only one SE
    std::string const & src_se = urlsHost.begin()->second;
    bool same_src_se = true;

    std::multimap<std::string,std::string>::const_iterator it;
    for (it = urlsHost.begin(); it != urlsHost.end(); ++it)
        {
            same_src_se = it->second == src_se;
            if (!same_src_se) break;
        }

    try
        {
            sql.begin();

            if(same_src_se)
                {
                    soci::statement insertJob = (	sql.prepare << "INSERT INTO  t_job ( job_id, job_state, vo_name,submit_host, submit_time, user_dn, cred_id, source_se)"
                                                    "VALUES (:jobId, :jobState, :voName , :hostname, sys_extract_utc(systimestamp), :userDN, :credID, :src_se)",
                                                    soci::use(jobId),
                                                    soci::use(initialState),
                                                    soci::use(voName),
                                                    soci::use(hostname),
                                                    soci::use(userDN),
                                                    soci::use(credID),
                                                    soci::use(src_se)
                                                );
                    insertJob.execute(true);
                }
            else
                {
                    soci::statement insertJob = (	sql.prepare << "INSERT INTO  t_job ( job_id, job_state, vo_name,submit_host, submit_time, user_dn, cred_id)"
                                                    "VALUES (:jobId, :jobState, :voName , :hostname, sys_extract_utc(systimestamp), :userDN, :credID)",
                                                    soci::use(jobId),
                                                    soci::use(initialState),
                                                    soci::use(voName),
                                                    soci::use(hostname),
                                                    soci::use(userDN),
                                                    soci::use(credID)
                                                );
                    insertJob.execute(true);
                }

            // Insert all operations in a bulk manner
            soci::statement insert_dm_stmt(sql);
            std::ostringstream query;
            unsigned index = 0;

            query << "INSERT ALL ";
            for(it = urlsHost.begin(); it != urlsHost.end(); ++it)
                {
                    query << " INTO t_dm (vo_name, job_id, file_state, source_surl, source_se, hashed_id) VALUES ("
                          << " :vo_name" << index << ", "
                          << " :job_id" << index << ", "
                          << " :file_state" << index << ", "
                          << " :surl" << index << ", "
                          << " :se" << index << ", "
                          << " :hash" << index << ") ";

                    insert_dm_stmt.exchange(soci::use(voName));
                    insert_dm_stmt.exchange(soci::use(jobId));
                    insert_dm_stmt.exchange(soci::use(initialState));
                    insert_dm_stmt.exchange(soci::use(it->first));
                    insert_dm_stmt.exchange(soci::use(it->second));
                    insert_dm_stmt.exchange(soci::use(getHashedId()));

                    ++index;
                }

            std::string queryStr = query.str() + " SELECT * FROM DUAL ";

            insert_dm_stmt.alloc();
            insert_dm_stmt.prepare(queryStr);
            insert_dm_stmt.define_and_bind();
            insert_dm_stmt.execute(true);

            sql.commit();
        }
    catch(std::exception& e)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    catch(...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

}




TransferJobs* OracleAPI::getTransferJob(std::string jobId, bool archive)
{
    soci::session sql(*connectionPool);

    std::string query;
    if (archive)
        {
            query = "SELECT t_job_backup.vo_name, t_job_backup.user_dn, t_job_backup.job_state "
                    "FROM t_job_backup WHERE t_job_backup.job_id = :jobId";
        }
    else
        {
            query = "SELECT t_job.vo_name, t_job.user_dn, t_job.job_state "
                    "FROM t_job WHERE t_job.job_id = :jobId";
        }

    TransferJobs* job = NULL;
    try
        {
            job = new TransferJobs();

            sql << query,
                soci::use(jobId),
                soci::into(job->VO_NAME),
                soci::into(job->USER_DN),
                soci::into(job->JOB_STATE);

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

            static const boost::regex re("^\\s*\\{\\s*\"([ a-zA-Z0-9\\._-]+)\"\\s*:\\s*((0\\.)?\\d+)\\s*\\}\\s*$");
            static const int ACTIVITY_NAME = 1;
            static const int ACTIVITY_SHARE = 2;

            for (it = tokens.begin(); it != tokens.end(); it++)
                {
                    // parse single activity share
                    std::string str = *it;

                    boost::smatch what;
                    boost::regex_match(str, what, re, boost::match_extra);

                    std::string activity_name(what[ACTIVITY_NAME]);
                    boost::algorithm::to_lower(activity_name);
                    ret[activity_name] = boost::lexical_cast<double>(what[ACTIVITY_SHARE]);
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
                    ret.push_back(it->get<std::string>("VO"));
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

void OracleAPI::revertToSubmitted()
{
    soci::session sql(*connectionPool);

    try
        {
            struct tm startTime;
            int fileId=0;
            std::string jobId, reuseJob;
            time_t now2 = getUTC(0);

            soci::indicator reuseInd = soci::i_ok;
            soci::statement readyStmt = (sql.prepare << "SELECT f.start_time, f.file_id, f.job_id, j.reuse_job "
                                         " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                         " WHERE f.file_state = 'READY' and j.job_finished is null "
                                         " AND (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd) ",
                                         soci::use(hashSegment.start), soci::use(hashSegment.end),
                                         soci::into(startTime),
                                         soci::into(fileId),
                                         soci::into(jobId),
                                         soci::into(reuseJob, reuseInd));

            //check if the file belongs to a multiple replica job
            long long replicaJob = 0;
            long long replicaJobCountAll = 0;

            sql.begin();
            if (readyStmt.execute(true))
                {
                    do
                        {
                            //don't do anything to multiple replica jobs
                            sql << "select count(*), count(distinct file_index) from t_file where job_id=:job_id",
                                soci::use(jobId), soci::into(replicaJobCountAll), soci::into(replicaJob);

                            //this is a m-replica job
                            if(replicaJobCountAll > 1 && replicaJob == 1)
                                continue;


                            time_t startTimestamp = timegm(&startTime);
                            double diff = difftime(now2, startTimestamp);

                            if (diff > 200 && reuseJob != "Y")
                                {
                                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "The transfer with file id " << fileId << " seems to be stalled, restart it" << commit;

                                    sql << "UPDATE t_file SET start_time=NULL, file_state = 'SUBMITTED', reason='', transferhost='',finish_time=NULL, job_finished=NULL "
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

                                                    sql << "UPDATE t_file SET start_time=NULL, file_state = 'SUBMITTED', reason='', transferhost='',finish_time=NULL, job_finished=NULL "
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



std::map<std::string, long long> OracleAPI::getActivitiesInQueue(soci::session& sql, std::string src, std::string dst, std::string vo)
{
    std::map<std::string, long long> ret;

    time_t now = time(NULL);
    struct tm tTime;
    gmtime_r(&now, &tTime);

    try
        {
            std::string activityInput;
            std::string activityOutput;
            int howMany = 0;
            soci::statement st = (sql.prepare <<
                                  " SELECT distinct activity FROM t_file WHERE vo_name=:vo_name and source_se = :source_se AND "
                                  " dest_se = :dest_se and file_state='SUBMITTED' and job_finished is null ",
                                  soci::use(vo),
                                  soci::use(src),
                                  soci::use(dst),
                                  soci::into(activityInput));

            st.execute();
            while (st.fetch())
                {
                    activityOutput = activityInput;
                    howMany++;
                }

            //no reason to execute the expensive query below if for this link there are only 'default' or '' activity shares
            if(howMany == 1 && (activityOutput == "default" || activityOutput == ""))
                return ret;


            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT activity, COUNT(DISTINCT f.job_id || f.file_index) AS count "
                                             " FROM t_job j, t_file f "
                                             " WHERE j.job_id = f.job_id AND j.vo_name = f.vo_name AND f.file_state = 'SUBMITTED' AND "
                                             "	f.source_se = :source AND f.dest_se = :dest AND "
                                             "	f.vo_name = :vo_name AND "
                                             "	f.wait_timestamp IS NULL AND "
                                             "	(f.retry_timestamp is NULL OR f.retry_timestamp < :tTime) AND "
                                             "	(f.hashed_id >= :hStart AND f.hashed_id <= :hEnd) AND "
                                             "  j.job_state in ('ACTIVE','SUBMITTED') AND "
                                             "  (j.reuse_job = 'N' OR j.reuse_job = 'R' OR j.reuse_job IS NULL) "
                                             " GROUP BY activity ",
                                             soci::use(src),
                                             soci::use(dst),
                                             soci::use(vo),
                                             soci::use(tTime),
                                             soci::use(hashSegment.start), soci::use(hashSegment.end)
                                         );

            soci::rowset<soci::row>::const_iterator it;
            for (it = rs.begin(); it != rs.end(); it++)
                {
                    std::string activity_name;

                    if (it->get_indicator("activity") == soci::i_null) {
                        activity_name = "default";
                    }
                    else {
                        activity_name = it->get<std::string>("activity");
                        if (activity_name.empty()) {
                            activity_name = "default";
                        }
                    }

                    boost::algorithm::to_lower(activity_name);
                    long long nFiles = it->get<long long>("count");
                    ret[activity_name] = nFiles;
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

std::map<std::string, int> OracleAPI::getFilesNumPerActivity(soci::session& sql, std::string src, std::string dst, std::string vo, int filesNum, std::set<std::string> & default_activities)
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
                    std::map<std::string, double>::iterator pos = activityShares.find(it->first);
                    if (pos != activityShares.end() && it->first != "default")
                        {
                            sum += pos->second;
                        }
                    else
                        {
                            // if the activity has not been defined it falls to default
                            default_activities.insert(it->first);
                        }
                }
            // if default was used add it as well
            if (!default_activities.empty())
                sum += activityShares["default"];

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
                            // get the activity name (if it was not defined use default)
                            std::string activity_name = default_activities.count(it->first) ? "default" : it->first;
                            // calculate the interval
                            interval += activityShares[activity_name] / sum;
                            // if the slot has been assigned to the given activity ...
                            if (r < interval)
                                {
                                    ++activityFilesNum[activity_name];
                                    --it->second;
                                    // if there are no more files for the given ativity remove it from the sum
                                    if (it->second == 0) sum -= activityShares[activity_name];
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


void OracleAPI::getVOPairs(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct)
{

    soci::session sql(*connectionPool);

    try
        {
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
                    std::string source_se = r.get<std::string>("SOURCE_SE","");
                    std::string dest_se = r.get<std::string>("DEST_SE","");

                    distinct.push_back(
                        boost::tuple< std::string, std::string, std::string>(
                            r.get<std::string>("SOURCE_SE",""),
                            r.get<std::string>("DEST_SE",""),
                            r.get<std::string>("VO_NAME","")
                        )

                    );

                    long long int linkExists = 0;
                    sql << "select count(*) from t_optimize_active where source_se=:source_se and dest_se=:dest_se and datetime >= (sys_extract_utc(systimestamp) - interval '5' MINUTE)",
                        soci::use(source_se),
                        soci::use(dest_se),
                        soci::into(linkExists);
                    if(linkExists == 0) //for some reason does not exist, add it
                        {
                            sql.begin();
                            sql << " MERGE INTO t_optimize_active USING "
                                "    (SELECT :source_se as source, :dest_se as dest FROM dual) Pair "
                                " ON (t_optimize_active.source_se = Pair.source AND t_optimize_active.dest_se = Pair.dest) "
                                " WHEN MATCHED THEN UPDATE SET datetime = sys_extract_utc(systimestamp) "
                                " WHEN NOT MATCHED THEN INSERT (source_se, dest_se, datetime) VALUES (Pair.source, Pair.dest, sys_extract_utc(systimestamp) )",
                                soci::use(source_se), soci::use(dest_se);
                            sql.commit();
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
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
}

void OracleAPI::getVOPairsWithReuse(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT DISTINCT t_file.source_se, t_file.dest_se, t_file.vo_name "
                                             " FROM t_file "
                                             " INNER JOIN t_job ON t_file.job_id = t_job.job_id "
                                             " WHERE "
                                             "      file_state = 'SUBMITTED' AND "
                                             "      (hashed_id >= :hStart AND hashed_id <= :hEnd) AND "
                                             "      t_job.reuse_job = 'Y' ",
                                             soci::use(hashSegment.start), soci::use(hashSegment.end)
                                         );
            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    soci::row const& r = *i;
                    std::string source_se = r.get<std::string>("SOURCE_SE","");
                    std::string dest_se = r.get<std::string>("DEST_SE","");

                    distinct.push_back(
                        boost::tuple< std::string, std::string, std::string>(
                            r.get<std::string>("SOURCE_SE",""),
                            r.get<std::string>("DEST_SE",""),
                            r.get<std::string>("VO_NAME","")
                        )

                    );

                    long long int linkExists = 0;
                    sql << "select count(*) from t_optimize_active where source_se=:source_se and dest_se=:dest_se and datetime >= (sys_extract_utc(systimestamp) - interval '5' MINUTE)",
                        soci::use(source_se),
                        soci::use(dest_se),
                        soci::into(linkExists);
                    if(linkExists == 0) //for some reason does not exist, add it
                        {
                            sql.begin();
                            sql << " MERGE INTO t_optimize_active USING "
                                "    (SELECT :source_se as source, :dest_se as dest FROM dual) Pair "
                                " ON (t_optimize_active.source_se = Pair.source AND t_optimize_active.dest_se = Pair.dest) "
                                " WHEN MATCHED THEN UPDATE SET datetime = sys_extract_utc(systimestamp) "
                                " WHEN NOT MATCHED THEN INSERT (source_se, dest_se, datetime) VALUES (Pair.source, Pair.dest, sys_extract_utc(systimestamp) )",
                                soci::use(source_se), soci::use(dest_se);
                            sql.commit();
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
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
}

void OracleAPI::getByJobId(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct, std::map< std::string, std::list<TransferFiles> >& files)
{
    soci::session sql(*connectionPool);

    time_t now = time(NULL);
    struct tm tTime;
    gmtime_r(&now, &tTime);

    try
        {
            int defaultFilesNum = 10;

            if(distinct.empty())
                return;

            long long hostCount = 0;
            sql <<
                " SELECT COUNT(hostname) "
                " FROM t_hosts "
                "  WHERE beat >= (sys_extract_utc(systimestamp) - interval '2' minute)",
                soci::into(hostCount)
                ;

            if(hostCount < 1)
                hostCount = 1;


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

                            sql << " select count(*) from t_file where source_se=:source_se and dest_se=:dest_se and file_state = 'ACTIVE' ",
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
                                        {
                                            continue;
                                        }
                                    else
                                        {
                                            filesNum /= int(hostCount);
                                            if(filesNum < 2)
                                                filesNum = 2;
                                        }
                                }
                        }

                    std::set<std::string> default_activities;
                    std::map<std::string, int> activityFilesNum =
                        getFilesNumPerActivity(sql, boost::get<0>(triplet), boost::get<1>(triplet), boost::get<2>(triplet), filesNum, default_activities);

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
                                                                 "       f.source_se, f.dest_se, f.selection_strategy, j.internal_job_params, j.reuse_job, j.user_cred  "
                                                                 " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) WHERE  "
                                                                 " j.vo_name = f.vo_name AND f.file_state = 'SUBMITTED' AND  "
                                                                 "    f.source_se = :source AND f.dest_se = :dest AND "
                                                                 "    f.vo_name = :vo_name AND "
                                                                 "    f.wait_timestamp IS NULL AND "
                                                                 "    (f.retry_timestamp is NULL OR f.retry_timestamp < :tTime) AND "
                                                                 "    (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd) AND "
                                                                 "    j.job_state in ('ACTIVE','SUBMITTED','STAGING') AND "
                                                                 "    (j.reuse_job = 'N' OR j.reuse_job = 'R' OR j.reuse_job IS NULL) AND j.vo_name=:vo_name "
                                                                 "     ORDER BY j.priority DESC, j.submit_time) "
                                                                 " WHERE rn <= :filesNum ",
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
                                    TransferFiles& tfile = *ti;

                                    if(tfile.REUSE_JOB == "R")
                                        {
                                            int total = 0;
                                            int remain = 0;
                                            sql <<
                                                " SELECT"
                                                "   (SELECT COUNT(*) FROM t_file WHERE job_id=:job_id) AS c1,"
                                                "   (SELECT COUNT(*) FROM t_file WHERE file_state<>'NOT_USED' AND job_id=:job_id) AS c2 "
                                                " FROM dual",
                                                soci::use(tfile.JOB_ID),
                                                soci::use(tfile.JOB_ID),
                                                soci::into(total),
                                                soci::into(remain);

                                            tfile.LAST_REPLICA = (total == remain)? 1: 0;
                                        }

                                    files[tfile.VO_NAME].push_back(tfile);
                                }
                        }
                    else
                        {
                            // we are allways checking empty string
                            std::string def_act = " (''";
                            if (!default_activities.empty())
                                {
                                    std::set<std::string>::const_iterator it_def;
                                    for (it_def = default_activities.begin(); it_def != default_activities.end(); ++it_def)
                                        {
                                            def_act += ", '" + *it_def + "'";
                                        }
                                }
                            def_act += ") ";

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
                                        "       f.source_se, f.dest_se, f.selection_strategy, j.internal_job_params, j.reuse_job, j.user_cred  "
                                        " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) WHERE "
                                        " j.vo_name = f.vo_name AND f.file_state = 'SUBMITTED' AND  "
                                        "    f.source_se = :source AND f.dest_se = :dest AND "
                                        "    f.vo_name = :vo_name AND ";
                                    select +=
                                        it_act->first == "default" ?
                                        "     (f.activity = :activity OR f.activity IS NULL OR f.activity IN " + def_act + ") AND "
                                        :
                                        "     f.activity = :activity AND ";
                                    select +=
                                        "    f.wait_timestamp IS NULL AND "
                                        "    (f.retry_timestamp is NULL OR f.retry_timestamp < :tTime) AND "
                                        "    (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd) AND "
                                        "    j.job_state in ('ACTIVE','SUBMITTED','STAGING') AND "
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
                                            TransferFiles& tfile = *ti;

                                            if(tfile.REUSE_JOB == "R")
                                                {
                                                    int total = 0;
                                                    int remain = 0;
                                                    sql << " select count(*) as c1, "
                                                        " (select count(*) from t_file where file_state<>'NOT_USED' and  job_id=:job_id)"
                                                        " as c2 from t_file where job_id=:job_id",
                                                        soci::use(tfile.JOB_ID),
                                                        soci::use(tfile.JOB_ID),
                                                        soci::into(total),
                                                        soci::into(remain);

                                                    tfile.LAST_REPLICA = (total == remain)? 1: 0;
                                                }


                                            files[tfile.VO_NAME].push_back(tfile);
                                        }
                                }
                        }
                }
        }
    catch (std::exception& e)
        {
            files.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            files.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
}


static
int freeSlotForPair(soci::session& sql, std::list<std::pair<std::string, std::string> >& visited,
                    const std::string& source_se, const std::string& dest_se)
{
    int count = 0;
    int limit = 10;

    // Manual configuration
    sql << "SELECT COUNT(*) FROM t_link_config WHERE (source = :source OR source = '*') AND (destination = :dest OR destination = '*')",
        soci::use(source_se), soci::use(dest_se), soci::into(count);
    if (count == 0)
        sql << "SELECT COUNT(*) FROM t_group_members WHERE (member=:source OR member=:dest)",
            soci::use(source_se), soci::use(dest_se), soci::into(count);

    // No luck? Ask the optimizer
    if (count == 0)
        {
            int active = 0, max_active = 0;
            soci::indicator is_active_null = soci::i_ok;

            sql << "SELECT COUNT(*) FROM t_file WHERE source_se=:source_se AND dest_se=:dest_se AND file_state = 'ACTIVE' AND job_finished is NULL ",
                soci::use(source_se), soci::use(dest_se), soci::into(active);

            sql << "SELECT active FROM t_optimize_active WHERE source_se=:source_se AND dest_se=:dest_se",
                soci::use(source_se), soci::use(dest_se), soci::into(max_active, is_active_null);

            if (!is_active_null)
                limit = (max_active - active);
            if (limit <= 0)
                return 0;
        }

    // This pair may have transfers already queued, so take them into account
    std::list<std::pair<std::string, std::string> >::iterator i;
    for (i = visited.begin(); i != visited.end(); ++i)
        {
            if (i->first == source_se && i->second == dest_se)
                --limit;
        }

    return limit;
}


void OracleAPI::getMultihopJobs(std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFiles> > > >& files)
{
    soci::session sql(*connectionPool);

    try
        {
            time_t now = time(NULL);
            struct tm tTime;
            gmtime_r(&now, &tTime);

            soci::rowset<soci::row> jobs_rs = (sql.prepare <<
                                               " SELECT DISTINCT t_file.vo_name, t_file.job_id "
                                               " FROM t_file "
                                               " INNER JOIN t_job ON t_file.job_id = t_job.job_id "
                                               " WHERE "
                                               "      t_file.file_state = 'SUBMITTED' AND "
                                               "      (t_file.hashed_id >= :hStart AND t_file.hashed_id <= :hEnd) AND"
                                               "      t_job.reuse_job = 'H' AND "
                                               "      t_file.wait_timestamp is null AND "
                                               "      (t_file.retry_timestamp IS NULL OR t_file.retry_timestamp < :tTime) ",
                                               soci::use(hashSegment.start), soci::use(hashSegment.end),
                                               soci::use(tTime)
                                              );

            std::list<std::pair<std::string, std::string> > visited;

            for (soci::rowset<soci::row>::iterator i = jobs_rs.begin(); i != jobs_rs.end(); ++i)
                {
                    std::string vo_name = i->get<std::string>("VO_NAME", "");
                    std::string job_id = i->get<std::string>("JOB_ID", "");

                    soci::rowset<TransferFiles> rs =
                        (
                            sql.prepare <<
                            " SELECT "
                            "       f.file_state, f.source_surl, f.dest_surl, f.job_id, j.vo_name, "
                            "       f.file_id, j.overwrite_flag, j.user_dn, j.cred_id, "
                            "       f.checksum, j.checksum_method, j.source_space_token, "
                            "       j.space_token, j.copy_pin_lifetime, j.bring_online, "
                            "       f.user_filesize, f.file_metadata, j.job_metadata, f.file_index, "
                            "       f.bringonline_token, f.source_se, f.dest_se, f.selection_strategy, "
                            "       j.internal_job_params, j.user_cred, j.reuse_job "
                            " FROM t_job j INNER JOIN t_file f ON (j.job_id = f.job_id) "
                            " WHERE j.job_id = :job_id "
                            " ORDER BY f.file_id ASC",
                            soci::use(job_id)
                        );

                    std::list<TransferFiles> tf;
                    for (soci::rowset<TransferFiles>::const_iterator ti = rs.begin(); ti != rs.end(); ++ti)
                        {
                            tf.push_back(*ti);
                        }

                    // Check link config only for the first pair, and, if there are slots, proceed
                    if (!tf.empty() && freeSlotForPair(sql, visited, tf.front().SOURCE_SE, tf.front().DEST_SE) > 0)
                        {
                            files[vo_name].push(std::make_pair(job_id, tf));
                            visited.push_back(std::make_pair(tf.front().SOURCE_SE, tf.front().DEST_SE));
                        }
                }
        }
    catch (std::exception& e)
        {
            files.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            files.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
}


void OracleAPI::useFileReplica(soci::session& sql, std::string jobId, int fileId)
{
    //auto-pick and manuall replica logic must be added here

    try
        {
            soci::indicator ind = soci::i_ok;
            std::string selection_strategy;
            std::string vo_name;

            //check if the file belongs to a multiple replica job
            std::string mreplica;
            std::string job_state;
            sql << "select reuse_job, job_state from t_job where job_id=:job_id",
                soci::use(jobId), soci::into(mreplica), soci::into(job_state);

            //this is a m-replica job
            if(mreplica == "R" && job_state != "CANCELED" && job_state != "FAILED")
                {
                    //check if it's auto or manual
                    sql << " select selection_strategy, vo_name from t_file where file_id = :file_id",
                        soci::use(fileId), soci::into(selection_strategy, ind), soci::into(vo_name);

                    if (ind == soci::i_ok)
                        {
                            if(selection_strategy == "auto") //pick the "best-next replica to process"
                                {
                                    int bestFileId = getBestNextReplica(sql, jobId, vo_name);
                                    if(bestFileId > 0)
                                        {
                                            sql <<
                                                " UPDATE t_file "
                                                " SET file_state = 'SUBMITTED' "
                                                " WHERE job_id = :jobId AND file_id = :file_id  "
                                                " AND file_state = 'NOT_USED' ",
                                                soci::use(jobId), soci::use(bestFileId);
                                        }
                                    else
                                        {
                                            sql <<
                                                " UPDATE t_file "
                                                " SET file_state = 'SUBMITTED' "
                                                " WHERE job_id = :jobId "
                                                " AND file_state = 'NOT_USED' AND file_id = ( select min(file_id) from t_file where file_state = 'NOT_USED' and job_id=:job_id )",
                                                soci::use(jobId), soci::use(jobId);
                                        }
                                }
                            else if (selection_strategy == "orderly")
                                {
                                    sql <<
                                        " UPDATE t_file "
                                        " SET file_state = 'SUBMITTED' "
                                        " WHERE job_id = :jobId "
                                        " AND file_state = 'NOT_USED'  AND file_id = ( select min(file_id) from t_file where file_state = 'NOT_USED' and job_id=:job_id )",
                                        soci::use(jobId), soci::use(jobId);
                                }
                            else
                                {
                                    sql <<
                                        " UPDATE t_file "
                                        " SET file_state = 'SUBMITTED' "
                                        " WHERE job_id = :jobId "
                                        " AND file_state = 'NOT_USED'  AND file_id = ( select min(file_id) from t_file where file_state = 'NOT_USED' and job_id=:job_id ) ",
                                        soci::use(jobId), soci::use(jobId);
                                }
                        }
                    else //it's NULL, default is orderly
                        {
                            sql <<
                                " UPDATE t_file "
                                " SET file_state = 'SUBMITTED' "
                                " WHERE job_id = :jobId "
                                " AND file_state = 'NOT_USED'  AND file_id = ( select min(file_id) from t_file where file_state = 'NOT_USED' and job_id=:job_id ) ",
                                soci::use(jobId), soci::use(jobId);
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


bool pairCompare( std::pair<std::pair<std::string, std::string>, int> i, pair<std::pair<std::string, std::string>, int> j)
{
    return i.second < j.second;
}

int OracleAPI::getBestNextReplica(soci::session& sql, const std::string & job_id, const std::string & vo_name)
{
    //for now consider only the less queued transfers, later add throughput and success rate
    int bestFileId = 0;
    std::string bestSource;
    std::string bestDestination;
    std::map<std::pair<std::string, std::string>, int> pair;
    soci::indicator ind = soci::i_ok;

    try
        {
            //get available pairs
            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             "select distinct source_se, dest_se from t_file where job_id=:job_id and file_state='NOT_USED'",
                                             soci::use(job_id)
                                         );

            soci::rowset<soci::row>::const_iterator it;
            for (it = rs.begin(); it != rs.end(); ++it)
                {
                    std::string source_se = it->get<std::string>("SOURCE_SE","");
                    std::string dest_se = it->get<std::string>("DEST_SE","");
                    int queued = 0;

                    //get queued for this link and vo
                    sql << " select count(*) from t_file where file_state='SUBMITTED' and "
                        " source_se=:source_se and dest_se=:dest_se and "
                        " vo_name=:vo_name ",
                        soci::use(source_se), soci::use(dest_se),soci::use(vo_name), soci::into(queued);

                    //get distinct source_se / dest_se
                    std::pair<std::string, std::string> key(source_se, dest_se);
                    pair.insert(std::make_pair(key, queued));

                    if(queued == 0) //pick the first one if the link is free
                        break;

                    //get success rate & throughput for this link, it is mapped to "filesize" column in t_optimizer_evolution table :)
                    /*
                    sql << " select filesize, throughput from t_optimizer_evolution where "
                        " source_se=:source_se and dest_se=:dest_se and filesize is not NULL and agrthroughput is not NULL "
                        " order by datetime desc limit 1 ",
                        soci::use(source_se), soci::use(dest_se),soci::into(successRate),soci::into(throughput);
                    */

                }

            //not waste queries
            if(!pair.empty())
                {
                    //get min queue length
                    std::pair<std::pair<std::string, std::string>, int> minValue = *min_element(pair.begin(), pair.end(), pairCompare );
                    bestSource =      (minValue.first).first;
                    bestDestination = (minValue.first).second;

                    //finally get the next-best file_id to be processed
                    sql << "select file_id from t_file where file_state='NOT_USED' and source_se=:source_se and dest_se=:dest_se and job_id=:job_id",
                        soci::use(bestSource), soci::use(bestDestination), soci::use(job_id), soci::into(bestFileId, ind);

                    if (ind != soci::i_ok)
                        bestFileId = 0;
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

    return bestFileId;
}


unsigned int OracleAPI::updateFileStatusReuse(TransferFiles const & file, const std::string status)
{
    soci::session sql(*connectionPool);

    unsigned int updated = 0;


    try
        {
            sql.begin();

            soci::statement stmt(sql);

            stmt.exchange(soci::use(status, "state"));
            stmt.exchange(soci::use(file.JOB_ID, "jobId"));
            stmt.exchange(soci::use(hostname, "hostname"));
            stmt.alloc();
            stmt.prepare("UPDATE t_file SET "
                         "    file_state = :state, start_time = sys_extract_utc(systimestamp), transferHost = :hostname "
                         "WHERE job_id = :jobId AND file_state = 'SUBMITTED'");
            stmt.define_and_bind();
            stmt.execute(true);

            updated = (unsigned int) stmt.get_affected_rows();

            if (updated > 0)
                {
                    soci::statement jobStmt(sql);
                    jobStmt.exchange(soci::use(status, "state"));
                    jobStmt.exchange(soci::use(file.JOB_ID, "jobId"));
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



unsigned int OracleAPI::updateFileStatus(TransferFiles& file, const std::string status)
{
    soci::session sql(*connectionPool);

    unsigned int updated = 0;

    try
        {
            sql.begin();

            soci::statement stmt(sql);

            stmt.exchange(soci::use(status, "state"));
            stmt.exchange(soci::use(file.FILE_ID, "fileId"));
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
                    jobStmt.exchange(soci::use(file.JOB_ID, "jobId"));
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


void OracleAPI::getByJobIdReuse(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct, std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFiles> > > >& files)
{
    if(distinct.empty()) return;

    soci::session sql(*connectionPool);

    time_t now = time(NULL);
    struct tm tTime;
    soci::indicator isNull;

    try
        {
            // Iterate through pairs, getting jobs IF the VO has not run out of credits
            // AND there are pending file transfers within the job
            std::vector< boost::tuple<std::string, std::string, std::string> >::iterator it;
            for (it = distinct.begin(); it != distinct.end(); ++it)
                {
                    boost::tuple<std::string, std::string, std::string>& triplet = *it;
                    gmtime_r(&now, &tTime);
                    std::string job;

                    sql<<
                       "select distinct job_id from ( "
                       "  select j.job_id as job_id "
                       "  from t_job j inner join t_file f on j.job_id = f.job_id "
                       "  where j.source_se=:source_se and j.dest_se=:dest_se and "
                       "      j.vo_name=:vo_name and (j.reuse_job IS NOT NULL AND j.reuse_job != 'N') and "
                       "      j.job_state = 'SUBMITTED' and "
                       "      (f.hashed_id >= :hStart and f.hashed_id <= :hEnd) and "
                       "      f.wait_timestamp is null and "
                       "      (f.retry_timestamp is null or f.retry_timestamp < :tTime) "
                       "  order by j.priority DESC, j.submit_time "
                       ") where rownum <= 1 ",
                       soci::use(boost::get<0>(triplet)),
                       soci::use(boost::get<1>(triplet)),
                       soci::use(boost::get<2>(triplet)),
                       soci::use(hashSegment.start),
                       soci::use(hashSegment.end),
                       soci::use(tTime),
                       soci::into(job, isNull)
                       ;

                    if (isNull != soci::i_null)
                        {
                            soci::rowset<TransferFiles> rs =
                                (
                                    sql.prepare <<
                                    " SELECT  "
                                    "       f.file_state, f.source_surl, f.dest_surl, f.job_id, j.vo_name, "
                                    "       f.file_id, j.overwrite_flag, j.user_dn, j.cred_id, "
                                    "       f.checksum, j.checksum_method, j.source_space_token, "
                                    "       j.space_token, j.copy_pin_lifetime, j.bring_online, "
                                    "       f.user_filesize, f.file_metadata, j.job_metadata, f.file_index, "
                                    "       f.bringonline_token, f.source_se, f.dest_se, f.selection_strategy, "
                                    "       j.internal_job_params, j.user_cred, j.reuse_job "
                                    " FROM t_job j INNER JOIN t_file f ON (j.job_id = f.job_id) "
                                    " WHERE j.job_id = :job_id ",
                                    soci::use(job)
                                );

                            std::list<TransferFiles> tf;
                            for (soci::rowset<TransferFiles>::const_iterator ti = rs.begin(); ti != rs.end(); ++ti)
                                {
                                    tf.push_back(*ti);

                                }
                            if (!tf.empty())
                                files[boost::get<2>(triplet)].push(std::make_pair(job, tf));
                        }
                }
        }
    catch (std::exception& e)
        {
            files.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            files.clear();
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
}

void OracleAPI::submitPhysical(const std::string & jobId, std::list<job_element_tupple>& src_dest_pair,
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
    const std::string strictCopy       = params.get(JobParameterHandler::STRICT_COPY);

    std::string reuseFlag = "N";
    if (reuse == "Y")
        reuseFlag = "Y";
    else if (hop == "Y")
        reuseFlag = "H";

    //check if it's multiple-replica or multi-hop and set hashedId and file_index accordingly
    bool mreplica = is_mreplica(src_dest_pair);
    bool mhop     = is_mhop(src_dest_pair) || hop == "Y";

    if( reuseFlag != "N" && ((reuseFlag != "H" && mhop) || mreplica))
        {
            throw Err_Custom("Session reuse (-r) can't be used with multiple replicas or multi-hop jobs!");
        }

    if( (bringOnline > 0 || copyPinLifeTime > 0) && (mreplica || mhop || reuse == "Y"))
        {
            throw Err_Custom("bringOnline or copyPinLifeTime can't be used with multiple replicas or multi-hop jobs!");
        }

    if(mhop) //since H is not passed when plain text submission (e.g. glite client) we need to set into DB
        reuseFlag = "H";
    else if(mreplica)
        reuseFlag = "R";
    else if (reuse == "Y")
        reuseFlag = "Y"; //session reuse
    else
        reuseFlag = "N"; //plain job


    std::string initialState = bringOnline > 0 || copyPinLifeTime > 0 ? "STAGING" : "SUBMITTED";
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
    // strict-copy was specified
    if (strictCopy == "Y")
        {
            if (!jobParams.empty()) jobParams += ",";
            jobParams += "strict";
        }

    //multiple insert statements
    std::ostringstream pairQuery;
    std::ostringstream pairQuerySeBlaklisted;

    time_t now = time(NULL);
    struct tm tTime;
    gmtime_r(&now, &tTime);

    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
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
                                            soci::use(reuseFlag), soci::use(bringOnline),
                                            soci::use(retry), soci::use(retryDelay), soci::use(metadata),
                                            soci::use(sourceSe), soci::use(destinationSe));

            insertJob.execute(true);

            // Insert src/dest pair
            unsigned jobHashedId = 0;
            typedef std::pair<std::string, std::string> Key;
            typedef std::map< Key , int> Mapa;
            Mapa mapa;

            //create the insertion statements here and populate values inside the loop
            pairQuery << std::fixed << "INSERT ALL ";

            pairQuerySeBlaklisted << std::fixed << "INSERT ALL ";

            // When reuse is enabled, we use the same random number for the whole job
            // This guarantees that the whole set belong to the same machine, but keeping
            // the load balance between hosts
            jobHashedId = getHashedId();

            std::list<job_element_tupple>::iterator iter;
            int index = 0;
            int insert_index = 0;

            soci::statement insert_file_stmt(sql);

            for (iter = src_dest_pair.begin(); iter != src_dest_pair.end(); ++iter)
                {
                    ++insert_index;

                    if(iter->activity.empty())
                        iter->activity = "default";

                    /*
                        N = no reuse
                        Y = reuse
                        H = multi-hop
                        R = replica
                    */
                    if(bringOnline > 0 || copyPinLifeTime > 0)
                        {
                            iter->hashedId = jobHashedId;
                            iter->state = "STAGING";
                        }
                    else if (mreplica)
                        {
                            iter->fileIndex = 0;
                            iter->hashedId = jobHashedId;
                            if(index == 0) //only the first file
                                iter->state = "SUBMITTED";
                            else
                                iter->state = "NOT_USED";
                            index++;
                        }
                    else if (mhop)
                        {
                            iter->hashedId = jobHashedId;
                        }
                    else if(reuseFlag == "Y" && !mreplica && !mhop)
                        {
                            iter->hashedId = jobHashedId;
                        }
                    else if (reuseFlag == "N" && !mreplica && !mhop)
                        {
                            iter->hashedId = getHashedId();
                        }
                    else
                        {
                            iter->hashedId = getHashedId();
                        }

                    //get distinct source_se / dest_se
                    Key p1 (iter->source_se, iter->dest_se);
                    mapa.insert(std::make_pair(p1, 0));

                    if (iter->wait_timeout.is_initialized())
                        {
                            pairQuerySeBlaklisted
                                    << " INTO t_file (vo_name, job_id, file_state, source_surl, dest_surl,"
                                    "    checksum, user_filesize, file_metadata, selection_strategy, file_index, "
                                    "    source_se, dest_se, wait_timestamp, wait_timeout, activity, hashed_id) VALUES ("
                                    << ":voName" << insert_index << ", "
                                    << ":jobId" << insert_index << ", "
                                    << ":state" << insert_index << ", "
                                    << ":sourceSurl" << insert_index << ", "
                                    << ":destSurl" << insert_index << ", "
                                    << ":checksum" << insert_index << ", "
                                    << ":filesize" << insert_index << ", "
                                    << ":metadata" << insert_index << ", "
                                    << ":strategy" << insert_index << ", "
                                    << ":fileIndex" << insert_index << ", "
                                    << ":sourceSe" << insert_index << ", "
                                    << ":destSe" << insert_index << ", "
                                    << "sys_extract_utc(systimestamp), "
                                    << ":waitTimeout" << insert_index << ", "
                                    << ":activity" << insert_index << ", "
                                    << ":hashId" << insert_index
                                    << ") ";

                            insert_file_stmt.exchange(soci::use(voName));
                            insert_file_stmt.exchange(soci::use(jobId));
                            insert_file_stmt.exchange(soci::use(iter->state));
                            insert_file_stmt.exchange(soci::use(iter->source));
                            insert_file_stmt.exchange(soci::use(iter->destination));
                            insert_file_stmt.exchange(soci::use(iter->checksum));
                            insert_file_stmt.exchange(soci::use(iter->filesize));
                            insert_file_stmt.exchange(soci::use(iter->metadata));
                            insert_file_stmt.exchange(soci::use(iter->selectionStrategy));
                            insert_file_stmt.exchange(soci::use(iter->fileIndex));
                            insert_file_stmt.exchange(soci::use(iter->source_se));
                            insert_file_stmt.exchange(soci::use(iter->dest_se));
                            insert_file_stmt.exchange(soci::use(iter->wait_timeout.get()));
                            insert_file_stmt.exchange(soci::use(iter->activity));
                            insert_file_stmt.exchange(soci::use(iter->hashedId));
                        }
                    else
                        {
                            pairQuerySeBlaklisted
                                    << " INTO t_file (vo_name, job_id, file_state, source_surl, dest_surl,"
                                    "    checksum, user_filesize, file_metadata, selection_strategy, file_index, "
                                    "    source_se, dest_se, wait_timestamp, wait_timeout, activity, hashed_id) VALUES ("
                                    << ":voName" << insert_index << ", "
                                    << ":jobId" << insert_index << ", "
                                    << ":state" << insert_index << ", "
                                    << ":sourceSurl" << insert_index << ", "
                                    << ":destSurl" << insert_index << ", "
                                    << ":checksum" << insert_index << ", "
                                    << ":filesize" << insert_index << ", "
                                    << ":metadata" << insert_index << ", "
                                    << ":strategy" << insert_index << ", "
                                    << ":fileIndex" << insert_index << ", "
                                    << ":sourceSe" << insert_index << ", "
                                    << ":destSe" << insert_index << ", "
                                    << "NULL" << ", "
                                    << "NULL" << ", "
                                    << ":activity" << insert_index << ", "
                                    << ":hashId" << insert_index
                                    << ") ";

                            insert_file_stmt.exchange(soci::use(voName));
                            insert_file_stmt.exchange(soci::use(jobId));
                            insert_file_stmt.exchange(soci::use(iter->state));
                            insert_file_stmt.exchange(soci::use(iter->source));
                            insert_file_stmt.exchange(soci::use(iter->destination));
                            insert_file_stmt.exchange(soci::use(iter->checksum));
                            insert_file_stmt.exchange(soci::use(iter->filesize));
                            insert_file_stmt.exchange(soci::use(iter->metadata));
                            insert_file_stmt.exchange(soci::use(iter->selectionStrategy));
                            insert_file_stmt.exchange(soci::use(iter->fileIndex));
                            insert_file_stmt.exchange(soci::use(iter->source_se));
                            insert_file_stmt.exchange(soci::use(iter->dest_se));
                            insert_file_stmt.exchange(soci::use(iter->activity));
                            insert_file_stmt.exchange(soci::use(iter->hashedId));
                        }
                }

            std::string queryStr = pairQuerySeBlaklisted.str();

            queryStr += " SELECT * FROM dual ";

            insert_file_stmt.alloc();
            insert_file_stmt.prepare(queryStr);
            insert_file_stmt.define_and_bind();
            insert_file_stmt.execute(true);

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

void OracleAPI::getDmJobStatus(std::string requestID, bool archive, std::vector<JobStatus*>& jobs)
{
    soci::session sql(*connectionPool);

    std::string fileCountQuery;
    std::string statusQuery;

    if (archive)
        {
            fileCountQuery = "SELECT COUNT(file_id) FROM t_dm_backup WHERE t_dm_backup.job_id = :jobId";
            statusQuery = "SELECT t_job_backup.job_id, t_job_backup.job_state, t_dm_backup.file_state, "
                          "    t_job_backup.user_dn, t_job_backup.reason, t_job_backup.submit_time, t_job_backup.priority, "
                          "    t_job_backup.vo_name, t_dm_backup.file_id "
                          "FROM t_job_backup, t_dm_backup "
                          "WHERE t_dm_backup.job_id = :jobId and t_dm_backup.job_id= t_job_backup.job_id";
        }
    else
        {
            fileCountQuery = "SELECT COUNT(file_id) FROM t_dm WHERE t_dm.job_id = :jobId";
            statusQuery = "SELECT t_job.job_id, t_job.job_state, t_dm.file_state, "
                          "    t_job.user_dn, t_job.reason, t_job.submit_time, t_job.priority, "
                          "    t_job.vo_name, t_dm.file_id "
                          "FROM t_job, t_dm "
                          "WHERE t_dm.job_id = :jobId and t_dm.job_id=t_job.job_id";
        }

    try
        {
            long long numFiles = 0;
            sql << fileCountQuery, soci::use(requestID), soci::into(numFiles);

            soci::rowset<JobStatus> rs = (
                                             sql.prepare << statusQuery,
                                             soci::use(requestID, "jobId"));

            int index = 0;
            for (soci::rowset<JobStatus>::iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    JobStatus& job = *i;
                    job.numFiles = numFiles;
                    // make sure each deletion has different file index
                    job.fileIndex = index++;
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
void OracleAPI::listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates, std::string restrictToClientDN, std::string forDN, std::string VOname, std::string src, std::string dst)
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
                                     std::bind2nd(std::equal_to<std::string>(), std::string("CANCELED")));
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


            if (!src.empty())
                {
                    query << " AND source_se = :src ";
                    stmt.exchange(soci::use(src, "src"));
                }

            if (!dst.empty())
                {
                    query << " AND dest_se = :dst ";
                    stmt.exchange(soci::use(dst, "dst"));
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
                            if (job.numFiles > 0)
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

void OracleAPI::listRequestsDm(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates,
                               std::string restrictToClientDN, std::string forDN, std::string VOname, std::string src, std::string dst)
{
    soci::session sql(*connectionPool);

    try
        {
            std::ostringstream query;
            soci::statement stmt(sql);
            bool searchForCanceling = false;


            query << "SELECT DISTINCT job_id, job_state, reason, submit_time, user_dn, "
                  "                 vo_name, priority, cancel_job, "
                  "                 (SELECT COUNT(t_dm.file_id) FROM t_dm WHERE t_dm.job_id = t_job.job_id) as numFiles "
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
                                     std::bind2nd(std::equal_to<std::string>(), std::string("CANCELED")));
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

            if (!src.empty())
                {
                    query << " AND source_se = :src ";
                    stmt.exchange(soci::use(src, "src"));
                }

            if (!dst.empty())
                {
                    query << " AND dest_se = :dst ";
                    stmt.exchange(soci::use(dst, "dst"));
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
                            if (job.numFiles > 0)
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
                            "       t_file.staging_start, t_file.staging_finished, "
                            "       t_file_backup.reason, t_file_backup.start_time, "
                            "       t_file_backup.finish_time, t_file_backup.retry, t_file_backup.tx_duration "
                            "FROM t_file_backup WHERE t_file_backup.job_id = :jobId ";
                }
            else
                {
                    query = "SELECT * FROM (SELECT rownum as rn, t_file.file_id, t_file.source_surl, t_file.dest_surl, t_file.file_state, "
                            "       t_file.staging_start, t_file.staging_finished, "
                            "       t_file.reason, t_file.start_time, t_file.finish_time, t_file.retry, t_file.tx_duration "
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

void OracleAPI::getDmFileStatus(std::string requestID, bool archive, unsigned offset, unsigned limit, std::vector<FileTransferStatus*>& files)
{
    soci::session sql(*connectionPool);

    try
        {
            std::string query;

            if (archive)
                {
                    query = "SELECT * FROM (SELECT t_dm_backup.file_id, t_dm_backup.source_surl, t_dm_backup.dest_surl, t_dm_backup.file_state, "
                            "       t_dm_backup.reason, t_dm_backup.start_time, t_dm_backup.finish_time, t_dm_backup.retry, t_dm_backup.tx_duration "
                            "FROM t_dm_backup WHERE t_dm_backup.job_id = :jobId ";
                }
            else
                {
                    query = "SELECT * FROM (SELECT t_dm.file_id, t_dm.source_surl, t_dm.dest_surl, t_dm.file_state, "
                            "       t_dm.reason, t_dm.start_time, t_dm.finish_time, t_dm.retry, t_dm.tx_duration "
                            "FROM t_dm WHERE t_dm.job_id = :jobId ";
                }

            if (limit)
                query += " ) WHERE ROWNUM > :offset AND ROWNUM <= :offset + :limit";
            else
                query += " ) WHERE ROWNUM > :offset";



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
        int process_id, double filesize, double duration, bool retry)
{

    soci::session sql(*connectionPool);
    try
        {
            return updateFileTransferStatusInternal(sql, throughputIn, job_id, file_id, transfer_status, transfer_message, process_id, filesize, duration, retry);
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
        int process_id, double filesize, double duration, bool retry)
{
    try
        {
            sql.begin();

            double throughput = 0.0;

            bool staging = false;

            int current_failures = retry;

            time_t now = time(NULL);
            struct tm tTime;
            gmtime_r(&now, &tTime);

            std::string st;

            // query for the file state in DB
            sql << "SELECT file_state FROM t_file WHERE file_id=:fileId and job_id=:jobId",
                soci::use(file_id),
                soci::use(job_id),
                soci::into(st);

            staging = (st == "STAGING");

            // If file is in terminal and trying to set a non-terminal, don't do anything, just return
            if(st == "FAILED" || st == "FINISHED" || st == "CANCELED" )
            {
                if(transfer_status == "SUBMITTED" || transfer_status == "READY" || transfer_status == "ACTIVE")
                {
                    sql.rollback();
                    return false;
                }
            }

            // If the file already in the same state, don't do anything either
            if (st == transfer_status) {
                sql.rollback();
                return false;
            }

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


            query << ", transferHost = :hostname";
            stmt.exchange(soci::use(hostname, "hostname"));


            if (transfer_status == "FINISHED")
                {
                    query << ", transferred = :filesize";
                    stmt.exchange(soci::use(filesize, "filesize"));
                }

            if (transfer_status == "FAILED" || transfer_status == "CANCELED")
                {
                    query << ", transferred = :transferred";
                    stmt.exchange(soci::use(0, "transferred"));
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

            query << "   , pid = :pid, filesize = :filesize, tx_duration = :duration, throughput = :throughput, current_failures = :current_failures "
                  "WHERE file_id = :fileId AND file_state NOT IN ('FAILED', 'FINISHED', 'CANCELED')";
            stmt.exchange(soci::use(process_id, "pid"));
            stmt.exchange(soci::use(filesize, "filesize"));
            stmt.exchange(soci::use(duration, "duration"));
            stmt.exchange(soci::use(throughput, "throughput"));
            stmt.exchange(soci::use(current_failures, "current_failures"));
            stmt.exchange(soci::use(file_id, "fileId"));
            stmt.alloc();
            stmt.prepare(query.str());
            stmt.define_and_bind();
            stmt.execute(true);

            sql.commit();

            if(transfer_status == "FAILED")
                {
                    sql.begin();
                    useFileReplica(sql, job_id, file_id);
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
    return true;
}

bool OracleAPI::updateJobTransferStatus(std::string job_id, const std::string status, int pid)
{

    soci::session sql(*connectionPool);

    try
        {
            updateJobTransferStatusInternal(sql, job_id, status, pid);
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



bool OracleAPI::updateJobTransferStatusInternal(soci::session& sql, std::string job_id, const std::string status, int pid)
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
            soci::indicator isNull = soci::i_ok;

            std::string currentState("");
            std::string reuseFlag;

            if(job_id.empty())
                sql << " SELECT job_id from t_file where pid=:pid and job_finished is NOT NULL",soci::use(pid), soci::into(job_id);

            // prevent multiple updates of the same state for the same job
            sql <<
                " SELECT job_state, reuse_job from t_job  "
                " WHERE job_id = :job_id ",
                soci::use(job_id),
                soci::into(currentState),
                soci::into(reuseFlag, isNull)
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

                    // Update job_finished in files
                    sql << "UPDATE t_file SET "
                        " job_finished = sys_extract_utc(systimestamp) "
                        "WHERE job_id = :jobId AND job_finished IS NULL",
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

    try
        {
            double throughput = 0.0;
            double transferred = 0.0;
            int file_id = 0;
            std::string source_surl;
            std::string dest_surl;
            std::string source_turl;
            std::string dest_turl;
            std::string file_state;

            soci::statement stmt = (sql.prepare << "UPDATE t_file SET throughput = :throughput, transferred = :transferred WHERE file_id = :fileId  AND file_state not in ('FINISHED','FAILED','CANCELED') ",
                                    soci::use(throughput), soci::use(transferred), soci::use(file_id));


            sql.begin();

            std::vector<struct message_updater>::iterator iter;
            for (iter = messages.begin(); iter != messages.end(); ++iter)
                {
                    throughput = 0.0;
                    transferred = 0.0;
                    file_id = 0;
                    file_state = "";
                    source_surl = "";
                    dest_surl = "";
                    source_turl = "";
                    dest_turl = "";

                    if (iter->msg_errno == 0 && (*iter).file_id > 0)
                        {
                            file_state = std::string((*iter).transfer_status);

                            if(file_state == "ACTIVE")
                                {
                                    file_id = (*iter).file_id;

                                    if((*iter).throughput > 0.0 && file_id > 0)
                                        {
                                            throughput = (*iter).throughput;
                                            transferred = (*iter).transferred;
                                            stmt.execute(true);
                                        }
                                }
                            else
                                {
                                    source_surl = (*iter).source_surl;
                                    dest_surl = (*iter).dest_surl;
                                    source_turl = (*iter).source_turl;
                                    dest_turl = (*iter).dest_turl;
                                    file_id = (*iter).file_id;

                                    if(source_turl == "gsiftp:://fake" && dest_turl == "gsiftp:://fake")
                                        continue;

                                    if((*iter).throughput > 0.0 && file_id > 0)
                                        {
                                            throughput = (*iter).throughput;
                                        }

                                    if(file_state == "FINISHED" && file_id > 0)
                                        {
                                            sql <<  " MERGE INTO t_turl "
                                                " USING (SELECT :source_surl as source_surl, :destin_surl as destin_surl, :source_turl as source_turl, :destin_turl as destin_turl FROM dual)"
                                                " Pair ON (t_turl.source_surl = Pair.source_surl AND t_turl.destin_surl = Pair.destin_surl AND t_turl.source_turl = Pair.source_turl AND t_turl.destin_turl = Pair.destin_turl)"
                                                " WHEN MATCHED THEN UPDATE SET datetime = sys_extract_utc(systimestamp), throughput = :throughput, finish = finish + 1 "
                                                " WHEN NOT MATCHED THEN INSERT (source_surl, destin_surl, source_turl, destin_turl, datetime, throughput, finish) "
                                                " VALUES (Pair.source_surl, Pair.destin_surl, Pair.source_turl, Pair.destin_turl, sys_extract_utc(systimestamp), :throughput, 1)",
                                                soci::use(source_surl),
                                                soci::use(dest_surl),
                                                soci::use(source_turl),
                                                soci::use(dest_turl),
                                                soci::use(throughput),
                                                soci::use(throughput);
                                        }
                                    else if (file_state == "FAILED"  && file_id > 0)
                                        {
                                            sql <<  " MERGE INTO t_turl "
                                                " USING (SELECT :source_surl as source_surl, :destin_surl as destin_surl, :source_turl as source_turl, :destin_turl as destin_turl FROM dual)"
                                                " Pair ON (t_turl.source_surl = Pair.source_surl AND t_turl.destin_surl = Pair.destin_surl AND t_turl.source_turl = Pair.source_turl AND t_turl.destin_turl = Pair.destin_turl)"
                                                " WHEN MATCHED THEN UPDATE SET datetime = sys_extract_utc(systimestamp), throughput = :throughput, fail = fail + 1 "
                                                " WHEN NOT MATCHED THEN INSERT (source_surl, destin_surl, source_turl, destin_turl, datetime, throughput, fail) "
                                                " VALUES (Pair.source_surl, Pair.destin_surl, Pair.source_turl, Pair.destin_turl, sys_extract_utc(systimestamp), :throughput, 1)",
                                                soci::use(source_surl),
                                                soci::use(dest_surl),
                                                soci::use(source_turl),
                                                soci::use(dest_turl),
                                                soci::use(throughput),
                                                soci::use(throughput);
                                        }
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
            std::string job_id;

            soci::statement stmt1 = (sql.prepare << "UPDATE t_job SET job_state = 'CANCELED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), cancel_job='Y' "
                                     "                 ,reason = :reason "
                                     "WHERE job_id = :jobId AND job_state NOT IN ('CANCELED','FINISHEDDIRTY', 'FINISHED', 'FAILED')",
                                     soci::use(reason, "reason"), soci::use(job_id, "jobId"));

            soci::statement stmt2 = (sql.prepare << "UPDATE t_file SET file_state = 'CANCELED',  finish_time = sys_extract_utc(systimestamp), "
                                     "                  reason = :reason "
                                     "WHERE job_id = :jobId AND file_state NOT IN ('CANCELED','FINISHED','FAILED')",
                                     soci::use(reason, "reason"), soci::use(job_id, "jobId"));

            sql.begin();

            for (std::vector<std::string>::const_iterator i = requestIDs.begin(); i != requestIDs.end(); ++i)
                {
                    job_id = (*i);

                    // Cancel job
                    stmt1.execute(true);

                    // Cancel files
                    stmt2.execute(true);
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


void OracleAPI::cancelAllJobs(const std::string& voName, std::vector<std::string>& canceledJobs)
{
    soci::session sql(*connectionPool);

    try
        {
            std::string jobId;
            std::ostringstream selectQuery, jobQuery, fileQuery, dmQuery;
            sql.begin();

            // First recover the jobs ids that will be canceled
            soci::statement getIdsStmt(sql);

            selectQuery << "SELECT job_id FROM t_job "
                        " WHERE job_state NOT IN ('CANCELED','FINISHEDDIRTY', 'FINISHED', 'FAILED') AND job_finished IS NULL";
            if (!voName.empty())
                {
                    selectQuery << " AND vo_name = :vo_name";
                    getIdsStmt.exchange(soci::use(voName));
                }

            getIdsStmt.exchange(soci::into(jobId));
            getIdsStmt.alloc();
            getIdsStmt.prepare(selectQuery.str());
            getIdsStmt.define_and_bind();

            if (getIdsStmt.execute(true))
                {
                    do
                        {
                            canceledJobs.push_back(jobId);
                        }
                    while (getIdsStmt.fetch());
                }

            // Bulk cancel jobs
            soci::statement cancelJobsStmt(sql);

            jobQuery << "UPDATE t_job "
                     "SET job_state = 'CANCELED', job_finished = UTC_TIMESTAMP(),"
                     "        finish_time = UTC_TIMESTAMP(), cancel_job='Y',"
                     "        reason='Jobs canceled by the site admin' "
                     "WHERE job_state NOT IN ('CANCELED','FINISHEDDIRTY', 'FINISHED', 'FAILED') AND job_finished IS NULL";
            if (!voName.empty())
                {
                    jobQuery << " AND vo_name = :vo_name";
                    cancelJobsStmt.exchange(soci::use(voName));
                }

            cancelJobsStmt.alloc();
            cancelJobsStmt.prepare(jobQuery.str());
            cancelJobsStmt.define_and_bind();
            cancelJobsStmt.execute(true);

            // Bulk cancel files
            soci::statement cancelFilesStmt(sql);

            fileQuery << "UPDATE t_file "
                      "SET file_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), "
                      "    finish_time = UTC_TIMESTAMP(), reason='Jobs canceled by the site admin' "
                      "WHERE file_state NOT IN ('CANCELED','FINISHED', 'FAILED') AND job_finished IS NULL";
            if (!voName.empty())
                {
                    fileQuery << " AND vo_name = :vo_name";
                    cancelFilesStmt.exchange(soci::use(voName));
                }

            cancelFilesStmt.alloc();
            cancelFilesStmt.prepare(fileQuery.str());
            cancelFilesStmt.define_and_bind();
            cancelFilesStmt.execute(true);

            // Bulk cancel namespace operations
            soci::statement cancelDmStmt(sql);

            dmQuery << "UPDATE t_dm "
                    "SET file_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), "
                    "    finish_time = UTC_TIMESTAMP(), reason='Jobs canceled by the site admin' "
                    "WHERE file_state NOT IN ('CANCELED','FINISHED', 'FAILED') AND job_finished IS NULL";
            if (!voName.empty())
                {
                    dmQuery << " AND vo_name = :vo_name";
                    cancelDmStmt.exchange(soci::use(voName));
                }

            cancelDmStmt.alloc();
            cancelDmStmt.prepare(dmQuery.str());
            cancelDmStmt.define_and_bind();
            cancelDmStmt.execute(true);

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


void OracleAPI::getCancelJob(std::vector<int>& requestIDs)
{
    soci::session sql(*connectionPool);
    long long pid = 0;
    long long file_id = 0;

    try
        {
            soci::rowset<soci::row> rs = (sql.prepare << " select distinct pid, file_id from t_file where "
                                          " file_state='CANCELED' and job_finished is NULL "
                                          " AND (hashed_id >= :hStart AND hashed_id <= :hEnd) "
                                          " AND staging_start is NULL ",
                                          soci::use(hashSegment.start), soci::use(hashSegment.end));

            soci::statement stmt1 = (sql.prepare << "UPDATE t_file SET  job_finished = sys_extract_utc(systimestamp) "
                                     "WHERE file_id = :file_id ",
                                     soci::use(pid, "file_id"));

            // Cancel files
            sql.begin();
            for (soci::rowset<soci::row>::const_iterator i2 = rs.begin(); i2 != rs.end(); ++i2)
                {
                    soci::row const& row = *i2;
                    pid = row.get<long long>("PID",0);
                    file_id = row.get<long long>("FILE_ID");

                    if(pid > 0)
                        requestIDs.push_back(boost::lexical_cast<int>(pid));

                    stmt1.execute(true);
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
bool OracleAPI::insertCredentialCache(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs)
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
    catch (soci::oracle_soci_error const &e)
        {
            sql.rollback();
            unsigned int err_code = e.err_num_;

            // the magic '1' is the error code of
            // ORA-00001: unique constraint (XXX) violated
            if (err_code == 1) return false;

            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
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

    return true;
}



CredCache* OracleAPI::findCredentialCache(std::string delegationID, std::string dn)
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



void OracleAPI::deleteCredentialCache(std::string delegationID, std::string dn)
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



void OracleAPI::insertCredential(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time)
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



void OracleAPI::updateCredential(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time)
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



Cred* OracleAPI::findCredential(std::string delegationID, std::string dn)
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



void OracleAPI::deleteCredential(std::string delegationID, std::string dn)
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



unsigned OracleAPI::getDebugLevel(std::string source_hostname, std::string destin_hostname)
{
    soci::session sql(*connectionPool);

    try
        {
            unsigned level = 0;
            soci::indicator isNull = soci::i_ok;


            sql <<
                " SELECT debug_level "
                " FROM t_debug "
                " WHERE source_se = :source OR dest_se= :dest_se ",
                soci::use(source_hostname),
                soci::use(destin_hostname),
                soci::into(level, isNull)
                ;

            if(isNull != soci::i_null)
                return level;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return 0;
}



void OracleAPI::setDebugLevel(std::string source_hostname, std::string destin_hostname, unsigned level)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            if (!source_hostname.empty())
                {
                    sql << "DELETE FROM t_debug WHERE source_se = :source AND dest_se IS NULL",
                        soci::use(source_hostname);
                    sql << "INSERT INTO t_debug (source_se, debug_level) VALUES (:source, :level)",
                        soci::use(source_hostname), soci::use(level);
                }
            if (!destin_hostname.empty())
                {
                    sql << "DELETE FROM t_debug WHERE source_se IS NULL AND dest_se = :dest",
                        soci::use(destin_hostname);
                    sql << "INSERT INTO t_debug (dest_se, debug_level) VALUES (:dest, :level)",
                        soci::use(destin_hostname), soci::use(level);
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

bool OracleAPI::isTrAllowed(const std::string & source_hostname, const std::string & destin_hostname, int &currentActive)
{
    soci::session sql(*connectionPool);

    int maxActive = 0;
    int active = 0;
    bool allowed = false;
    soci::indicator isNull = soci::i_ok;
    int max_per_se = 0;
    int max_per_link = 0;;

    try
        {
            int highDefault = MIN_ACTIVE;

            sql << "SELECT max_per_se, max_per_link "
                "FROM t_server_config "
                "WHERE vo_name IS NULL OR vo_name = '*'",
                soci::into(max_per_se), soci::into(max_per_link);


            if(max_per_link > 0)
                MAX_ACTIVE_PER_LINK = max_per_link;
            if(max_per_se > 0)
                MAX_ACTIVE_ENDPOINT_LINK = max_per_se;


            soci::statement stmt1 = (
                                        sql.prepare << "SELECT active FROM t_optimize_active "
                                        "WHERE source_se = :source AND dest_se = :dest_se ",
                                        soci::use(source_hostname),soci::use(destin_hostname), soci::into(maxActive, isNull));
            stmt1.execute(true);

            soci::statement stmt2 = (
                                        sql.prepare << "SELECT count(*) FROM t_file "
                                        "WHERE source_se = :source AND dest_se = :dest_se and file_state = 'ACTIVE' ",
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

            currentActive = active;

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

void OracleAPI::getMaxActive(soci::session& sql, int& source, int& destination, const std::string & source_hostname, const std::string & destination_hostname)
{
    int maxActiveSource = 0;
    int maxActiveDest = 0;
    int max_per_se = 0;
    int max_per_link = 0;

    try
        {
            sql << "SELECT max_per_se, max_per_link "
                "FROM t_server_config "
                "WHERE vo_name IS NULL OR vo_name = '*'",
                soci::into(max_per_se), soci::into(max_per_link);

            if(max_per_link > 0)
                MAX_ACTIVE_PER_LINK = max_per_link;
            if(max_per_se > 0)
                MAX_ACTIVE_ENDPOINT_LINK = max_per_se;

            int maxDefault = MAX_ACTIVE_ENDPOINT_LINK;

            //check for source
            sql << " select active from t_optimize where source_se = :source_se and active is not NULL ",
                soci::use(source_hostname),
                soci::into(maxActiveSource);

            if(!sql.got_data())
                {
                    source = maxDefault;
                }
            else if(sql.got_data() && maxActiveSource == -1)
                {
                    source = maxDefault;
                }
            else if(sql.got_data() && maxActiveSource == 0)
                {
                    source = 0; //stop processing for this source endpoint
                }
            else
                {
                    source = maxActiveSource;
                }

            sql << " select active from t_optimize where dest_se = :dest_se and active is not NULL ",
                soci::use(destination_hostname),
                soci::into(maxActiveDest);

            if(!sql.got_data())
                {
                    destination = maxDefault;
                }
            else if(sql.got_data() && maxActiveDest == -1)
                {
                    destination = maxDefault;
                }
            else if(sql.got_data() && maxActiveDest == 0)
                {
                    destination = 0; //stop processing for this destination endpoint
                }
            else
                {
                    destination = maxActiveDest;
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

bool OracleAPI::updateOptimizer()
{
    //prevent more than on server to update the optimizer decisions
    if(hashSegment.start != 0)
        return false;

    soci::session sql(*connectionPool);

    bool recordsFound = false;
    std::string source_hostname;
    std::string destin_hostname;
    int active = 0;
    int maxActive = 0;
    soci::indicator isNullRetry = soci::i_ok;
    soci::indicator isNullMaxActive = soci::i_ok;
    soci::indicator isNullRate = soci::i_ok;
    soci::indicator isNullFixed = soci::i_ok;
    double retry = 0.0;   //latest from db
    double lastSuccessRate = 0.0;
    long long retrySet = 0;
    soci::indicator isRetry = soci::i_ok;
    soci::indicator isNullStreamsFound = soci::i_ok;
    long long streamsFound = 0;
    long long recordFound = 0;
    int insertStreams = -1;
    soci::indicator isNullStartTime = soci::i_ok;
    soci::indicator isNullRecordsFound = soci::i_ok;
    long long int streamsCurrent = 0;
    soci::indicator isNullStreamsCurrent = soci::i_ok;
    long long singleDest = 0;
    bool lanTransferBool = false;
    double ema = 0.0;
    double submitted = 0.0;
    std::string active_fixed;
    soci::indicator isNullStreamsOptimization = soci::i_ok;
    soci::indicator isNullDatetime = soci::i_ok;
    int maxNoStreams = 16;
    int nostreams = 1;
    double throughput=0.0;
    double maxThroughput = 0.0;
    long long int testedThroughput = 0;
    int updateStream = 0;
    int allTested = 0;
    int activeSource = 0;
    int activeDestination = 0;
    double avgDuration = 0.0;
    soci::indicator isNullAvg = soci::i_ok;
    int max_per_se = 0;
    int max_per_link = 0;


    try
        {
            //check optimizer level, minimum active per link
            int highDefault = MIN_ACTIVE;


            //based on the level, how many transfers will be spawned
            int spawnActive = getOptimizerDefaultMode(sql);

            //fetch the records from db for distinct links
            soci::rowset<soci::row> rs = ( sql.prepare <<
                                           " select  distinct o.source_se, o.dest_se from t_optimize_active o INNER JOIN "
                                           " t_file f ON (o.source_se = f.source_se) where o.dest_se=f.dest_se and "
                                           " f.file_state in ('ACTIVE','SUBMITTED') and f.job_finished is NULL ");

            soci::statement stmtActiveSource = (
                                                   sql.prepare << "SELECT count(*) FROM t_file "
                                                   "WHERE source_se = :source and file_state = 'ACTIVE' and job_finished is null ",
                                                   soci::use(source_hostname), soci::into(activeSource));

            soci::statement stmtActiveDest = (
                                                 sql.prepare << "SELECT count(*) FROM t_file "
                                                 "WHERE dest_se = :dest_se and file_state = 'ACTIVE' and job_finished is null ",
                                                 soci::use(destin_hostname), soci::into(activeDestination));


            //is the number of actives fixed?
            soci::statement stmt_fixed = (
                                             sql.prepare << "SELECT fixed from t_optimize_active "
                                             "WHERE source_se = :source AND dest_se = :dest",
                                             soci::use(source_hostname), soci::use(destin_hostname), soci::into(active_fixed, isNullFixed));

            soci::statement stmt_avg_duration = (
                                                    sql.prepare << "SELECT avg(tx_duration)  from t_file "
                                                    " WHERE source_se = :source AND dest_se = :dest and file_state='FINISHED' and tx_duration > 0 AND tx_duration is NOT NULL and "
                                                    " job_finished > (sys_extract_utc(systimestamp) - interval '30' minute) ",
                                                    soci::use(source_hostname), soci::use(destin_hostname), soci::into(avgDuration, isNullAvg));

            //snapshot of active transfers
            soci::statement stmt7 = (
                                        sql.prepare << "SELECT count(*) FROM t_file "
                                        "WHERE source_se = :source AND dest_se = :dest_se and file_state = 'ACTIVE' and job_finished is null ",
                                        soci::use(source_hostname),soci::use(destin_hostname), soci::into(active));

            //max number of active allowed per link
            soci::statement stmt8 = (
                                        sql.prepare << "SELECT active, ema FROM t_optimize_active "
                                        "WHERE source_se = :source AND dest_se = :dest_se ",
                                        soci::use(source_hostname),soci::use(destin_hostname), soci::into(maxActive, isNullMaxActive), soci::into(ema));

            //sum of retried transfers per link
            soci::statement stmt9 = (
                                        sql.prepare << "select * from (SELECT sum(retry) from t_file WHERE source_se = :source AND dest_se = :dest_se and "
                                        "file_state in ('ACTIVE','SUBMITTED') order by start_time DESC) WHERE ROWNUM <= 50 ",
                                        soci::use(source_hostname),soci::use(destin_hostname), soci::into(retry, isNullRetry));

            soci::statement stmt10 = (
                                         sql.prepare << "update t_optimize_active set active=:active, ema=:ema, datetime=sys_extract_utc(systimestamp) where "
                                         " source_se=:source and dest_se=:dest ",
                                         soci::use(active), soci::use(ema), soci::use(source_hostname), soci::use(destin_hostname));


            soci::statement stmt18 = (
                                         sql.prepare << " select count(distinct source_se) from t_file where "
                                         " file_state in ('ACTIVE','SUBMITTED') and "
                                         " dest_se=:dest and "
                                         " job_finished is null",
                                         soci::use(destin_hostname), soci::into(singleDest));

            //snapshot of submitted transfers
            soci::statement stmt19 = (
                                         sql.prepare << "SELECT count(*) FROM t_file "
                                         "WHERE source_se = :source AND dest_se = :dest_se and file_state ='SUBMITTED' and job_finished is null ",
                                         soci::use(source_hostname),soci::use(destin_hostname), soci::into(submitted));



            //check if retry is set at global level
            sql <<
                " SELECT retry "
                " FROM t_server_config ",soci::into(retrySet, isRetry)
                ;

            //if not set, flag as 0
            if (isRetry == soci::i_null || retrySet == 0)
                retrySet = 0;


            /* Start of TCP streams optimization "zone" */
            soci::statement stmt20 = (
                                         sql.prepare << "SELECT max(nostreams) FROM t_optimize_streams  WHERE source_se=:source_se and dest_se=:dest_se ",
                                         soci::use(source_hostname),
                                         soci::use(destin_hostname),
                                         soci::into(nostreams, isNullStreamsOptimization));

            soci::statement stmt23 = (
                                         sql.prepare << "SELECT throughput FROM t_optimize_streams  WHERE source_se=:source_se and dest_se=:dest_se and "
                                         " tested=1 and nostreams = :nostreams and throughput is not NULL   and throughput > 0",
                                         soci::use(source_hostname),
                                         soci::use(destin_hostname),
                                         soci::use(nostreams),
                                         soci::into(maxThroughput));
            soci::statement stmt24 = (
                                         sql.prepare << "SELECT nostreams FROM (SELECT nostreams FROM t_optimize_streams   "
                                         " WHERE source_se=:source_se and dest_se=:dest_se and tested=1 ORDER BY throughput DESC) WHERE  rownum <= 1 ",
                                         soci::use(source_hostname),
                                         soci::use(destin_hostname),
                                         soci::into(updateStream));

            soci::statement stmt26 = (
                                         sql.prepare << "SELECT tested FROM t_optimize_streams  WHERE source_se=:source_se AND dest_se=:dest_se "
                                         " AND throughput IS NOT NULL and throughput > 0 and tested = 1  ",
                                         soci::use(source_hostname),
                                         soci::use(destin_hostname),
                                         soci::into(testedThroughput));

            soci::statement stmt28 = (
                                         sql.prepare << " UPDATE t_optimize_streams set throughput = :throughput, datetime = sys_extract_utc(systimestamp) "
                                         " WHERE source_se=:source_se AND dest_se=:dest_se AND nostreams = :nostreams   and tested = 1   ",
                                         soci::use(throughput),
                                         soci::use(source_hostname),
                                         soci::use(destin_hostname),
                                         soci::use(nostreams));

            soci::statement stmt29 = (
                                         sql.prepare << " select count(*) from  t_optimize_streams where "
                                         " source_se=:source_se AND dest_se=:dest_se AND tested = 1 and throughput IS NOT NULL  and throughput > 0",
                                         soci::use(source_hostname),
                                         soci::use(destin_hostname),
                                         soci::into(allTested));

            sql << "select max_per_se, max_per_link from t_server_config", soci::into(max_per_se), soci::into(max_per_link);

            if(max_per_link > 0)
                MAX_ACTIVE_PER_LINK = max_per_link;
            if(max_per_se > 0)
                MAX_ACTIVE_ENDPOINT_LINK = max_per_se;


            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {

                    recordsFound = true;

                    source_hostname = i->get<std::string>("SOURCE_SE");
                    destin_hostname = i->get<std::string>("DEST_SE");

                    sql.begin();
                    sql << " UPDATE t_optimize_active set datetime = sys_extract_utc(systimestamp) WHERE source_se=:source_se and dest_se=:dest_se",
                        soci::use(source_hostname),soci::use(destin_hostname);
                    sql.commit();

                    double nFailedLastHour=0.0, nFinishedLastHour=0.0;
                    throughput=0.0;
                    double filesize = 0.0;
                    double totalSize = 0.0;
                    retry = 0.0;   //latest from db
                    double retryStored = 0.0; //stored in mem
                    double thrStored = 0.0; //stored in mem
                    double rateStored = 0.0; //stored in mem
                    int activeStored = 0; //stored in mem
                    double ratioSuccessFailure = 0.0;
                    int thrSamplesStored = 0; //stored in mem
                    int throughputSamples = 0;
                    active = 0;
                    maxActive = 0;
                    isNullRetry = soci::i_ok;
                    isNullMaxActive = soci::i_ok;
                    lastSuccessRate = 0.0;
                    isNullRate = soci::i_ok;
                    isNullStreamsFound = soci::i_ok;
                    streamsFound = 0;
                    recordFound = 0;
                    insertStreams = -1;
                    isNullStartTime = soci::i_ok;
                    isNullRecordsFound = soci::i_ok;
                    streamsCurrent = 0;
                    isNullStreamsCurrent = soci::i_ok;
                    singleDest = 0;
                    lanTransferBool = false;
                    submitted = 0.0;
                    ema = 0.0;
                    nostreams = 1;
                    isNullStreamsOptimization = soci::i_ok;
                    isNullDatetime = soci::i_ok;
                    maxThroughput = 0.0;
                    testedThroughput = 0;
                    updateStream = 0;
                    struct tm datetimeStreams;
                    soci::indicator isNullStreamsdatetimeStreams = soci::i_ok;
                    allTested = 0;
                    activeSource = 0;
                    activeDestination = 0;
                    avgDuration = 0.0;
                    isNullAvg = soci::i_ok;
                    active_fixed = "off";
                    highDefault = MIN_ACTIVE;

                    // Weighted average
                    soci::rowset<soci::row> rsSizeAndThroughput = (sql.prepare <<
                            "   SELECT filesize, throughput "
                            "   FROM t_file "
                            "   WHERE source_se = :source AND dest_se = :dest AND "
                            "       file_state IN ('ACTIVE','FINISHED') AND throughput > 0 AND "
                            "       filesize > 0  AND (job_finished is NULL OR"
                            "        job_finished >= (sys_extract_utc(systimestamp) - interval '1' minute)) ",
                            soci::use(source_hostname),soci::use(destin_hostname));

                    for (soci::rowset<soci::row>::const_iterator j = rsSizeAndThroughput.begin();
                            j != rsSizeAndThroughput.end(); ++j)
                        {
                            filesize    = static_cast<double>(j->get<long long>("FILESIZE", 0.0));
                            throughput += (j->get<double>("THROUGHPUT", 0.0) * filesize);
                            totalSize  += filesize;
                        }
                    if (totalSize > 0)
                        throughput /= totalSize;

                    if(spawnActive == 2) //only executhe streams optimization when level/plan is 2
                        {

                            /* apply streams optimization, no matter the level here since if it's switch to level 2 to have info ready*/

                            //get max streams
                            stmt20.execute(true);

                            //make sure that the current stream has been tested, throughput is not null and tested = 1
                            stmt26.execute(true);

                            stmt23.execute(true);

                            stmt29.execute(true);


                            if (isNullStreamsOptimization == soci::i_ok) //there is at least one entry
                                {
                                    if(nostreams < maxNoStreams && allTested < maxNoStreams) //haven't completed yet with 1-16 TCP streams range
                                        {
                                            sql << " SELECT max(datetime) FROM t_optimize_streams  WHERE source_se=:source_se and "
                                                " dest_se=:dest_se and nostreams = :nostreams and tested = 1 and throughput is NOT NULL and throughput > 0   ",
                                                soci::use(source_hostname),
                                                soci::use(destin_hostname),
                                                soci::use(nostreams),
                                                soci::into(datetimeStreams, isNullStreamsdatetimeStreams);

                                            bool timeIsOk = false;
                                            if (isNullStreamsdatetimeStreams == soci::i_ok)
                                                timeIsOk = true;

                                            time_t lastTime = timegm(&datetimeStreams); //from db
                                            time_t now = getUTC(0);
                                            double diff = difftime(now, lastTime);

                                            if(timeIsOk && diff >= STREAMS_UPDATE_SAMPLE && testedThroughput == 1 && maxThroughput > 0.0) //every 15min experiment with diff number of streams
                                                {
                                                    nostreams += 1;
                                                    throughput = 0.0;
                                                    sql.begin();
                                                    sql << " MERGE INTO t_optimize_streams USING "
                                                        " (SELECT :source as source, :dest as dest, :streams as streams FROM dual) Pair "
                                                        " ON (t_optimize_streams.source_se = Pair.source AND t_optimize_streams.dest_se = Pair.dest AND t_optimize_streams.nostreams = Pair.streams) "
                                                        " WHEN MATCHED THEN UPDATE SET throughput = :throughput where tested = 1 "
                                                        " WHEN NOT MATCHED THEN INSERT (source_se, dest_se, nostreams, datetime, throughput, tested) "
                                                        " VALUES (Pair.source, Pair.dest, Pair.streams, sys_extract_utc(systimestamp), :throughput, 0)",
                                                        soci::use(source_hostname),
                                                        soci::use(destin_hostname),
                                                        soci::use(nostreams),
                                                        soci::use(throughput),
                                                        soci::use(throughput);
                                                    sql.commit();
                                                }
                                            else
                                                {
                                                    if(throughput > 0.0)
                                                        {
                                                            sql.begin();
                                                            sql << " MERGE INTO t_optimize_streams USING "
                                                                " (SELECT :source as source, :dest as dest, :streams as streams FROM dual) Pair "
                                                                " ON (t_optimize_streams.source_se = Pair.source AND t_optimize_streams.dest_se = Pair.dest AND t_optimize_streams.nostreams = Pair.streams) "
                                                                " WHEN MATCHED THEN UPDATE SET throughput = :throughput where tested = 1 "
                                                                " WHEN NOT MATCHED THEN INSERT (source_se, dest_se, nostreams, datetime, throughput, tested) "
                                                                " VALUES (Pair.source, Pair.dest, Pair.streams, sys_extract_utc(systimestamp), :throughput, 0)",
                                                                soci::use(source_hostname),
                                                                soci::use(destin_hostname),
                                                                soci::use(nostreams),
                                                                soci::use(throughput),
                                                                soci::use(throughput);
                                                            sql.commit();
                                                        }
                                                }
                                        }
                                    else //all samples taken, max is 16 streams
                                        {
                                            stmt24.execute(true);	//get current stream used with max throughput
                                            nostreams = updateStream;

                                            sql << " SELECT max(datetime) FROM t_optimize_streams  WHERE source_se=:source_se and dest_se=:dest_se "
                                                " and nostreams = :nostreams and tested = 1 and throughput is NOT NULL and throughput > 0  ",
                                                soci::use(source_hostname),
                                                soci::use(destin_hostname),
                                                soci::use(nostreams),
                                                soci::into(datetimeStreams, isNullStreamsdatetimeStreams);

                                            bool timeIsOk = false;
                                            if (isNullStreamsdatetimeStreams == soci::i_ok)
                                                timeIsOk = true;

                                            time_t lastTime = timegm(&datetimeStreams); //from db
                                            time_t now = getUTC(0);
                                            double diff = difftime(now, lastTime);

                                            if (timeIsOk && diff >= STREAMS_UPDATE_MAX && throughput > 0.0) //almost half a day has passed, compare throughput with max sample
                                                {
                                                    sql.begin();
                                                    stmt28.execute(true);	//update stream currently used with new throughput and timestamp this time
                                                    sql.commit();
                                                }
                                        }
                                }
                            else //it's NULL, no sample yet, insert the first record for this pair
                                {
                                    throughput = 0.0;
                                    sql.begin();
                                    sql << " MERGE INTO t_optimize_streams USING "
                                        " (SELECT :source as source, :dest as dest, :streams as streams FROM dual) Pair "
                                        " ON (t_optimize_streams.source_se = Pair.source AND t_optimize_streams.dest_se = Pair.dest AND t_optimize_streams.nostreams = Pair.streams) "
                                        " WHEN MATCHED THEN UPDATE SET throughput = :throughput where tested = 1 "
                                        " WHEN NOT MATCHED THEN INSERT (source_se, dest_se, nostreams, datetime, throughput, tested) "
                                        " VALUES (Pair.source, Pair.dest, Pair.streams, sys_extract_utc(systimestamp), :throughput, 0)",
                                        soci::use(source_hostname),
                                        soci::use(destin_hostname),
                                        soci::use(nostreams),
                                        soci::use(throughput),
                                        soci::use(throughput);
                                    sql.commit();
                                }
                        }

                    lanTransferBool = lanTransfer(source_hostname, destin_hostname);
                    if(lanTransferBool)
                        {
                            highDefault = LAN_ACTIVE;
                        }

                    // first thing, check if the number of actives have been fixed for this pair
                    stmt_fixed.execute(true);
                    if (isNullFixed == soci::i_ok && active_fixed == "on")
                        continue;

                    //get the average transfer duration for this link
                    stmt_avg_duration.execute(true);

                    int calcutateTimeFrame = 0;
                    if(avgDuration > 0 && avgDuration < 30)
                        calcutateTimeFrame  = 5;
                    else if(avgDuration > 30 && avgDuration < 900)
                        calcutateTimeFrame  = 15;
                    else
                        calcutateTimeFrame  = 30;


                    // Ratio of success
                    soci::rowset<soci::row> rs = (sql.prepare <<
                                                  " SELECT file_state, retry, current_failures, reason "
                                                  " FROM t_file "
                                                  " WHERE "
                                                  "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
                                                  "      ( "
                                                  "		    (t_file.job_finished is NULL AND current_failures > 0)  OR "
                                                  "		    (t_file.job_finished > (sys_extract_utc(systimestamp) - numtodsinterval(:calcutateTimeFrame, 'minute'))) "
                                                  "	     ) "
                                                  "	    AND file_state IN ('FAILED','FINISHED','SUBMITTED') ",
                                                  soci::use(source_hostname), soci::use(destin_hostname), soci::use(calcutateTimeFrame));


                    //we need to exclude non-recoverable errors so as not to count as failures and affect effiency
                    for (soci::rowset<soci::row>::const_iterator i = rs.begin();
                            i != rs.end(); ++i)
                        {
                            std::string state = i->get<std::string>("FILE_STATE", "");
                            int retryNum = static_cast<int>(i->get<double>("RETRY", 0.0));
                            int current_failures = static_cast<int>(i->get<long long>("CURRENT_FAILURES", 0.0));
                            std::string reason = i->get<std::string>("REASON", "");

                            //we do not want BringOnline errors to affect transfer success rate, exclude them
                            bool exists1 = (reason.find("BringOnline") != std::string::npos);
                            bool exists2 = (reason.find("bring-online") != std::string::npos);

                            if(state.compare("FAILED") == 0 && (exists1 || exists2) )
                                {
                                    //do nothing, it's a non recoverable error so do not consider it
                                }
                            else if(state.compare("FAILED") == 0 && current_failures == 0)
                                {
                                    //do nothing, it's a non recoverable error so do not consider it
                                }
                            else if ( (state.compare("FAILED") == 0 || state.compare("SUBMITTED") == 0) && retryNum > 0)
                                {
                                    nFailedLastHour+=1.0;
                                }
                            else if(state.compare("FAILED") == 0 && current_failures == 1)
                                {
                                    nFailedLastHour+=1.0;
                                }
                            else if (state.compare("FINISHED") == 0)
                                {
                                    nFinishedLastHour+=1.0;
                                }
                        }

                    //round up efficiency
                    if(nFinishedLastHour > 0.0)
                        {
                            ratioSuccessFailure = ceil(nFinishedLastHour/(nFinishedLastHour + nFailedLastHour) * (100.0/1.0));
                        }

                    // Active transfers
                    stmt7.execute(true);

                    //get submitted for this link
                    stmt19.execute(true);

                    //check if there is any other source for a given dest
                    stmt18.execute(true);

                    // Max active transfers
                    stmt8.execute(true);

                    //check if have been retried
                    if (retrySet > 0)
                        {
                            stmt9.execute(true);
                            if (isNullRetry == soci::i_null)
                                retry = 0;
                        }

                    if (isNullMaxActive == soci::i_null)
                        maxActive = highDefault;

                    //The smaller alpha becomes the longer moving average is. ( e.g. it becomes smoother, but less reactive to new samples )
                    double throughputEMA = ceil(exponentialMovingAverage( throughput, EMA, ema));

                    //only apply the logic below if any of these values changes
                    bool changed = getChangedFile (source_hostname, destin_hostname, ratioSuccessFailure, rateStored, throughputEMA, thrStored, retry, retryStored, maxActive, activeStored, throughputSamples, thrSamplesStored);
                    if(!changed && retry > 0)
                        changed = true;
                    else if(ratioSuccessFailure == 0)
                        changed = true;

                    //check if bandwidth limitation exists, if exists and throughput exceeds the limit then do not proccess with auto-tuning
                    int bandwidthIn = 0;
                    bool bandwidth = bandwidthChecker(sql, source_hostname, destin_hostname, bandwidthIn);

                    int pathFollowed = 0;

                    //make sure bandwidth is respected as also active should be no less than the minimum for each link
                    if(!bandwidth)
                        {
                            sql.begin();

                            active = ((maxActive - 1) < highDefault)? highDefault: (maxActive - 1);
                            ema = throughputEMA;
                            stmt10.execute(true);

                            sql.commit();

                            updateOptimizerEvolution(sql, source_hostname, destin_hostname, active, throughput, ratioSuccessFailure, 0, bandwidthIn);

                            continue;
                        }

                    //ratioSuccessFailure, rateStored, throughput, thrStored MUST never be zero
                    if(changed)
                        {
                            //get current active for this source
                            stmtActiveSource.execute(true);

                            //get current active for this destination
                            stmtActiveDest.execute(true);

                            //make sure it doesn't grow beyond the limits
                            int maxSource = 0;
                            int maxDestination = 0;
                            getMaxActive(sql, maxSource, maxDestination, source_hostname, destin_hostname);

                            //FTS3 admin requested to stop processing for this source or destination endpoints
                            if(maxSource == 0 || maxDestination == 0)
                                {
                                    updateOptimizerEvolution(sql, source_hostname, destin_hostname, maxActive, throughput, ratioSuccessFailure, 1, bandwidthIn);
                                    continue;
                                }
                            else if(maxSource ==  MAX_ACTIVE_ENDPOINT_LINK && maxDestination == MAX_ACTIVE_ENDPOINT_LINK)
                                {
                                    //do nothing, use default for both
                                }
                            else if (maxSource !=  MAX_ACTIVE_ENDPOINT_LINK && maxDestination != MAX_ACTIVE_ENDPOINT_LINK) //both have been set
                                {
                                    if(maxSource > maxDestination)
                                        {
                                            maxSource = 	maxDestination; //take the min
                                        }
                                    else if( maxDestination > maxSource)
                                        {
                                            maxDestination = maxSource;
                                        }
                                    else
                                        {
                                            //do nothing
                                        }
                                }
                            else if(maxSource !=  MAX_ACTIVE_ENDPOINT_LINK && maxDestination == MAX_ACTIVE_ENDPOINT_LINK)
                                {
                                    maxDestination = maxSource;
                                }
                            else if(maxSource ==  MAX_ACTIVE_ENDPOINT_LINK && maxDestination != MAX_ACTIVE_ENDPOINT_LINK)
                                {
                                    maxSource = maxDestination;
                                }
                            else
                                {
                                    //do nothing, use default
                                }



                            if( activeSource >= maxSource || activeDestination >= maxDestination || maxActive >= MAX_ACTIVE_PER_LINK)
                                {
                                    if(ratioSuccessFailure >= MED_SUCCESS_RATE )
                                        {
                                            sql.begin();
                                            active = maxActive;
                                            pathFollowed = 13;
                                            ema = throughputEMA;
                                            stmt10.execute(true);
                                            sql.commit();

                                            updateOptimizerEvolution(sql, source_hostname, destin_hostname, maxActive, throughput, ratioSuccessFailure, 1, bandwidthIn);
                                            continue;
                                        }
                                    else
                                        {
                                            sql.begin();
                                            active = ((maxActive - 2) < highDefault)? highDefault: (maxActive - 2);
                                            pathFollowed = 13;
                                            ema = throughputEMA;
                                            stmt10.execute(true);
                                            sql.commit();

                                            updateOptimizerEvolution(sql, source_hostname, destin_hostname, maxActive, throughput, ratioSuccessFailure, 1, bandwidthIn);
                                            continue;
                                        }
                                }

                            //ensure minumin per link and do not overflow before taking sample
                            if(active == maxActive)
                                {
                                    //do nothing for now
                                }
                            else
                                {
                                    updateOptimizerEvolution(sql, source_hostname, destin_hostname, maxActive, throughput, ratioSuccessFailure, 15, bandwidthIn);
                                    continue;
                                }

                            sql.begin();

                            if( (ratioSuccessFailure == MAX_SUCCESS_RATE || (ratioSuccessFailure > rateStored && ratioSuccessFailure >= MED_SUCCESS_RATE )) && throughputEMA > 0 &&  retry <= retryStored)
                                {
                                    if(throughputEMA > thrStored)
                                        {
                                            active = maxActive + 1;
                                            pathFollowed = 2;
                                        }
                                    else if((throughputEMA >= HIGH_THROUGHPUT || avgDuration <= AVG_TRANSFER_DURATION))
                                        {
                                            active = maxActive + 1;
                                            pathFollowed = 3;
                                        }
                                    else if(throughputSamples == 10 && throughputEMA >= thrStored)
                                        {
                                            active = maxActive + 1;
                                            pathFollowed = 4;
                                        }
                                    else if( (singleDest == 1 || lanTransferBool || spawnActive > 1) && (throughputEMA >= thrStored || avgDuration <= AVG_TRANSFER_DURATION) )
                                        {
                                            active = maxActive + 1;
                                            pathFollowed = 5;
                                        }
                                    else
                                        {
                                            active = maxActive;
                                            pathFollowed = 6;
                                        }

                                    ema = throughputEMA;
                                    stmt10.execute(true);

                                }
                            else if( (ratioSuccessFailure == MAX_SUCCESS_RATE || (ratioSuccessFailure > rateStored  && ratioSuccessFailure >= MED_SUCCESS_RATE)) && throughputEMA > 0 && throughputEMA < thrStored)
                                {
                                    if(retry > retryStored)
                                        {
                                            active = ((maxActive - 1) < highDefault)? highDefault: (maxActive - 1);
                                            pathFollowed = 7;
                                        }
                                    else if(thrSamplesStored == 3)
                                        {
                                            active = ((maxActive - 1) < highDefault)? highDefault: (maxActive - 1);
                                            pathFollowed = 8;
                                        }
                                    else if (avgDuration > MAX_TRANSFER_DURATION)
                                        {
                                            active = ((maxActive - 1) < highDefault)? highDefault: (maxActive - 1);
                                            pathFollowed = 9;
                                        }
                                    else
                                        {
                                            if(maxActive >= activeStored)
                                                {
                                                    active = ((maxActive - 1) < highDefault)? highDefault: (maxActive - 1);
                                                    pathFollowed = 10;
                                                }
                                            else
                                                {
                                                    active = maxActive;
                                                    pathFollowed = 11;
                                                }
                                        }
                                    ema = throughputEMA;
                                    stmt10.execute(true);
                                }
                            else if (ratioSuccessFailure < LOW_SUCCESS_RATE)
                                {
                                    if(ratioSuccessFailure > rateStored && ratioSuccessFailure == BASE_SUCCESS_RATE && retry <= retryStored)
                                        {
                                            active = maxActive;
                                            pathFollowed = 12;
                                        }
                                    else
                                        {
                                            active = ((maxActive - 2) < highDefault)? highDefault: (maxActive - 2);
                                            pathFollowed = 13;
                                        }
                                    ema = throughputEMA;
                                    stmt10.execute(true);
                                }
                            else
                                {
                                    active = maxActive;
                                    pathFollowed = 14;
                                    ema = throughputEMA;
                                    stmt10.execute(true);
                                }

                            sql.commit();

                            updateOptimizerEvolution(sql, source_hostname, destin_hostname, active, throughput, ratioSuccessFailure, pathFollowed, bandwidthIn);

                        }
                } //end for
        } //end try
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
    return recordsFound;
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
                "WHERE t_file.source_se = :source AND t_file.file_state  = 'ACTIVE'  ",
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
                "WHERE t_file.dest_se = :dst AND t_file.file_state  = 'ACTIVE' ",
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
            sql << " select count(*) from t_file where source_se=:source_se and dest_se=:dest_se and file_state  = 'ACTIVE' ",
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


void OracleAPI::forceFailTransfers(std::map<int, std::string>& collectJobs)
{
    soci::session sql(*connectionPool);

    try
        {
            std::string jobId, params, tHost,reuse;
            int fileId=0, timeout=0;
            long long pid = 0;
            struct tm startTimeSt;
            time_t startTime;
            double diff = 0.0;
            soci::indicator isNull = soci::i_ok;
            soci::indicator isNullParams = soci::i_ok;
            soci::indicator isNullPid = soci::i_ok;
            int count = 0;

            soci::statement stmt = (
                                       sql.prepare <<
                                       " SELECT f.job_id, f.file_id, f.start_time, f.pid, f.internal_file_params, "
                                       " j.reuse_job "
                                       " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                       " WHERE f.file_state='ACTIVE' AND f.pid IS NOT NULL and f.job_finished is NULL "
                                       " AND (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd) ",
                                       soci::use(hashSegment.start), soci::use(hashSegment.end),
                                       soci::into(jobId), soci::into(fileId), soci::into(startTimeSt),
                                       soci::into(pid, isNullPid), soci::into(params, isNullParams), soci::into(reuse, isNull)
                                   );

            soci::statement stmt1 = (
                                        sql.prepare <<
                                        " SELECT COUNT(*) FROM t_file WHERE job_id = :jobId ", soci::use(jobId), soci::into(count)
                                    );


            if (stmt.execute(true))
                {

                    do
                        {
                            startTime = timegm(&startTimeSt); //from db
                            time_t now2 = getUTC(0);

                            if (isNullParams != soci::i_null)
                                {
                                    timeout = extractTimeout(params);
                                    if(timeout == 0)
                                        timeout = 7200;
                                    else
                                        timeout += 3600;
                                }
                            else
                                {
                                    timeout = 7200;
                                }

                            int terminateTime = timeout + 1000;

                            if (isNull != soci::i_null && reuse == "Y")
                                {
                                    count = 0;

                                    stmt1.execute(true);

                                    if(count > 0)
                                        terminateTime = count * 360;
                                }

                            diff = difftime(now2, startTime);
                            if (diff > terminateTime)
                                {
                                    if(isNullPid != soci::i_null && pid > 0)
                                        {
                                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Killing pid:" << pid << ", jobid:" << jobId << ", fileid:" << fileId << " because it was stalled" << commit;
                                            kill(boost::lexical_cast<__pid_t>(pid), SIGUSR1);
                                        }
                                    collectJobs.insert(std::make_pair(fileId, jobId));
                                    updateFileTransferStatusInternal(sql, 0.0, jobId, fileId,
                                                                     "FAILED", "Transfer has been forced-killed because it was stalled",
                                                                     boost::lexical_cast<int>(pid), 0, 0, false);
                                    updateJobTransferStatusInternal(sql, jobId, "FAILED",0);

                                    std::vector<struct message_state> files;
                                    //send state monitoring message for the state transition
                                    files = getStateOfTransferInternal(sql, jobId, fileId);
                                    if(!files.empty())
                                        {
                                            std::vector<struct message_state>::iterator it;
                                            for (it = files.begin(); it != files.end(); ++it)
                                                {
                                                    struct message_state tmp = (*it);
                                                    constructJSONMsg(&tmp);
                                                }
                                            files.clear();
                                        }
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


bool OracleAPI::terminateReuseProcess(const std::string & jobId, int pid, const std::string & message)
{
    bool ok = true;
    soci::session sql(*connectionPool);
    std::string job_id;
    std::string reuse;
    soci::indicator reuseInd = soci::i_ok;

    try
        {
            if(jobId.length() == 0)
                {
                    sql << " SELECT job_id from t_file where pid=:pid and job_finished is NULL AND ROWNUM =1",
                        soci::use(pid), soci::into(job_id);

                    sql << " SELECT reuse_job FROM t_job WHERE job_id = :jobId AND reuse_job IS NOT NULL",
                        soci::use(job_id), soci::into(reuse, reuseInd);

                    if (sql.got_data() && (reuse == "Y" || reuse == "H"))
                        {
                            sql.begin();
                            sql << " UPDATE t_file SET file_state = 'FAILED', job_finished=sys_extract_utc(systimestamp), finish_time=sys_extract_utc(systimestamp), "
                                " reason=:message WHERE (job_id = :jobId OR pid=:pid) AND file_state not in ('FINISHED','FAILED','CANCELED') ",
                                soci::use(message),
                                soci::use(job_id),
                                soci::use(pid);
                            sql.commit();
                        }

                }
            else
                {
                    sql << " SELECT reuse_job FROM t_job WHERE job_id = :jobId AND reuse_job IS NOT NULL",
                        soci::use(jobId), soci::into(reuse, reuseInd);

                    if (sql.got_data() && (reuse == "Y" || reuse == "H"))
                        {
                            sql.begin();
                            sql << " UPDATE t_file SET file_state = 'FAILED', job_finished=sys_extract_utc(systimestamp), finish_time=sys_extract_utc(systimestamp), "
                                " reason=:message WHERE (job_id = :jobId OR pid=:pid) AND file_state not in ('FINISHED','FAILED','CANCELED') ",
                                soci::use(message),
                                soci::use(job_id),
                                soci::use(pid);
                            sql.commit();
                        }
                }
        }
    catch (std::exception& e)
        {
            sql.rollback();
            return ok;
        }
    catch (...)
        {
            sql.rollback();
            return ok;
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

            sql << "UPDATE t_file SET pid = :pid WHERE job_id = :jobId ", soci::use(pid), soci::use(pids.begin()->second);

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

    unsigned index=0, count1=0, start=0, end=0;
    std::string service_name = "fts_backup";
    *nJobs = 0;
    *nFiles = 0;
    std::ostringstream jobIdStmt;
    std::string job_id;
    std::string stmt;
    int count = 0;
    bool drain = false;
    long long activeHosts = 0;

    try
        {
            // Total number of working instances, prevent from starting a second one
            sql << "SELECT COUNT(hostname) FROM t_hosts "
                "  WHERE beat >= (sys_extract_utc(systimestamp) - interval '30' minute) and service_name = :service_name",
                soci::use(service_name),
                soci::into(activeHosts);

            if(activeHosts > 0)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Backup already running, won't start" << commit;
                    return;
                }


            //update heartbeat first, the first must get 0
            updateHeartBeatInternal(sql, &index, &count1, &start, &end, service_name);

            //prevent more than on server to update the optimizer decisions
            if(hashSegment.start == 0)
                {
                    soci::rowset<soci::row> rs = (
                                                     sql.prepare <<
                                                     "  select  job_id from t_job where job_finished < (systimestamp - interval '7' DAY ) "
                                                 );

                    for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                        {
                            count++;

                            if(count == 1000)
                                {
                                    //update heartbeat first
                                    updateHeartBeatInternal(sql, &index, &count1, &start, &end, service_name);

                                    drain = getDrainInternal(sql);
                                    if(drain)
                                        {
                                            sql.commit();
                                            sleep(15);
                                            return;
                                        }
                                }

                            soci::row const& r = *i;
                            job_id = r.get<std::string>("JOB_ID");
                            jobIdStmt << "'";
                            jobIdStmt << job_id;
                            jobIdStmt << "',";

                            if(count == 5000)
                                {
                                    std::string queryStr = jobIdStmt.str();
                                    job_id = queryStr.substr(0, queryStr.length() - 1);

                                    sql.begin();

                                    stmt = "INSERT INTO t_job_backup SELECT * FROM t_job WHERE job_id  in (" +job_id+ ")";
                                    sql << stmt;
                                    stmt = "INSERT INTO t_file_backup SELECT * FROM t_file WHERE  job_id  in (" +job_id+ ")";
                                    sql << stmt;

                                    stmt = "DELETE FROM t_file WHERE job_id in (" +job_id+ ")";
                                    sql << stmt;

                                    stmt = "DELETE FROM t_dm WHERE job_id in (" +job_id+ ")";
                                    sql << stmt;

                                    stmt = "DELETE FROM t_job WHERE job_id in (" +job_id+ ")";
                                    sql << stmt;

                                    count = 0;
                                    jobIdStmt.str(std::string());
                                    jobIdStmt.clear();
                                    sql.commit();
                                    sleep(1); // give it sometime to breath
                                }
                        }

                    //delete from t_optimizer_evolution > 7 days old records
                    sql.begin();
                    sql << "delete from t_optimizer_evolution where datetime < (systimestamp - interval '7' DAY )";
                    sql.commit();

                    //delete from t_file_retry_errors > 7 days old records
                    sql.begin();
                    sql << "delete from t_file_retry_errors where datetime < (systimestamp - interval '7' DAY )";
                    sql.commit();
                }

            jobIdStmt.str(std::string());
            jobIdStmt.clear();
        }
    catch (std::exception& e)
        {
            jobIdStmt.str(std::string());
            jobIdStmt.clear();
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            jobIdStmt.str(std::string());
            jobIdStmt.clear();
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
                "      file_state NOT IN ('FINISHED','FAILED','CANCELED','NOT_USED')",
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


            soci::statement stmt = (sql.prepare << " UPDATE t_file SET file_state = 'FAILED', transferhost=:hostname, "
                                    " job_finished=sys_extract_utc(systimestamp), finish_time= sys_extract_utc(systimestamp), reason='Transfer failed to fork, check fts3server.log for more details'"
                                    " WHERE file_id = :fileId AND job_id = :jobId AND "
                                    "      file_state NOT IN ('FINISHED','FAILED','CANCELED')",
                                    soci::use(hostname), soci::use(fileId), soci::use(jobId));


            sql.begin();
            for (std::map<int, std::string>::const_iterator i = pids.begin(); i != pids.end(); ++i)
                {
                    fileId = i->first;
                    jobId  = i->second;
                    stmt.execute(true);
                }

            sql << "update t_job set job_state='FAILED',job_finished=sys_extract_utc(systimestamp), finish_time=sys_extract_utc(systimestamp), reason='Transfer failed to fork, check fts3server.log for more details' where job_id=:job_id",
                soci::use(jobId);

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



bool OracleAPI::retryFromDead(std::vector<struct message_updater>& messages, bool diskFull)
{
    soci::session sql(*connectionPool);

    bool ok = false;
    std::vector<struct message_updater>::const_iterator iter;
    const std::string transfer_status = "FAILED";
    std::string transfer_message;
    if(diskFull)
        {
            transfer_message = "No space left on device in " + hostname;
        }
    else
        {
            transfer_message = "no FTS server had updated the transfer status the last 600 seconds, probably stalled in " + hostname;
        }

    const std::string status = "FAILED";

    try
        {
            for (iter = messages.begin(); iter != messages.end(); ++iter)
                {

                    soci::rowset<long long> rs = (
                                                     sql.prepare <<
                                                     " SELECT file_id FROM t_file "
                                                     " WHERE file_id = :fileId AND job_id = :jobId AND file_state  = 'ACTIVE' AND"
                                                     " (hashed_id >= :hStart AND hashed_id <= :hEnd) ",
                                                     soci::use(iter->file_id),
                                                     soci::use(std::string(iter->job_id)),
                                                     soci::use(hashSegment.start),
                                                     soci::use(hashSegment.end));

                    if (rs.begin() != rs.end())
                        {
                            ok = true;
                            updateFileTransferStatusInternal(sql, 0.0, (*iter).job_id, (*iter).file_id, transfer_status, transfer_message, (*iter).process_id, 0, 0,false);
                            updateJobTransferStatusInternal(sql, (*iter).job_id, status,0);

                            std::vector<struct message_state> files;
                            //send state monitoring message for the state transition
                            files = getStateOfTransferInternal(sql, (*iter).job_id, (*iter).file_id);
                            if(!files.empty())
                                {
                                    std::vector<struct message_state>::iterator it;
                                    for (it = files.begin(); it != files.end(); ++it)
                                        {
                                            struct message_state tmp = (*it);
                                            constructJSONMsg(&tmp);
                                        }
                                    files.clear();
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
                "   AND f.file_state IN ('ACTIVE','SUBMITTED') and f.job_finished is NULL "
                "   AND NOT EXISTS ( "
                "       SELECT NULL "
                "       FROM t_bad_ses "
                "       WHERE (se = f.source_se OR se = f.dest_se) AND "
                "             (status = 'WAIT' OR status = 'WAIT_AS')"
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
                "         AND t_file.file_state in ('ACTIVE','SUBMITTED') "
                "         AND NOT EXISTS (SELECT NULL "
                "                         FROM t_bad_dns "
                "                         WHERE dn = t_job.user_dn AND (status = 'WAIT' OR status = 'WAIT_AS') )"
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
                    ret[i->get<std::string>("SE")] =  i->get<int>("WAIT_TIMEOUT");
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
    std::string vo_name;
    std::string dest_se;
    std::string dest_surl;

    try
        {
            sql << "SELECT file_state, transferHost, dest_surl, vo_name, dest_se FROM t_file WHERE file_id = :fileId",
                soci::use(fileID),
                soci::into(state),
                soci::into(host, isNull),
                soci::into(dest_surl),
                soci::into(vo_name),
                soci::into(dest_se);

            isReadyState = (state == "READY");

            if (isNull != soci::i_null)
                isReadyHost = (host == hostname);

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
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

            sql << "INSERT /*+ IGNORE_ROW_ON_DUPKEY_INDEX (t_link_config, T_LINK_CONFIG_PK) */ INTO t_share_config (source, destination, vo, active) "
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
                "                    VALUES (:vo, :share_string, :state)",
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
                " SET activity_share = :share_string, active = :active "
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
                "WHERE t_file.file_state  = 'ACTIVE'  AND "
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
                "WHERE t_file.file_state  = 'ACTIVE'  AND "
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
                "WHERE t_file.file_state  = 'ACTIVE' AND "
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

void OracleAPI::setSeProtocol(std::string protocol, std::string se, std::string state)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            int count = 0;

            sql <<
                " SELECT count(auto_number) "
                " FROM t_optimize "
                " WHERE source_se = :se ",
                soci::use(se), soci::into(count)
                ;

            if (count)
                {
                    sql <<
                        " UPDATE t_optimize "
                        " SET " << protocol << " = :state "
                        " WHERE source_se = :se ",
                        soci::use(state),
                        soci::use(se)
                        ;
                }
            else
                {
                    sql <<
                        " INSERT INTO t_optimize (source_se, " << protocol << ", file_id) "
                        " VALUES (:se, :state, 0)",
                        soci::use(se),
                        soci::use(state)
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

void OracleAPI::setShowUserDn(bool show)
{
    soci::session sql(*connectionPool);
    int count = 0;

    try
        {
            sql << "SELECT count(*) FROM t_server_config WHERE vo_name IS NULL or vo_name = '*'", soci::into(count);
            if (!count)
                {
                    sql.begin();
                    sql << "INSERT INTO t_server_config (show_user_dn, vo_name) VALUES (:show, '*')",
                        soci::use(std::string(show ? "on" : "off"));
                    sql.commit();

                }
            else
                {
                    sql.begin();
                    sql << "UPDATE t_server_config SET show_user_dn = :show WHERE show_user_dn IS NOT NULL",
                        soci::use(std::string(show ? "on" : "off"));
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


void OracleAPI::setRetry(int retry, const std::string & vo_name)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            int count = 0;

            sql <<
                " SELECT count(vo_name) "
                " FROM t_server_config "
                " WHERE vo_name = :vo_name ",
                soci::use(vo_name), soci::into(count)
                ;

            if (count)
                {
                    sql <<
                        " UPDATE t_server_config "
                        " SET retry = :retry "
                        " WHERE vo_name = :vo_name ",
                        soci::use(retry),
                        soci::use(vo_name)
                        ;
                }
            else
                {
                    sql <<
                        " INSERT INTO t_server_config (retry, vo_name) "
                        " VALUES(:retry, :vo_name)",
                        soci::use(retry),
                        soci::use(vo_name);
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


int OracleAPI::getRetry(const std::string & jobId)
{
    soci::session sql(*connectionPool);

    int nRetries = 0;
    soci::indicator isNull = soci::i_ok;
    std::string vo_name;

    try
        {
            sql <<
                " SELECT retry, vo_name "
                " FROM t_job "
                " WHERE job_id = :jobId ",
                soci::use(jobId),
                soci::into(nRetries, isNull),
                soci::into(vo_name)
                ;

            if (isNull == soci::i_null || nRetries == 0)
                {
                    sql <<
                        " SELECT retry FROM (SELECT rownum as rn, retry "
                        "  FROM t_server_config where vo_name=:vo_name) WHERE rn = 1",
                        soci::use(vo_name), soci::into(nRetries)
                        ;
                }
            else if (nRetries <= 0)
                {
                    nRetries = -1;
                }

            if(nRetries > 0)
                {
                    //do not retry multiple replica jobs
                    long long mreplica = 0;
                    long long mreplicaCount = 0;
                    sql << "select count(*), count(distinct file_index) from t_file where job_id=:job_id", soci::use(jobId), soci::into(mreplicaCount), soci::into(mreplica);
                    if(mreplicaCount > 1 && mreplica == 1)
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

            if(isNull == soci::i_null)
                return 0;
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

    long long maxTime = 0;
    try
        {
            soci::indicator isNull = soci::i_ok;

            sql << "SELECT max_time_queue FROM (SELECT rownum as rn, max_time_queue FROM t_server_config WHERE vo_name IS NULL OR vo_name = '*') WHERE rn = 1",
                soci::into(maxTime, isNull);

            //just in case soci it is reseting the value to NULL
            if(isNull != soci::i_null && maxTime > 0)
                return boost::lexical_cast<int>(maxTime);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
    return boost::lexical_cast<int>(maxTime);
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


void OracleAPI::setGlobalLimits(const int* maxActivePerLink, const int* maxActivePerSe)
{
    soci::session sql(*connectionPool);
    int existsLink = 0;
    int existsSe = 0;

    try
        {
            sql << "SELECT max_per_link FROM t_server_config WHERE vo_name IS NULL or vo_name = '*'", soci::into(existsLink);
            sql << "SELECT max_per_se FROM t_server_config WHERE vo_name IS NULL or vo_name = '*'", soci::into(existsSe);

            sql.begin();

            if (maxActivePerLink)
                {
                    if(existsLink > 0)
                        {
                            sql << "UPDATE t_server_config SET max_per_link = :maxLink",
                                soci::use(*maxActivePerLink);
                        }
                    else
                        {
                            sql << "INSERT into t_server_config(max_per_link, vo_name) VALUES(:maxLink, '*')",
                                soci::use(*maxActivePerLink);
                        }
                }
            if (maxActivePerSe)
                {
                    if(existsSe > 0)
                        {
                            sql << "UPDATE t_server_config SET max_per_se = :maxSe",
                                soci::use(*maxActivePerSe);
                        }
                    else
                        {
                            sql << "INSERT into t_server_config(max_per_se, vo_name) VALUES(:maxSe, '*')",
                                soci::use(*maxActivePerSe);
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


void OracleAPI::authorize(bool add, const std::string& op, const std::string& dn)
{
    soci::session sql(*connectionPool);
    try
        {
            if (add)
                {
                    sql << "MERGE INTO t_authz_dn USING "
                        "    (SELECT :op AS operation, :dn as dn FROM dual) Authz"
                        " ON (t_authz_dn.operation = Authz.operation AND t_authz_dn.dn = Authz.dn) "
                        " WHEN NOT MATCHED THEN "
                        " INSERT (operation, dn) VALUES "
                        "     (Authz.operation, Authz.dn)",
                        soci::use(op), soci::use(dn);

                }
            else
                {
                    sql << "DELETE FROM t_authz_dn WHERE operation = :op AND dn = :dn",
                        soci::use(op), soci::use(dn);
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
            if(hashSegment.start == 0)
                {
                    soci::rowset<std::string> rs = (sql.prepare << "SELECT job_id FROM t_job WHERE "
                                                    "    submit_time < (sys_extract_utc(systimestamp) - numtodsinterval(:interval, 'hour')) AND "
                                                    "    job_state  = 'SUBMITTED' and job_finished is NULL ",
                                                    soci::use(maxTime));

                    sql.begin();
                    for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                        {

                            sql << "UPDATE t_file SET job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), "
                                "    file_state = 'CANCELED', reason = :reason "
                                "WHERE job_id = :jobId AND "
                                "      file_state  = 'SUBMITTED'",
                                soci::use(message), soci::use(*i);

                            sql << "UPDATE t_job SET job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), "
                                "    job_state = 'CANCELED', reason = :reason "
                                "WHERE job_id = :jobId AND job_state  = 'SUBMITTED'",
                                soci::use(message), soci::use(*i);

                            jobs.push_back(*i);
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


void OracleAPI::setMaxStageOp(const std::string& se, const std::string& vo, int val, const std::string & opt)
{
    soci::session sql(*connectionPool);

    try
        {
            int exist = 0;

            sql <<
                " SELECT COUNT(*) "
                " FROM t_stage_req "
                " WHERE vo_name = :vo AND host = :se AND operation = :opt ",
                soci::use(vo),
                soci::use(se),
                soci::use(opt),
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
                        " WHERE vo_name = :vo AND host = :se AND operation = :opt ",
                        soci::use(val),
                        soci::use(vo),
                        soci::use(se),
                        soci::use(opt)
                        ;
                }
            else
                {
                    // otherwise insert
                    sql <<
                        " INSERT "
                        " INTO t_stage_req (host, vo_name, concurrent_ops, operation) "
                        " VALUES (:se, :vo, :value, :opt)",
                        soci::use(se),
                        soci::use(vo),
                        soci::use(val),
                        soci::use(opt)
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

void OracleAPI::updateProtocol(std::vector<struct message>& messages)
{
    soci::session sql(*connectionPool);

    std::stringstream internalParams;
    double filesize = 0;
    int fileId = 0;
    std::string params;

    try
        {
            soci::statement stmt = (
                                       sql.prepare << "UPDATE t_file set INTERNAL_FILE_PARAMS=:1, FILESIZE=:2 where file_id=:fileId ",
                                       soci::use(params),
                                       soci::use(filesize),
                                       soci::use(fileId));


            sql.begin();

            std::vector<struct message>::const_iterator iter;
            for (iter = messages.begin(); iter != messages.end(); ++iter)
                {
                    internalParams.str(std::string());
                    internalParams.clear();

                    struct message msg = *iter;
                    if(iter->msg_errno == 0 && std::string(msg.transfer_status).compare("UPDATE") == 0)
                        {
                            fileId = msg.file_id;
                            filesize = msg.filesize;
                            internalParams << "nostreams:" << static_cast<int> (msg.nostreams) << ",timeout:" << static_cast<int> (msg.timeout) << ",buffersize:" << static_cast<int> (msg.buffersize);
                            params = internalParams.str();
                            stmt.execute(true);
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
                                             "  AND file_state IN ('ACTIVE','SUBMITTED', 'NOT_USED')",
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
                                             "  AND f.file_state IN ('ACTIVE','SUBMITTED', 'NOT_USED') ",
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
                    updateJobTransferStatusInternal(sql, *job_it, std::string(),0);
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
                                             "  AND job_state IN ('ACTIVE','SUBMITTED')",
                                             soci::use(dn)
                                         );

            soci::rowset<soci::row>::const_iterator it;
            for (it = rs.begin(); it != rs.end(); ++it)
                {

                    jobs.push_back(it->get<std::string>("JOB_ID"));
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

            std::map<int, struct message_log>::iterator iterLog = messagesLog.begin();
            while (iterLog != messagesLog.end())
                {
                    if(((*iterLog).second).msg_errno == 0)
                        {
                            filePath = ((*iterLog).second).filePath;
                            fileId = ((*iterLog).second).file_id;
                            debugFile = ((*iterLog).second).debugFile;
                            stmt.execute(true);

                            if (stmt.get_affected_rows() > 0)
                                {
                                    // erase
                                    messagesLog.erase(iterLog++);
                                }
                            else
                                {
                                    ++iterLog;
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
    catch (...)
        {
            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
}


std::vector<struct message_state> OracleAPI::getStateOfDeleteInternal(soci::session& sql, const std::string& jobId, int fileId)
{
    message_state ret;
    std::vector<struct message_state> temp;

    try
        {
            soci::rowset<soci::row> rs = (fileId ==-1) ? (
                                             sql.prepare <<
                                             " SELECT "
                                             "	j.user_dn, j.submit_time, j.job_id, j.job_state, j.vo_name, "
                                             "	j.job_metadata, j.retry AS retry_max, f.file_id, "
                                             "	f.file_state, f.retry AS retry_counter, f.file_metadata, "
                                             "	f.source_se, f.start_time, f.source_surl  "
                                             " FROM t_dm f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                             " WHERE "
                                             " 	j.job_id = :jobId ",
                                             soci::use(jobId)
                                         )
                                         :
                                         (
                                             sql.prepare <<
                                             " SELECT "
                                             "	j.user_dn, j.submit_time, j.job_id, j.job_state, j.vo_name, "
                                             "	j.job_metadata, j.retry AS retry_max, f.file_id, "
                                             "	f.file_state, f.retry AS retry_counter, f.file_metadata, "
                                             "	f.source_se, f.start_time, f.source_surl "
                                             " FROM t_dm f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                             " WHERE "
                                             " 	j.job_id = :jobId "
                                             "  AND f.file_id = :fileId ",
                                             soci::use(jobId),
                                             soci::use(fileId)
                                         );


            soci::rowset<soci::row>::const_iterator it;
            time_t aux_time;

            bool show_user_dn = getUserDnVisibleInternal(sql);

            for (it = rs.begin(); it != rs.end(); ++it)
                {
                    ret.job_id = it->get<std::string>("JOB_ID");
                    ret.job_state = it->get<std::string>("JOB_STATE");
                    ret.vo_name = it->get<std::string>("VO_NAME");

                    soci::indicator isNull1 = it->get_indicator("JOB_METADATA");
                    if (isNull1 == soci::i_ok)
                        ret.job_metadata = it->get<std::string>("JOB_METADATA","");
                    else
                        ret.job_metadata = "";



                    ret.retry_max = static_cast<int>(it->get<long long>("RETRY_MAX", 0));
                    ret.file_id = static_cast<int>(it->get<long long>("FILE_ID"));
                    ret.file_state = it->get<std::string>("FILE_STATE");
                    if(ret.file_state == "SUBMITTED")
                        {
                            aux_time = soci::getTimeT(*it, "SUBMIT_TIME");
                            ret.timestamp = boost::lexical_cast<std::string>(aux_time * 1000);
                        }
                    else if(ret.file_state == "STAGING")
                        {
                            aux_time = soci::getTimeT(*it, "SUBMIT_TIME");
                            ret.timestamp = boost::lexical_cast<std::string>(aux_time * 1000);
                        }
                    else if(ret.file_state == "DELETE")
                        {
                            aux_time = soci::getTimeT(*it, "SUBMIT_TIME");
                            ret.timestamp = boost::lexical_cast<std::string>(aux_time * 1000);
                        }
                    else if(ret.file_state == "ACTIVE")
                        {
                            soci::indicator isNull3 = it->get_indicator("START_TIME");
                            if (isNull3 == soci::i_ok)
                                {
                                    aux_time = soci::getTimeT(*it, "START_TIME");
                                    ret.timestamp = boost::lexical_cast<std::string>(aux_time * 1000);
                                }
                            else
                                {
                                    ret.timestamp = "";
                                }

                        }
                    else
                        {
                            ret.timestamp = getStrUTCTimestamp();
                        }
                    ret.retry_counter = static_cast<int>(it->get<double>("RETRY_COUNTER",0));

                    soci::indicator isNull2 = it->get_indicator("FILE_METADATA");
                    if (isNull2 == soci::i_ok)
                        ret.file_metadata = it->get<std::string>("FILE_METADATA","");
                    else
                        ret.file_metadata = "";


                    ret.source_se = it->get<std::string>("SOURCE_SE");
                    ret.dest_se = "";

                    if(!show_user_dn)
                        ret.user_dn = std::string("");
                    else
                        ret.user_dn = it->get<std::string>("USER_DN","");
                    ret.source_url = it->get<std::string>("SOURCE_SURL","");
                    ret.dest_url = "";

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


std::vector<struct message_state> OracleAPI::getStateOfTransferInternal(soci::session& sql, const std::string& jobId, int fileId)
{
    message_state ret;
    std::vector<struct message_state> temp;

    try
        {
            soci::rowset<soci::row> rs = (fileId ==-1) ? (
                                             sql.prepare <<
                                             " SELECT "
                                             "	j.user_dn, j.submit_time, j.job_id, j.job_state, j.vo_name, "
                                             "	j.job_metadata, j.retry AS retry_max, f.file_id, "
                                             "	f.file_state, f.retry AS retry_counter, f.file_metadata, "
                                             "	f.source_se, f.dest_se, f.start_time, f.source_surl, f.dest_surl "
                                             " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                             " WHERE "
                                             " 	j.job_id = :jobId ",
                                             soci::use(jobId)
                                         )
                                         :
                                         (
                                             sql.prepare <<
                                             " SELECT "
                                             "	j.user_dn, j.submit_time, j.job_id, j.job_state, j.vo_name, "
                                             "	j.job_metadata, j.retry AS retry_max, f.file_id, "
                                             "	f.file_state, f.retry AS retry_counter, f.file_metadata, "
                                             "	f.source_se, f.dest_se, f.start_time, f.source_surl, f.dest_surl "
                                             " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                             " WHERE "
                                             " 	j.job_id = :jobId "
                                             "  AND f.file_id = :fileId ",
                                             soci::use(jobId),
                                             soci::use(fileId)
                                         );


            bool show_user_dn = getUserDnVisibleInternal(sql);

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
                    if(ret.file_state == "SUBMITTED")
                        {
                            ret.timestamp = boost::lexical_cast<std::string>(soci::getTimeT(*it, "SUBMIT_TIME"));
                        }
                    else if(ret.file_state == "STAGING")
                        {
                            ret.timestamp = boost::lexical_cast<std::string>(soci::getTimeT(*it, "SUBMIT_TIME"));
                        }
                    else if(ret.file_state == "DELETE")
                        {
                            ret.timestamp = boost::lexical_cast<std::string>(soci::getTimeT(*it, "SUBMIT_TIME"));
                        }
                    else if(ret.file_state == "ACTIVE")
                        {
                            ret.timestamp = boost::lexical_cast<std::string>(soci::getTimeT(*it, "START_TIME"));
                        }
                    else
                        {
                            ret.timestamp = getStrUTCTimestamp();
                        }
                    ret.retry_counter = static_cast<int>(it->get<double>("RETRY_COUNTER",0));
                    ret.file_metadata = it->get<std::string>("FILE_METADATA","");
                    ret.source_se = it->get<std::string>("SOURCE_SE");
                    ret.dest_se = it->get<std::string>("DEST_SE");

                    if(!show_user_dn)
                        ret.user_dn = std::string("");
                    else
                        ret.user_dn = it->get<std::string>("USER_DN","");

                    ret.source_url = it->get<std::string>("SOURCE_SURL","");
                    ret.dest_url = it->get<std::string>("DEST_SURL","");

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

std::vector<struct message_state> OracleAPI::getStateOfTransfer(const std::string& jobId, int fileId)
{
    soci::session sql(*connectionPool);
    std::vector<struct message_state> temp;

    try
        {
            temp = getStateOfTransferInternal(sql, jobId, fileId);
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
                        "   AND file_state IN ('ACTIVE','SUBMITTED', 'NOT_USED') "
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
                        "   AND file_state IN ('ACTIVE','SUBMITTED', 'NOT_USED') "
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
                "               t_file.file_state IN ('ACTIVE','SUBMITTED', 'NOT_USED') "
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
            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT file_id, job_id "
                                             " FROM t_file "
                                             " WHERE wait_timeout <> 0 "
                                             "  AND (sys_extract_utc(systimestamp) - wait_timestamp) > numtodsinterval(wait_timeout, 'second') "
                                             "  AND file_state IN ('ACTIVE','SUBMITTED', 'NOT_USED')"
                                             "  AND (hashed_id >= :hStart AND hashed_id <= :hEnd) ",
                                             soci::use(hashSegment.start), soci::use(hashSegment.end)
                                         );

            soci::rowset<soci::row>::iterator it;

            sql.begin();
            for (it = rs.begin(); it != rs.end(); ++it)
                {

                    jobs.insert(it->get<std::string>("JOB_ID"));

                    sql <<
                        " UPDATE t_file "
                        " SET file_state = 'CANCELED' , finish_time = sys_extract_utc(systimestamp) "
                        " WHERE file_id = :fileId AND file_state IN ('ACTIVE','SUBMITTED', 'NOT_USED')",
                        soci::use(static_cast<int>(it->get<long long>("FILE_ID")))
                        ;
                }

            sql.commit();

            std::set<std::string>::iterator job_it;
            for (job_it = jobs.begin(); job_it != jobs.end(); ++job_it)
                {
                    updateJobTransferStatusInternal(sql, *job_it, std::string(),0);
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

            soci::rowset<std::string> rs = (
                                               sql.prepare <<
                                               "select distinct f.job_id from t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                               " WHERE file_state = 'NOT_USED' and j.job_finished is NULL"
                                               "  AND (hashed_id >= :hStart AND hashed_id <= :hEnd) ",
                                               soci::use(hashSegment.start), soci::use(hashSegment.end)
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
                        "       ) t_file_tmp "
                        "       WHERE t_file_tmp.job_id = f1.job_id "
                        "           AND t_file_tmp.file_index = f1.file_index  "
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
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity states check thread started " << commit;

    soci::session sql(*connectionPool);

    unsigned int numberOfFiles = 0;
    unsigned int terminalState = 0;
    unsigned int allFinished = 0;
    unsigned int allFailed = 0;
    unsigned int allCanceled = 0;
    unsigned int numberOfFilesRevert = 0;
    unsigned int numberOfFilesDelete = 0;

    std::string mreplica;

    std::string canceledMessage = "Transfer canceled by the user";
    std::string failed = "One or more files failed. Please have a look at the details for more information";
    std::string job_id;

    try
        {
            if(hashSegment.start == 0)
                {
                    soci::rowset<std::string> rs = (
                                                       sql.prepare <<
                                                       " select job_id from t_job  where job_finished is null "
                                                   );

                    soci::statement stmt1 = (sql.prepare << "SELECT COUNT(DISTINCT file_index) FROM t_file where job_id=:jobId ", soci::use(job_id), soci::into(numberOfFiles));

                    soci::statement stmt2 = (sql.prepare << "UPDATE t_job SET "
                                             "    job_state = 'CANCELED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), "
                                             "    reason = :canceledMessage "
                                             "    WHERE job_id = :jobId and  job_state <> 'CANCELED' ", soci::use(canceledMessage), soci::use(job_id));

                    soci::statement stmt3 = (sql.prepare << "UPDATE t_job SET "
                                             "    job_state = 'FINISHED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp) "
                                             "    WHERE job_id = :jobId and  job_state <> 'FINISHED'  ", soci::use(job_id));

                    soci::statement stmt4 = (sql.prepare << "UPDATE t_job SET "
                                             "    job_state = 'FAILED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), "
                                             "    reason = :failed "
                                             "    WHERE job_id = :jobId and  job_state <> 'FAILED' ", soci::use(failed), soci::use(job_id));


                    soci::statement stmt5 = (sql.prepare << "UPDATE t_job SET "
                                             "    job_state = 'FINISHEDDIRTY', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), "
                                             "    reason = :failed "
                                             "    WHERE job_id = :jobId and  job_state <> 'FINISHEDDIRTY'", soci::use(failed), soci::use(job_id));

                    soci::statement stmt6 = (sql.prepare << "SELECT COUNT(*) FROM t_file where job_id=:jobId AND file_state in ('ACTIVE','SUBMITTED','STAGING','STARTED') ", soci::use(job_id), soci::into(numberOfFilesRevert));

                    soci::statement stmt7 = (sql.prepare << "UPDATE t_file SET "
                                             "    file_state = 'FAILED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), "
                                             "    reason = 'Force failure due to file state inconsistency' "
                                             "    WHERE file_state in ('ACTIVE','SUBMITTED','STAGING','STARTED') and job_id = :jobId ", soci::use(job_id));

                    soci::statement stmt8 = (sql.prepare << " select count(*)  "
                                             " from t_file "
                                             " where job_id = :jobId "
                                             "	and  file_state = 'FINISHED' ",
                                             soci::use(job_id),
                                             soci::into(allFinished));

                    soci::statement stmt9 = (sql.prepare << " select count(distinct f1.file_index) "
                                             " from t_file f1 "
                                             " where f1.job_id = :jobId "
                                             "	and f1.file_state = 'CANCELED' ",
                                             soci::use(job_id),
                                             soci::into(allCanceled));

                    soci::statement stmt10 = (sql.prepare << " select count(distinct f1.file_index) "
                                              " from t_file f1 "
                                              " where f1.job_id = :jobId "
                                              "	and f1.file_state = 'FAILED' ",
                                              soci::use(job_id),
                                              soci::into(allFailed));


                    soci::statement stmt_m_replica = (sql.prepare << " select reuse_job from t_job where job_id=:job_id  ",
                                                      soci::use(job_id),
                                                      soci::into(mreplica));

                    //this section is for deletion jobs
                    soci::statement stmtDel1 = (sql.prepare << "SELECT COUNT(*) FROM t_dm where job_id=:jobId AND file_state in ('DELETE','STARTED') ", soci::use(job_id), soci::into(numberOfFilesDelete));

                    soci::statement stmtDel2 = (sql.prepare << "UPDATE t_dm SET "
                                                "    file_state = 'FAILED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), "
                                                "    reason = 'Force failure due to file state inconsistency' "
                                                "    WHERE file_state in ('DELETE','STARTED') and job_id = :jobId", soci::use(job_id));


                    sql.begin();
                    for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                        {
                            try
                                {
                                    job_id = (*i);
                                    numberOfFiles = 0;
                                    allFinished = 0;
                                    allCanceled = 0;
                                    allFailed = 0;
                                    terminalState = 0;
                                    mreplica = std::string("");

                                    stmt1.execute(true);

                                    //check for m-replicas job
                                    stmt_m_replica.execute(true);

                                    //check if the file belongs to a multiple replica job
                                    long long replicaJob = 0;
                                    long long replicaJobCountAll = 0;
                                    sql << "select count(*), count(distinct file_index) from t_file where job_id=:job_id",
                                        soci::use(job_id), soci::into(replicaJobCountAll), soci::into(replicaJob);

                                    if(numberOfFiles > 0 && (mreplica == "N" || mreplica == "Y" || mreplica == "H") &&  !(replicaJobCountAll > 1 && replicaJob == 1))
                                        {
                                            stmt8.execute(true);
                                            stmt9.execute(true);
                                            stmt10.execute(true);

                                            terminalState = allFinished + allCanceled + allFailed;

                                            if(numberOfFiles == terminalState)  /* all files terminal state but job in ('ACTIVE','READY','SUBMITTED','STAGING') */
                                                {
                                                    if(allCanceled > 0)
                                                        {
                                                            stmt2.execute(true);
                                                        }
                                                    else   //non canceled, check other states: "FINISHED" and FAILED"
                                                        {
                                                            if(numberOfFiles == allFinished)  /*all files finished*/
                                                                {
                                                                    stmt3.execute(true);
                                                                }
                                                            else
                                                                {
                                                                    if(numberOfFiles == allFailed)  /*all files failed*/
                                                                        {
                                                                            stmt4.execute(true);
                                                                        }
                                                                    else   // otherwise it is FINISHEDDIRTY
                                                                        {
                                                                            stmt5.execute(true);
                                                                        }
                                                                }
                                                        }
                                                }
                                        }

                                    if(mreplica == "R" ||  (replicaJobCountAll > 1 && replicaJob == 1))
                                        {
                                            if(mreplica != "R")
                                                {
                                                    sql << "UPDATE t_job set reuse_job='R' where job_id=:job_id", soci::use(job_id);
                                                }
                                            std::string job_state;
                                            soci::rowset<soci::row> rsReplica = (
                                                                                    sql.prepare <<
                                                                                    " select file_state, COUNT(file_state) from t_file where job_id=:job_id group by file_state order by null ",
                                                                                    soci::use(job_id)
                                                                                );

                                            sql << "SELECT job_state from t_job where job_id=:job_id", soci::use(job_id), soci::into(job_state);

                                            soci::rowset<soci::row>::const_iterator iRep;
                                            for (iRep = rsReplica.begin(); iRep != rsReplica.end(); ++iRep)
                                                {
                                                    std::string file_state = iRep->get<std::string>("FILE_STATE");
                                                    //long long countStates = iRep->get<long long>("COUNT(file_state)",0);

                                                    if(job_state == "CANCELED")
                                                        {
                                                            sql << "UPDATE t_file SET "
                                                                "    file_state = 'CANCELED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), "
                                                                "    reason = 'Job canceled by the user' "
                                                                "    WHERE file_state in ('ACTIVE','SUBMITTED') and job_id = :jobId", soci::use(job_id);
                                                            break;
                                                        }

                                                    if(file_state == "FINISHED") //if at least one is finished, reset the rest
                                                        {
                                                            sql << "UPDATE t_file SET "
                                                                "    file_state = 'NOT_USED', job_finished = NULL, finish_time = NULL, "
                                                                "    reason = '' "
                                                                "    WHERE file_state in ('ACTIVE','SUBMITTED') and job_id = :jobId", soci::use(job_id);

                                                            if(job_state != "FINISHED")
                                                                {
                                                                    stmt3.execute(true); //set the job_state to finished if at least one finished
                                                                }
                                                            break;
                                                        }
                                                }

                                            //do some more sanity checks for m-replica jobs to avoid state incosistencies
                                            if(job_state == "ACTIVE" || job_state == "READY")
                                                {
                                                    long long countSubmittedActiveReady = 0;
                                                    sql << " SELECT count(*) from t_file where file_state in ('ACTIVE','SUBMITTED') and job_id = :job_id",
                                                        soci::use(job_id), soci::into(countSubmittedActiveReady);

                                                    if(countSubmittedActiveReady == 0)
                                                        {
                                                            long long countNotUsed = 0;
                                                            sql << " SELECT count(*) from t_file where file_state = 'NOT_USED' and job_id = :job_id",
                                                                soci::use(job_id), soci::into(countNotUsed);
                                                            if(countNotUsed > 0)
                                                                {
                                                                    sql << "UPDATE t_file SET "
                                                                        "    file_state = 'SUBMITTED', job_finished = NULL, finish_time = NULL, "
                                                                        "    reason = '' "
                                                                        "    WHERE file_state = 'NOT_USED' and job_id = :jobId AND file_id = ( select min(file_id) from t_file where file_state = 'NOT_USED' and job_id=:job_id )", soci::use(job_id), soci::use(job_id);
                                                                }
                                                        }
                                                }
                                        }
                                }
                            catch(...)
                                {

                                }
                        }
                    sql.commit();


                    //now check reverse sanity checks, JOB can't be FINISH,  FINISHEDDIRTY, FAILED is at least one tr is in SUBMITTED, READY, ACTIVE
                    //special case for canceled
                    soci::rowset<std::string> rs2 = (
                                                        sql.prepare <<
                                                        " select  j.job_id from t_job j inner join t_file f on (j.job_id = f.job_id) where j.job_finished is not NULL and f.file_state in ('SUBMITTED','ACTIVE') "
                                                    );


                    sql.begin();
                    for (soci::rowset<std::string>::const_iterator i2 = rs2.begin(); i2 != rs2.end(); ++i2)
                        {
                            job_id = (*i2);
                            stmt7.execute(true);
                        }
                    sql.commit();

                    soci::rowset<std::string> rs444 = (
                                                          sql.prepare <<
                                                          " select  j.job_id from t_job j where job_state='FINISHED' and reuse_job='R' "
                                                      );

                    //multiple replicas with finished state
                    sql.begin();
                    for (soci::rowset<std::string>::const_iterator i444 = rs444.begin(); i444 != rs444.end(); ++i444)
                        {
                            mreplica = std::string("");

                            job_id = (*i444);

                            //check for m-replicas sanity
                            stmt_m_replica.execute(true);

                            long long replicaJob = 0;
                            long long replicaJobCountAll = 0;
                            sql << "select count(*), count(distinct file_index) from t_file where job_id=:job_id",
                                soci::use(job_id), soci::into(replicaJobCountAll), soci::into(replicaJob);

                            //this is a m-replica job
                            if(mreplica == "R" ||  (replicaJobCountAll > 1 && replicaJob == 1))
                                {
                                    std::string job_state;
                                    soci::rowset<soci::row> rsReplica = (
                                                                            sql.prepare <<
                                                                            " select distinct file_state from t_file where job_id=:job_id  ",
                                                                            soci::use(job_id)
                                                                        );

                                    sql << "SELECT job_state from t_job where job_id=:job_id", soci::use(job_id), soci::into(job_state);

                                    soci::rowset<soci::row>::const_iterator iRep;
                                    for (iRep = rsReplica.begin(); iRep != rsReplica.end(); ++iRep)
                                        {
                                            std::string file_state = iRep->get<std::string>("FILE_STATE");

                                            if(file_state == "FINISHED") //if at least one is finished, reset the rest
                                                {
                                                    sql << "UPDATE t_file SET "
                                                        "    file_state = 'NOT_USED', job_finished = NULL, finish_time = NULL, "
                                                        "    reason = '' "
                                                        "    WHERE file_state in ('ACTIVE','SUBMITTED') and job_id = :jobId", soci::use(job_id);

                                                    if(job_state != "FINISHED")
                                                        {
                                                            stmt3.execute(true); //set the job_state to finished if at least one finished
                                                        }
                                                    break;
                                                }
                                        }
                                }
                        }
                    sql.commit();


                    //now check reverse sanity checks, JOB can't be FINISH,  FINISHEDDIRTY, FAILED is at least one tr is in STARTED/DELETE
                    soci::rowset<std::string> rs3 = (
                                                        sql.prepare <<
                                                        " select  j.job_id from t_job j inner join t_dm f on (j.job_id = f.job_id) where j.job_finished >= (sys_extract_utc(systimestamp) - interval '24' HOUR ) and f.file_state in ('STARTED','DELETE')  "
                                                    );

                    sql.begin();
                    for (soci::rowset<std::string>::const_iterator i3 = rs3.begin(); i3 != rs3.end(); ++i3)
                        {
                            job_id = (*i3);
                            stmtDel2.execute(true);
                        }
                    sql.commit();


                    //now check if a host has been offline for more than 120 min and set its transfers to failed
                    soci::rowset<std::string> rsCheckHosts = (
                                sql.prepare <<
                                " SELECT hostname "
                                " FROM t_hosts "
                                " WHERE beat < (sys_extract_utc(systimestamp) - interval '120' MINUTE ) and service_name = 'fts_server' "
                            );

                    std::vector<struct message_state> files;

                    for (soci::rowset<std::string>::const_iterator irsCheckHosts = rsCheckHosts.begin(); irsCheckHosts != rsCheckHosts.end(); ++irsCheckHosts)
                        {
                            std::string deadHost = (*irsCheckHosts);

                            //now check and collect if there are any active/ready in these hosts
                            soci::rowset<soci::row> rsCheckHostsActive = (
                                        sql.prepare <<
                                        " SELECT file_id, job_id from t_file where file_state  = 'ACTIVE' and transferHost = :transferHost ", soci::use(deadHost)
                                    );
                            for (soci::rowset<soci::row>::const_iterator iCheckHostsActive = rsCheckHostsActive.begin(); iCheckHostsActive != rsCheckHostsActive.end(); ++iCheckHostsActive)
                                {
                                    int file_id = iCheckHostsActive->get<int>("FILE_ID");
                                    std::string job_id = iCheckHostsActive->get<std::string>("JOB_ID");
                                    std::string errorMessage = "Transfer has been forced-canceled because host " + deadHost + " is offline and transfers still assigned to it";

                                    updateFileTransferStatusInternal(sql, 0.0, job_id, file_id, "CANCELED", errorMessage, 0, 0, 0, false);
                                    updateJobTransferStatusInternal(sql, job_id, "CANCELED",0);

                                    //send state monitoring message for the state transition
                                    files = getStateOfTransferInternal(sql, job_id, file_id);
                                    if(!files.empty())
                                        {
                                            std::vector<struct message_state>::iterator it;
                                            for (it = files.begin(); it != files.end(); ++it)
                                                {
                                                    struct message_state tmp = (*it);
                                                    constructJSONMsg(&tmp);
                                                }
                                            files.clear();
                                        }
                                }
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
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Sanity states check thread ended " << commit;
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
            soci::rowset<long long> rs = (
                                             sql.prepare <<
                                             " select f.file_id "
                                             " from t_file f  "
                                             " where f.source_se like :source "
                                             "    and f.dest_se like :destination "
                                             "    and f.vo_name = :vo "
                                             "    and f.file_state  = 'ACTIVE' ",
                                             soci::use(source == "*" ? "%" : source),
                                             soci::use(destination == "*" ? "%" : destination),
                                             soci::use(vo)
                                         );

            for (soci::rowset<long long>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    out.push_back(static_cast<int>(*i));
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
        "   and f.file_state  = 'ACTIVE'  ";
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
                "           and file_state  = 'ACTIVE'"
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

            std::map<std::string, fts3::Profile> profiles = prof->getProfiles();
            std::map<std::string, fts3::Profile>::const_iterator i;
            for (i = profiles.begin(); i != profiles.end(); ++i)
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


int OracleAPI::getOptimizerDefaultMode(soci::session& sql)
{
    int modeDefault = 1;
    long long int mode = 0;
    soci::indicator ind = soci::i_ok;

    try
        {
            sql <<
                " select mode_opt "
                " from t_optimize_mode",
                soci::into(mode, ind)
                ;

            if (ind == soci::i_ok)
                {
                    if(mode == 0)
                        return boost::lexical_cast<int>(mode + 1);
                    else
                        return boost::lexical_cast<int>(mode);
                }
            return modeDefault;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught mode exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }

    return modeDefault;
}


int OracleAPI::getOptimizerMode(soci::session& sql)
{
    int modeDefault = MIN_ACTIVE;
    int mode = 0;
    soci::indicator ind = soci::i_ok;

    try
        {
            sql <<
                " select mode_opt "
                " from t_optimize_mode",
                soci::into(mode, ind)
                ;

            if (ind == soci::i_ok)
                {

                    if(mode==1)
                        {
                            return modeDefault;
                        }
                    else if(mode==2)
                        {
                            return (modeDefault *2);
                        }
                    else if(mode==3)
                        {
                            return (modeDefault *3);
                        }
                    else
                        {
                            return modeDefault;
                        }
                }
            return modeDefault;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught mode exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }

    return modeDefault;
}


void OracleAPI::setRetryTransfer(const std::string & jobId, int fileId, int retry, const std::string& reason)
{

    soci::session sql(*connectionPool);

    //expressed in secs, default delay
    const int default_retry_delay = DEFAULT_RETRY_DELAY;
    long long retry_delay = 0;
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


            struct tm tTime;
            if (retry_delay > 0)
                {
                    // update
                    time_t now = getUTC(retry_delay);
                    gmtime_r(&now, &tTime);
                }
            else
                {
                    // update
                    time_t now = getUTC(default_retry_delay);
                    gmtime_r(&now, &tTime);
                }

            int bring_online = -1;
            int copy_pin_lifetime = -1;

            // query for the file state in DB
            sql << "SELECT bring_online, copy_pin_lifetime FROM t_job WHERE job_id=:jobId",
                soci::use(jobId),
                soci::into(bring_online),
                soci::into(copy_pin_lifetime);


            bool exists = (reason.find("ETIMEDOUT") != std::string::npos);

            //staging exception, if file failed with timeout and was staged before, reset it
            if( (bring_online > 0 || copy_pin_lifetime > 0) && exists)
                {
                    sql << "update t_file set retry = :retry, current_failures = 0, file_state='STAGING', internal_file_params=NULL, transferHost=NULL,start_time=NULL, pid=NULL, "
                        " filesize=0, staging_start=NULL, staging_finished=NULL where file_id=:file_id and job_id=:job_id AND file_state NOT IN ('STAGING','FINISHED','SUBMITTED','FAILED','CANCELED') ",
                        soci::use(retry),
                        soci::use(fileId),
                        soci::use(jobId);
                }
            else
                {
                    sql << "UPDATE t_file SET retry_timestamp=:1, retry = :retry, file_state = 'SUBMITTED', start_time=NULL, transferHost=NULL, t_log_file=NULL,"
                        " t_log_file_debug=NULL, throughput = 0, current_failures = 1 "
                        " WHERE  file_id = :fileId AND  job_id = :jobId AND file_state NOT IN ('FINISHED','SUBMITTED','FAILED','CANCELED')",
                        soci::use(tTime), soci::use(retry), soci::use(fileId), soci::use(jobId);

                }

            // Keep log
            sql << "INSERT /*+ IGNORE_ROW_ON_DUPKEY_INDEX (t_file_retry_errors, t_file_retry_errors_pk) */  INTO t_file_retry_errors "
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

void OracleAPI::updateHeartBeat(unsigned* index, unsigned* count, unsigned* start, unsigned* end, std::string service_name)
{
    soci::session sql(*connectionPool);

    try
        {
            updateHeartBeatInternal(sql, index, count, start, end, service_name);
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

void OracleAPI::updateHeartBeatInternal(soci::session& sql, unsigned* index, unsigned* count, unsigned* start, unsigned* end, std::string service_name)
{
    try
        {
            sql.begin();

            // Update beat
            sql << "MERGE INTO t_hosts USING "
                " (SELECT :hostname AS hostname, :service_name AS service_name FROM dual) Hostname ON "
                " (t_hosts.hostname = Hostname.hostname AND t_hosts.service_name = Hostname.service_name ) "
                " WHEN     MATCHED THEN UPDATE SET t_hosts.beat = sys_extract_utc(systimestamp)"
                " WHEN NOT MATCHED THEN INSERT (hostname, beat, service_name) VALUES (Hostname.hostname, sys_extract_utc(systimestamp), Hostname.service_name)",
                soci::use(hostname),soci::use(service_name);

            // Total number of working instances
            sql << "SELECT COUNT(hostname) FROM t_hosts "
                "  WHERE beat >= (sys_extract_utc(systimestamp) - interval '2' minute) and service_name = :service_name",
                soci::use(service_name),
                soci::into(*count);

            // This instance index
            soci::rowset<std::string> rsHosts = (sql.prepare <<
                                                 "SELECT hostname FROM t_hosts "
                                                 "WHERE beat >= (sys_extract_utc(systimestamp) - interval '2' minute)  and service_name = :service_name "
                                                 "ORDER BY hostname ", soci::use(service_name) );

            soci::rowset<std::string>::const_iterator i;
            for (*index = 0, i = rsHosts.begin(); i != rsHosts.end(); ++i, ++(*index))
                {
                    std::string& host = *i;
                    if (host == hostname)
                        break;
                }

            sql.commit();

            if(*count != 0)
                {
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

            // Delete old entries
            if(hashSegment.start == 0)
                {
                    sql.begin();
                    soci::statement stmt3 = (sql.prepare <<
                                             "DELETE FROM t_hosts "
                                             "WHERE beat <= (sys_extract_utc(systimestamp) - interval '120' MINUTE) "
                                             "   AND drain = 0");
                    stmt3.execute(true);
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


void OracleAPI::updateOptimizerEvolution(soci::session& sql, const std::string & source_hostname, const std::string & destination_hostname, int active, double throughput, double successRate, int buffer, int bandwidth)
{
    try
        {
            if(throughput > 0 && successRate > 0)
                {
                    double agrthroughput = 0.0;

                    sql.begin();
                    sql << " INSERT INTO t_optimizer_evolution (datetime, source_se, dest_se, active, throughput, filesize, buffer, nostreams, agrthroughput) "
                        " values(sys_extract_utc(systimestamp), :source, :dest, :active, :throughput, :filesize, :buffer, :nostreams, :agrthroughput) ",
                        soci::use(source_hostname),
                        soci::use(destination_hostname),
                        soci::use(active),
                        soci::use(throughput),
                        soci::use(successRate),
                        soci::use(buffer),
                        soci::use(bandwidth),
                        soci::use(agrthroughput);
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
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
}



void OracleAPI::snapshot(const std::string & vo_name, const std::string & source_se_p, const std::string & dest_se_p, const std::string &, std::stringstream & result)
{
    soci::session sql(*connectionPool);

    std::string vo_name_local;
    std::string dest_se;
    std::string source_se;
    std::string reason;
    long long countReason = 0;
    long long active = 0;
    long long maxActive = 0;
    long long submitted = 0;
    double throughput1h = 0.0;
    double throughput30min = 0.0;
    double throughput15min = 0.0;
    double throughput5min = 0.0;
    long long nFailedLastHour = 0;
    long long  nFinishedLastHour = 0;
    double  ratioSuccessFailure = 0.0;
    std::string querySe;
    std::string queryVO;

    if(!vo_name.empty())
        querySe = " SELECT DISTINCT source_se, dest_se FROM t_job where vo_name='" + vo_name + "'";
    else
        querySe = " SELECT DISTINCT source_se, dest_se FROM t_file";

    time_t now = time(NULL);
    struct tm tTime;
    gmtime_r(&now, &tTime);

    soci::indicator isNull1 = soci::i_ok;
    soci::indicator isNull2 = soci::i_ok;
    soci::indicator isNull3 = soci::i_ok;

    if(vo_name.empty())
        {
            queryVO = "select distinct vo_name from t_job ";
        }

    bool sourceEmpty = true;
    bool destinEmpty = true;

    if(!source_se_p.empty())
        {
            source_se = source_se_p;
            if(!vo_name.empty())
                {
                    querySe += " and source_se = '" + source_se + "'";
                    queryVO += " and source_se = '" + source_se + "'";
                }
            else
                {
                    querySe += " where source_se ='" + source_se + "'";
                    queryVO += " where source_se ='" + source_se + "'";
                }
            sourceEmpty = false;
        }

    if(!dest_se_p.empty())
        {
            destinEmpty = false;
            if(sourceEmpty)
                {
                    dest_se = dest_se_p;
                    if(!vo_name.empty())
                        {
                            querySe += " and dest_se = '" + dest_se + "'";
                            queryVO += " and dest_se = '" + dest_se + "'";
                        }
                    else
                        {
                            querySe += " where dest_se = '" + dest_se + "'";
                            queryVO += " where dest_se = '" + dest_se + "'";
                        }
                }
            else
                {
                    dest_se = dest_se_p;
                    querySe += " AND dest_se = '" + dest_se + "'";
                    queryVO += " AND dest_se = '" + dest_se + "'";
                }
        }

    try
        {

            soci::statement pairsStmt((sql.prepare << querySe, soci::into(source_se), soci::into(dest_se)));
            soci::statement voStmt((sql.prepare << queryVO, soci::into(vo_name_local)));

            soci::statement st1((sql.prepare << "select count(*) from t_file where "
                                 " file_state='ACTIVE' and vo_name=:vo_name_local and "
                                 " source_se=:source_se and dest_se=:dest_se",
                                 soci::use(vo_name_local),
                                 soci::use(source_se),
                                 soci::use(dest_se),
                                 soci::into(active)));

            soci::statement st2((sql.prepare << "select active from t_optimize_active where "
                                 " source_se=:source_se and dest_se=:dest_se",
                                 soci::use(source_se),
                                 soci::use(dest_se),
                                 soci::into(maxActive, isNull1)
                                ));

            soci::statement st3((sql.prepare << "select count(*) from t_file where "
                                 " file_state='SUBMITTED' and vo_name=:vo_name_local and "
                                 " source_se=:source_se and dest_se=:dest_se",
                                 soci::use(vo_name_local),
                                 soci::use(source_se),
                                 soci::use(dest_se),
                                 soci::into(submitted)
                                ));


            soci::statement st41((sql.prepare << "select avg(throughput) from t_file where  "
                                  " source_se=:source_se and dest_se=:dest_se "
                                  " AND  file_state in ('ACTIVE','FINISHED') and (job_finished is NULL OR  job_finished >= (sys_extract_utc(systimestamp) - interval '60' minute)) "
                                  " AND throughput > 0 AND throughput is not NULL ",
                                  soci::use(source_se),
                                  soci::use(dest_se),
                                  soci::into(throughput1h, isNull2)
                                 ));

            soci::statement st42((sql.prepare << "select avg(throughput) from t_file where  "
                                  " source_se=:source_se and dest_se=:dest_se "
                                  " AND  file_state in ('ACTIVE','FINISHED') and (job_finished is NULL OR  job_finished >= (sys_extract_utc(systimestamp) - interval '30' minute)) "
                                  " AND throughput > 0 AND throughput is not NULL ",
                                  soci::use(source_se),
                                  soci::use(dest_se),
                                  soci::into(throughput30min, isNull2)
                                 ));

            soci::statement st43((sql.prepare << "select avg(throughput) from t_file where  "
                                  " source_se=:source_se and dest_se=:dest_se "
                                  " AND  file_state in ('ACTIVE','FINISHED') and (job_finished is NULL OR  job_finished >= (sys_extract_utc(systimestamp) - interval '15' minute)) "
                                  " AND throughput > 0 AND throughput is not NULL ",
                                  soci::use(source_se),
                                  soci::use(dest_se),
                                  soci::into(throughput15min, isNull2)
                                 ));

            soci::statement st44((sql.prepare << "select avg(throughput) from t_file where  "
                                  " source_se=:source_se and dest_se=:dest_se "
                                  " AND  file_state in ('ACTIVE','FINISHED') and (job_finished is NULL OR  job_finished >= (sys_extract_utc(systimestamp) - interval '5' minute)) "
                                  " AND throughput > 0 AND throughput is not NULL ",
                                  soci::use(source_se),
                                  soci::use(dest_se),
                                  soci::into(throughput5min, isNull2)
                                 ));


            soci::statement st5((sql.prepare << "select * from (select reason, count(reason) as c from t_file where "
                                 " (job_finished >= (sys_extract_utc(systimestamp) - interval '60' minute)) "
                                 " AND file_state='FAILED' and "
                                 " source_se=:source_se and dest_se=:dest_se and vo_name =:vo_name_local and reason is not NULL "
                                 " group by reason order by c desc) WHERE ROWNUM=1",
                                 soci::use(source_se),
                                 soci::use(dest_se),
                                 soci::use(vo_name_local),
                                 soci::into(reason, isNull3),
                                 soci::into(countReason)
                                ));

            soci::statement st6((sql.prepare << "select count(*) from t_file where "
                                 " file_state='FAILED' and vo_name=:vo_name_local and "
                                 " source_se=:source_se and dest_se=:dest_se AND job_finished >= (sys_extract_utc(systimestamp) - interval '60' minute)",
                                 soci::use(vo_name_local),
                                 soci::use(source_se),
                                 soci::use(dest_se),
                                 soci::into(nFailedLastHour)
                                ));

            soci::statement st7((sql.prepare << "select count(*) from t_file where "
                                 " file_state='FINISHED' and vo_name=:vo_name_local and "
                                 " source_se=:source_se and dest_se=:dest_se AND job_finished >= (sys_extract_utc(systimestamp) - interval '60' minute)",
                                 soci::use(vo_name_local),
                                 soci::use(source_se),
                                 soci::use(dest_se),
                                 soci::into(nFinishedLastHour)
                                ));

            if(vo_name.empty())
                {
                    result << "{\"snapshot\" : [";

                    voStmt.execute();
                    while (voStmt.fetch()) //distinct vo
                        {
                            if(source_se_p.empty())
                                source_se = "";
                            if(dest_se_p.empty())
                                dest_se = "";

                            bool first = true;

                            pairsStmt.execute();
                            while (pairsStmt.fetch()) //distinct source_se / dest_se
                                {
                                    active = 0;
                                    maxActive = 0;
                                    submitted = 0;
                                    throughput1h = 0.0;
                                    throughput30min = 0.0;
                                    throughput15min = 0.0;
                                    throughput5min = 0.0;
                                    nFailedLastHour = 0;
                                    nFinishedLastHour = 0;
                                    ratioSuccessFailure = 0.0;

                                    st1.execute(true);
                                    st2.execute(true);
                                    st7.execute(true);
                                    st6.execute(true);
                                    st3.execute(true);

                                    //if all of the above return 0 then continue
                                    if(active == 0 && nFinishedLastHour == 0 &&  nFailedLastHour == 0 && submitted == 0 && source_se_p.empty() && dest_se_p.empty())
                                        continue;


                                    if (!first) result << "\n,\n";
                                    first = false;

                                    result << "{\n";

                                    result << std::fixed << "\"VO\":\"";
                                    result <<   vo_name_local;
                                    result <<   "\",\n";

                                    result <<   "\"Source endpoint\":\"";
                                    result <<   source_se;
                                    result <<   "\",\n";

                                    result <<   "\"Destination endpoint\":\"";
                                    result <<   dest_se;
                                    result <<   "\",\n";

                                    //get active for this pair and vo
                                    result <<   "\"Current active transfers\":\"";
                                    result <<   active;
                                    result <<   "\",\n";

                                    //get max active for this pair no matter the vo
                                    result <<   "\"Max active transfers\":\"";
                                    result <<   maxActive;
                                    result <<   "\",\n";

                                    result <<   "\"Number of finished (last hour)\":\"";
                                    result <<   long(nFinishedLastHour);
                                    result <<   "\",\n";

                                    result <<   "\"Number of failed (last hour)\":\"";
                                    result <<   long(nFailedLastHour);
                                    result <<   "\",\n";

                                    //get submitted for this pair and vo
                                    result <<   "\"Number of queued\":\"";
                                    result <<   submitted;
                                    result <<   "\",\n";


                                    //average throughput block
                                    st41.execute(true);
                                    result <<   "\"Avg throughput (last 60min)\":\"";
                                    result <<  std::setprecision(2) << throughput1h;
                                    result <<   " MB/s\",\n";

                                    st42.execute(true);
                                    result <<   "\"Avg throughput (last 30min)\":\"";
                                    result <<  std::setprecision(2) << throughput30min;
                                    result <<   " MB/s\",\n";

                                    st43.execute(true);
                                    result <<   "\"Avg throughput (last 15min)\":\"";
                                    result <<  std::setprecision(2) << throughput15min;
                                    result <<   " MB/s\",\n";

                                    st44.execute(true);
                                    result <<   "\"Avg throughput (last 5min)\":\"";
                                    result <<  std::setprecision(2) << throughput5min;
                                    result <<   " MB/s\",\n";


                                    //round up efficiency
                                    if(nFinishedLastHour > 0)
                                        {
                                            ratioSuccessFailure = ceil((double)nFinishedLastHour/((double)nFinishedLastHour + (double)nFailedLastHour) * (100.0));
                                        }

                                    result <<   "\"Link efficiency (last hour)\":\"";
                                    result <<   ratioSuccessFailure;
                                    result <<   "%\",\n";

                                    //most frequent error and number the last 30min
                                    reason = "";
                                    countReason = 0;
                                    st5.execute(true);

                                    result <<   "\"Most frequent error (last hour)\":\"";
                                    result <<   countReason;
                                    result <<   " times: ";
                                    result <<   reason;
                                    result <<   "\"\n";

                                    result << "}\n";
                                }
                        }

                    result << "]}";
                }
            else
                {
                    vo_name_local = vo_name;
                    if(source_se_p.empty())
                        source_se = "";
                    if(dest_se_p.empty())
                        dest_se = "";

                    result << "{\"snapshot\" : [";
                    bool first = true;


                    pairsStmt.execute();
                    while (pairsStmt.fetch())
                        {
                            active = 0;
                            maxActive = 0;
                            submitted = 0;
                            throughput1h = 0.0;
                            throughput30min = 0.0;
                            throughput15min = 0.0;
                            throughput5min = 0.0;
                            nFailedLastHour = 0;
                            nFinishedLastHour = 0;
                            ratioSuccessFailure = 0.0;

                            st1.execute(true);
                            st2.execute(true);
                            st7.execute(true);
                            st6.execute(true);
                            st3.execute(true);


                            //if all of the above return 0 then continue
                            if(active == 0 && nFinishedLastHour == 0 &&  nFailedLastHour == 0 && submitted == 0 && source_se_p.empty() && dest_se_p.empty())
                                continue;

                            if (!first) result << "\n,\n";
                            first = false;

                            result << "{\n";

                            result << std::fixed << "\"VO\":\"";
                            result <<   vo_name_local;
                            result <<   "\",\n";

                            result <<   "\"Source endpoint\":\"";
                            result <<   source_se;
                            result <<   "\",\n";

                            result <<   "\"Destination endpoint\":\"";
                            result <<   dest_se;
                            result <<   "\",\n";

                            //get active for this pair and vo
                            result <<   "\"Current active transfers\":\"";
                            result <<   active;
                            result <<   "\",\n";

                            //get max active for this pair no matter the vo
                            result <<   "\"Max active transfers\":\"";
                            result <<   maxActive;
                            result <<   "\",\n";

                            result <<   "\"Number of finished (last hour)\":\"";
                            result <<   long(nFinishedLastHour);
                            result <<   "\",\n";

                            result <<   "\"Number of failed (last hour)\":\"";
                            result <<   long(nFailedLastHour);
                            result <<   "\",\n";

                            //get submitted for this pair and vo
                            result <<   "\"Number of queued\":\"";
                            result <<   submitted;
                            result <<   "\",\n";


                            //average throughput block
                            st41.execute(true);
                            result <<   "\"Avg throughput (last 60min)\":\"";
                            result <<  std::setprecision(2) << throughput1h;
                            result <<   " MB/s\",\n";

                            st42.execute(true);
                            result <<   "\"Avg throughput (last 30min)\":\"";
                            result <<  std::setprecision(2) << throughput30min;
                            result <<   " MB/s\",\n";

                            st43.execute(true);
                            result <<   "\"Avg throughput (last 15min)\":\"";
                            result <<  std::setprecision(2) << throughput15min;
                            result <<   " MB/s\",\n";

                            st44.execute(true);
                            result <<   "\"Avg throughput (last 5min)\":\"";
                            result <<  std::setprecision(2) << throughput5min;
                            result <<   " MB/s\",\n";


                            //round up efficiency
                            if(nFinishedLastHour > 0)
                                {
                                    ratioSuccessFailure = ceil((double)nFinishedLastHour/((double)nFinishedLastHour + (double)nFailedLastHour) * (100.0));
                                }

                            result <<   "\"Link efficiency (last hour)\":\"";
                            result <<   ratioSuccessFailure;
                            result <<   "%\",\n";

                            //most frequent error and number the last 30min
                            reason = "";
                            countReason = 0;
                            st5.execute(true);

                            result <<   "\"Most frequent error (last hour)\":\"";
                            result <<   countReason;
                            result <<   " times: ";
                            result <<   reason;
                            result <<   "\"\n";

                            result << "}\n";
                        }

                    result << "]}";
                }
            result.unsetf(std::ios::floatfield);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
}

bool OracleAPI::getDrain()
{
    soci::session sql(*connectionPool);

    try
        {
            return getDrainInternal(sql);
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

bool OracleAPI::getDrainInternal(soci::session& sql)
{

    int drain = 0;

    try
        {
            soci::indicator isNull = soci::i_ok;

            sql << "SELECT drain FROM t_hosts WHERE hostname = :hostname",soci::use(hostname), soci::into(drain, isNull);

            if(isNull == soci::i_null || drain == 0)
                return false;

            return true;
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


void OracleAPI::setDrain(bool drain)
{

    soci::session sql(*connectionPool);
    unsigned index=0, count1=0, start=0, end=0;
    std::string service_name = "fts_server";

    try
        {
            if(drain == true)
                {
                    sql.begin();
                    sql << " update t_hosts set drain=1 where hostname = :hostname ",soci::use(hostname);
                    sql.commit();
                }
            else
                {
                    //update heartbeat first to avoid overlapping of hash range when moving out of draining mode
                    updateHeartBeatInternal(sql, &index, &count1, &start, &end, service_name);
                    sleep(2);

                    sql.begin();
                    sql << " update t_hosts set drain=0 where hostname = :hostname ",soci::use(hostname);
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
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
}

bool OracleAPI::bandwidthChecker(soci::session& sql, const std::string & source_hostname, const std::string & destination_hostname, int& bandwidthIn)
{
    long long int bandwidthSrc = 0;
    long long int bandwidthDst = 0;
    double througputSrc = 0.0;
    double througputDst = 0.0;

    soci::indicator isNullBandwidthSrc = soci::i_ok;
    soci::indicator isNullBandwidthDst = soci::i_ok;
    soci::indicator isNullThrougputSrc = soci::i_ok;
    soci::indicator isNullThrougputDst = soci::i_ok;

    //get limit for source
    sql << "select throughput from t_optimize where source_se= :name  and throughput is not NULL ",
        soci::use(source_hostname), soci::into(bandwidthSrc, isNullBandwidthSrc);

    //get limit for dest
    sql << "select throughput from t_optimize where dest_se= :name  and throughput is not NULL ",
        soci::use(destination_hostname), soci::into(bandwidthDst, isNullBandwidthDst);

    if(!sql.got_data() || bandwidthSrc == -1)
        bandwidthSrc = -1;

    if(!sql.got_data() || bandwidthDst == -1)
        bandwidthDst = -1;

    //no limits are applied either for source or dest, stop here before executing more expensive queries
    if(bandwidthDst == -1 && bandwidthSrc == -1)
        {
            return true;
        }

    //get aggregated thr from source
    sql << "select sum(throughput) from t_file where source_se= :name and file_state='ACTIVE'  and throughput is not NULL ",
        soci::use(source_hostname), soci::into(througputSrc, isNullThrougputSrc);


    //get aggregated thr towards dest
    sql << "select sum(throughput) from t_file where dest_se= :name and file_state='ACTIVE' and throughput is not NULL ",
        soci::use(destination_hostname), soci::into(througputDst, isNullThrougputDst);

    if(bandwidthSrc > 0 )
        {
            if(bandwidthDst > 0) //both source and dest have limits, take the lowest
                {
                    //get the lowest limit to be respected
                    long long int lowest = (bandwidthSrc < bandwidthDst) ? bandwidthSrc : bandwidthDst;

                    if (througputSrc > lowest || througputDst > lowest)
                        {
                            bandwidthIn = boost::lexical_cast<int>(lowest);
                            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Bandwidth limitation of " << lowest  << " MB/s is set for " << source_hostname << " or " <<  destination_hostname << commit;
                            return false;
                        }
                }
            //only source limit is set
            if (througputSrc > bandwidthSrc)
                {
                    bandwidthIn = boost::lexical_cast<int>(bandwidthSrc);
                    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Bandwidth limitation of " << bandwidthSrc  << " MB/s is set for " << source_hostname << commit;
                    return false;
                }

        }
    else if(bandwidthDst > 0)  //only destination has limit
        {
            bandwidthIn = boost::lexical_cast<int>(bandwidthDst);

            if(througputDst > bandwidthDst)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Bandwidth limitation of " << bandwidthDst  << " MB/s is set for " << destination_hostname << commit;
                    return false;
                }
        }
    else
        {

            return true;
        }

    return true;
}




std::string OracleAPI::getBandwidthLimit()
{
    soci::session sql(*connectionPool);

    std::string result;

    try
        {
            std::string source_hostname;
            std::string destination_hostname;
            result = getBandwidthLimitInternal(sql, source_hostname, destination_hostname);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }

    return result;
}

std::string OracleAPI::getBandwidthLimitInternal(soci::session& sql, const std::string & source_hostname, const std::string & destination_hostname)
{

    std::ostringstream result;

    try
        {
            if(!source_hostname.empty())
                {
                    double bandwidth = 0;
                    soci::indicator isNullBandwidth = soci::i_ok;
                    sql << " select throughput from t_optimize where source_se = :source_se and throughput is not NULL ",
                        soci::use(source_hostname), soci::into(bandwidth, isNullBandwidth);

                    if(sql.got_data() && bandwidth > 0)
                        result << "Source endpoint: " << source_hostname << "   Bandwidth restriction: " << bandwidth << " MB/s\n";
                }
            else if (!destination_hostname.empty())
                {
                    double bandwidth = 0;
                    soci::indicator isNullBandwidth = soci::i_ok;
                    sql << " select throughput from t_optimize where dest_se = :dest_se  and throughput is not NULL ",
                        soci::use(destination_hostname), soci::into(bandwidth, isNullBandwidth);

                    if(sql.got_data() && bandwidth > 0)
                        result << "Destination endpoint: " << destination_hostname << "   Bandwidth restriction: " << bandwidth << " MB/s\n";
                }
            else
                {
                    soci::rowset<soci::row> rs = (
                                                     sql.prepare <<
                                                     " SELECT source_se, dest_se, throughput from t_optimize "
                                                     " WHERE  "
                                                     " throughput is not NULL "
                                                 );

                    soci::rowset<soci::row>::const_iterator it;
                    for (it = rs.begin(); it != rs.end(); ++it)
                        {
                            std::string source_se = it->get<std::string>("SOURCE_SE","");
                            std::string dest_se = it->get<std::string>("DEST_SE","");
                            double bandwidth = it->get<double>("THROUGHPUT");

                            if(!source_se.empty() != 0 && bandwidth > 0)
                                {
                                    result << "Source endpoint: " << source_se << "   Bandwidth restriction: " << bandwidth << " MB/s\n";
                                }
                            if(!dest_se.empty() != 0 && bandwidth > 0)
                                {
                                    result << "Destination endpoint: " << dest_se   << "   Bandwidth restriction: " << bandwidth << " MB/s\n";
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
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }

    return result.str();
}

void OracleAPI::setBandwidthLimit(const std::string & source_hostname, const std::string & destination_hostname, int bandwidthLimit)
{

    soci::session sql(*connectionPool);

    try
        {
            long long int countSource = 0;
            long long int countDest = 0;

            if(!source_hostname.empty())
                {
                    sql << "select count(*) from t_optimize where source_se=:source_se and throughput is not NULL ",
                        soci::use(source_hostname), soci::into(countSource);

                    if(countSource == 0 && bandwidthLimit > 0)
                        {
                            sql.begin();

                            sql << " insert into t_optimize(throughput, source_se) values(:throughput, :source_se) ",
                                soci::use(bandwidthLimit), soci::use(source_hostname);

                            sql.commit();
                        }
                    else if (countSource > 0)
                        {
                            if(bandwidthLimit == -1)
                                {
                                    sql.begin();
                                    sql << "update t_optimize set throughput=NULL where source_se=:source_se ",
                                        soci::use(source_hostname);
                                    sql.commit();
                                }
                            else
                                {
                                    sql.begin();
                                    sql << "update t_optimize set throughput=:throughput where source_se=:source_se  and throughput is not NULL",
                                        soci::use(bandwidthLimit), soci::use(source_hostname);
                                    sql.commit();
                                }
                        }
                }

            if(!destination_hostname.empty())
                {
                    sql << "select count(*) from t_optimize where dest_se=:dest_se  and throughput is not NULL",
                        soci::use(destination_hostname), soci::into(countDest);

                    if(countDest == 0 && bandwidthLimit > 0)
                        {
                            sql.begin();

                            sql << " insert into t_optimize(throughput, dest_se) values(:throughput, :dest_se) ",
                                soci::use(bandwidthLimit), soci::use(destination_hostname);

                            sql.commit();
                        }
                    else if (countDest > 0)
                        {
                            if(bandwidthLimit == -1)
                                {
                                    sql.begin();
                                    sql << "update t_optimize set throughput=NULL where dest_se=:dest_se ",
                                        soci::use(destination_hostname);
                                    sql.commit();
                                }
                            else
                                {
                                    sql.begin();
                                    sql << "update t_optimize set throughput=:throughput where dest_se=:dest_se  and throughput is not NULL",
                                        soci::use(bandwidthLimit), soci::use(destination_hostname);
                                    sql.commit();
                                }

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
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }
}


bool OracleAPI::isProtocolUDT(const std::string & source_hostname, const std::string & destination_hostname)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::indicator isNullUDT = soci::i_ok;
            std::string udt;

            sql << " select udt from t_optimize where (source_se = :source_se OR source_se = :dest_se)  and udt is not NULL ",
                soci::use(source_hostname), soci::use(destination_hostname), soci::into(udt, isNullUDT);

            if(sql.got_data() && udt == "on")
                return true;

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }

    return false;
}


bool OracleAPI::isProtocolIPv6(const std::string & source_hostname, const std::string & destination_hostname)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::indicator isNullUDT = soci::i_ok;
            std::string ipv6;

            sql << " select ipv6 from t_optimize where (source_se = :source_se OR source_se = :dest_se)  and ipv6 is not NULL ",
                soci::use(source_hostname), soci::use(destination_hostname), soci::into(ipv6, isNullUDT);

            if(sql.got_data() && ipv6 == "on")
                return true;

        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }

    return false;
}


int OracleAPI::getStreamsOptimization(const std::string & source_hostname, const std::string & destination_hostname)
{
    soci::session sql(*connectionPool);
    long long int maxNoStreams = 0;
    long long int optimumNoStreams = 0;
    int defaultStreams = 1;
    soci::indicator isNullMaxStreamsFound = soci::i_ok;
    soci::indicator isNullOptimumStreamsFound = soci::i_ok;
    int allTested = 0;
    int global_tcp_stream = 0;

    try
        {
            sql << "SELECT global_tcp_stream from t_server_config where (vo_name IS NULL OR vo_name = '*') and  global_tcp_stream > 0",
                soci::into(global_tcp_stream);
            if(sql.got_data() && global_tcp_stream > 0)
                {
                    return global_tcp_stream;
                }


            sql << " SELECT count(*) from t_optimize_streams where source_se=:source_se "
                " and dest_se=:dest_se and tested = 1 and throughput is not NULL  and throughput > 0",
                soci::use(source_hostname), soci::use(destination_hostname), soci::into(allTested);

            if(sql.got_data())
                {
                    if(allTested == 16) //this is the maximum, meaning taken all samples from 1-16 TCP strreams
                        {
                            sql << " SELECT nostreams FROM (SELECT nostreams FROM t_optimize_streams   WHERE "
                                " source_se=:source_se and dest_se=:dest_se ORDER BY throughput DESC) WHERE  rownum <= 1 ",
                                soci::use(source_hostname), soci::use(destination_hostname), soci::into(optimumNoStreams, isNullOptimumStreamsFound);

                            if(sql.got_data())
                                {
                                    return (int) optimumNoStreams;
                                }
                            else
                                {
                                    return defaultStreams;
                                }
                        }
                    else
                        {
                            sql << " SELECT max(nostreams) from t_optimize_streams where source_se=:source_se and dest_se=:dest_se ",
                                soci::use(source_hostname), soci::use(destination_hostname), soci::into(maxNoStreams, isNullMaxStreamsFound);

                            sql.begin();
                            sql << "update t_optimize_streams set tested = 1, datetime = sys_extract_utc(systimestamp) where source_se=:source and dest_se=:dest and tested = 0 and nostreams = :nostreams",
                                soci::use(source_hostname), soci::use(destination_hostname), soci::use(maxNoStreams);
                            sql.commit();
                            return (int) maxNoStreams;
                        }
                }
            else  //it's NULL, no info yet stored, use default 1
                {
                    return defaultStreams;
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
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }

    return defaultStreams;
}



int OracleAPI::getGlobalTimeout()
{
    soci::session sql(*connectionPool);
    long long timeout = 0;

    try
        {
            soci::indicator isNullTimeout = soci::i_ok;

            sql << "SELECT global_timeout FROM t_server_config WHERE vo_name IS NULL OR vo_name = '*'", soci::into(timeout, isNullTimeout);

            if(sql.got_data() && timeout > 0)
                {
                    return boost::lexical_cast<int>(timeout);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }

    return boost::lexical_cast<int>(timeout);

}

void OracleAPI::setGlobalTimeout(int timeout)
{
    soci::session sql(*connectionPool);
    long long timeoutLocal = 0;
    soci::indicator isNullTimeout = soci::i_ok;

    try
        {
            sql << "SELECT global_timeout FROM t_server_config WHERE vo_name IS NULL or vo_name = '*'", soci::into(timeoutLocal, isNullTimeout);
            if (!sql.got_data())
                {
                    sql.begin();

                    sql << "INSERT INTO t_server_config (global_timeout, vo_name) VALUES (:timeout, '*') ",
                        soci::use(timeout);

                    sql.commit();

                }
            else
                {
                    sql.begin();

                    sql << "UPDATE t_server_config SET global_timeout = :timeout",
                        soci::use(timeout);

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

int OracleAPI::getSecPerMb()
{
    soci::session sql(*connectionPool);
    long long seconds = 0;

    try
        {
            soci::indicator isNullSeconds = soci::i_ok;

            sql << "SELECT sec_per_mb FROM t_server_config WHERE vo_name IS NULL OR vo_name = '*'", soci::into(seconds, isNullSeconds);

            if(sql.got_data() && seconds > 0)
                {
                    return boost::lexical_cast<int>(seconds);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception ");
        }

    return boost::lexical_cast<int>(seconds);

}

void OracleAPI::setSecPerMb(int seconds)
{
    soci::session sql(*connectionPool);
    long long secondsLocal = 0;
    soci::indicator isNullSeconds = soci::i_ok;

    try
        {
            sql << "SELECT sec_per_mb FROM t_server_config WHERE vo_name IS NULL OR vo_name = '*'", soci::into(secondsLocal, isNullSeconds);
            if (!sql.got_data())
                {
                    sql.begin();

                    sql << "INSERT INTO t_server_config (sec_per_mb, vo_name) VALUES (:seconds, '*') ",
                        soci::use(seconds);

                    sql.commit();

                }
            else
                {
                    sql.begin();

                    sql << "UPDATE t_server_config SET sec_per_mb = :seconds",
                        soci::use(seconds);

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


void OracleAPI::setSourceMaxActive(const std::string & source_hostname, int maxActive)
{
    soci::session sql(*connectionPool);
    std::string source_se;
    soci::indicator isNullSourceSe = soci::i_ok;

    try
        {
            sql << "select source_se from t_optimize where source_se = :source_se and active is not NULL ", soci::use(source_hostname), soci::into(source_se, isNullSourceSe);

            if (!sql.got_data())
                {
                    sql.begin();

                    if (maxActive == -1)
                        {
                            sql << "INSERT INTO t_optimize (file_id, source_se, active) VALUES (1, :source_se, NULL)  ",
                                soci::use(source_hostname);
                        }
                    else
                        {
                            sql << "INSERT INTO t_optimize (file_id, source_se, active) VALUES (1, :source_se, :active)  ",
                                soci::use(source_hostname), soci::use(maxActive);
                        }
                    sql.commit();
                }
            else
                {
                    sql.begin();

                    if (maxActive == -1)
                        {
                            sql << "update t_optimize set active = NULL where source_se = :source_se ",
                                soci::use(source_hostname);
                        }
                    else
                        {
                            sql << "update t_optimize set active = :active where source_se = :source_se and active is not NULL ",
                                soci::use(maxActive), soci::use(source_hostname);
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

void OracleAPI::setDestMaxActive(const std::string & destination_hostname, int maxActive)
{
    soci::session sql(*connectionPool);
    std::string dest_se;
    soci::indicator isNullDestSe = soci::i_ok;

    try
        {
            sql << "select dest_se from t_optimize where dest_se = :dest_se and active is not NULL", soci::use(destination_hostname), soci::into(dest_se, isNullDestSe);

            if (!sql.got_data())
                {
                    sql.begin();

                    if (maxActive > 0)
                        {
                            sql << "INSERT INTO t_optimize (file_id, dest_se, active) VALUES (1, :dest_se, :active)  ",
                                soci::use(destination_hostname), soci::use(maxActive);
                        }
                    sql.commit();
                }
            else
                {
                    sql.begin();

                    if (maxActive == -1)
                        {
                            sql << "update t_optimize set active = NULL where dest_se = :dest_se ",
                                soci::use(destination_hostname);
                        }
                    else
                        {
                            sql << "update t_optimize set active = :active where dest_se = :dest_se and active is not NULL ",
                                soci::use(maxActive), soci::use(destination_hostname);
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

void OracleAPI::setFixActive(const std::string & source, const std::string & destination, int active)
{
    soci::session sql(*connectionPool);
    try
        {
            sql.begin();
            if (active > 0)
                {
                    sql << " MERGE INTO t_optimize_active USING "
                        " (SELECT :source as source, :dest as dest FROM dual) Pair "
                        " ON (t_optimize_active.source_se = Pair.source AND t_optimize_active.dest_se = Pair.dest) "
                        " WHEN MATCHED THEN UPDATE SET datetime = sys_extract_utc(systimestamp), fixed='on', active=:active "
                        " WHEN NOT MATCHED THEN INSERT (source_se, dest_se, active, fixed, ema, datetime) "
                        "   VALUES (Pair.source, Pair.dest, :active, 'on', 0, sys_extract_utc(systimestamp))",
                        soci::use(source, "source"), soci::use(destination, "dest"), soci::use(active, "active");
                }
            else
                {
                    sql << " MERGE INTO t_optimize_active USING "
                        " (SELECT :source as source, :dest as dest FROM dual) Pair "
                        " ON (t_optimize_active.source_se = Pair.source AND t_optimize_active.dest_se = Pair.dest) "
                        " WHEN MATCHED THEN UPDATE SET datetime = sys_extract_utc(systimestamp), fixed='off' "
                        " WHEN NOT MATCHED THEN INSERT (source_se, dest_se, active, fixed, ema, datetime) "
                        "   VALUES (Pair.source, Pair.dest, 2, 'off', 0, sys_extract_utc(systimestamp))",
                        soci::use(source, "source"), soci::use(destination, "dest");
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

int OracleAPI::getBufferOptimization()
{
    soci::session sql(*connectionPool);

    try
        {
            return getOptimizerDefaultMode(sql);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }

    return 1; //default level
}


void OracleAPI::getTransferJobStatusDetailed(std::string job_id, std::vector<boost::tuple<std::string, std::string, int, std::string, std::string> >& files)
{
    soci::session sql(*connectionPool);
    files.reserve(400);

    try
        {

            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT job_id, file_state, file_id, source_surl, dest_surl from t_file where job_id=:job_id order by file_id asc", soci::use(job_id)
                                         );


            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    std::string job_id = i->get<std::string>("JOB_ID");
                    std::string file_state = i->get<std::string>("FILE_STATE");
                    int file_id = static_cast<int>(i->get<long long>("FILE_ID"));
                    std::string source_surl = i->get<std::string>("SOURCE_SURL");
                    std::string dest_surl = i->get<std::string>("DEST_SURL");

                    boost::tuple<std::string, std::string, int, std::string, std::string> record(job_id, file_state, file_id, source_surl, dest_surl);
                    files.push_back(record);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}



void OracleAPI::updateDeletionsState(std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files)
{
    soci::session sql(*connectionPool);
    try
        {
            updateDeletionsStateInternal(sql, files);
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

void OracleAPI::updateStagingState(std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files)
{
    soci::session sql(*connectionPool);
    try
        {
            updateStagingStateInternal(sql, files);
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

void OracleAPI::updateBringOnlineToken(std::map< std::string, std::map<std::string, std::vector<int> > > const & jobs, std::string const & token)
{
    soci::session sql(*connectionPool);
    try
        {
            sql.begin();
            for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j)
                {
                    std::string const & job_id = it_j->first;
                    std::string file_ids = "(";
                    auto const & urls = it_j->second;
                    for (auto it_u = urls.begin(); it_u != urls.end(); ++it_u)
                        {
                            auto it_f = it_u->second.begin();
                            file_ids += boost::lexical_cast<std::string>(*it_f);
                            for(; it_f != it_u->second.end(); ++it_f)
                                file_ids += ", " + boost::lexical_cast<std::string>(*it_f);
                        }
                    file_ids += ")";

                    std::stringstream query;
                    query << "update t_file set bringonline_token = :token where job_id = :jobId and file_id IN " << file_ids;

                    sql << query.str(),
                        soci::use(token),
                        soci::use(job_id);
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


//NEW deletions and staging API
//need to diffenetiate delete and staging cancelations so as to avoid clash with transfers
//check job state for staging and deletions

//deletions						 //file_id / state / reason / job_id
void OracleAPI::updateDeletionsStateInternal(soci::session& sql, std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files)
{
    int file_id = 0;
    std::string state;
    std::string reason;
    std::string job_id;
    bool retry = false;
    std::vector<struct message_state> filesMsg;

    try
        {
            sql.begin();

            std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >::iterator itFind;
            for (itFind = files.begin(); itFind < files.end(); ++itFind)
                {
                    boost::tuple<int, std::string, std::string, std::string, bool >& tupleRecord = *itFind;
                    file_id = boost::get<0>(tupleRecord);
                    state = boost::get<1>(tupleRecord);
                    reason = boost::get<2>(tupleRecord);
                    job_id  = boost::get<3>(tupleRecord);
                    retry = boost::get<4>(tupleRecord);

                    if (state == "STARTED")
                        {
                            sql <<
                                " UPDATE t_dm "
                                " SET start_time = sys_extract_utc(systimestamp), dmHost=:thost, file_state='STARTED' "
                                " WHERE  "
                                "	file_id= :fileId "
                                "	AND file_state='DELETE'",
                                soci::use(hostname),
                                soci::use(file_id)
                                ;
                        }
                    else if(state == "FAILED")
                        {
                            bool shouldBeRetried = resetForRetryDelete(sql, file_id, job_id, retry);
                            if (!shouldBeRetried)
                                {
                                    sql <<
                                        " UPDATE t_dm "
                                        " SET  job_finished=sys_extract_utc(systimestamp), finish_time=sys_extract_utc(systimestamp), reason = :reason, file_state = :fileState "
                                        " WHERE "
                                        "	file_id = :fileId ",
                                        soci::use(reason),
                                        soci::use(state),
                                        soci::use(file_id)
                                        ;
                                }
                        }
                    else
                        {
                            sql <<
                                " UPDATE t_dm "
                                " SET  job_finished=sys_extract_utc(systimestamp), finish_time=sys_extract_utc(systimestamp), reason = :reason, file_state = :fileState "
                                " WHERE "
                                "	file_id = :fileId ",
                                soci::use(reason),
                                soci::use(state),
                                soci::use(file_id)
                                ;
                        }

                }

            sql.commit();

            sql.begin();

            for (itFind = files.begin(); itFind < files.end(); ++itFind)
                {

                    boost::tuple<int, std::string, std::string, std::string, bool >& tupleRecord = *itFind;
                    file_id = boost::get<0>(tupleRecord);
                    state = boost::get<1>(tupleRecord);
                    reason = boost::get<2>(tupleRecord);
                    job_id  = boost::get<3>(tupleRecord);
                    retry = boost::get<4>(tupleRecord);
                    //now update job state
                    long long numberOfFilesCanceled = 0;
                    long long numberOfFilesFinished = 0;
                    long long numberOfFilesFailed = 0;
                    long long numberOfFilesStarted = 0;
                    long long numberOfFilesDelete = 0;
                    long long totalNumOfFilesInJob= 0;
                    long long totalInTerminal = 0;

                    soci::rowset<soci::row> rsReplica = (
                                                            sql.prepare <<
                                                            " select file_state, COUNT(file_state) from t_dm where job_id=:job_id group by file_state order by null ",
                                                            soci::use(job_id)
                                                        );

                    soci::rowset<soci::row>::const_iterator iRep;
                    for (iRep = rsReplica.begin(); iRep != rsReplica.end(); ++iRep)
                        {
                            std::string file_state = iRep->get<std::string>("FILE_STATE");
                            int countStates = iRep->get<int>("COUNT(FILE_STATE)");

                            if(file_state == "FINISHED")
                                {
                                    numberOfFilesFinished = countStates;
                                }
                            else if(file_state == "FAILED")
                                {
                                    numberOfFilesFailed = countStates;
                                }
                            else if(file_state == "STARTED")
                                {
                                    numberOfFilesStarted = countStates;
                                }
                            else if(file_state == "CANCELED")
                                {
                                    numberOfFilesCanceled = countStates;
                                }
                            else if(file_state == "DELETE")
                                {
                                    numberOfFilesDelete = countStates;
                                }
                        }

                    totalNumOfFilesInJob = (numberOfFilesFinished + numberOfFilesFailed + numberOfFilesStarted + numberOfFilesCanceled + numberOfFilesDelete);
                    totalInTerminal = (numberOfFilesFinished + numberOfFilesFailed + numberOfFilesCanceled);

                    if(totalNumOfFilesInJob == numberOfFilesFinished) //all finished / job finished
                        {
                            sql << " UPDATE t_job SET "
                                " job_state = 'FINISHED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp) "
                                " WHERE job_id = :jobId ", soci::use(job_id);
                        }
                    else if (totalNumOfFilesInJob == numberOfFilesFailed) // all failed / job failed
                        {
                            sql << " UPDATE t_job SET "
                                " job_state = 'FAILED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), reason='Job failed, check files for more details' "
                                " WHERE job_id = :jobId ", soci::use(job_id);
                        }
                    else if (totalNumOfFilesInJob == numberOfFilesCanceled) // all canceled / job canceled
                        {
                            sql << " UPDATE t_job SET "
                                " job_state = 'CANCELED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), reason='Job failed, check files for more details' "
                                " WHERE job_id = :jobId ", soci::use(job_id);
                        }
                    else if (numberOfFilesStarted >= 1 &&  numberOfFilesDelete >= 1) //one file STARTED FILE/ JOB ACTIVE
                        {
                            std::string job_state;
                            sql << "SELECT job_state from t_job where job_id=:job_id", soci::use(job_id), soci::into(job_state);
                            if(job_state == "ACTIVE") //do not commit if already active
                                {
                                    //do nothings
                                }
                            else //set job to ACTIVE, >=1 in STARTED and there are DELETE
                                {
                                    sql << " UPDATE t_job SET "
                                        " job_state = 'ACTIVE' "
                                        " WHERE job_id = :jobId ", soci::use(job_id);
                                }
                        }
                    else if(totalNumOfFilesInJob == totalInTerminal && numberOfFilesCanceled == 0 && numberOfFilesFailed > 0) //FINISHEDDIRTY CASE
                        {
                            sql << " UPDATE t_job SET "
                                " job_state = 'FINISHEDDIRTY', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), reason='Job failed, check files for more details' "
                                " WHERE job_id = :jobId ", soci::use(job_id);
                        }
                    else if(totalNumOfFilesInJob == totalInTerminal && numberOfFilesCanceled >= 1) //CANCELED
                        {
                            sql << " UPDATE t_job SET "
                                " job_state = 'CANCELED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), reason='Job canceled, check files for more details' "
                                " WHERE job_id = :jobId ", soci::use(job_id);
                        }
                    else
                        {
                            //it should never go here, if it does it means the state machine is bad!
                        }
                }
            sql.commit();

            for (itFind = files.begin(); itFind < files.end(); ++itFind)
                {
                    boost::tuple<int, std::string, std::string, std::string, bool >& tupleRecord = *itFind;
                    file_id = boost::get<0>(tupleRecord);
                    job_id  = boost::get<3>(tupleRecord);

                    //send state message
                    filesMsg = getStateOfDeleteInternal(sql, job_id, file_id);
                    if(!filesMsg.empty())
                        {
                            std::vector<struct message_state>::iterator it;
                            for (it = filesMsg.begin(); it != filesMsg.end(); ++it)
                                {
                                    struct message_state tmp = (*it);
                                    constructJSONMsg(&tmp);
                                }
                        }
                    filesMsg.clear();
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

//file_id / surl / proxy
void OracleAPI::getFilesForDeletion(std::vector< boost::tuple<std::string, std::string, std::string, int, std::string, std::string> >& files)
{
    soci::session sql(*connectionPool);
    std::vector<struct message_bringonline> messages;
    std::vector< boost::tuple<int, std::string, std::string, std::string, bool> > filesState;

    try
        {
            int exitCode = runConsumerDeletions(messages);
            if(exitCode != 0)
                {
                    char buffer[128]= {0};
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not get the status messages for staging:" << strerror_r(errno, buffer, sizeof(buffer)) << commit;
                }

            if(!messages.empty())
                {
                    std::vector<struct message_bringonline>::iterator iterUpdater;
                    for (iterUpdater = messages.begin(); iterUpdater != messages.end(); ++iterUpdater)
                        {
                            if (iterUpdater->msg_errno == 0)
                                {
                                    //now restore messages : //file_id / state / reason / job_id / retry
                                    filesState.push_back(boost::make_tuple((*iterUpdater).file_id,
                                                                           std::string((*iterUpdater).transfer_status),
                                                                           std::string((*iterUpdater).transfer_message),
                                                                           std::string((*iterUpdater).job_id),
                                                                           0));
                                }
                        }
                }

            soci::rowset<soci::row> rs2 = (sql.prepare <<
                                           " SELECT DISTINCT vo_name, source_se "
                                           " FROM t_dm "
                                           " WHERE "
                                           "      file_state = 'DELETE' AND "
                                           "      (hashed_id >= :hStart AND hashed_id <= :hEnd)  ",
                                           soci::use(hashSegment.start), soci::use(hashSegment.end)
                                          );

            for (soci::rowset<soci::row>::const_iterator i2 = rs2.begin(); i2 != rs2.end(); ++i2)
                {
                    soci::row const& r = *i2;
                    std::string source_se = r.get<std::string>("SOURCE_SE","");
                    std::string vo_name = r.get<std::string>("VO_NAME","");

                    int maxValueConfig = 0;
                    int currentDeleteActive = 0;
                    int limit = 0;
                    soci::indicator isNull = soci::i_ok;

                    //check max configured
                    sql << 	"SELECT concurrent_ops from t_stage_req "
                        "WHERE vo_name=:vo_name and host = :endpoint and operation='delete' and concurrent_ops is NOT NULL ",
                        soci::use(vo_name), soci::use(source_se), soci::into(maxValueConfig, isNull);
                    if (isNull == soci::i_null || maxValueConfig <= 0) {
                        sql <<  "SELECT concurrent_ops from t_stage_req "
                            "WHERE vo_name=:vo_name and host = '*' and operation='delete' and concurrent_ops is NOT NULL ",
                            soci::use(vo_name), soci::into(maxValueConfig, isNull);
                    }

                    //check current staging
                    sql << 	"SELECT count(*) from t_dm "
                        "WHERE vo_name=:vo_name and source_se = :endpoint and file_state='STARTED' and job_finished is NULL ",
                        soci::use(vo_name), soci::use(source_se), soci::into(currentDeleteActive);


                    if(isNull != soci::i_null && maxValueConfig > 0)
                        {
                            if(currentDeleteActive > 0)
                                {
                                    limit = maxValueConfig - currentDeleteActive;
                                }
                            else
                                {
                                    limit = maxValueConfig;
                                }
                        }
                    else
                        {
                            if(currentDeleteActive > 0)
                                {
                                    limit = 2000 - currentDeleteActive;
                                }
                            else
                                {
                                    limit = 2000;
                                }
                        }

                    if(limit == 0)
                        continue;

                    soci::rowset<soci::row> rs = (
                                                     sql.prepare <<
                                                     " SELECT distinct f.source_se, j.user_dn "
                                                     " FROM t_dm f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                                     " WHERE "
                                                     "	f.file_state = 'DELETE' "
                                                     "	AND f.start_time IS NULL and j.job_finished is null "
                                                     "  AND (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd)"
                                                     "  AND f.vo_name = :vo_name AND f.source_se=:source_se ",
                                                     soci::use(hashSegment.start), soci::use(hashSegment.end),
                                                     soci::use(vo_name), soci::use(source_se)
                                                 );

                    for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                        {
                            soci::row const& row = *i;

                            source_se = row.get<std::string>("SOURCE_SE");
                            std::string user_dn = row.get<std::string>("USER_DN");

                            soci::rowset<soci::row> rs3 = (
                                                              sql.prepare <<
                                                              " SELECT * FROM (SELECT f.source_surl, f.job_id, f.file_id, "
                                                              " j.user_dn, j.cred_id "
                                                              " FROM t_dm f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                                              " WHERE  "
                                                              "	f.start_time is NULL "
                                                              "	AND f.file_state = 'DELETE' "
                                                              " AND (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd)"
                                                              "	AND f.source_se = :source_se  "
                                                              " AND j.user_dn = :user_dn "
                                                              " AND j.vo_name = :vo_name "
                                                              "	AND j.job_finished is null  ORDER BY SYS_EXTRACT_UTC(j.submit_time)) WHERE ROWNUM <= :limit ",
                                                              soci::use(hashSegment.start), soci::use(hashSegment.end),
                                                              soci::use(source_se),
                                                              soci::use(user_dn),
                                                              soci::use(vo_name),
                                                              soci::use(limit)
                                                          );

                            std::string initState = "STARTED";
                            std::string reason;

                            for (soci::rowset<soci::row>::const_iterator i3 = rs3.begin(); i3 != rs3.end(); ++i3)
                                {
                                    soci::row const& row = *i3;
                                    std::string source_url = row.get<std::string>("SOURCE_SURL");
                                    std::string job_id = row.get<std::string>("JOB_ID");
                                    int file_id = static_cast<int>(row.get<long long>("FILE_ID"));
                                    user_dn = row.get<std::string>("USER_DN");
                                    std::string cred_id = row.get<std::string>("CRED_ID");

                                    boost::tuple<std::string, std::string, std::string, int, std::string, std::string> record(vo_name, source_url, job_id, file_id, user_dn, cred_id);
                                    files.push_back(record);

                                    boost::tuple<int, std::string, std::string, std::string, bool> recordState(file_id, initState, reason, job_id, false);
                                    filesState.push_back(recordState);
                                }
                        }
                }
            //now update the initial state
            if(!filesState.empty())
                {
                    std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >::iterator itFind;
                    for (itFind = filesState.begin(); itFind < filesState.end(); ++itFind)
                        {
                            boost::tuple<int, std::string, std::string, std::string, bool>& tupleRecord = *itFind;
                            int file_id = boost::get<0>(tupleRecord);
                            std::string job_id = boost::get<3>(tupleRecord);

                            //send state message
                            std::vector<struct message_state> filesMsg;
                            filesMsg = getStateOfDeleteInternal(sql, job_id, file_id);
                            if(!filesMsg.empty())
                                {
                                    std::vector<struct message_state>::iterator it;
                                    for (it = filesMsg.begin(); it != filesMsg.end(); ++it)
                                        {
                                            struct message_state tmp = (*it);
                                            constructJSONMsg(&tmp);
                                        }
                                }
                            filesMsg.clear();
                        }

                    try
                        {
                            updateDeletionsStateInternal(sql, filesState);
                            filesState.clear();
                        }
                    catch(...)
                        {
                            //save state and restore afterwards
                            if(!filesState.empty())
                                {
                                    std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >::iterator itFind;
                                    struct message_bringonline msg;
                                    for (itFind = filesState.begin(); itFind < filesState.end(); ++itFind)
                                        {
                                            boost::tuple<int, std::string, std::string, std::string, bool>& tupleRecord = *itFind;
                                            int file_id = boost::get<0>(tupleRecord);
                                            std::string transfer_status = boost::get<1>(tupleRecord);
                                            std::string transfer_message = boost::get<2>(tupleRecord);
                                            std::string job_id = boost::get<3>(tupleRecord);

                                            msg.file_id = file_id;
                                            strncpy(msg.job_id, job_id.c_str(), sizeof(msg.job_id));
                                            msg.job_id[sizeof(msg.job_id) -1] = '\0';
                                            strncpy(msg.transfer_status, transfer_status.c_str(), sizeof(msg.transfer_status));
                                            msg.transfer_status[sizeof(msg.transfer_status) -1] = '\0';
                                            strncpy(msg.transfer_message, transfer_message.c_str(), sizeof(msg.transfer_message));
                                            msg.transfer_message[sizeof(msg.transfer_message) -1] = '\0';

                                            //store the states into fs to be restored in the next run of this function
                                            runProducerDeletions(msg);
                                        }
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
}


void OracleAPI::revertDeletionToStarted()
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql <<
                " UPDATE t_dm SET file_state = 'DELETE', start_time = NULL "
                " WHERE file_state = 'STARTED' "
                "   AND (hashed_id >= :hStart AND hashed_id <= :hEnd)",
                soci::use(hashSegment.start), soci::use(hashSegment.end)
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

//STAGING
//need messaging, both for canceling and state transitions


//WORKHORSE
//alter table t_job add index t_staging_index(vo_name, source_se, dest_se, user_dn);
//f.source_surl, f.job_id, f.file_id, j.copy_pin_lifetime, j.bring_online  , j.user_dn, j.cred_id, j.source_space_token
void OracleAPI::getFilesForStaging(std::vector< boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string > >& files)
{
    soci::session sql(*connectionPool);
    std::vector< boost::tuple<int, std::string, std::string, std::string, bool> > filesState;
    std::vector<struct message_bringonline> messages;

    try
        {
            int exitCode = runConsumerStaging(messages);
            if(exitCode != 0)
                {
                    char buffer[128]= {0};
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not get the status messages for staging:" << strerror_r(errno, buffer, sizeof(buffer)) << commit;
                }

            if(!messages.empty())
                {
                    std::vector<struct message_bringonline>::iterator iterUpdater;
                    for (iterUpdater = messages.begin(); iterUpdater != messages.end(); ++iterUpdater)
                        {
                            if (iterUpdater->msg_errno == 0)
                                {
                                    //now restore messages : //file_id / state / reason / job_id / retry
                                    filesState.push_back(boost::make_tuple((*iterUpdater).file_id,
                                                                           std::string((*iterUpdater).transfer_status),
                                                                           std::string((*iterUpdater).transfer_message),
                                                                           std::string((*iterUpdater).job_id),
                                                                           0));
                                }
                        }
                }

            soci::rowset<soci::row> rs2 = (sql.prepare <<
                                           " SELECT DISTINCT vo_name, source_se "
                                           " FROM t_file "
                                           " WHERE "
                                           "      file_state = 'STAGING' AND "
                                           "      (hashed_id >= :hStart AND hashed_id <= :hEnd)  ",
                                           soci::use(hashSegment.start), soci::use(hashSegment.end)
                                          );

            for (soci::rowset<soci::row>::const_iterator i2 = rs2.begin(); i2 != rs2.end(); ++i2)
                {
                    soci::row const& r = *i2;
                    std::string source_se = r.get<std::string>("SOURCE_SE","");
                    std::string vo_name = r.get<std::string>("VO_NAME","");

                    int maxValueConfig = 0;
                    int currentStagingActive = 0;
                    int limit = 0;

                    //check max configured
                    sql << 	"SELECT concurrent_ops from t_stage_req "
                        "WHERE vo_name=:vo_name and host = :endpoint and operation='staging' and concurrent_ops is NOT NULL ",
                        soci::use(vo_name), soci::use(source_se), soci::into(maxValueConfig);
                    if (maxValueConfig <= 0) {
                        sql <<  "SELECT concurrent_ops from t_stage_req "
                            "WHERE vo_name=:vo_name and host = '*' and operation='staging' and concurrent_ops is NOT NULL ",
                            soci::use(vo_name), soci::into(maxValueConfig);
                    }

                    if(maxValueConfig > 0)
                        {
                            //check current staging
                            sql << 	"SELECT count(*) from t_file "
                                "WHERE vo_name=:vo_name and source_se = :endpoint and file_state='STARTED' and job_finished is NULL ",
                                soci::use(vo_name), soci::use(source_se), soci::into(currentStagingActive);

                            if(currentStagingActive > 0)
                                {
                                    limit = maxValueConfig - currentStagingActive;
                                }
                            else
                                {
                                    limit = maxValueConfig;
                                }

                            if(limit <= 0)
                                continue;
                        }
                    else
                        {
                            limit = MAX_STAGING_BULK_SIZE; // Use a sensible default
                        }

                    //now check for max concurrent active requests, must no exceed 200
                    long long countActiveRequests = 0;
                    sql << " select count(distinct bringonline_token) from t_file where "
                        " vo_name=:vo_name and file_state='STARTED' and source_se=:source_se and bringonline_token is not NULL ",
                        soci::use(vo_name), soci::use(source_se), soci::into(countActiveRequests);

                    if(countActiveRequests > 200)
                        continue;

                    //now make sure there are enough files to put in a single request
                    int countQueuedFiles = 0;
                    sql << " SELECT count(*) from t_file where vo_name=:vo_name and source_se=:source_se and file_state='STAGING' ",
                        soci::use(vo_name), soci::use(source_se), soci::into(countQueuedFiles);


                    if(countQueuedFiles < 2000)
                        {
                            std::map<std::string, int>::iterator itQueue = queuedStagingFiles.find(source_se);
                            if(itQueue != queuedStagingFiles.end())
                                {
                                    int counter = itQueue->second;

                                    if(counter < 30)
                                        {
                                            queuedStagingFiles[source_se] = counter + 1;
                                            continue;
                                        }
                                    else
                                        {
                                            queuedStagingFiles.erase (itQueue);
                                        }
                                }
                            else
                                {
                                    queuedStagingFiles[source_se] = 1;
                                    continue;
                                }
                        }

                    soci::rowset<soci::row> rs = (
                                                     sql.prepare <<
                                                     " SELECT distinct f.source_se, j.user_dn "
                                                     " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                                     " WHERE "
                                                     "	(j.BRING_ONLINE >= 0 OR j.COPY_PIN_LIFETIME >= 0) "
                                                     "	AND f.file_state = 'STAGING' "
                                                     "	AND f.start_time IS NULL and j.job_finished is null "
                                                     "  AND (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd)"
                                                     "  AND f.vo_name = :vo_name AND f.source_se=:source_se ",
                                                     soci::use(hashSegment.start), soci::use(hashSegment.end),
                                                     soci::use(vo_name), soci::use(source_se)
                                                 );

                    for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                        {
                            soci::row const& row = *i;

                            source_se = row.get<std::string>("SOURCE_SE");
                            std::string user_dn = row.get<std::string>("USER_DN");

                            soci::rowset<soci::row> rs3 = (
                                                              sql.prepare <<
                                                              " SELECT * FROM (SELECT f.source_surl, f.job_id, f.file_id, j.copy_pin_lifetime, j.bring_online, "
                                                              " j.user_dn, j.cred_id, j.source_space_token"
                                                              " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                                              " WHERE  "
                                                              " (j.BRING_ONLINE >= 0 OR j.COPY_PIN_LIFETIME >= 0) "
                                                              "	AND f.start_time IS NULL "
                                                              "	AND f.file_state = 'STAGING' "
                                                              " AND (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd)"
                                                              "	AND f.source_se = :source_se  "
                                                              " AND j.user_dn = :user_dn "
                                                              " AND j.vo_name = :vo_name "
                                                              "	AND j.job_finished is null "
                                                              "   AND NOT EXISTS ( "
                                                              "       SELECT "
                                                              "           1 FROM t_file f1 "
                                                              "       WHERE "
                                                              "           f1.source_surl = f.source_surl AND f1.file_state='STARTED' "
                                                              "           AND f1.vo_name = f.vo_name AND f1.job_finished IS NULL "
                                                              "           AND f1.source_se = f.source_se"
                                                              "   ) "
                                                              " ORDER BY SYS_EXTRACT_UTC(j.submit_time) )  WHERE ROWNUM <= :limit ",
                                                              soci::use(hashSegment.start), soci::use(hashSegment.end),
                                                              soci::use(source_se),
                                                              soci::use(user_dn),
                                                              soci::use(vo_name),
                                                              soci::use(limit)
                                                          );

                            std::string initState = "STARTED";
                            std::string reason;

                            for (soci::rowset<soci::row>::const_iterator i3 = rs3.begin(); i3 != rs3.end(); ++i3)
                                {
                                    soci::row const& row = *i3;
                                    std::string source_url = row.get<std::string>("SOURCE_SURL");
                                    std::string job_id = row.get<std::string>("JOB_ID");
                                    int file_id = static_cast<int>(row.get<long long>("FILE_ID"));
                                    int copy_pin_lifetime = static_cast<int>(row.get<double>("COPY_PIN_LIFETIME", 0));
                                    int bring_online = static_cast<int>(row.get<double>("BRING_ONLINE",0));

                                    if(copy_pin_lifetime > 0 && bring_online <= 0)
                                        bring_online = 28800;
                                    else if (bring_online > 0 && copy_pin_lifetime <= 0)
                                        copy_pin_lifetime = 28800;

                                    user_dn = row.get<std::string>("USER_DN");
                                    std::string cred_id = row.get<std::string>("CRED_ID");
                                    std::string source_space_token = row.get<std::string>("SOURCE_SPACE_TOKEN","");

                                    boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string > record(vo_name, source_url,job_id, file_id, copy_pin_lifetime, bring_online, user_dn, cred_id , source_space_token);
                                    files.push_back(record);

                                    boost::tuple<int, std::string, std::string, std::string, bool> recordState(file_id, initState, reason, job_id, false);
                                    if(!job_id.empty() && file_id > 0)
                                        {
                                            filesState.push_back(recordState);
                                        }
                                }
                        }
                }

            //now update the initial state
            if(!filesState.empty())
                {
                    std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >::iterator itFind;
                    for (itFind = filesState.begin(); itFind < filesState.end(); ++itFind)
                        {
                            try
                                {
                                    boost::tuple<int, std::string, std::string, std::string, bool>& tupleRecord = *itFind;
                                    int file_id = boost::get<0>(tupleRecord);
                                    std::string job_id = boost::get<3>(tupleRecord);

                                    //send state message
                                    std::vector<struct message_state> filesMsg;
                                    if(!job_id.empty() && file_id > 0)
                                        {
                                            filesMsg = getStateOfTransferInternal(sql, job_id, file_id);
                                            if(!filesMsg.empty())
                                                {
                                                    std::vector<struct message_state>::iterator it;
                                                    for (it = filesMsg.begin(); it != filesMsg.end(); ++it)
                                                        {
                                                            struct message_state tmp = (*it);
                                                            constructJSONMsg(&tmp);
                                                        }
                                                }
                                            filesMsg.clear();
                                        }
                                }
                            catch(...)
                                {
                                    //do not let the process die due to messaging
                                }
                        }

                    try
                        {
                            updateStagingStateInternal(sql, filesState);
                            filesState.clear();
                        }
                    catch(...)
                        {
                            //save state and restore afterwards
                            if(!filesState.empty())
                                {
                                    std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >::iterator itFind;
                                    struct message_bringonline msg;
                                    for (itFind = filesState.begin(); itFind < filesState.end(); ++itFind)
                                        {
                                            boost::tuple<int, std::string, std::string, std::string, bool>& tupleRecord = *itFind;
                                            int file_id = boost::get<0>(tupleRecord);
                                            std::string transfer_status = boost::get<1>(tupleRecord);
                                            std::string transfer_message = boost::get<2>(tupleRecord);
                                            std::string job_id = boost::get<3>(tupleRecord);

                                            msg.file_id = file_id;
                                            strncpy(msg.job_id, job_id.c_str(), sizeof(msg.job_id));
                                            msg.job_id[sizeof(msg.job_id) -1] = '\0';
                                            strncpy(msg.transfer_status, transfer_status.c_str(), sizeof(msg.transfer_status));
                                            msg.transfer_status[sizeof(msg.transfer_status) -1] = '\0';
                                            strncpy(msg.transfer_message, transfer_message.c_str(), sizeof(msg.transfer_message));
                                            msg.transfer_message[sizeof(msg.transfer_message) -1] = '\0';

                                            //store the states into fs to be restored in the next run of this function
                                            runProducerStaging(msg);
                                        }
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
}

void OracleAPI::getAlreadyStartedStaging(std::vector< boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string, std::string> >& files)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();
            sql <<
                " UPDATE t_file "
                " SET start_time = NULL, staging_start=NULL, transferhost=NULL, file_state='STAGING' "
                " WHERE  "
                "   file_state='STARTED'"
                "   AND (bringonline_token = '' OR bringonline_token IS NULL)"
                "   AND start_time IS NOT NULL "
                "   AND staging_start IS NOT NULL "
                "   AND (hashed_id >= :hStart AND hashed_id <= :hEnd)",
                soci::use(hashSegment.start), soci::use(hashSegment.end)
                ;
            sql.commit();

            soci::rowset<soci::row> rs3 =
                (
                    sql.prepare <<
                    " SELECT f.vo_name, f.source_surl, f.job_id, f.file_id, j.copy_pin_lifetime, j.bring_online, "
                    " j.user_dn, j.cred_id, j.source_space_token, f.bringonline_token "
                    " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                    " WHERE  "
                    " (j.BRING_ONLINE >= 0 OR j.COPY_PIN_LIFETIME >= 0) "
                    " AND f.start_time IS NOT NULL "
                    " AND f.file_state = 'STARTED' "
                    " AND (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd)"
                    " AND j.job_finished is null ",
                    soci::use(hashSegment.start), soci::use(hashSegment.end)
                );

            for (soci::rowset<soci::row>::const_iterator i3 = rs3.begin(); i3 != rs3.end(); ++i3)
                {
                    soci::row const& row = *i3;
                    std::string vo_name = row.get<std::string>("VO_NAME");
                    std::string source_url = row.get<std::string>("SOURCE_SURL");
                    std::string job_id = row.get<std::string>("JOB_ID");
                    int file_id = static_cast<int>(row.get<long long>("FILE_ID"));
                    int copy_pin_lifetime = static_cast<int>(row.get<double>("COPY_PIN_LIFETIME"),0);
                    int bring_online = static_cast<int>(row.get<double>("BRING_ONLINE"),0);

                    if(copy_pin_lifetime > 0 && bring_online <= 0)
                        bring_online = 28800;
                    else if (bring_online > 0 && copy_pin_lifetime <= 0)
                        copy_pin_lifetime = 28800;

                    std::string user_dn = row.get<std::string>("USER_DN");
                    std::string cred_id = row.get<std::string>("CRED_ID");
                    std::string source_space_token = row.get<std::string>("SOURCE_SPACE_TOKEN","");

                    std::string bringonline_token;
                    soci::indicator isNull = row.get_indicator("BRINGONLINE_TOKEN");
                    if (isNull == soci::i_ok) bringonline_token = row.get<std::string>("BRINGONLINE_TOKEN");

                    boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string, std::string > record(vo_name, source_url,job_id, file_id, copy_pin_lifetime, bring_online, user_dn, cred_id , source_space_token, bringonline_token);
                    files.push_back(record);
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


//file_id / state / reason / job_id
void OracleAPI::updateStagingStateInternal(soci::session& sql, std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >& files)
{
    int file_id = 0;
    std::string state;
    std::string reason;
    std::string job_id;
    bool retry = false;
    std::vector<struct message_state> filesMsg;

    try
        {

            sql.begin();

            std::vector< boost::tuple<int, std::string, std::string, std::string, bool> >::iterator itFind;
            for (itFind = files.begin(); itFind < files.end(); ++itFind)
                {
                    boost::tuple<int, std::string, std::string, std::string, bool>& tupleRecord = *itFind;
                    file_id = boost::get<0>(tupleRecord);
                    state = boost::get<1>(tupleRecord);
                    reason = boost::get<2>(tupleRecord);
                    job_id = boost::get<3>(tupleRecord);
                    retry = boost::get<4>(tupleRecord);

                    if (state == "STARTED")
                        {
                            sql <<
                                " UPDATE t_file "
                                " SET start_time = sys_extract_utc(systimestamp), staging_start=sys_extract_utc(systimestamp), agent_dn=:thost, transferhost=:thost, file_state='STARTED' "
                                " WHERE  "
                                "	file_id= :fileId "
                                "	AND file_state='STAGING'",
                                soci::use(hostname),
                                soci::use(hostname),
                                soci::use(file_id)
                                ;
                        }
                    else if(state == "FAILED")
                        {
                            bool shouldBeRetried = retry;

                            if(retry)
                                {
                                    int times = 0;
                                    shouldBeRetried = resetForRetryStaging(sql, file_id, job_id, retry, times);
                                    if(shouldBeRetried)
                                        {
                                            if(times > 0)
                                                {
                                                    sql << "INSERT /*+ IGNORE_ROW_ON_DUPKEY_INDEX (t_file_retry_errors, t_file_retry_errors_pk) */  INTO t_file_retry_errors "
                                                        "    (file_id, attempt, datetime, reason) "
                                                        "VALUES (:fileId, :attempt, sys_extract_utc(systimestamp), :reason)",
                                                        soci::use(file_id), soci::use(times), soci::use(reason);
                                                }

                                            continue;
                                        }
                                }

                            if (!retry || !shouldBeRetried)
                                {
                                    sql <<
                                        " UPDATE t_file "
                                        " SET job_finished=sys_extract_utc(systimestamp), finish_time=sys_extract_utc(systimestamp), staging_finished=sys_extract_utc(systimestamp), reason = :reason, file_state = :fileState "
                                        " WHERE "
                                        "	file_id = :fileId "
                                        "	AND file_state in ('STAGING','STARTED') ",
                                        soci::use(reason),
                                        soci::use(state),
                                        soci::use(file_id)
                                        ;
                                }
                        }
                    else
                        {
                            std::string dbState;
                            std::string dbReason;
                            int stage_in_only = 0;

                            sql << "select count(*) from t_file where file_id=:file_id and source_surl=dest_surl",
                                soci::use(file_id),
                                soci::into(stage_in_only);

                            if(stage_in_only == 0)  //stage-in and transfer
                                {
                                    dbState = state == "FINISHED" ? "SUBMITTED" : state;
                                    dbReason = state == "FINISHED" ? std::string() : reason;
                                }
                            else //stage-in only
                                {
                                    dbState = state == "FINISHED" ? "FINISHED" : state;
                                    dbReason = state == "FINISHED" ? std::string() : reason;
                                }

                            if(dbState == "SUBMITTED")
                                {
                                    unsigned hashedId = getHashedId();

                                    sql <<
                                        " UPDATE t_file "
                                        " SET retry=0, hashed_id = :hashed_id, staging_finished=sys_extract_utc(systimestamp), job_finished=NULL, finish_time=NULL, start_time=NULL, transferhost=NULL, reason = '', file_state = :fileState "
                                        " WHERE "
                                        "	file_id = :fileId "
                                        "   AND file_state in ('STAGING','STARTED')",
                                        soci::use(hashedId),
                                        soci::use(dbState),
                                        soci::use(file_id)
                                        ;
                                }
                            else if(dbState == "FINISHED")
                                {
                                    sql <<
                                        " UPDATE t_file "
                                        " SET job_finished=sys_extract_utc(systimestamp), staging_finished=sys_extract_utc(systimestamp), finish_time=sys_extract_utc(systimestamp), reason = :reason, file_state = :fileState "
                                        " WHERE "
                                        "	file_id = :fileId "
                                        "	AND file_state in ('STAGING','STARTED')",
                                        soci::use(dbReason),
                                        soci::use(dbState),
                                        soci::use(file_id)
                                        ;
                                }
                            else
                                {
                                    sql <<
                                        " UPDATE t_file "
                                        " SET job_finished=sys_extract_utc(systimestamp), staging_finished=sys_extract_utc(systimestamp), finish_time=sys_extract_utc(systimestamp), reason = :reason, file_state = :fileState "
                                        " WHERE "
                                        "	file_id = :fileId "
                                        "	AND file_state in ('STAGING','STARTED')",
                                        soci::use(dbReason),
                                        soci::use(dbState),
                                        soci::use(file_id)
                                        ;
                                }
                        }
                }
            sql.commit();

            for (itFind = files.begin(); itFind < files.end(); ++itFind)
                {
                    boost::tuple<int, std::string, std::string, std::string, bool>& tupleRecord = *itFind;
                    file_id = boost::get<0>(tupleRecord);
                    state = boost::get<1>(tupleRecord);
                    reason = boost::get<2>(tupleRecord);
                    job_id = boost::get<3>(tupleRecord);
                    retry = boost::get<4>(tupleRecord);

                    if(state == "SUBMITTED")
                        updateJobTransferStatusInternal(sql, job_id, "ACTIVE",0);
                    else
                        updateJobTransferStatusInternal(sql, job_id, state,0);

                    //send state message
                    filesMsg = getStateOfTransferInternal(sql, job_id, file_id);
                    if(!filesMsg.empty())
                        {
                            std::vector<struct message_state>::iterator it;
                            for (it = filesMsg.begin(); it != filesMsg.end(); ++it)
                                {
                                    struct message_state tmp = (*it);
                                    constructJSONMsg(&tmp);
                                }
                        }
                    filesMsg.clear();

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




//file_id / surl / token
void OracleAPI::getStagingFilesForCanceling(std::set< std::pair<std::string, std::string> >& files)
{
    soci::session sql(*connectionPool);
    int file_id = 0;
    std::string source_surl;
    std::string token;
    std::string job_id;

    try
        {
            soci::rowset<soci::row> rs = (sql.prepare << " SELECT job_id, file_id, source_surl, bringonline_token from t_file WHERE "
                                          "  file_state='CANCELED' and job_finished is NULL "
                                          "  AND (hashed_id >= :hStart AND hashed_id <= :hEnd) AND staging_start is NOT NULL ",
                                          soci::use(hashSegment.start), soci::use(hashSegment.end));

            soci::statement stmt1 = (sql.prepare << "UPDATE t_file SET  job_finished = sys_extract_utc(systimestamp) "
                                     "WHERE file_id = :file_id ", soci::use(file_id, "file_id"));

            // Cancel staging files
            sql.begin();
            for (soci::rowset<soci::row>::const_iterator i2 = rs.begin(); i2 != rs.end(); ++i2)
                {
                    soci::row const& row = *i2;
                    job_id  = row.get<std::string>("JOB_ID", "");
                    file_id = static_cast<int>(row.get<long long>("FILE_ID",0));
                    source_surl = row.get<std::string>("SOURCE_SURL","");
                    token = row.get<std::string>("BRINGONLINE_TOKEN","");
                    boost::tuple<std::string, int, std::string, std::string> record(job_id, file_id, source_surl, token);
                    files.insert({job_id, source_surl});

                    stmt1.execute(true);
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


bool OracleAPI::resetForRetryStaging(soci::session& sql, int file_id, const std::string & job_id, bool retry, int& times)
{

    bool willBeRetried = false;

    if(retry)
        {

            int nRetries = 0;
            soci::indicator isNull = soci::i_ok;
            std::string vo_name;
            int retry_delay = 0;

            try
                {
                    sql <<
                        " SELECT retry, vo_name, retry_delay "
                        " FROM t_job "
                        " WHERE job_id = :job_id ",
                        soci::use(job_id),
                        soci::into(nRetries, isNull),
                        soci::into(vo_name),
                        soci::into(retry_delay)
                        ;


                    if (isNull == soci::i_null || nRetries <= 0)
                        {
                            sql <<
                                " SELECT retry "
                                " FROM t_server_config WHERE vo_name=:vo_name ",
                                soci::use(vo_name), soci::into(nRetries, isNull)
                                ;
                        }
                    else if (isNull != soci::i_null && nRetries <= 0)
                        {
                            nRetries = 0;
                        }

                    int nRetriesTimes = 0;
                    soci::indicator isNull2 = soci::i_ok;

                    sql << "SELECT retry FROM t_file WHERE file_id = :file_id AND job_id = :jobId ",
                        soci::use(file_id), soci::use(job_id), soci::into(nRetriesTimes, isNull2);


                    if(isNull != soci::i_null &&  isNull2 != soci::i_null  && nRetries > 0 && nRetriesTimes <= nRetries-1 )
                        {
                            //expressed in secs, default delay
                            const int default_retry_delay = 120;


                            if (retry_delay > 0)
                                {
                                    // update
                                    time_t now = getUTC(retry_delay);
                                    struct tm tTime;
                                    gmtime_r(&now, &tTime);

                                    sql.begin();

                                    sql << "UPDATE t_file SET retry_timestamp=:1, retry = :retry, file_state = 'STAGING', staging_start=NULL, start_time=NULL, transferHost=NULL, t_log_file=NULL,"
                                        " t_log_file_debug=NULL, throughput = 0, current_failures = 1 "
                                        " WHERE  file_id = :fileId AND  job_id = :jobId AND file_state NOT IN ('FINISHED','STAGING','FAILED','CANCELED')",
                                        soci::use(tTime), soci::use(nRetriesTimes+1), soci::use(file_id), soci::use(job_id);

                                    times = nRetriesTimes+1;

                                    willBeRetried = true;

                                    sql.commit();
                                }
                            else
                                {
                                    // update
                                    time_t now = getUTC(default_retry_delay);
                                    struct tm tTime;
                                    gmtime_r(&now, &tTime);

                                    sql.begin();

                                    sql << "UPDATE t_file SET retry_timestamp=:1, retry = :retry, file_state = 'STAGING', staging_start=NULL, start_time=NULL, transferHost=NULL, "
                                        " t_log_file=NULL, t_log_file_debug=NULL, throughput = 0,  current_failures = 1 "
                                        " WHERE  file_id = :fileId AND  job_id = :jobId AND file_state NOT IN ('FINISHED','STAGING','FAILED','CANCELED')",
                                        soci::use(tTime), soci::use(nRetriesTimes+1), soci::use(file_id), soci::use(job_id);

                                    times = nRetriesTimes+1;

                                    willBeRetried = true;

                                    sql.commit();
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

    return willBeRetried;
}


bool OracleAPI::resetForRetryDelete(soci::session& sql, int file_id, const std::string & job_id, bool retry)
{

    bool willBeRetried = false;
    if(retry)
        {

            int nRetries = 0;
            int retry_delay = 0;
            soci::indicator isNull = soci::i_ok;
            std::string vo_name;

            try
                {
                    sql <<
                        " SELECT retry, vo_name, retry_delay "
                        " FROM t_job "
                        " WHERE job_id = :job_id ",
                        soci::use(job_id),
                        soci::into(nRetries, isNull),
                        soci::into(vo_name),
                        soci::into(retry_delay)
                        ;

                    if (isNull == soci::i_null || nRetries <= 0)
                        {
                            sql <<
                                " SELECT retry "
                                " FROM t_server_config where vo_name=:vo_name ",
                                soci::use(vo_name), soci::into(nRetries, isNull)
                                ;
                        }
                    else if (isNull != soci::i_null && nRetries <= 0)
                        {
                            nRetries = 0;
                        }

                    int nRetriesTimes = 0;
                    soci::indicator isNull2 = soci::i_ok;

                    sql << "SELECT retry FROM t_dm WHERE file_id = :file_id AND job_id = :jobId ",
                        soci::use(file_id), soci::use(job_id), soci::into(nRetriesTimes, isNull2);


                    if(isNull != soci::i_null &&  isNull2 != soci::i_null  && nRetries > 0 && nRetriesTimes <= nRetries-1 )
                        {
                            //expressed in secs, default delay
                            const int default_retry_delay = 120;

                            if (retry_delay > 0)
                                {
                                    // update
                                    time_t now = getUTC(retry_delay);
                                    struct tm tTime;
                                    gmtime_r(&now, &tTime);

                                    sql.begin();

                                    sql << "UPDATE t_dm SET retry_timestamp=:1, retry = :retry, file_state = 'DELETE', start_time=NULL, dmHost=NULL "
                                        " WHERE  file_id = :fileId AND  job_id = :jobId AND file_state NOT IN ('FINISHED','DELETE','FAILED','CANCELED')",
                                        soci::use(tTime), soci::use(nRetries+1), soci::use(file_id), soci::use(job_id);

                                    willBeRetried = true;

                                    sql.commit();
                                }
                            else
                                {
                                    // update
                                    time_t now = getUTC(default_retry_delay);
                                    struct tm tTime;
                                    gmtime_r(&now, &tTime);

                                    sql.begin();

                                    sql << "UPDATE t_dm SET retry_timestamp=:1, retry = :retry, file_state = 'DELETE', start_time=NULL, dmHost=NULL  "
                                        " WHERE  file_id = :fileId AND  job_id = :jobId AND file_state NOT IN ('FINISHED','SUBMITTED','FAILED','CANCELED')",
                                        soci::use(tTime), soci::use(nRetries+1), soci::use(file_id), soci::use(job_id);

                                    willBeRetried = true;

                                    sql.commit();
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

    return willBeRetried;
}


bool OracleAPI::getOauthCredentials(const std::string& user_dn,
                                    const std::string& vo, const std::string& cloud_name,
                                    OAuth& oauth)
{
    soci::session sql(*connectionPool);

    try
        {
            sql << "SELECT app_key, app_secret, access_token, access_token_secret "
                "FROM t_cloudStorage cs, t_cloudStorageUser cu "
                "WHERE (cu.user_dn=:user_dn OR cu.vo_name=:vo) AND cs.cloudStorage_name=:cs_name AND cs.cloudStorage_name = cu.cloudStorage_name",
                soci::use(user_dn), soci::use(vo), soci::use(cloud_name), soci::into(oauth);
            if (!sql.got_data())
                return false;
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

void OracleAPI::setCloudStorageCredential(std::string const & dn, std::string const & vo, std::string const & storage, std::string const & accessKey, std::string const & secretKey)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            int count = 0;

            sql <<
                " SELECT count(*) "
                " FROM t_cloudStorageUser "
                " WHERE user_dn = :dn and vo_name = :vo and cloudStorage_name = :storage ",
                soci::use(dn),
                soci::use(vo),
                soci::use(storage),
                soci::into(count)
                ;

            if (count)
                {
                    sql <<
                        " UPDATE t_cloudStorageUser "
                        " SET access_token = :accessKey, access_token_secret = :secretKey "
                        " WHERE user_dn = :dn and vo_name = :vo and cloudStorage_name = :storage ",
                        soci::use(accessKey),
                        soci::use(secretKey),
                        soci::use(dn),
                        soci::use(vo),
                        soci::use(storage)
                        ;
                }
            else
                {
                    // first make sure that the corresponding object in t_cloudStorage exists
                    sql <<
                        " INSERT INTO t_cloudStorage (cloudStorage_name) "
                        " SELECT :storage FROM dual "
                        " WHERE NOT EXISTS ( "
                        "   SELECT NULL FROM t_cloudStorage WHERE cloudStorage_name = :storage "
                        " ) ",
                        soci::use(storage),
                        soci::use(storage)
                        ;
                    // then add the record
                    sql <<
                        "INSERT INTO t_cloudStorageUser (user_dn, vo_name, cloudStorage_name, access_token, access_token_secret) "
                        "VALUES(:dn, :vo, :storage, :accessKey, :secretKey)",
                        soci::use(dn),
                        soci::use(vo),
                        soci::use(storage),
                        soci::use(accessKey),
                        soci::use(secretKey)
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

void OracleAPI::setCloudStorage(std::string const & storage, std::string const & appKey, std::string const & appSecret, std::string const & apiUrl)
{
    soci::session sql(*connectionPool);

    try
        {
            sql.begin();

            sql <<
                " MERGE INTO t_cloudStorage c "
                " USING (SELECT :storage AS cloudStorage_name FROM dual) tmp "
                " ON (c.cloudStorage_name = tmp.cloudStorage_name) "
                " WHEN MATCHED THEN "
                "   UPDATE SET app_key = :appKey, app_secret = :appSecret, service_api_url = :apiUrl "
                " WHEN NOT MATCHED THEN "
                "   INSERT (cloudStorage_name, app_key, app_secret, service_api_url) "
                "   VALUES (:storage, :appKey, :appSecret, :apiUrl)",
                soci::use(storage),
                soci::use(appKey),
                soci::use(appSecret),
                soci::use(apiUrl),
                soci::use(storage),
                soci::use(appKey),
                soci::use(appSecret),
                soci::use(apiUrl)
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

bool OracleAPI::isDmJob(std::string const & job)
{
    soci::session sql(*connectionPool);

    try
        {
            int count = 0;

            sql <<
                "SELECT COUNT(file_id) "
                "FROM t_dm "
                "WHERE job_id = :jobId ",
                soci::use(job),
                soci::into(count)
                ;

            return count > 0;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " +  e.what());
        }
    catch (...)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}


void OracleAPI::cancelDmJobs(std::vector<std::string> const & jobs)
{
    soci::session sql(*connectionPool);

    const std::string reason = "Job canceled by the user";
    std::string job_id;
    std::ostringstream cancelStmt1;
    std::ostringstream cancelStmt2;
    std::ostringstream jobIdStmt;

    try
        {
            for (std::vector<std::string>::const_iterator i = jobs.begin(); i != jobs.end(); ++i)
                {
                    job_id = (*i);
                    jobIdStmt << "'";
                    jobIdStmt << job_id;
                    jobIdStmt << "',";
                }

            std::string queryStr = jobIdStmt.str();
            job_id = queryStr.substr(0, queryStr.length() - 1);

            cancelStmt1 << "UPDATE t_job SET job_state = 'CANCELED', job_finished = sys_extract_utc(systimestamp), finish_time = sys_extract_utc(systimestamp), cancel_job='Y' ";
            cancelStmt1 << " ,reason = '";
            cancelStmt1 << reason;
            cancelStmt1 << "'";
            cancelStmt1 << " WHERE job_id IN (";
            cancelStmt1 << job_id;
            cancelStmt1 << ")";
            cancelStmt1 << " AND job_state NOT IN ('CANCELED','FINISHEDDIRTY', 'FINISHED', 'FAILED')";

            cancelStmt2 << "UPDATE t_dm SET file_state = 'CANCELED',  finish_time = sys_extract_utc(systimestamp) ";
            cancelStmt2 << " ,reason = '";
            cancelStmt2 << reason;
            cancelStmt2 << "'";
            cancelStmt2 << " WHERE job_id IN (";
            cancelStmt2 << job_id;
            cancelStmt2 << ")";
            cancelStmt2 << " AND file_state NOT IN ('CANCELED','FINISHED','FAILED')";

            soci::statement stmt1 = (sql.prepare << cancelStmt1.str());
            soci::statement stmt2 = (sql.prepare << cancelStmt2.str());


            sql.begin();
            // Cancel job
            stmt1.execute(true);

            // Cancel files
            stmt2.execute(true);
            sql.commit();

            jobIdStmt.str(std::string());
            jobIdStmt.clear();
            cancelStmt1.str(std::string());
            cancelStmt1.clear();
            cancelStmt2.str(std::string());
            cancelStmt2.clear();

        }
    catch (std::exception& e)
        {
            jobIdStmt.str(std::string());
            jobIdStmt.clear();
            cancelStmt1.str(std::string());
            cancelStmt1.clear();
            cancelStmt2.str(std::string());
            cancelStmt2.clear();

            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    catch (...)
        {
            jobIdStmt.str(std::string());
            jobIdStmt.clear();
            cancelStmt1.str(std::string());
            cancelStmt1.clear();
            cancelStmt2.str(std::string());
            cancelStmt2.clear();

            sql.rollback();
            throw Err_Custom(std::string(__func__) + ": Caught exception " );
        }
}


bool OracleAPI::getUserDnVisibleInternal(soci::session& sql)
{
    std::string show_user_dn;
    soci::indicator isNullShow = soci::i_ok;

    try
        {
            sql << "select show_user_dn from t_server_config", soci::into(show_user_dn, isNullShow);

            if (isNullShow == soci::i_null)
                {
                    return true;
                }
            else if(show_user_dn == "on")
                {
                    return true;
                }
            else if(show_user_dn == "off")
                {
                    return false;
                }
            else
                {
                    return true;
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

    return true;
}

bool OracleAPI::getUserDnVisible()
{
    soci::session sql(*connectionPool);

    try
        {
            return getUserDnVisibleInternal(sql);
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
