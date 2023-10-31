/*
 * Copyright (c) CERN 2013-2017
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/time.h>
#include <boost/lexical_cast.hpp>
#include <boost/range/algorithm/transform.hpp>

#include <map>
#include <chrono>
#include <soci/mysql/soci-mysql.h>
#include "MySqlAPI.h"
#include "sociConversions.h"
#include "db/generic/DbUtils.h"
#include <random>

#include "common/Exceptions.h"
#include "common/Logger.h"
#include "monitoring/msg-ifce.h"

using namespace fts3::common;
using namespace db;


static int thread_random(void)
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
        initstate_r(static_cast<unsigned>(clock()), statbuf, sizeof(statbuf), &rand_data);
    }

    int value = 0;
    random_r(&rand_data, &value);
    return value;
}


static unsigned getHashedId(void)
{
    return thread_random() % UINT16_MAX;
}


// There is a bug in SOCI where stmt.get_affected_rows
// will return 0 even if there were updated record, so this is an ugly workaround
// See https://github.com/SOCI/soci/pull/221
// Apparently this only affects when soci::use is used
static unsigned int get_affected_rows(soci::session& sql)
{
    soci::mysql_session_backend* be = static_cast<soci::mysql_session_backend*>(sql.get_backend());
    return (unsigned int)mysql_affected_rows(static_cast<MYSQL*>(be->conn_));
}


MySqlAPI::MySqlAPI(): poolSize(10), connectionPool(NULL), hostname(getFullHostname())
{
    // Pass
}



MySqlAPI::~MySqlAPI()
{
    if(connectionPool)
    {
        for (size_t i = 0; i < poolSize; ++i)
        {
            soci::session& sql = (*connectionPool).at(i);
            sql << "select concat('KILL ',id,';') from information_schema.processlist where user=:username", soci::use(username_);
        }

        delete connectionPool;
        connectionPool = NULL;
    }
}


static void getHostAndPort(const std::string& conn, std::string* host, int* port)
{
    host->clear();
    *port = 0;

    std::string remaining;

    size_t close;
    if (conn.size() > 0 && conn[0] == '[' && (close = conn.find(']')) != std::string::npos)
    {
        host->assign(conn.substr(1, close - 1));
        remaining = conn.substr(close + 1);
    }
    else
    {
        size_t colon = conn.find(':');
        if (colon == std::string::npos)
        {
            host->assign(conn);
        }
        else
        {
            host->assign(conn.substr(0, colon));
            remaining = conn.substr(colon);
        }
    }

    if (remaining[0] == ':')
    {
        *port = atoi(remaining.c_str() + 1);
    }
}


static void validateSchemaVersion(soci::connection_pool *connectionPool)
{
    static const unsigned expect[] = {9, 0};
    unsigned major, minor;

    soci::session sql(*connectionPool);
    sql << "SELECT major, minor FROM t_schema_vers ORDER BY major DESC, minor DESC, patch DESC",
        soci::into(major), soci::into(minor);

    auto schemaComparisonMessage = [&](const std::string& action) -> std::string {
        std::ostringstream out;
        out << "The database schema version is different than expected"
            << " (expected: " << expect[0] << "." << expect[1] << ", found: " << major << "." << minor << ")! "
            << action;
        return out.str();
    };

    if (expect[0] < major) {
        throw SystemError(schemaComparisonMessage("Please upgrade FTS"));
    } else if (expect[0] > major || expect[1] > minor) {
        throw SystemError(schemaComparisonMessage("Please upgrade the database schema"));
    } else if (expect[1] < minor) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << __func__ << " "
            << schemaComparisonMessage("FTS should be able to run, but it should be upgraded")
            << commit;
    }
}


void MySqlAPI::init(const std::string& username, const std::string& password,
        const std::string& connectString, int pooledConn)
{
    std::ostringstream connParams;
    std::string host, db;
    int port;

    try
    {
        connectionPool = new soci::connection_pool(pooledConn);

        // From connectString, get host and db
        size_t slash = connectString.find('/');
        if (slash != std::string::npos)
        {
            getHostAndPort(connectString.substr(0, slash), &host, &port);
            db   = connectString.substr(slash + 1, std::string::npos);

            connParams << "host='" << host << "' "
                       << "db='" << db << "' ";

            if (port != 0)
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
        username_ = username;

        std::string connStr = connParams.str();

        // Connect
        static const my_bool reconnect = 1;

        poolSize = (size_t) pooledConn;

        for (size_t i = 0; i < poolSize; ++i)
        {
            soci::session& sql = (*connectionPool).at(i);
            sql.open(soci::mysql, connStr);

            (*connectionPool).at(i) << "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED;";

            soci::mysql_session_backend* be = static_cast<soci::mysql_session_backend*>(sql.get_backend());
            mysql_options(static_cast<MYSQL*>(be->conn_), MYSQL_OPT_RECONNECT, &reconnect);
        }

        validateSchemaVersion(connectionPool);
    }
    catch (std::exception& e)
    {
        if(connectionPool)
        {
            delete connectionPool;
            connectionPool = NULL;
        }
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        if(connectionPool)
        {
            delete connectionPool;
            connectionPool = NULL;
        }
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


std::list<fts3::events::MessageUpdater> MySqlAPI::getActiveInHost(const std::string &host)
{
    soci::session sql(*connectionPool);

    try {
        soci::rowset<soci::row> rs = (sql.prepare <<
            "SELECT job_id, file_id, pid FROM t_file "
            " WHERE file_state IN ('ACTIVE', 'READY') AND transfer_host = :host",
            soci::use(host)
        );

        std::list<fts3::events::MessageUpdater> msgs;

        for (auto i = rs.begin(); i != rs.end(); ++i) {
            fts3::events::MessageUpdater msg;

            msg.set_job_id(i->get<std::string>("job_id"));
            msg.set_file_id(i->get<unsigned long long>("file_id"));
            msg.set_process_id(i->get<int>("pid"));
            msg.set_timestamp(millisecondsSinceEpoch());

            msgs.push_back(msg);
        }

        return msgs;
    }
    catch (std::exception &e)
    {
        throw SystemError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw SystemError(std::string(__func__) + ": Caught exception " );
    }
}


std::map<std::string, long long> MySqlAPI::getActivitiesInQueue(soci::session& sql, std::string src, std::string dst, std::string vo)
{
    std::map<std::string, long long> ret;

    try
    {
        std::string vo_exists;
        soci::indicator isNull = soci::i_ok;

        sql << "SELECT vo FROM t_activity_share_config where vo=:vo", soci::use(vo), soci::into(vo_exists, isNull);
        if(isNull == soci::i_null)
            return ret;


        soci::rowset<soci::row> rs = (
                                         sql.prepare <<
                                         " SELECT activity, COUNT(DISTINCT f.job_id, f.file_index) AS count "
                                         " FROM t_file f USE INDEX(idx_link_state_vo) INNER JOIN t_job j ON (f.job_id = j.job_id) WHERE "
                                         "  j.vo_name = f.vo_name AND f.file_state = 'SUBMITTED' AND "
                                         "  f.source_se = :source AND f.dest_se = :dest AND "
                                         "  f.vo_name = :vo_name AND j.vo_name = f.vo_name AND "
                                         "  (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd) AND "
                                         "  (j.job_type = 'N' OR j.job_type = 'R' OR j.job_type IS NULL) "
                                         " GROUP BY activity ORDER BY NULL ",
                                         soci::use(src),
                                         soci::use(dst),
                                         soci::use(vo),
                                         soci::use(hashSegment.start),
                                         soci::use(hashSegment.end)
                                     );

        ret.clear();

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
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return ret;
}


//check if called by multiple threads

std::map<std::string, int> MySqlAPI::getFilesNumPerActivity(soci::session& sql,
        std::string src, std::string dst, std::string vo, int filesNum,
        std::set<std::string> & defaultActivities)
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
        double sum = 0.0;

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
                defaultActivities.insert(it->first);
            }
        }

        // if default was used add it as well
        if (!defaultActivities.empty())
            sum += activityShares["default"];

        // assign slots to activities
        for (int i = 0; i < filesNum; i++)
        {
            // if sum <= 0 there is nothing to assign
            if (sum <= 0) break;
            // a random number from (0, 1)
            double r = ((double) thread_random() / (double)RAND_MAX);

            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << __func__ << ": Dice result: " << r << commit;

            // interval corresponding to given activity
            double interval = 0;

            for (it = activitiesInQueue.begin(); it != activitiesInQueue.end(); it++)
            {
                // if there are no more files for this activity continue
                if (it->second <= 0) continue;
                // get the activity name (if it was not defined use default)
                std::string activity_name = defaultActivities.count(it->first) ? "default" : it->first;

                // calculate the interval (normalize)
                interval += activityShares[activity_name] / sum;

                // if the slot has been assigned to the given activity ...

                if (r < interval)
                {
                    ++activityFilesNum[activity_name];

                    --it->second;
                    // if there are no more files for the given ativity remove it from the sum
                    if (it->second == 0)
                    {
                        sum -= activityShares[activity_name];
                    }
                    break;

                }
            }
        }

        // Debug output
        std::map<std::string, int>::const_iterator j;
        for (j = activityFilesNum.begin(); j != activityFilesNum.end(); ++j)
        {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << __func__ << ": " << j->first << " assigned " << j->second << commit;
        }
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return activityFilesNum;
}


void MySqlAPI::getQueuesWithPending(std::vector<QueueId>& queues)
{
    soci::session sql(*connectionPool);
    unsigned activeCount;
    std::string sourceSe;
    std::string destSe;
    std::string voName;

    try
    {
        soci::rowset<soci::row> rs1 = (sql.prepare <<
           "SELECT f.vo_name, f.source_se, f.dest_se FROM t_file f "
           "WHERE f.file_state = 'SUBMITTED' "
           "GROUP BY f.source_se, f.dest_se, f.file_state, f.vo_name "
           "ORDER BY null");

        soci::statement activeStmt = (sql.prepare <<
            "SELECT COUNT(*) FROM t_file "
            "WHERE source_se = :source_se AND dest_se = :dest_se AND vo_name = :vo_name "
            "   AND file_state = 'ACTIVE'",
            soci::use(sourceSe), soci::use(destSe), soci::use(voName), soci::into(activeCount));

        for (soci::rowset<soci::row>::const_iterator i1 = rs1.begin(); i1 != rs1.end(); ++i1)
        {
            soci::row const& r1 = *i1;
            voName = r1.get<std::string>("vo_name","");
            sourceSe = r1.get<std::string>("source_se","");
            destSe = r1.get<std::string>("dest_se","");

            activeStmt.execute(true);
            queues.emplace_back(
                 sourceSe,
                 destSe,
                 voName,
                 activeCount
            );
        }
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

}


void MySqlAPI::getQueuesWithSessionReusePending(std::vector<QueueId>& queues)
{
    soci::session sql(*connectionPool);

    try
    {
        std::string sourceSe, destSe, voName;
        unsigned activeCount;

        soci::rowset<soci::row> rs2 = (sql.prepare <<
           " SELECT DISTINCT t_file.vo_name, t_file.source_se, t_file.dest_se "
           " FROM t_file "
           " INNER JOIN t_job ON t_file.job_id = t_job.job_id "
           " WHERE "
           "      t_file.file_state = 'SUBMITTED' AND "
           "      (t_file.hashed_id BETWEEN :hStart AND :hEnd) AND"
           "      t_job.job_type = 'Y' ",
           soci::use(hashSegment.start), soci::use(hashSegment.end)
        );

        soci::statement activeStmt = (sql.prepare <<
            "SELECT COUNT(*) FROM t_file "
            "WHERE source_se = :source_se AND dest_se = :dest_se AND vo_name = :vo_name "
            "   AND file_state = 'ACTIVE'",
            soci::use(sourceSe), soci::use(destSe), soci::use(voName), soci::into(activeCount));

        for (soci::rowset<soci::row>::const_iterator i2 = rs2.begin(); i2 != rs2.end(); ++i2)
        {
            soci::row const& r = *i2;
            sourceSe = r.get<std::string>("source_se", "");
            destSe = r.get<std::string>("dest_se", "");
            voName = r.get<std::string>("vo_name", "");

            activeStmt.execute(true);

            queues.emplace_back(
                sourceSe,
                destSe,
                voName,
                activeCount
            );
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}

/// Count how many transfers are running for the given pair
/// @param source Source storage
/// @param dest Destination storage
/// @return Number of running (or scheduled) transfers
static int getActiveCount(soci::session& sql, const std::string &source, const std::string &dest)
{
    int activeCount = 0;

    // Running Transfers (R+N+Y+H job type)
    sql << "SELECT COUNT(*) FROM t_file f JOIN t_job j ON j.job_id = f.job_id "
        " WHERE f.source_se = :source_se AND f.dest_se = :dest_se"
        " AND f.file_state = 'ACTIVE'",
        soci::use(source), soci::use(dest),
        soci::into(activeCount);

    return activeCount;
}


void MySqlAPI::getReadyTransfers(const std::vector<QueueId>& queues,
        std::map<std::string, std::list<TransferFile> >& files)
{
    soci::session sql(*connectionPool);
    time_t now = time(NULL);

    try
    {
        // Iterate through queues, getting jobs IF the VO has not run out of credits
        // AND there are pending file transfers within the job
        for (auto it = queues.begin(); it != queues.end(); ++it)
        {
            int maxActive = 0;
            soci::indicator maxActiveNull = soci::i_ok;
            int filesNum = 10;

            int activeCount = getActiveCount(sql, it->sourceSe, it->destSe);

            // How many can we run
            sql << "SELECT active FROM t_optimizer WHERE source_se = :source_se AND dest_se = :dest_se",
                   soci::use(it->sourceSe),
                   soci::use(it->destSe),
                   soci::into(maxActive, maxActiveNull);

            // Calculate how many tops we should pick
            if (maxActiveNull != soci::i_null && maxActive > 0)
            {
                filesNum = (maxActive - activeCount);
                if(filesNum <= 0 ) {
                    continue;
                }
            }

            int fixedPriority =  ServerConfig::instance().get<int> ("UseFixedJobPriority");
            soci::indicator isMaxPriorityNull = soci::i_ok;
            int maxPriority = 3;
            if (fixedPriority == 0) {
                // Get highest priority waiting for this queue
                // We then filter by this, and order by file_id
                // Doing this, we avoid a order by priority, which would trigger a filesort, which
                // can be pretty slow...
                sql << "SELECT MAX(priority) "
                   "FROM t_file "
                   "WHERE "
                   "    vo_name=:voName AND source_se=:source AND dest_se=:dest AND "
                   "    file_state = 'SUBMITTED' AND "
                   "    hashed_id BETWEEN :hStart AND :hEnd",
                   soci::use(it->voName), soci::use(it->sourceSe), soci::use(it->destSe),
                   soci::use(hashSegment.start), soci::use(hashSegment.end),
                   soci::into(maxPriority, isMaxPriorityNull);
                if (isMaxPriorityNull == soci::i_null) {
                   FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "NULL MAX(priority), skip entry" << commit;
                   continue;
                }
            } 
            else 
            {
                FTS3_COMMON_LOGGER_NEWLOG(WARNING) << __func__
                << " Using fixed priority for Jobs."
                << commit;

                maxPriority=fixedPriority;
            }
            std::set<std::string> default_activities;
            std::map<std::string, int> activityFilesNum =
                getFilesNumPerActivity(sql, it->sourceSe, it->destSe, it->voName, filesNum, default_activities);

            struct tm tTime;
            gmtime_r(&now, &tTime);

            if (activityFilesNum.empty())
            {
                soci::rowset<TransferFile> rs = (sql.prepare <<
                      " SELECT f.file_state, f.source_surl, f.dest_surl, f.job_id, j.vo_name, "
                      "       f.file_id, j.overwrite_flag, j.archive_timeout, j.dst_file_report, "
                      "       j.user_dn, j.cred_id, f.checksum, j.checksum_method, j.source_space_token, "
                      "       j.space_token, j.copy_pin_lifetime, j.bring_online, "
                      "       f.user_filesize, f.file_metadata, f.archive_metadata, j.job_metadata,"
                      "       f.file_index, f.bringonline_token, f.scitag, "
                      "       f.source_se, f.dest_se, f.selection_strategy, j.internal_job_params, j.job_type "
                      " FROM t_file f USE INDEX(idx_link_state_vo), t_job j "
                      " WHERE f.job_id = j.job_id and  f.file_state = 'SUBMITTED' AND "
                      "     f.source_se = :source_se AND f.dest_se = :dest_se AND  "
                      "     f.vo_name = :vo_name AND "
                      "     (f.retry_timestamp is NULL OR f.retry_timestamp < :tTime) AND "
                      "     j.job_type IN ('N', 'R', 'H') AND "
                      "     f.hashed_id BETWEEN :hStart AND :hEnd AND "
                      "     j.priority = :maxPriority "
                      " ORDER BY file_id ASC "
                      " LIMIT :filesNum",
                      soci::use(it->sourceSe),
                      soci::use(it->destSe),
                      soci::use(it->voName),
                      soci::use(tTime),
                      soci::use(hashSegment.start), soci::use(hashSegment.end),
                      soci::use(maxPriority),
                      soci::use(filesNum));

                for (auto ti = rs.begin(); ti != rs.end(); ++ti)
                {
                    TransferFile& tfile = *ti;

                    if(tfile.jobType == Job::kTypeMultipleReplica)
                    {
                        int total = 0;
                        int remain = 0;
                        sql << " select count(*) as c1, "
                            " (select count(*) from t_file where file_state<>'NOT_USED' and  job_id=:job_id)"
                            " as c2 from t_file where job_id=:job_id",
                            soci::use(tfile.jobId),
                            soci::use(tfile.jobId),
                            soci::into(total),
                            soci::into(remain);

                        tfile.lastReplica = (total == remain)? 1: 0;
                    }
                    if(tfile.jobType == Job::kTypeMultiHop)
                    {
                        int maxIndex = 0;
                        sql << "SELECT MAX(file_index) "
                               "FROM t_file "
                               "WHERE job_id = :job_id ",
                                soci::use(tfile.jobId),
                                soci::into(maxIndex);

                        tfile.lastHop = (maxIndex == tfile.fileIndex)? 1: 0;
                    }

                    files[tfile.voName].push_back(tfile);
                }
            }
            else
            {
                // we are always checking empty string
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

                // Shuffle the map via intermediary vector
                using ActivityNumTuple = std::pair<std::string, int>;
                std::vector<ActivityNumTuple> vActivityFilesNum;
                vActivityFilesNum.reserve(activityFilesNum.size());

                for (auto it_act = activityFilesNum.begin(); it_act != activityFilesNum.end(); ++it_act)
                {
                    vActivityFilesNum.emplace_back(it_act->first, it_act->second);
                }

                auto seed = std::chrono::system_clock::now().time_since_epoch().count();
                auto random_engine = std::default_random_engine{seed};
                std::shuffle(vActivityFilesNum.begin(), vActivityFilesNum.end(), random_engine);

                for (auto it_act = vActivityFilesNum.begin(); it_act != vActivityFilesNum.end(); ++it_act)
                {
                    if (it_act->second == 0) continue;

                    std::string select = " SELECT f.file_state, f.source_surl, f.dest_surl, f.job_id, j.vo_name, "
                                         "       f.file_id, j.overwrite_flag, j.archive_timeout, j.dst_file_report, "
                                         "       j.user_dn, j.cred_id, f.checksum, j.checksum_method, j.source_space_token, "
                                         "       j.space_token, j.copy_pin_lifetime, j.bring_online, "
                                         "       f.user_filesize, f.file_metadata, f.archive_metadata, j.job_metadata, "
                                         "       f.file_index, f.bringonline_token, f.scitag, "
                                         "       f.source_se, f.dest_se, f.selection_strategy, j.internal_job_params, j.job_type "
                                         " FROM t_file f USE INDEX(idx_link_state_vo), t_job j "
                                         " WHERE f.job_id = j.job_id and  f.file_state = 'SUBMITTED' AND    "
                                         "      f.source_se = :source_se AND f.dest_se = :dest_se AND "
                                         "      (j.job_type = 'N' OR j.job_type = 'R') AND  "
                                         "      f.vo_name = :vo_name AND "
                                         "      (f.retry_timestamp is NULL OR f.retry_timestamp < :tTime) AND ";
                    select +=
                        it_act->first == "default" ?
                        "     (f.activity = :activity OR f.activity IS NULL OR f.activity IN " + def_act + ") AND "
                        :
                        "     f.activity = :activity AND ";
                    select +=
                        "   (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd) AND "
                        "   j.priority = :maxPriority "
                        "   ORDER BY file_id ASC "
                        "   LIMIT :filesNum";


                    soci::rowset<TransferFile> rs = (
                         sql.prepare <<
                         select,
                         soci::use(it->sourceSe),
                         soci::use(it->destSe),
                         soci::use(it->voName),
                         soci::use(tTime),
                         soci::use(it_act->first),
                         soci::use(hashSegment.start), soci::use(hashSegment.end),
                         soci::use(maxPriority),
                         soci::use(it_act->second)
                    );

                    for (auto ti = rs.begin(); ti != rs.end(); ++ti)
                    {
                        TransferFile& tfile = *ti;

                        if(tfile.jobType == Job::kTypeMultipleReplica)
                        {
                            int total = 0;
                            int remain = 0;
                            sql << " select count(*) as c1, "
                                " (select count(*) from t_file where file_state<>'NOT_USED' and  job_id=:job_id)"
                                " as c2 from t_file where job_id=:job_id",
                                soci::use(tfile.jobId),
                                soci::use(tfile.jobId),
                                soci::into(total),
                                soci::into(remain);

                            tfile.lastReplica = (total == remain)? 1: 0;
                        }
                        if(tfile.jobType == Job::kTypeMultiHop)
                        {
                            int maxIndex = 0;
                            sql << "SELECT MAX(file_index) "
                                   "FROM t_file "
                                   "WHERE job_id = :job_id ",
                                    soci::use(tfile.jobId),
                                    soci::into(maxIndex);

                            tfile.lastHop = (maxIndex == tfile.fileIndex)? 1: 0;
                        }

                        tfile.activity = it_act->first;
                        files[tfile.voName].push_back(tfile);
                    }
                }
            }
        }
    }
    catch (std::exception& e)
    {
        files.clear();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        files.clear();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


void MySqlAPI::useFileReplica(soci::session& sql, std::string jobId, uint64_t fileId, std::string destSurlUuid, soci::indicator destSurlUuidInd )
{
    soci::indicator selectionStrategyInd = soci::i_ok;
    std::string selectionStrategy;
    std::string vo_name;
    uint64_t nextReplica = 0, alreadyActive;
    soci::indicator nextReplicaInd = soci::i_ok;

    // make sure there hasn't been another already picked up
    sql << "SELECT COUNT(*) FROM t_file WHERE file_state IN ('READY', 'ACTIVE') AND job_id=:job_id",
        soci::use(jobId), soci::into(alreadyActive);
    if (alreadyActive > 0) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Tried to look for another replica for "
            << jobId << " when there is already at least one READY or ACTIVE "
            << commit;
        return;
    }

    //check if it's auto or manual
    sql << "SELECT selection_strategy, vo_name FROM t_file WHERE file_id = :file_id",
        soci::use(fileId), soci::into(selectionStrategy, selectionStrategyInd), soci::into(vo_name);

    // default is orderly
    if (selectionStrategyInd == soci::i_null) {
        selectionStrategy = "orderly";
    }

    sql << "SELECT min(file_id) FROM t_file WHERE file_state = 'NOT_USED' AND job_id=:job_id ",
        soci::use(jobId), soci::into(nextReplica, nextReplicaInd);

    if (selectionStrategy == "auto") {
        uint64_t bestFileId = getBestNextReplica(sql, jobId, vo_name);
        if (bestFileId > 0) {
            sql <<
                " UPDATE t_file "
                    " SET file_state = 'SUBMITTED', finish_time=NULL, dest_surl_uuid = :destSurlUuid "
                    " WHERE job_id = :jobId AND file_id = :file_id  "
                    " AND file_state = 'NOT_USED' ",
                    soci::use(destSurlUuid, destSurlUuidInd ), soci::use(jobId), soci::use(bestFileId);
        }
        else {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Out of replicas for " << jobId << commit;
        }
    }
    else {
        sql <<
            " UPDATE t_file "
            " SET file_state = 'SUBMITTED', finish_time=NULL, dest_surl_uuid = :destSurlUuid "
            " WHERE job_id = :jobId "
            " AND file_state = 'NOT_USED' and file_id=:file_id ",
            soci::use(destSurlUuid, destSurlUuidInd), soci::use(jobId), soci::use(nextReplica);
    }
}


uint64_t MySqlAPI::getNextHop(soci::session& sql, const std::string& jobId)
{
    uint64_t nextFileId = 0;

    soci::rowset<soci::row> rs = (sql.prepare <<
        "SELECT file_id FROM t_file WHERE job_id = :jobId AND file_state = 'NOT_USED' "
        "ORDER BY file_id ASC LIMIT 1", soci::use(jobId)
    );

    for (auto it = rs.begin(); it != rs.end(); ++it) {
        nextFileId = it->get<unsigned long long>("file_id", 0);
    }

    return nextFileId;
}


void MySqlAPI::useNextHop(soci::session& sql, std::string jobId)
{
    uint64_t nextFileId = getNextHop(sql, jobId);

    if (nextFileId == 0) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "No more hops for " << jobId << commit;
        return;
    }

    sql << "UPDATE t_file "
           "SET file_state = 'SUBMITTED', finish_time=NULL "
           "WHERE file_id = :fileId AND file_state = 'NOT_USED'",
           soci::use(nextFileId);

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Next hop for " << jobId << ": " << nextFileId << commit;
}


void MySqlAPI::setNullDestSURLMultiHop(soci::session& sql, std::string jobId)
{

    sql << "UPDATE t_file "
           "SET dest_surl_uuid = NULL "
           "WHERE job_id = :jobId AND file_state = 'NOT_USED'",
           soci::use(jobId);
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Nullify multihop dest_surl_uuid for " << jobId << commit;
}


bool MySqlAPI::isArchivingTransfer(soci::session& sql, const std::string& jobId,
                                   const Job::JobType& jobType, int archiveTimeout)
{
    uint64_t nextHop = 0;

    if (jobType == Job::kTypeMultiHop) {
        nextHop = getNextHop(sql, jobId);
    }

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Check if archiving transfer: archive_timeout=" << archiveTimeout
                                     << " next_hop=" << nextHop << " final_transfer=" << (nextHop == 0) << commit;

    return (nextHop == 0) && (archiveTimeout > -1);
}


bool pairCompare( std::pair<std::pair<std::string, std::string>, int> i, std::pair<std::pair<std::string, std::string>, int> j)
{
    return i.second < j.second;
}


uint64_t MySqlAPI::getBestNextReplica(soci::session& sql, const std::string & jobId, const std::string & voName)
{
    //for now consider only the less queued transfers, later add throughput and success rate
    uint64_t bestFileId = 0;
    std::string bestSource;
    std::string bestDestination;
    std::map<std::pair<std::string, std::string>, int> pair;
    soci::indicator ind = soci::i_ok;

    try
    {
        //get available pairs
        soci::rowset<soci::row> rs = (sql.prepare <<
            "select distinct source_se, dest_se from t_file where job_id=:jobId and file_state='NOT_USED'",
            soci::use(jobId)
        );

        soci::rowset<soci::row>::const_iterator it;
        for (it = rs.begin(); it != rs.end(); ++it)
        {
            std::string source_se = it->get<std::string>("source_se","");
            std::string dest_se = it->get<std::string>("dest_se","");
            int queued = 0;

            //get queued for this link and vo
            sql << " select count(*) from t_file where file_state='SUBMITTED' and "
                " source_se=:source_se and dest_se=:dest_se and "
                " vo_name=:voName ",
                soci::use(source_se), soci::use(dest_se),soci::use(voName), soci::into(queued);

            //get distinct source_se / dest_se
            std::pair<std::string, std::string> key(source_se, dest_se);
            pair.insert(std::make_pair(key, queued));

            if(queued == 0) //pick the first one if the link is free
                break;

        }

        //not waste queries
        if(!pair.empty())
        {
            //get min queue length
            std::pair<std::pair<std::string, std::string>, int> minValue = *min_element(pair.begin(), pair.end(), pairCompare );
            bestSource =      (minValue.first).first;
            bestDestination = (minValue.first).second;

            //finally get the next-best file_id to be processed
            sql << "select file_id from t_file where file_state='NOT_USED' and source_se=:source_se and dest_se=:dest_se and job_id=:jobId",
                soci::use(bestSource), soci::use(bestDestination), soci::use(jobId), soci::into(bestFileId, ind);

            if (ind != soci::i_ok) {
                bestFileId = 0;
            }
        }
    }
    catch (std::exception& e)
    {
        throw SystemError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw SystemError(std::string(__func__) + ": Caught exception " );
    }

    return bestFileId;
}

unsigned int MySqlAPI::updateFileStatusReuse(const TransferFile &file, const std::string &status)
{
    soci::session sql(*connectionPool);

    unsigned int updated = 0;

    try
    {
        sql.begin();

        soci::statement stmt(sql);

        stmt.exchange(soci::use(status, "state"));
        stmt.exchange(soci::use(file.jobId, "jobId"));
        stmt.exchange(soci::use(hostname, "hostname"));
        stmt.alloc();
        stmt.prepare("UPDATE t_file SET "
                     "    file_state = :state, start_time = UTC_TIMESTAMP(), transfer_host = :hostname "
                     "WHERE job_id = :jobId AND file_state = 'SUBMITTED'");
        stmt.define_and_bind();
        stmt.execute(true);

        updated = get_affected_rows(sql);

        if (updated > 0)
        {
            soci::statement jobStmt(sql);
            jobStmt.exchange(soci::use(status, "state"));
            jobStmt.exchange(soci::use(file.jobId, "jobId"));
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
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return updated;
}


void MySqlAPI::getReadySessionReuseTransfers(const std::vector<QueueId>& queues,
        std::map<std::string, std::queue<std::pair<std::string, std::list<TransferFile>>>>& files)
{
    if(queues.empty()) {
        return;
    }

    soci::session sql(*connectionPool);

    time_t now = time(NULL);
    struct tm tTime;

    gmtime_r(&now, &tTime);

    try
    {
        // Iterate through queues, getting jobs IF the VO has not run out of credits
        // AND there are pending file transfers within the job
        for (auto it = queues.begin(); it != queues.end(); ++it)
        {
            int maxActive;
            soci::indicator maxActiveNull;

            // How many already running
            int activeCount = getActiveCount(sql, it->sourceSe, it->destSe);

            // How many can we run
            sql << "SELECT active FROM t_optimizer WHERE source_se = :source_se AND dest_se = :dest_se",
                soci::use(it->sourceSe),
                soci::use(it->destSe),
                soci::into(maxActive, maxActiveNull);

            // This is what is left
            int limit = maxActive - activeCount;
            if (limit <= 0) {
                continue;
            }


            soci::rowset<std::string> jobs = (sql.prepare <<
                "SELECT job_id "
                "FROM t_job "
                "WHERE source_se=:source_se AND dest_se=:dest_se AND "
                "      vo_name=:vo_name AND job_type = 'Y' AND "
                "      job_state IN ('SUBMITTED', 'ACTIVE') AND "
                "      EXISTS (SELECT 1 FROM t_file WHERE t_file.job_id = t_job.job_id AND file_state = 'SUBMITTED') "
                "ORDER BY priority DESC, submit_time "
                "LIMIT :limit ",
                soci::use(it->sourceSe), soci::use(it->destSe),
                soci::use(it->voName),
                soci::use(limit)
            );

            for (auto ji = jobs.begin(); ji != jobs.end(); ++ji) {
                std::string jobId = *ji;

                soci::rowset<TransferFile> rs =
                    (
                        sql.prepare <<
                        " SELECT SQL_NO_CACHE "
                        "       f.file_state, f.source_surl, f.dest_surl, f.job_id, j.vo_name, "
                        "       f.file_id, j.overwrite_flag, j.archive_timeout, j.dst_file_report, "
                        "       j.user_dn, j.cred_id, f.checksum, j.checksum_method, j.source_space_token, "
                        "       j.space_token, j.copy_pin_lifetime, j.bring_online, "
                        "       f.user_filesize, f.file_metadata, f.archive_metadata, j.job_metadata, f.file_index, "
                        "       f.bringonline_token, f.scitag, f.source_se, f.dest_se, f.selection_strategy, "
                        "       j.internal_job_params, j.job_type "
                        " FROM t_job j INNER JOIN t_file f ON (j.job_id = f.job_id) "
                        " WHERE j.job_id = :job_id AND "
                        "       f.file_state = 'SUBMITTED' AND "
                        "       f.hashed_id BETWEEN :hStart AND :hEnd AND "
                        "       (f.retry_timestamp is null or f.retry_timestamp < :tTime)",
                        soci::use(jobId),
                        soci::use(hashSegment.start), soci::use(hashSegment.end),
                        soci::use(tTime)
                    );

                std::list<TransferFile> tf;
                for (auto ti = rs.begin(); ti != rs.end(); ++ti)
                {
                    tf.push_back(*ti);
                }
                if (!tf.empty()) {
                    files[it->voName].push(std::make_pair(jobId, tf));
                }
            }
        }
    }
    catch (std::exception& e)
    {
        files.clear();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        files.clear();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


boost::tuple<bool, std::string>
MySqlAPI::updateTransferStatus(const std::string& jobId, uint64_t fileId, int processId,
                               const std::string& transferState, const std::string& errorReason,
                               uint64_t filesize, double duration, double throughput,
                               bool retry, const std::string& fileMetadata)
{
    soci::session sql(*connectionPool);
    return updateFileTransferStatusInternal(sql, jobId, fileId, processId,
                                            transferState, errorReason,
                                            filesize, duration, throughput,
                                            retry, fileMetadata);
}


boost::tuple<bool, std::string>
MySqlAPI::updateFileTransferStatusInternal(soci::session &sql,
                                           std::string jobId, uint64_t fileId, int processId,
                                           std::string newFileState, const std::string& errorReason,
                                           uint64_t filesize, double duration, double throughput,
                                           bool retry, const std::string& fileMetadata)
{
    std::string storedState;
    soci::indicator destSurlUuidInd;
    std::string destSurlUuid;

    try
    {
        sql.begin();
        
        std::string storedState, jobState, archiveTimeoutStr;
        soci::indicator archiveTimeoutInd = soci::i_ok;
        int archiveTimeout = -1;
        Job::JobType jobType;

        time_t now = time(NULL);
        struct tm tTime;
        gmtime_r(&now, &tTime);

        if(jobId.empty() || fileId == 0) {
            sql << "SELECT job_id, file_id FROM t_file WHERE pid=:pid AND file_state = 'ACTIVE' LIMIT 1 ",
                soci::use(processId), soci::into(jobId), soci::into(fileId);
        }

        // Need to know the type of job, to know what's coming next
        sql << "SELECT job_type, job_state, archive_timeout FROM t_job WHERE job_id = :job_id",
            soci::use(jobId), soci::into(jobType), soci::into(jobState),
            soci::into(archiveTimeoutStr, archiveTimeoutInd);

        // query for the file state in DB
        sql << "SELECT file_state, dest_surl_uuid FROM t_file WHERE file_id=:fileId",
            soci::use(fileId),
            soci::into(storedState),
            soci::into(destSurlUuid, destSurlUuidInd);

        if (archiveTimeoutInd == soci::i_ok) {
            archiveTimeout = std::atoi(archiveTimeoutStr.c_str());
        }

        bool isStaging = (storedState == "STAGING");

        // If file is in terminal don't do anything, just return
        if(storedState == "FAILED" || storedState == "FINISHED" || storedState == "CANCELED" )
        {
            sql.rollback();
            return boost::tuple<bool, std::string>(false, storedState);
        }

        // If trying to go from ACTIVE back to READY, do nothing either
        if (storedState == "ACTIVE" && newFileState == "READY") {
            sql.rollback();
            return boost::tuple<bool, std::string>(false, storedState);
        }

        // If the file already in the same state, don't do anything either
        // avoid 2 url-copy on the same file id to start ( condition processId==0)
        if (storedState == newFileState) {
            if (newFileState == "READY" && processId != 0) {}
            else {
                sql.rollback();
                return boost::tuple<bool, std::string>(false, storedState);
            }
        }

        soci::statement stmt(sql);
        std::ostringstream query;

        query << "UPDATE t_file SET "
              "    file_state = :state, reason = :reason";
        stmt.exchange(soci::use(newFileState, "state"));
        stmt.exchange(soci::use(errorReason, "reason"));

        if (newFileState == "FINISHED" || newFileState == "FAILED" || newFileState == "CANCELED")
        {
            query << ", FINISH_TIME = :time1, DEST_SURL_UUID = NULL";
            stmt.exchange(soci::use(tTime, "time1"));
        }
        if (newFileState == "ACTIVE" || newFileState == "READY")
        {
            query << ", START_TIME = :time1";
            stmt.exchange(soci::use(tTime, "time1"));
        }

        query << ", transfer_Host = :hostname";
        stmt.exchange(soci::use(hostname, "hostname"));

        if (newFileState == "FINISHED")
        {
            query << ", transferred = :filesize";
            stmt.exchange(soci::use(filesize, "filesize"));
        }

        if (newFileState == "FAILED" || newFileState == "CANCELED")
        {
            query << ", transferred = :transferred";
            stmt.exchange(soci::use(0, "transferred"));
        }

        if (newFileState == "STAGING")
        {
            if (isStaging)
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
        
        // Move the new state to ARCHIVING if the transfer completed and the archive timeout is set
        if (newFileState == "FINISHED" && isArchivingTransfer(sql, jobId, jobType, archiveTimeout)) {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Moving transfer " << jobId << " " << fileId
                                             << " to ARCHIVING state" << commit;
            newFileState = "ARCHIVING";
        }

        if (!fileMetadata.empty()) {
            query << ", file_metadata = :file_metadata";
            stmt.exchange(soci::use(fileMetadata, "file_metadata"));
        }

        query << "   , pid = :pid, filesize = :filesize, tx_duration = :duration, throughput = :throughput, current_failures = :current_failures "
              "WHERE file_id = :fileId AND file_state = :oldState";
        stmt.exchange(soci::use(processId, "pid"));
        stmt.exchange(soci::use(filesize, "filesize"));
        stmt.exchange(soci::use(duration, "duration"));
        stmt.exchange(soci::use(throughput, "throughput"));
        stmt.exchange(soci::use(static_cast<int>(retry), "current_failures"));
        stmt.exchange(soci::use(fileId, "fileId"));
        stmt.exchange(soci::use(storedState, "oldState"));
        stmt.alloc();
        stmt.prepare(query.str());
        stmt.define_and_bind();

        stmt.execute(true);

        if (get_affected_rows(sql) == 0) {
            sql.rollback();
            return boost::tuple<bool, std::string>(false, storedState);
        }

        sql.commit();

        switch (jobType) {
            // Multiple replica job, on failure, need to pick next option
            case Job::kTypeMultipleReplica:
                if ((jobState != "CANCELED" && jobState != "FAILED") &&
                    (newFileState == "FAILED" || newFileState == "CANCELED")) {
                    sql.begin();
                    useFileReplica(sql, jobId, fileId, destSurlUuid, destSurlUuidInd);
                    sql.commit();
                }
                break;
            // For multihop jobs, on success enable next option
            case Job::kTypeMultiHop:
                if ((jobState != "CANCELED" && jobState != "FAILED") &&
                    (newFileState == "FINISHED")) {
                    sql.begin();
                    useNextHop(sql, jobId);
                    sql.commit();
                }
                else {
                    // need to remove all dest_surl_uuid from all jobs
                    sql.begin();
                    setNullDestSURLMultiHop(sql, jobId);
                    sql.commit();
                }
                break;
            // Nothing special for other type of jobs
            default:
                break;
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
    return boost::tuple<bool, std::string>(true, storedState);
}


bool MySqlAPI::updateJobStatus(const std::string& jobId, const std::string& jobState)
{
    soci::session sql(*connectionPool);
    return updateJobTransferStatusInternal(sql, jobId, jobState);
}


bool MySqlAPI::updateJobTransferStatusInternal(soci::session& sql, std::string jobId, const std::string& state)
{
    try
    {
        int numberOfFilesInJob = 0;
        int numberOfFilesCanceled = 0;
        int numberOfFilesFinished = 0;
        int numberOfFilesFailed = 0;
        int numberOfStaging = 0;
        int numberOfFilesArchiving = 0;

        int numberOfFilesNotCanceled = 0;
        int numberOfFilesNotCanceledNorFailed = 0;

        std::string currentState("");
        Job::JobType jobType;
        soci::indicator isNull = soci::i_ok;
        std::string sourceSe;
        soci::indicator isNullFileId = soci::i_ok;

        soci::statement stmt1 = (
            sql.prepare << " SELECT job_state, job_type FROM t_job  "
            " WHERE job_id = :job_id ",
            soci::use(jobId),
            soci::into(currentState),
            soci::into(jobType, isNull));
        stmt1.execute(true);

        if(currentState == state)
            return true;

        if(currentState == "STAGING" && state == "STARTED")
            return true;

        if(jobType == Job::kTypeMultipleReplica && currentState == "ARCHIVING" && state == "FAILED") {
            // FTS-1882: Fail multi-replica archiving jobs when archive monitoring step fails
            std::string reason = "Archive monitoring failed in a multiple replica job";

            sql <<  " SELECT source_se FROM t_file WHERE job_id=:job_id AND file_state='FAILED' LIMIT 1 ",
            soci::use(jobId), soci::into(sourceSe);


            sql.begin();
            // Update job
            soci::statement stmt = (
                    sql.prepare << "UPDATE t_job SET "
                                   "    job_state = :state, job_finished = UTC_TIMESTAMP(), "
                                   "    reason = :reason, source_se = :sourceSe "
                                   "WHERE job_id = :jobId and job_state NOT IN ('FAILED','FINISHEDDIRTY','CANCELED','FINISHED')  ",
                            soci::use(state, "state"), soci::use(reason, "reason"),
                            soci::use(sourceSe, "sourceSe"),
                            soci::use(jobId, "jobId"));
            stmt.execute(true);
            sql.commit();

            return true;
        }

        if(state == "ACTIVE" && jobType == Job::kTypeRegular)
        {
            sql.begin();

            soci::statement stmt8 = (
                sql.prepare << "UPDATE t_job "
                "SET job_state = :state "
                "WHERE job_id = :jobId AND job_state NOT IN ('ACTIVE','FINISHEDDIRTY','CANCELED','FINISHED','FAILED') ",
                soci::use(state, "state"), soci::use(jobId, "jobId"));
            stmt8.execute(true);

            sql.commit();

            return true;
        }
        else if ( (state == "FINISHED" || state == "FAILED") && jobType == Job::kTypeRegular)
        {
            uint64_t file_id = 0;
            sql <<  " SELECT file_id from t_file where job_id=:job_id and file_state='SUBMITTED' LIMIT 1 ", soci::use(jobId), soci::into(file_id, isNullFileId);
            if(isNullFileId != soci::i_null && file_id > 0)
                return true;
        }
        else if ( state == "FINISHED" && jobType == Job::kTypeMultipleReplica)
        {
            sql <<  " SELECT source_se FROM t_file WHERE job_id=:job_id AND file_state='FINISHED' LIMIT 1 ", soci::use(jobId), soci::into(sourceSe);
        }

        sql << " SELECT COUNT(DISTINCT file_index) "
            " FROM t_file "
            " WHERE job_id = :job_id ",
            soci::use(jobId),
            soci::into(numberOfFilesInJob);

        sql << " SELECT COUNT(DISTINCT file_index) "
            " FROM t_file "
            " WHERE job_id = :jobId "
            "   AND file_state <> 'CANCELED' ", // all the replicas have to be in CANCELED state in order to count a file as canceled
            soci::use(jobId),                  // so if just one replica is in a different state it is enough to count it as not canceled
            soci::into(numberOfFilesNotCanceled);


        // number of files that were canceled
        numberOfFilesCanceled = numberOfFilesInJob - numberOfFilesNotCanceled;

        // number of files that were finished
        sql << " SELECT COUNT(DISTINCT file_index) "
            " FROM t_file "
            " WHERE job_id = :jobId "
            "   AND file_state = 'FINISHED' ", // at least one replica has to be in FINISH state in order to count the file as finished
            soci::use(jobId),
            soci::into(numberOfFilesFinished);

        // number of files still staging
        sql << " SELECT COUNT(DISTINCT file_index) "
            " FROM t_file "
            " WHERE job_id = :jobId "
            "   AND file_state IN ('STAGING', 'STARTED') ",
            soci::use(jobId),
            soci::into(numberOfStaging);

        // number of files archiving
        sql << " SELECT COUNT(DISTINCT file_index) "
            " FROM t_file "
            " WHERE job_id = :jobId "
            "   AND file_state = 'ARCHIVING' ",
            soci::use(jobId),
            soci::into(numberOfFilesArchiving);

        // number of files that were not canceled nor failed
        sql << " SELECT COUNT(DISTINCT file_index) "
            " FROM t_file "
            " WHERE job_id = :jobId "
            "   AND file_state <> 'CANCELED' " // for not canceled files see above
            "   AND file_state <> 'FAILED' ",  // all the replicas have to be either in CANCELED or FAILED state in order to count
            soci::use(jobId),                 // a files as failed so if just one replica is not in CANCEL neither in FAILED state
            soci::into(numberOfFilesNotCanceledNorFailed);

        // number of files that failed
        numberOfFilesFailed = numberOfFilesInJob - numberOfFilesNotCanceledNorFailed - numberOfFilesCanceled;

        // aggregated number of files in terminal states (FINISHED, FAILED and CANCELED)
        int numberOfFilesTerminal = numberOfFilesCanceled + numberOfFilesFailed + numberOfFilesFinished;

        bool jobFinished = (numberOfFilesInJob == numberOfFilesTerminal) ||
            (jobType == Job::kTypeMultiHop && numberOfFilesFailed + numberOfFilesCanceled > 0);

        if (jobFinished)
        {
            std::string state;
            std::string reason = "One or more files failed. Please have a look at the details for more information";
            if (numberOfFilesFinished > 0 && numberOfFilesFailed > 0)
            {
                if (jobType == Job::kTypeMultiHop)
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

            //re-execute here just in case
            stmt1.execute(true);

            if(currentState == state)
                return true;

            if(sourceSe.length() > 0)
            {
                sql.begin();
                // Update job
                soci::statement stmt6 = (
                    sql.prepare << "UPDATE t_job SET "
                    "    job_state = :state, job_finished = UTC_TIMESTAMP(), "
                    "    reason = :reason, source_se = :sourceSe "
                    "WHERE job_id = :jobId and job_state NOT IN ('FAILED','FINISHEDDIRTY','CANCELED','FINISHED')  ",
                    soci::use(state, "state"), soci::use(reason, "reason"), soci::use(sourceSe, "sourceSe"),
                    soci::use(jobId, "jobId"));
                stmt6.execute(true);
                sql.commit();
            }
            else
            {
                sql.begin();
                // Update job
                soci::statement stmt6 = (
                    sql.prepare << "UPDATE t_job SET "
                    "    job_state = :state, job_finished = UTC_TIMESTAMP(), "
                    "    reason = :reason "
                    "WHERE job_id = :jobId and job_state NOT IN ('FAILED','FINISHEDDIRTY','CANCELED','FINISHED')  ",
                    soci::use(state, "state"), soci::use(reason, "reason"),
                    soci::use(jobId, "jobId"));
                stmt6.execute(true);
                sql.commit();
            }

            // Behavior on finished multihop jobs
            bool cancelUnusedMultihopFiles = ServerConfig::instance().get<bool>("CancelUnusedMultihopFiles");

            if (cancelUnusedMultihopFiles && jobType == Job::kTypeMultiHop) {
                sql.begin();
                soci::statement stmt7 = (
                    sql.prepare << "UPDATE t_file SET file_state = 'CANCELED' "
                                   "WHERE job_id = :jobId and file_state = 'NOT_USED' ",
                    soci::use(jobId));
                stmt7.execute(true);
                sql.commit();
            }
        }
        // Job not finished yet
        else
        {
            if (state == "ACTIVE" || state == "STAGING" || state == "SUBMITTED" || (currentState == "STAGING" && numberOfStaging == 0))
            {
                std::string newState = state;

                //re-execute here just in case
                stmt1.execute(true);

                if(currentState == state)
                    return true;

                // Move from staging to SUBMITTED if the job is session reuse and there are none in staging
                if (currentState == "STAGING" && numberOfStaging == 0) {
                    newState = "SUBMITTED";
                }

                sql.begin();

                soci::statement stmt8 = (
                                            sql.prepare << "UPDATE t_job "
                                            "SET job_state = :state "
                                            "WHERE job_id = :jobId AND job_state NOT IN ('FINISHEDDIRTY','CANCELED','FINISHED','FAILED') ",
                                            soci::use(newState, "state"), soci::use(jobId, "jobId"));
                stmt8.execute(true);

                sql.commit();
            }
            // all ongoing files are in archiving state
            else if (numberOfFilesArchiving > 0 &&
                     (numberOfFilesInJob == numberOfFilesTerminal + numberOfFilesArchiving)) {
                // re-execute here just in case
                stmt1.execute(true);

                if (currentState == "ARCHIVING")
                    return true;

                sql.begin();

                soci::statement stmt8 = (
                    sql.prepare << "UPDATE t_job SET job_state = 'ARCHIVING' "
                                   "WHERE job_id = :jobId and job_state NOT IN ('FAILED','FINISHEDDIRTY','CANCELED','FINISHED')",
                            soci::use(jobId, "jobId"));
                stmt8.execute(true);

                sql.commit();
            }
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    return true;
}


void MySqlAPI::updateFileTransferProgressVector(const std::vector<fts3::events::MessageUpdater>& messages)
{
    soci::session sql(*connectionPool);

    try
    {
        double throughput = 0.0;
        uint64_t transferred = 0.0;
        uint64_t fileId = 0;

        soci::statement stmt = (
                sql.prepare << "UPDATE t_file SET "
                               "    throughput = :throughput, "
                               "    transferred = :transferred "
                               "WHERE file_id = :fileId",
                        soci::use(throughput),
                        soci::use(transferred),
                        soci::use(fileId)
        );

        sql.begin();

        for (const auto& message:  messages) {
            if (message.file_id() > 0) {
                const auto& fileState = message.transfer_status();

                if (fileState == "ACTIVE") {
                    fileId = message.file_id();

                    if (message.throughput() > 0.0 && fileId > 0 ) {
                        throughput = message.throughput();
                        transferred = message.transferred();
                        stmt.execute(true);
                    }
                }
            }
        }

        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::getCancelJob(std::vector<int>& requestIDs)
{
    soci::session sql(*connectionPool);
    int pid = 0;
    uint64_t file_id = 0;

    try
    {
        soci::rowset<soci::row> rs = (sql.prepare << " select distinct pid, file_id from t_file where "
                                      " file_state='CANCELED' and finish_time is NULL "
                                      " AND transfer_host=:hostname "
                                      " AND staging_start is NULL ",
                                      soci::use(hostname));

        soci::statement stmt1 = (sql.prepare << "UPDATE t_file SET finish_time = UTC_TIMESTAMP() "
                                 "WHERE file_id = :file_id ", soci::use(file_id, "file_id"));

        // Cancel files
        sql.begin();
        for (soci::rowset<soci::row>::const_iterator i2 = rs.begin(); i2 != rs.end(); ++i2)
        {
            soci::row const& row = *i2;
            pid = row.get<int>("pid",0);
            file_id = row.get<unsigned long long>("file_id");

            if(pid > 0)
                requestIDs.push_back(pid);

            stmt1.execute(true);
        }
        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}

std::list<TransferFile> MySqlAPI::getForceStartTransfers()
{
    soci::session sql(*connectionPool);

    try
    {
        const soci::rowset<TransferFile> rs = (sql.prepare <<
                                         " SELECT f.file_state, f.source_surl, f.dest_surl, f.job_id, j.vo_name, "
                                         "       f.file_id, j.overwrite_flag, j.archive_timeout, j.dst_file_report, "
                                         "       j.user_dn, j.cred_id, f.checksum, j.checksum_method, j.source_space_token, "
                                         "       j.space_token, j.copy_pin_lifetime, j.bring_online, "
                                         "       f.user_filesize, f.file_metadata, f.archive_metadata, j.job_metadata, "
                                         "       f.file_index, f.bringonline_token, f.scitag, "
                                         "       f.source_se, f.dest_se, f.selection_strategy, j.internal_job_params, j.job_type "
                                         " FROM t_file f USE INDEX(idx_state), t_job j "
                                         " WHERE f.job_id = j.job_id and f.file_state = 'FORCE_START'"
                                         " ORDER BY null");

        return {rs.begin(), rs.end()};
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


bool MySqlAPI::isTrAllowed(const std::string& sourceStorage, const std::string& destStorage)
{
    soci::session sql(*connectionPool);

    try
    {
        int maxActive = 0;

        sql << "SELECT active FROM t_optimizer "
               "WHERE source_se = :source AND dest_se = :dest_se LIMIT 1 ",
               soci::use(sourceStorage), soci::use(destStorage),
               soci::into(maxActive);

        if (!sql.got_data()) {
            maxActive = DEFAULT_MIN_ACTIVE;
        }

        int currentActive = getActiveCount(sql, sourceStorage, destStorage);

        return (currentActive < maxActive);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::reapStalledTransfers(std::vector<TransferFile>& transfers)
{
    soci::session sql(*connectionPool);

    try
    {
        TransferFile transfer;
        struct tm startTimeSt;
        time_t startTime;
        soci::indicator isNull = soci::i_ok;
        soci::indicator isNullParams = soci::i_ok;
        soci::indicator isNullPid = soci::i_ok;

        soci::statement stmt = (sql.prepare <<
            " SELECT f.job_id, f.file_id, f.start_time, f.pid, f.internal_file_params, "
            " j.job_type "
            " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
            " WHERE f.file_state IN ('ACTIVE', 'READY') "
            " AND j.job_type != 'Y' "
            " AND f.transfer_host = :host",
            soci::use(hostname),
            soci::into(transfer.jobId), soci::into(transfer.fileId), soci::into(startTimeSt),
            soci::into(transfer.pid, isNullPid), soci::into(transfer.internalFileParams, isNullParams),
            soci::into(transfer.jobType, isNull)
        );

        if (stmt.execute(true)) {
            do {
                startTime = timegm(&startTimeSt);
                time_t now2 = getUTC(0);
                int timeout = 7200;

                if (isNullParams != soci::i_null) {
                    timeout = transfer.getProtocolParameters().timeout;
                    if(timeout == 0) {
                        timeout = 7200;
                    }
                    else {
                        timeout += 3600;
                    }
                }

                double diff = difftime(now2, startTime);
                if (diff > timeout) {
                    transfers.emplace_back(transfer);
                }
            } while (stmt.fetch());
        }
    }
    catch (std::exception& e) {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...) {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


bool MySqlAPI::terminateReuseProcess(const std::string& jobId, int pid, const std::string& message, bool force)
{
    soci::session sql(*connectionPool);
    std::string job_id = jobId;
    bool doUpdate = false;

    try
    {
        if (job_id.empty()) {
            sql << "SELECT job_id FROM t_file WHERE pid = :pid AND file_state = 'ACTIVE' LIMIT 1",
                soci::use(pid), soci::into(job_id);
        }

        if (!force) {
            std::string reuse;
            soci::indicator reuseInd = soci::i_ok;

            sql << "SELECT job_type FROM t_job WHERE job_id = :job_id AND job_type IS NOT NULL",
                soci::use(job_id), soci::into(reuse, reuseInd);
            doUpdate = (sql.got_data() && (reuse == "Y" || reuse == "H"));
        }

        if (force || doUpdate)
        {
            // Select all the rows to be updated
            soci::rowset<soci::row> rs = (sql.prepare <<
                    "SELECT file_id FROM t_file WHERE job_id = :jobId"
                    " AND file_state NOT IN ('FINISHED', 'FAILED', 'CANCELED')"
                    " ORDER BY file_id",
                    soci::use(jobId)
            );

            // Prepare statement for update
            uint64_t file_id;
            soci::statement stmt1 = (sql.prepare << "UPDATE t_file SET file_state  = 'FAILED',"
                                                    " finish_time = UTC_TIMESTAMP(),"
                                                    " dest_surl_uuid = NULL, reason = :message"
                                                    " WHERE file_id = :file_id",
                                                    soci::use(message),
                                                    soci::use(file_id));

            sql.begin();
            for (auto & row : rs) {
                file_id = row.get<unsigned long long>("file_id");
                stmt1.execute(true);
            }
            sql.commit();
        }
    }
    catch (...)
    {
        sql.rollback();
    }

    return true;
}


void MySqlAPI::setPidForJob(const std::string& jobId, int pid)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        sql << "UPDATE t_file SET pid = :pid WHERE job_id = :jobId ", soci::use(pid), soci::use(jobId);

        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::backup(int intervalDays, long bulkSize, long* nJobs, long* nFiles, long* nDeletions)
{

    soci::session sql(*connectionPool);

    unsigned index=0, activeHosts=0, start=0, end=0;
    std::string serviceName = "fts_backup";
    *nJobs = 0;
    *nFiles = 0;
    *nDeletions = 0;
    std::string job_id;
    int count = 0;
    int countBeat = 0;
    bool drain = false;
    int hostsRunningBackup = 0;
    bool doBackup = true;
    try
    {

        // Total number of working instances, prevent from starting a second one
        soci::statement stmtActiveHosts = (
                                              sql.prepare << "SELECT COUNT(hostname) FROM t_hosts "
                                              "  WHERE beat >= DATE_SUB(UTC_TIMESTAMP(), interval 30 minute) and service_name = :service_name",
                                              soci::use(serviceName),
                                              soci::into(hostsRunningBackup));
        stmtActiveHosts.execute(true);

        if(hostsRunningBackup > 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Backup already running, won't start" << commit;
            return;
        }

        try
        {
            //update heartbeat first, the first must get 0
            updateHeartBeatInternal(sql, &index, &activeHosts, &start, &end, serviceName);
        }
        catch(...)
        {
            try
            {
                sleep(1);
                //update heartbeat first, the first must get 0
                updateHeartBeatInternal(sql, &index, &activeHosts, &start, &end, serviceName);
            }
            catch(...)
            {

            }

        }
        doBackup =  ServerConfig::instance().get<bool> ("BackupTables");
        //prevent more than on server to update the optimizer decisions
        if(hashSegment.start == 0)
        {
            soci::rowset<soci::row> rs = (
                 sql.prepare <<
                 "  select  job_id from t_job where job_finished < (UTC_TIMESTAMP() - interval :days DAY ) ",
                 soci::use(intervalDays)
             );

            std::ostringstream jobIdStmt;
            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
            {
                ++count;
                ++countBeat;

                if(countBeat == 1000)
                {
                    // Give some progress
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Backup progress: "
                        << *nJobs << " jobs and " << *nFiles << " files affected" << commit;

                    //reset
                    countBeat = 0;

                    //update heartbeat first
                    try
                    {
                        //update heartbeat first, the first must get 0
                        updateHeartBeatInternal(sql, &index, &activeHosts, &start, &end, serviceName);
                    }
                    catch(...)
                    {
                        try
                        {
                            sleep(1);
                            //update heartbeat first, the first must get 0
                            updateHeartBeatInternal(sql, &index, &activeHosts, &start, &end, serviceName);
                        }
                        catch(...)
                        {

                        }

                    }

                    drain = getDrainInternal(sql);
                    if(drain)
                    {
                        return;
                    }
                }

                soci::row const& r = *i;
                job_id = r.get<std::string>("job_id");
                jobIdStmt << "'";
                jobIdStmt << job_id;
                jobIdStmt << "',";
                if(count == bulkSize)
                {
                    std::string queryStr = jobIdStmt.str();
                    job_id = queryStr.substr(0, queryStr.length() - 1);

                    sql.begin();

                    if (doBackup) {
                        sql << "INSERT INTO t_job_backup SELECT * FROM t_job WHERE job_id  in (" +job_id+ ")";

                        soci::statement insertFiles = (sql.prepare <<
                            "INSERT INTO t_file_backup SELECT * FROM t_file WHERE  job_id in (" +job_id+ ")");
                        insertFiles.execute();
                    }
                      
                    soci::statement deleteFiles = (sql.prepare << 
                           "DELETE FROM t_file WHERE job_id in (" +job_id+ ")");
                    deleteFiles.execute();
                    (*nFiles) += deleteFiles.get_affected_rows(); 

                    soci::statement deleteDeletions = (sql.prepare <<
                           "DELETE FROM t_dm WHERE job_id in (" +job_id+ ")");
                    deleteDeletions.execute();
                    (*nDeletions) += deleteDeletions.get_affected_rows();

                    sql << "DELETE FROM t_job WHERE job_id in (" +job_id+ ")";
                    (*nJobs) += count;

                    count = 0;
                    jobIdStmt.str(std::string());
                    jobIdStmt.clear();
                    sql.commit();
                    sleep(1); // give it sometime to breath
                }
            }

            //delete from t_optimizer_evolution old records bigger than the interval of days being passed
            sql.begin();
            soci::statement deleteOptEvo = (sql.prepare <<
                    "delete from t_optimizer_evolution where datetime < (UTC_TIMESTAMP() - interval :days DAY ) ",
                    soci::use(intervalDays)
            );
            deleteOptEvo.execute();
            sql.commit();

            //delete from t_file_retry_errors old records bigger than the interval of days being passed
            sql.begin();
            soci::statement deleteFileRetryErr = (
                    sql.prepare << "  delete from t_file_retry_errors where datetime < (UTC_TIMESTAMP() - interval :days DAY ) ",
                    soci::use(intervalDays)
            );
            deleteFileRetryErr.execute();
            sql.commit();
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::forkFailed(const std::string& jobId)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        sql << "UPDATE t_file "
               " SET file_state = 'FAILED', transfer_host=:hostname, dest_surl_uuid = NULL, "
               "    finish_time= UTC_TIMESTAMP(),"
               "    reason='Transfer failed to fork, check fts3server.log for more details'"
               " WHERE job_id = :jobId AND "
               "    file_state NOT IN ('FINISHED','FAILED','CANCELED')",
                soci::use(hostname), soci::use(jobId);

        sql << "UPDATE t_job "
               " SET job_state='FAILED', job_finished=UTC_TIMESTAMP(), "
               "     reason='Transfer failed to fork, check fts3server.log for more details'"
               " WHERE job_id=:job_id",
               soci::use(jobId);

        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::cancelExpiredJobsForVo(std::vector<std::string>& jobs, int maxTime, const std::string &vo)
{
    soci::session sql(*connectionPool);

    try {
        // Prepare common statements (normal and multihop jobs)
        // Multihop jobs should not cancel NOT_USED file states
        std::string message;
        std::string job_id;

        std::string cancelFileQuery =
            "UPDATE t_file SET "
            "   finish_time = UTC_TIMESTAMP(), dest_surl_uuid = NULL, "
            "   file_state = 'CANCELED', reason = :reason "
            "   WHERE job_id = :jobId AND file_state IN ";
        std::string cancelFileStates = "('SUBMITTED', 'NOT_USED', 'STAGING', 'ON_HOLD', 'ON_HOLD_STAGING')";
        std::string cancelMultihopFileStates = "('SUBMITTED', 'STAGING', 'ON_HOLD', 'ON_HOLD_STAGING')";

        soci::statement stmtCancelFile = (sql.prepare << cancelFileQuery + cancelFileStates,
                soci::use(message), soci::use(job_id));

        soci::statement stmtMultihopCancelFile = (sql.prepare << cancelFileQuery + cancelMultihopFileStates,
                soci::use(message), soci::use(job_id));

        auto performCancel = [&](const soci::rowset<soci::row>& rs) {
            for (auto i = rs.begin(); i != rs.end(); ++i) {
                auto job_type = i->get<std::string>("job_type", "N");
                job_id = i->get<std::string>("job_id");

                (job_type != "H") ? stmtCancelFile.execute(true)
                                  : stmtMultihopCancelFile.execute(true);

                updateJobTransferStatusInternal(sql, job_id, "CANCELED");
                jobs.push_back(job_id);
            }
        };

        // Cancel jobs using global timeout
        if (maxTime > 0) {
            message = "Job has been canceled because it stayed in the queue for too long (global timeout)";
            soci::rowset<soci::row> rs = (sql.prepare <<
                "SELECT job_id, job_type FROM t_job WHERE "
                "    submit_time < (UTC_TIMESTAMP() - interval :interval hour) AND "
                "    job_state IN ('SUBMITTED', 'STAGING') AND job_finished IS NULL AND vo_name = :vo ",
                soci::use(maxTime), soci::use(vo));

            sql.begin();
            performCancel(rs);
            sql.commit();
        }

        // Cancel jobs using their own timeout
        message = "Job has been canceled because it stayed in the queue for too long (max-time-in-queue timeout)";
        soci::rowset<soci::row> rs = (sql.prepare <<
            "SELECT job_id, job_type FROM t_job USE INDEX(idx_jobfinished) WHERE "
            "    max_time_in_queue IS NOT NULL AND max_time_in_queue < UNIX_TIMESTAMP() AND vo_name = :vo "
            "    AND job_state IN ('SUBMITTED', 'ACTIVE', 'STAGING') AND job_finished IS NULL ", soci::use(vo));

        sql.begin();
        performCancel(rs);
        sql.commit();
    }
    catch (std::exception& e) {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...) {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


std::vector<std::string> MySqlAPI::getVos(void)
{
    try {
        soci::session sql(*connectionPool);
        std::vector<std::string> vos;
        soci::rowset<std::string> query = (sql.prepare << "SELECT DISTINCT vo_name FROM t_job");
        for (auto i = query.begin(); i != query.end(); ++i) {
            vos.push_back(*i);
        }
        return vos;
    }
    catch (std::exception& e) {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...) {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

}


void MySqlAPI::setToFailOldQueuedJobs(std::vector<std::string>& jobs)
{
    // Only first host takes care of this task
    if (hashSegment.start != 0)
        return;

    auto vos = getVos();
    for (auto i = vos.begin(); i != vos.end(); ++i) {
      int maxTime = getMaxTimeInQueue(*i);
      cancelExpiredJobsForVo(jobs, maxTime, *i);
    }
}


void MySqlAPI::updateProtocol(const std::vector<fts3::events::Message>& messages)
{
    soci::session sql(*connectionPool);

    uint64_t filesize = 0;
    uint64_t fileId = 0;
    std::string params;

    soci::statement stmt = (
            sql.prepare << "UPDATE t_file SET "
                           "    internal_file_params = :params, "
                           "    filesize = :filesize "
                           "WHERE file_id = :fileId",
                    soci::use(params),
                    soci::use(filesize),
                    soci::use(fileId)
    );

    try
    {
        sql.begin();

        for (const auto& message: messages) {
            if (message.transfer_status() == "UPDATE") {
                std::ostringstream internalParams;
                fileId = message.file_id();
                filesize = message.filesize();
                internalParams << "nostreams:" << static_cast<int> (message.nostreams())
                               << ",timeout:" << static_cast<int> (message.timeout())
                               << ",buffersize:" << static_cast<int> (message.buffersize());
                params = internalParams.str();
                stmt.execute(true);
            }
        }

        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::transferLogFileVector(std::map<int, fts3::events::MessageLog>& messagesLog)
{
    soci::session sql(*connectionPool);
    std::string filePath;

    //soci doesn't access bool
    unsigned int debugFile = 0;
    uint64_t fileId = 0;

    try
    {
        soci::statement stmt = (sql.prepare << " update t_file set log_file=:filePath, log_file_debug=:debugFile where file_id=:fileId ",
                                soci::use(filePath),
                                soci::use(debugFile),
                                soci::use(fileId));

        sql.begin();

        std::map<int, fts3::events::MessageLog>::iterator iterLog = messagesLog.begin();
        while (iterLog != messagesLog.end())
        {
            filePath = ((*iterLog).second).log_path();
            fileId = ((*iterLog).second).file_id();
            debugFile = ((*iterLog).second).has_debug_file();
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

        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


std::vector<TransferState> MySqlAPI::getStateOfDeleteInternal(soci::session& sql, const std::string& jobId, uint64_t fileId)
{
    TransferState ret;
    std::vector<TransferState> temp;

    try
    {
        soci::rowset<soci::row> rs = (
                sql.prepare <<
                            " SELECT "
                            "   j.user_dn, j.submit_time, j.job_id, j.job_state, j.vo_name, "
                            "   j.job_metadata, j.retry AS retry_max, f.file_id, "
                            "   f.file_state, f.retry AS retry_counter, f.user_filesize, f.file_metadata, f.reason, "
                            "   f.source_se, f.start_time, f.source_surl "
                            " FROM t_dm f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                            " WHERE "
                            "   j.job_id = :jobId AND f.file_id = :fileId",
                        soci::use(jobId),
                        soci::use(fileId)
        );

        soci::rowset<soci::row>::const_iterator it;

        for (it = rs.begin(); it != rs.end(); ++it) {
            ret.job_id = it->get<std::string>("job_id");
            ret.job_state = it->get<std::string>("job_state");
            ret.vo_name = it->get<std::string>("vo_name");

            soci::indicator isNull1 = it->get_indicator("job_metadata");

            if (isNull1 == soci::i_ok) {
                ret.job_metadata = it->get<std::string>("job_metadata", "");
            } else {
                ret.job_metadata = "";
            }

            ret.retry_max = it->get<int>("retry_max", 0);
            ret.file_id = it->get<unsigned long long>("file_id");
            ret.file_state = it->get<std::string>("file_state");
            ret.timestamp = millisecondsSinceEpoch();
            auto aux_tm = it->get<struct tm>("submit_time");
            ret.submit_time = (timegm(&aux_tm) * 1000);
            ret.staging = false;

            ret.retry_counter = it->get<int>("retry_counter", 0);

            soci::indicator isNull2 = it->get_indicator("file_metadata");

            if (isNull2 == soci::i_ok) {
                ret.file_metadata = it->get<std::string>("file_metadata", "");
            } else {
                ret.file_metadata = "";
            }

            ret.user_filesize = it->get<long long>("user_filesize", 0);

            ret.source_se = it->get<std::string>("source_se");
            ret.dest_se = "";

            bool publishUserDn = publishUserDnInternal(sql, ret.vo_name);

            if (!publishUserDn) {
                ret.user_dn = std::string("");
            } else {
                ret.user_dn = it->get<std::string>("user_dn", "");
            }

            ret.source_url = it->get<std::string>("source_surl","");
            ret.dest_url = "";

            if (ret.job_state == "FAILED") {
                ret.reason = it->get<std::string>("reason");
            } else {
                ret.reason = std::string("");
            }

            temp.push_back(ret);
        }
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return temp;
}


std::vector<TransferState> MySqlAPI::getStateOfTransferInternal(soci::session& sql, const std::string& jobId, uint64_t fileId)
{
    TransferState ret;
    std::vector<TransferState> temp;

    try
    {
        soci::rowset<soci::row> rs = (
                sql.prepare <<
                            " SELECT "
                            "  j.user_dn, j.submit_time, j.job_id, j.job_state, j.vo_name, "
                            "  j.job_metadata, j.retry AS retry_max, f.file_id, "
                            "  f.file_state, f.retry AS retry_counter, f.user_filesize, f.file_metadata, f.reason, "
                            "  f.source_se, f.dest_se, f.start_time, f.source_surl, f.dest_surl, "
                            "  f.staging_start, f.staging_finished, f.archive_start_time, f.archive_finish_time "
                            " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                            " WHERE "
                            "  j.job_id = :jobId AND f.file_id = :fileId",
                        soci::use(jobId),
                        soci::use(fileId)
        );

        soci::rowset<soci::row>::const_iterator it;

        for (it = rs.begin(); it != rs.end(); ++it)
        {
            ret.job_id = it->get<std::string>("job_id");
            ret.job_state = it->get<std::string>("job_state");
            ret.vo_name = it->get<std::string>("vo_name");
            ret.job_metadata = it->get<std::string>("job_metadata","");
            ret.retry_max = it->get<int>("retry_max",0);
            ret.user_filesize = it->get<long long>("user_filesize", 0);
            ret.file_id = it->get<unsigned long long>("file_id");
            ret.file_state = it->get<std::string>("file_state");
            ret.reason = it->get<std::string>("reason", "");
            ret.timestamp = millisecondsSinceEpoch();
            auto aux_tm = it->get<struct tm>("submit_time");
            ret.submit_time = (timegm(&aux_tm) * 1000);

            if (it->get_indicator("staging_start") == soci::i_ok) {
                aux_tm = it->get<struct tm>("staging_start");
                ret.staging_start = (timegm(&aux_tm) * 1000);
            }
            if (it->get_indicator("staging_finished") == soci::i_ok) {
                aux_tm = it->get<struct tm>("staging_finished");
                ret.staging_finished = (timegm(&aux_tm) * 1000);
            }

            if (ret.staging_start != 0) {
                ret.staging = true;
            }

            if (it->get_indicator("archive_start_time") == soci::i_ok) {
                aux_tm = it->get<struct tm>("archive_start_time");
                ret.archiving_start = (timegm(&aux_tm) * 1000);
            }
            if (it->get_indicator("archive_finish_time") == soci::i_ok) {
                aux_tm = it->get<struct tm>("archive_finish_time");
                ret.archiving_finished = (timegm(&aux_tm) * 1000);
            }

            if (ret.archiving_start != 0) {
                ret.archiving = true;
            }

            ret.retry_counter = it->get<int>("retry_counter",0);
            ret.file_metadata = it->get<std::string>("file_metadata","");
            ret.source_se = it->get<std::string>("source_se");
            ret.dest_se = it->get<std::string>("dest_se");

            bool publishUserDn = publishUserDnInternal(sql, ret.vo_name);
            if (!publishUserDn) {
                ret.user_dn = std::string("");
            } else {
                ret.user_dn = it->get<std::string>("user_dn", "");
            }

            ret.source_url = it->get<std::string>("source_surl","");
            ret.dest_url = it->get<std::string>("dest_surl","");

            temp.push_back(ret);
        }
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return temp;
}

std::vector<TransferState> MySqlAPI::getStateOfTransfer(const std::string& jobId, uint64_t fileId)
{
    soci::session sql(*connectionPool);
    std::vector<TransferState> temp;

    try
    {
        temp = getStateOfTransferInternal(sql, jobId, fileId);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return temp;
}


void MySqlAPI::setRetryTransfer(const std::string& jobId, uint64_t fileId, int retryNo,
                                const std::string& reason, const std::string& logFile, int errcode)
{
    soci::session sql(*connectionPool);

    // Expressed in secs, default delay
    const int default_retry_delay = DEFAULT_RETRY_DELAY;
    int retry_delay = 0;
    std::string job_type;
    auto ind = soci::i_ok;

    try
    {
        sql << "SELECT retry_delay, job_type FROM t_job WHERE job_id = :jobId ",
                soci::use(jobId),
                soci::into(retry_delay),
                soci::into(job_type, ind);

        sql.begin();

        if ((ind == soci::i_ok) && (job_type == "Y"))
        {
            sql << "UPDATE t_job SET "
                   "    job_state = 'ACTIVE' "
                   "WHERE job_id = :jobId AND job_type = 'Y' AND "
                   "      job_state NOT IN ('FINISHEDDIRTY', 'FAILED', 'CANCELED', 'FINISHED')",
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

        // Query for the file state in DB
        sql << "SELECT bring_online, copy_pin_lifetime FROM t_job WHERE job_id = :jobId",
                soci::use(jobId),
                soci::into(bring_online),
                soci::into(copy_pin_lifetime);

        // Staging exception: if file failed with timeout and was staged before, reset it
        if ((bring_online > 0 || copy_pin_lifetime > 0) && (errcode == ETIMEDOUT))
        {
            sql << "UPDATE t_file SET "
                   "    retry = :retryNo, current_failures = 0, file_state = 'STAGING', "
                   "    filesize = 0, internal_file_params = NULL, transfer_host = NULL, pid = NULL, "
                   "    start_time = NULL, staging_start = NULL, staging_finished = NULL "
                   "WHERE file_id = :fileId AND job_id = :jobId AND "
                   "      file_state NOT IN ('FINISHED', 'STAGING', 'SUBMITTED', 'FAILED', 'CANCELED')",
                    soci::use(retryNo),
                    soci::use(fileId),
                    soci::use(jobId);
        }
        else
        {
            sql << "UPDATE t_file SET "
                   "    retry = :retryNo, retry_timestamp = :tTime, file_state = 'SUBMITTED', "
                   "    throughput = 0, current_failures = 1, start_time = NULL, "
                   "    transfer_host = NULL, log_file = NULL, log_file_debug = NULL "
                   "WHERE file_id = :fileId AND job_id = :jobId AND "
                   "      file_state NOT IN ('FINISHED', 'SUBMITTED', 'FAILED', 'CANCELED')",
                    soci::use(retryNo),
                    soci::use(tTime),
                    soci::use(fileId),
                    soci::use(jobId);
        }

        // Keep transfer retry log
        sql << "INSERT IGNORE INTO t_file_retry_errors "
               "       (file_id, attempt, datetime, reason, transfer_host, log_file) "
               "VALUES (:fileId, :retryNo, UTC_TIMESTAMP(), :reason, :hostname, :logFile)",
                soci::use(fileId), soci::use(retryNo),
                soci::use(reason), soci::use(hostname), soci::use(logFile);

        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


void MySqlAPI::updateHeartBeat(unsigned* index, unsigned* count, unsigned* start, unsigned* end, std::string service_name)
{
    soci::session sql(*connectionPool);

    try
    {
        updateHeartBeatInternal(sql, index, count, start, end, service_name);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::updateHeartBeatInternal(soci::session& sql, unsigned* index, unsigned* count, unsigned* start, unsigned* end,
                                       const std::string& serviceName)
{
    try
    {
        auto heartBeatGraceInterval = ServerConfig::instance().get<int>("HeartBeatGraceInterval");

        // Update beat
        sql.begin();
        soci::statement stmt1 = (
                sql.prepare
                        << "INSERT INTO t_hosts (hostname, beat, service_name) "
                           "    VALUES (:host, UTC_TIMESTAMP(), :service_name) "
                           "ON DUPLICATE KEY "
                           "    UPDATE beat = UTC_TIMESTAMP()",
                        soci::use(hostname),
                        soci::use(serviceName)
        );
        stmt1.execute(true);

        // Will be replaced by a simple count later on since we get the total number of hosts
        // Total number of working instances
        //soci::statement stmt2 = (
        //                            sql.prepare << "SELECT COUNT(hostname) FROM t_hosts "
        //                            "  WHERE beat >= DATE_SUB(UTC_TIMESTAMP(), interval :grace second) and service_name = :service_name",
        //                            soci::use(heartBeatGraceInterval),
        //                            soci::use(serviceName),
        //                            soci::into(*count));
        //stmt2.execute(true);

        // This instance index
        // Mind that MySQL does not have rownum
        soci::rowset<soci::row> rsHosts = (
                sql.prepare
                        << "SELECT ("
                           "    SELECT COUNT(hostname) FROM t_hosts "
                           "    WHERE beat >= DATE_SUB(UTC_TIMESTAMP(), interval :grace second) AND service_name = :service_name"
                           ") AS totalhosts, hostname FROM t_hosts "
                           "WHERE beat >= DATE_SUB(UTC_TIMESTAMP(), interval :grace second) AND service_name = :service_name "
                           "ORDER BY hostname",
                        soci::use(heartBeatGraceInterval),
                        soci::use(serviceName),
                        soci::use(heartBeatGraceInterval),
                        soci::use(serviceName)
        );

        *count = 0;
        soci::rowset<soci::row>::const_iterator i;
        for (*index = 0, i = rsHosts.begin(); i != rsHosts.end(); ++i, ++(*index)) {
            soci::row const& row = *i;

            if (row.get<std::string>(1) == hostname) {
                *count = row.get<unsigned>(0);
                break;
            }
        }

        sql.commit();

        // Calculate start and end hash values
        unsigned segsize = UINT16_MAX / *count;
        unsigned segmod  = UINT16_MAX % *count;

        *start = segsize * (*index);
        *end   = segsize * (*index + 1) - 1;

        // Last one take over what is left
        if (*index == *count - 1)
            *end += segmod + 1;

        this->hashSegment.start = *start;
        this->hashSegment.end   = *end;

        if (hashSegment.start == 0)
        {
            // Delete old entries
            sql.begin();
            soci::statement stmt3 = (
                    sql.prepare <<
                                "DELETE FROM t_hosts "
                                "WHERE "
                                "  (beat <= DATE_SUB(UTC_TIMESTAMP(), interval 120 MINUTE) AND drain = 0) OR "
                                "  (beat <= DATE_SUB(UTC_TIMESTAMP(), interval 15 DAY))"
            );
            stmt3.execute(true);
            sql.commit();
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::updateDeletionsState(const std::vector<MinFileStatus>& delOpsStatus)
{
    soci::session sql(*connectionPool);
    try
    {
        updateDeletionsStateInternal(sql, delOpsStatus);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


static std::string int2str(uint64_t v) {
    return boost::lexical_cast<std::string>(v);
}


void MySqlAPI::updateArchivingState(const std::vector<MinFileStatus>& archivingOpStatus)
{
    soci::session sql(*connectionPool);
    try
    {
        updateArchivingStateInternal(sql, archivingOpStatus);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::setArchivingStartTime(const std::map< std::string, std::map<std::string, std::vector<uint64_t> > > &jobs)
{
    soci::session sql(*connectionPool);
    try
    {
        sql.begin();
        for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j)
        {
            const std::string &jobId = it_j->first;
            const auto &urls = it_j->second;
            std::list<std::string> fileIdsStrList;

            for (auto it_u = urls.begin(); it_u != urls.end(); ++it_u) {
                std::vector<uint64_t> subFileIds = it_u->second;
                boost::range::transform(
                        subFileIds, std::back_inserter(fileIdsStrList),
                        int2str
                );
            }

            const std::string fileIdsStr = boost::algorithm::join(fileIdsStrList, ", ");

            std::stringstream query;
            query << "update t_file set archive_start_time = UTC_TIMESTAMP() where job_id = :jobId and file_id IN (" << fileIdsStr << ")";

            sql << query.str(),
                    soci::use(jobId);
        }
        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::updateArchivingStateInternal(soci::session& sql, const std::vector<MinFileStatus>& archivingOpStatus)
{
    // Updates to ARCHIVING state always lead to terminal state
    try
    {
        sql.begin();

        for (auto i = archivingOpStatus.begin(); i < archivingOpStatus.end(); ++i) {
            sql <<
                "UPDATE t_file "
                "SET archive_finish_time = UTC_TIMESTAMP(), file_state = :fileState, reason = :reason "
                "WHERE file_id = :fileId",
                soci::use(i->state),
                soci::use(i->reason),
                soci::use(i->fileId);
        }

        sql.commit();

        Producer producer(ServerConfig::instance().get<std::string>("MessagingDirectory"));

        for (auto i = archivingOpStatus.begin(); i < archivingOpStatus.end(); ++i) {
            updateJobTransferStatusInternal(sql, i->jobId, i->state);

            // Send monitoring state message
            std::vector<TransferState> filesMsg = getStateOfTransferInternal(sql, i->jobId, i->fileId);

            if (!filesMsg.empty()) {
                for (auto it = filesMsg.begin(); it != filesMsg.end(); ++it) {
                    MsgIfce::getInstance()->SendTransferStatusChange(producer, *it);
                }
            }
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::updateStagingState(const std::vector<MinFileStatus>& stagingOpsStatus)
{
    soci::session sql(*connectionPool);
    try
    {
        updateStagingStateInternal(sql, stagingOpsStatus);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::updateBringOnlineToken(std::map< std::string, std::map<std::string, std::vector<uint64_t> > > const & jobs, std::string const & token)
{
    soci::session sql(*connectionPool);
    try
    {
        sql.begin();
        for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j)
        {
            const std::string &jobId = it_j->first;
            const auto &urls = it_j->second;
            std::list<std::string> fileIdsStrList;

            for (auto it_u = urls.begin(); it_u != urls.end(); ++it_u) {
                std::vector<uint64_t> subFileIds = it_u->second;
                boost::range::transform(
                        subFileIds, std::back_inserter(fileIdsStrList),
                        int2str
                );
            }

            const std::string fileIdsStr = boost::algorithm::join(fileIdsStrList, ", ");

            std::stringstream query;
            query << "update t_file set bringonline_token = :token where job_id = :jobId and file_id IN (" << fileIdsStr << ")";

            sql << query.str(),
                soci::use(token),
                soci::use(jobId);
        }
        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::updateDeletionsStateInternal(soci::session& sql, const std::vector<MinFileStatus>& delOpsStatus)
{
    std::vector<TransferState> filesMsg;
    std::vector<std::string> distinctJobIds;

    try
    {
        sql.begin();

        for (auto i = delOpsStatus.begin(); i < delOpsStatus.end(); ++i)
        {
            if (i->state == "STARTED")
            {
                sql <<
                    " UPDATE t_dm "
                    " SET start_time = UTC_TIMESTAMP(), dmHost=:thost, file_state='STARTED' "
                    " WHERE  "
                    "   file_id= :fileId ",
                    soci::use(hostname),
                    soci::use(i->fileId)
                    ;
            }
            else if(i->state == "FAILED")
            {
                bool shouldBeRetried = resetForRetryDelete(sql, i->fileId, i->jobId, i->retry);
                if (!shouldBeRetried)
                {
                    sql <<
                        " UPDATE t_dm "
                        " SET  job_finished=UTC_TIMESTAMP(), finish_time=UTC_TIMESTAMP(), reason = :reason, file_state = :fileState "
                        " WHERE "
                        "   file_id = :fileId ",
                        soci::use(i->reason),
                        soci::use(i->state),
                        soci::use(i->fileId)
                        ;
                }
            }
            else
            {
                sql <<
                    " UPDATE t_dm "
                    " SET  job_finished=UTC_TIMESTAMP(), finish_time=UTC_TIMESTAMP(), reason = :reason, file_state = :fileState "
                    " WHERE "
                    "   file_id = :fileId ",
                    soci::use(i->reason),
                    soci::use(i->state),
                    soci::use(i->fileId)
                    ;
            }
        }

        sql.commit();

        sql.begin();

        for (auto i = delOpsStatus.begin(); i < delOpsStatus.end(); ++i)
        {
            //prevent multiple times of updating the same job id
            if (std::find(distinctJobIds.begin(), distinctJobIds.end(), i->jobId) != distinctJobIds.end())
            {
                continue;
            }
            else
            {
                distinctJobIds.push_back(i->jobId);
            }

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
                soci::use(i->jobId)
            );

            soci::rowset<soci::row>::const_iterator iRep;
            for (iRep = rsReplica.begin(); iRep != rsReplica.end(); ++iRep)
            {
                std::string file_state = iRep->get<std::string>("file_state");
                long long countStates = iRep->get<long long>("COUNT(file_state)",0);

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
                    " job_state = 'FINISHED', job_finished = UTC_TIMESTAMP() "
                    " WHERE job_id = :jobId ", soci::use(i->jobId);
            }
            else if (totalNumOfFilesInJob == numberOfFilesFailed) // all failed / job failed
            {
                sql << " UPDATE t_job SET "
                    " job_state = 'FAILED', job_finished = UTC_TIMESTAMP(), reason='Job failed, check files for more details' "
                    " WHERE job_id = :jobId ", soci::use(i->jobId);
            }
            else if (totalNumOfFilesInJob == numberOfFilesCanceled) // all canceled / job canceled
            {
                sql << " UPDATE t_job SET "
                    " job_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), reason='Job failed, check files for more details' "
                    " WHERE job_id = :jobId ", soci::use(i->jobId);
            }
            else if (numberOfFilesStarted >= 1 &&  numberOfFilesDelete >= 1) //one file STARTED FILE/ JOB ACTIVE
            {
                std::string job_state;
                sql << "SELECT job_state from t_job where job_id=:job_id", soci::use(i->jobId), soci::into(job_state);
                if(job_state == "ACTIVE") //do not commit if already active
                {
                    //do nothings
                }
                else //set job to ACTIVE, >=1 in STARTED and there are DELETE
                {
                    sql << " UPDATE t_job SET "
                        " job_state = 'ACTIVE' "
                        " WHERE job_id = :jobId ", soci::use(i->jobId);
                }
            }
            else if(totalNumOfFilesInJob == totalInTerminal && numberOfFilesCanceled == 0 && numberOfFilesFailed > 0) //FINISHEDDIRTY CASE
            {
                sql << " UPDATE t_job SET "
                    " job_state = 'FINISHEDDIRTY', job_finished = UTC_TIMESTAMP(), reason='Job failed, check files for more details' "
                    " WHERE job_id = :jobId ", soci::use(i->jobId);
            }
            else if(totalNumOfFilesInJob == totalInTerminal && numberOfFilesCanceled >= 1) //CANCELED
            {
                sql << " UPDATE t_job SET "
                    " job_state = 'CANCELED', job_finished = UTC_TIMESTAMP(), reason='Job canceled, check files for more details' "
                    " WHERE job_id = :jobId ", soci::use(i->jobId);
            }
            else
            {
                //it should never go here, if it does it means the state machine is bad!
            }
        }
        sql.commit();

        //now send monitoring messages
        Producer producer(ServerConfig::instance().get<std::string>("MessagingDirectory"));
        for (auto i = delOpsStatus.begin(); i < delOpsStatus.end(); ++i)
        {
            //send state message
            filesMsg = getStateOfDeleteInternal(sql, i->jobId, i->fileId);
            if(!filesMsg.empty())
            {
                std::vector<TransferState>::iterator it;
                for (it = filesMsg.begin(); it != filesMsg.end(); ++it)
                {
                    TransferState tmp = (*it);
                    MsgIfce::getInstance()->SendTransferStatusChange(producer, tmp);
                }
            }
            filesMsg.clear();
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::getFilesForDeletion(std::vector<DeleteOperation>& delOps)
{
    soci::session sql(*connectionPool);
    std::vector<fts3::events::MessageBringonline> messages;
    std::vector<MinFileStatus> filesState;

    Consumer consumer(ServerConfig::instance().get<std::string>("MessagingDirectory"));

    try
    {
        int exitCode = consumer.runConsumerDeletions(messages);
        if(exitCode != 0)
        {
            char buffer[128]= {0};
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not get the status messages for staging:" << strerror_r(errno, buffer, sizeof(buffer)) << commit;
        }

        if(!messages.empty())
        {
            for (auto iterUpdater = messages.begin(); iterUpdater != messages.end(); ++iterUpdater)
            {
                //now restore messages : //file_id / state / reason / job_id / retry
                filesState.emplace_back(
                    iterUpdater->job_id(), iterUpdater->file_id(),
                    iterUpdater->transfer_status(), iterUpdater->transfer_message(), false
                );
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
            std::string source_se = r.get<std::string>("source_se","");
            std::string vo_name = r.get<std::string>("vo_name","");

            int maxValueConfig = 0;
            int currentDeleteActive = 0;
            int limit = 0;
            soci::indicator isNull = soci::i_ok;

            //check max configured
            sql <<  "SELECT concurrent_ops from t_stage_req "
                "WHERE vo_name=:vo_name and host = :endpoint and operation='delete' and concurrent_ops is NOT NULL ",
                soci::use(vo_name), soci::use(source_se), soci::into(maxValueConfig, isNull);
            if (isNull == soci::i_null || maxValueConfig <= 0) {
                sql <<  "SELECT concurrent_ops from t_stage_req "
                    "WHERE vo_name=:vo_name and host = '*' and operation='delete' and concurrent_ops is NOT NULL ",
                    soci::use(vo_name), soci::into(maxValueConfig, isNull);
            }

            //check current deletion
            sql <<  "SELECT count(*) from t_dm "
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
            
            if(limit <= 0) //no free slots
                continue;


            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT distinct f.source_se, j.user_dn "
                                             " FROM t_dm f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                             " WHERE "
                                             "  f.file_state = 'DELETE' "
                                             "  AND f.start_time IS NULL and j.job_finished is null "
                                             "  AND (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd)"
                                             "  AND f.vo_name = :vo_name AND f.source_se=:source_se ",
                                             soci::use(hashSegment.start), soci::use(hashSegment.end),
                                             soci::use(vo_name), soci::use(source_se)
                                         );

            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
            {
                soci::row const& row = *i;

                source_se = row.get<std::string>("source_se");
                std::string user_dn = row.get<std::string>("user_dn");

                soci::rowset<soci::row> rs3 = (
                                                  sql.prepare <<
                                                  " SELECT f.source_surl, f.job_id, f.file_id, "
                                                  " j.user_dn, j.cred_id "
                                                  " FROM t_dm f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                                  " WHERE  "
                                                  " f.start_time is NULL "
                                                  " AND f.file_state = 'DELETE' "
                                                  " AND (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd)"
                                                  " AND f.source_se = :source_se  "
                                                  " AND j.user_dn = :user_dn "
                                                  " AND j.vo_name = :vo_name "
                                                  " AND j.job_finished is null  ORDER BY j.submit_time LIMIT :limit ",
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
                    std::string source_url = row.get<std::string>("source_surl");
                    std::string job_id = row.get<std::string>("job_id");
                    uint64_t file_id = row.get<unsigned long long>("file_id");
                    user_dn = row.get<std::string>("user_dn");
                    std::string cred_id = row.get<std::string>("cred_id");

                    delOps.emplace_back(job_id, file_id, vo_name, user_dn, cred_id, source_url);
                    filesState.emplace_back(job_id, file_id, initState, reason, false);
                }
            }
        }

        //now update the initial state
        if(!filesState.empty())
        {
            try
            {
                updateDeletionsStateInternal(sql, filesState);
                filesState.clear();
            }
            catch(...)
            {
                Producer producer(ServerConfig::instance().get<std::string>("MessagingDirectory"));
                //save state and restore afterwards
                if(!filesState.empty())
                {
                    fts3::events::MessageBringonline msg;
                    for (auto itFind = filesState.begin(); itFind < filesState.end(); ++itFind)
                    {
                        msg.set_file_id(itFind->fileId);
                        msg.set_job_id(itFind->jobId);
                        msg.set_transfer_status(itFind->state);
                        msg.set_transfer_message(itFind->reason);

                        //store the states into fs to be restored in the next run of this function
                        producer.runProducerDeletions(msg);
                    }
                }
            }
        }
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

}


void MySqlAPI::requeueStartedDeletes()
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
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::getFilesForArchiving(std::vector<ArchivingOperation> &archivingOps)
{
    soci::session sql(*connectionPool);
    //TODO: query for credentials to be checked when integrating OIDC
    //TODO: create a view as for staging
    try {
        soci::rowset<soci::row> rs2 = (sql.prepare <<
                                                   " SELECT DISTINCT j.job_id, f.file_id, f.dest_surl, f.vo_name,  c.dn, c.dlg_id, j.archive_timeout"
                                                   " FROM t_file f "
                                                   " INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                                   " INNER JOIN t_credential c ON (j.cred_id = c.dlg_id) "
                                                   " WHERE "
                                                   "         f.file_state = 'ARCHIVING' AND "
                                                   "         f.archive_start_time IS NULL AND "
                                                   "      (hashed_id >= :hStart AND hashed_id <= :hEnd)  ",
                soci::use(hashSegment.start), soci::use(hashSegment.end)
        );

        for (auto i2 = rs2.begin(); i2 != rs2.end(); ++i2)
        {
            soci::row const& r = *i2;
            std::string job_id = r.get<std::string>("job_id");
            uint64_t file_id = r.get<unsigned long long>("file_id");
            std::string dest_surl = r.get<std::string>("dest_surl");
            std::string voname = r.get<std::string>("vo_name");
            std::string dn = r.get<std::string>("dn");
            std::string dlg_id = r.get<std::string>("dlg_id");
            int archive_timeout = r.get<int>("archive_timeout",0);
            archivingOps.emplace_back(job_id, file_id,voname,dn, dlg_id, dest_surl, 0, archive_timeout);
        }

        //TODO: add throttling
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::getFilesForQosTransition(std::vector<QosTransitionOperation> &qosTransitionOps, const std::string& qosOp, bool matchHost)
{
    soci::session sql(*connectionPool);

    try {
        std::ostringstream query;
        query << " SELECT DISTINCT j.job_id, f.file_id, f.dest_surl, j.target_qos, c.proxy FROM t_file f"
              << " INNER JOIN t_job j ON (f.job_id = j.job_id) "
              << " INNER JOIN t_credential c ON (j.cred_id = c.dlg_id) "
              << " WHERE "
              << "      f.file_state = :qosOp AND "
              << "      (hashed_id >= :hStart AND hashed_id <= :hEnd) ";

        if (matchHost) {
            query << "AND f.transfer_host = \"" << hostname << "\"";
        }

        soci::rowset<soci::row> rs2 = (sql.prepare << query.str(),
                soci::use(qosOp), soci::use(hashSegment.start), soci::use(hashSegment.end));

        for (auto i2 = rs2.begin(); i2 != rs2.end(); ++i2)
            {
                soci::row const& r = *i2;
                std::string job_id = r.get<std::string>("job_id");
                uint64_t file_id = r.get<unsigned long long>("file_id");
                std::string dest_surl = r.get<std::string>("dest_surl");
                std::string target_qos = r.get<std::string>("target_qos");
                std::string token = r.get<std::string>("proxy").substr(0, r.get<std::string>("proxy").find(":"));

                qosTransitionOps.emplace_back(job_id, file_id, dest_surl, target_qos, token);
            }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


bool MySqlAPI::updateFileStateToQosRequestSubmitted(const std::string& jobId, uint64_t fileId)
{
    soci::session sql(*connectionPool);

    try {
        std::string storedState;
        sql.begin();

        // Get the current file state
        sql << "SELECT file_state FROM t_file WHERE job_id = :jobId AND file_id = :fileId",
            soci::use(jobId),
            soci::use(fileId),
            soci::into(storedState);

        if (storedState != "QOS_TRANSITION") {
            sql.rollback();
            return false;
        }

        sql << "UPDATE t_file SET "
               "    file_state = 'QOS_REQUEST_SUBMITTED', "
               "    start_time = UTC_TIMESTAMP(), "
               "    transfer_host = :hostname "
               "WHERE job_id = :jobId AND file_id = :fileId",
               soci::use(hostname),
               soci::use(jobId),
               soci::use(fileId);
        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return true;
}


void MySqlAPI::updateFileStateToQosTerminal(const std::string& jobId, uint64_t fileId, const std::string& fileState,
                                            const std::string& reason)
{
    struct tm startTime;
    struct tm finishTime;
    double duration;
    time_t now = time(NULL);
    gmtime_r(&now, &finishTime);

    std::string transferHost;
    soci::indicator nullStartTime = soci::i_ok;
    soci::indicator nullTransferHost = soci::i_ok;
    soci::session sql(*connectionPool);

    try {
        sql.begin();

        sql << "SELECT start_time, transfer_host FROM t_file WHERE file_id = :fileId",
            soci::use(fileId),
            soci::into(startTime, nullStartTime),
            soci::into(transferHost, nullTransferHost);

        if (nullStartTime == soci::i_null) {
            startTime = finishTime;
        }

        if (nullTransferHost == soci::i_null) {
            transferHost = hostname;
        }

        duration = difftime(now, timegm(&startTime));

        sql << "UPDATE t_file SET "
               "file_state = :fileState, start_time = :startTime, "
               "finish_time = :finishTime, tx_duration = :duration, "
               "transfer_host = :transferHost "
               "WHERE file_id = :fileId",
            soci::use(fileState), soci::use(startTime), soci::use(finishTime),
            soci::use(duration), soci::use(transferHost), soci::use(fileId);

        if (fileState == "FAILED" && !reason.empty()) {
            sql << "UPDATE t_file SET reason = :reason WHERE file_id = :fileId",
                soci::use(reason), soci::use(fileId);
        }

        // Update job state:
        // - FAILED, if all files of job are FAILED
        // - FINISHED, if all files of job are FINISHED
        // - FINISHEDDIRTY, if files have different final state

        long long numberFilesInJob = 0;
        long long numberFilesFailed = 0;
        long long numberFilesFinished = 0;
        long long numberFilesTerminal = 0;
        std::string jobState;

        soci::rowset<soci::row> rsReplica = (
            sql.prepare <<
            "SELECT file_state, COUNT(file_state) FROM t_file WHERE job_id = :jobId GROUP BY file_state ORDER BY NULL",
            soci::use(jobId)
        );

        for (auto it = rsReplica.begin(); it != rsReplica.end(); ++it) {
            std::string state = it->get<std::string>("file_state");
            long long count = it->get<long long>("COUNT(file_state)", 0);

            if (state == "FINISHED") {
                numberFilesFinished = count;
            } else if (state == "FAILED") {
                numberFilesFailed = count;
            }

            numberFilesInJob += count;
        }

        numberFilesTerminal = numberFilesFinished + numberFilesFailed;

        if (numberFilesInJob == numberFilesFinished) {
            jobState = "FINISHED";
        } else if (numberFilesInJob == numberFilesFailed) {
            jobState = "FAILED";
        } else if (numberFilesInJob == numberFilesTerminal && numberFilesFailed > 0) {
            jobState = "FINISHEDDIRTY";
        }

        if (!jobState.empty()) {
            sql << "UPDATE t_job SET job_state = :jobState, job_finished = :finishTime WHERE job_id = :jobId",
                soci::use(jobState), soci::use(finishTime), soci::use(jobId);
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "[" << jobState << "] QoS Transition job: " << jobId << " finalized" << commit;
        }

        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::getFilesForStaging(std::vector<StagingOperation> &stagingOps)
{
    soci::session sql(*connectionPool);
    std::vector<fts3::events::MessageBringonline> messages;

    int maxStagingBulkSize = ServerConfig::instance().get<int>("StagingBulkSize");
    int stagingWaitingFactor = ServerConfig::instance().get<int>("StagingWaitingFactor");
    int maxStagingConcurrentRequests = ServerConfig::instance().get<int>("StagingConcurrentRequests");

    try
    {
        //now get fresh states/files from the database
        soci::rowset<soci::row> rs2 = (sql.prepare <<
            " SELECT DISTINCT vo_name, source_se "
            " FROM t_file "
            " WHERE "
            "      file_state = 'STAGING' AND "
            "      (hashed_id >= :hStart AND hashed_id <= :hEnd)  ",
            soci::use(hashSegment.start), soci::use(hashSegment.end)
        );

        for (auto i2 = rs2.begin(); i2 != rs2.end(); ++i2)
        {
            soci::row const& r = *i2;
            std::string source_se = r.get<std::string>("source_se","");
            std::string vo_name = r.get<std::string>("vo_name","");

            int maxValueConfig = 0;
            int currentStagingActive = 0;
            int limit = 0;

            //check max configured
            sql <<  "SELECT concurrent_ops FROM t_stage_req "
                "WHERE vo_name=:vo_name AND host = :endpoint AND operation='staging' AND concurrent_ops IS NOT NULL ",
                soci::use(vo_name), soci::use(source_se), soci::into(maxValueConfig);
            if (maxValueConfig <= 0) {
                sql <<  "SELECT concurrent_ops FROM t_stage_req "
                    "WHERE vo_name=:vo_name AND host = '*' AND operation='staging' AND concurrent_ops IS NOT NULL ",
                    soci::use(vo_name), soci::into(maxValueConfig);
            }

            if(maxValueConfig > 0)
            {
                //check current staging
                sql <<  "SELECT count(*) FROM t_file "
                    "WHERE vo_name=:vo_name AND source_se = :endpoint AND file_state='STARTED'",
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
                limit = maxStagingBulkSize; // Use a sensible default
            }

            // Make sure we do not grab more than the limit for a bulk
            if (limit > maxStagingBulkSize)
                limit = maxStagingBulkSize;

            //now check for max concurrent active requests, must no exceed the limit
            int countActiveRequests = 0;
            sql << " SELECT COUNT(distinct bringonline_token) FROM t_file where "
                " vo_name=:vo_name AND file_state='STARTED' AND source_se=:source_se AND bringonline_token IS NOT NULL ",
                soci::use(vo_name), soci::use(source_se), soci::into(countActiveRequests);

            if(countActiveRequests > maxStagingConcurrentRequests)
                continue;

            //now make sure there are enough files to put in a single request
            int countQueuedFiles = 0;
            sql << " SELECT COUNT(*) FROM t_file WHERE vo_name=:vo_name AND source_se=:source_se AND file_state='STAGING' ",
                soci::use(vo_name), soci::use(source_se), soci::into(countQueuedFiles);


            // If we haven't got enough for a bulk request, give some time for more
            // requests to arrive
            if(countQueuedFiles < maxStagingBulkSize)
            {
                auto now = boost::posix_time::second_clock::local_time();
                auto itQueue = queuedStagingFiles.find(source_se);

                if(itQueue != queuedStagingFiles.end())
                {
                    auto nextSubmission = itQueue->second;

                    if(nextSubmission > now) {
                        continue;
                    }
                    queuedStagingFiles.erase(itQueue);
                }
                else
                {
                    queuedStagingFiles[source_se] = now + boost::posix_time::seconds(stagingWaitingFactor);
                    continue;
                }
            }


            soci::rowset<soci::row> rs = (
                                             sql.prepare <<
                                             " SELECT DISTINCT f.source_se, j.cred_id "
                                             " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                                             " WHERE "
                                             "  f.file_state = 'STAGING' "
                                             "  AND (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd)"
                                             "  AND f.vo_name = :vo_name AND f.source_se=:source_se ",
                                             soci::use(hashSegment.start), soci::use(hashSegment.end),
                                             soci::use(vo_name), soci::use(source_se)
                                         );

            for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
            {
                soci::row const& row = *i;

                source_se = row.get<std::string>("source_se");
                std::string cred_id = row.get<std::string>("cred_id");

                soci::rowset<soci::row> rs3 = (sql.prepare <<
                    "SELECT f.source_surl, f.staging_metadata, f.job_id, f.file_id,"
                    "   j.copy_pin_lifetime, j.bring_online, j.cred_id, j.user_dn, j.source_space_token "
                    "FROM t_file f JOIN t_job j ON f.job_id = j.job_id "
                    "WHERE "
                    "   f.file_state = 'STAGING'"
                    "   AND f.hashed_id BETWEEN :hStart AND :hEnd "
                    "   AND f.source_se=:source_se "
                    "   AND j.cred_id=:cred_id "
                    "   AND j.vo_name=:vo_name "
                    "LIMIT :limit",
                    soci::use(hashSegment.start), soci::use(hashSegment.end),
                    soci::use(source_se),
                    soci::use(cred_id),
                    soci::use(vo_name),
                    soci::use(limit)
                );

                std::string initState = "STARTED";
                std::string reason;

                for (soci::rowset<soci::row>::const_iterator i3 = rs3.begin(); i3 != rs3.end(); ++i3)
                {
                    soci::row const& row = *i3;
                    std::string source_url = row.get<std::string>("source_surl");
                    std::string metadata = row.get<std::string>("staging_metadata", "");
                    std::string job_id = row.get<std::string>("job_id");
                    uint64_t file_id = row.get<unsigned long long>("file_id");
                    int copy_pin_lifetime = row.get<int>("copy_pin_lifetime",0);
                    int bring_online = row.get<int>("bring_online",0);

                    if(copy_pin_lifetime > 0 && bring_online <= 0)
                        bring_online = ServerConfig::instance().get<int>("DefaultBringOnlineTimeout");
                    else if (bring_online > 0 && copy_pin_lifetime <= 0)
                        copy_pin_lifetime = ServerConfig::instance().get<int>("DefaultCopyPinLifetime");

                    std::string user_dn = row.get<std::string>("user_dn");
                    std::string cred_id = row.get<std::string>("cred_id");
                    std::string source_space_token = row.get<std::string>("source_space_token", "");

                    stagingOps.emplace_back(
                        job_id, file_id, vo_name,
                        user_dn, cred_id, source_url, metadata,
                        copy_pin_lifetime, bring_online, 0,
                        source_space_token, std::string()
                    );
                }
            }
        }
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::getAlreadyStartedArchiving(std::vector<ArchivingOperation> &archiveOps)
{
    soci::session sql(*connectionPool);

    try
    {

        soci::rowset<soci::row> rs =
                (
                        sql.prepare <<
                        " SELECT f.vo_name, f.dest_surl, f.job_id, f.file_id, f.archive_start_time, "
                        " j.archive_timeout, j.user_dn, j.cred_id "
                        " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                        " WHERE  "
                        " j.archive_timeout >= 0  "
                        " AND f.archive_start_time IS NOT NULL "
                        " AND f.file_state = 'ARCHIVING' "
                        " AND (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd)",
                        soci::use(hashSegment.start), soci::use(hashSegment.end)
                );

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Recovering archiving tasks with hashed_id between "
                                        << hashSegment.start << " and "
                                        << hashSegment.end << commit;

        for (soci::rowset<soci::row>::const_iterator i3 = rs.begin(); i3 != rs.end(); ++i3)
        {
            soci::row const& row = *i3;
            std::string vo_name = row.get<std::string>("vo_name");
            std::string dest_url = row.get<std::string>("dest_surl");
            std::string job_id = row.get<std::string>("job_id");
            uint64_t file_id = row.get<unsigned long long>("file_id");
            int archive_timeout = row.get<int>("archive_timeout", 0);

            time_t archive_start_time = time(0);
            if (row.get_indicator("archive_start_time") == soci::i_ok) {
                struct tm start_tm = row.get<struct tm>("archive_start_time");
                archive_start_time = timegm(&start_tm);
            }

            std::string user_dn = row.get<std::string>("user_dn");
            std::string cred_id = row.get<std::string>("cred_id");
            
            archiveOps.emplace_back(
                    job_id, file_id, vo_name,
                    user_dn, cred_id, dest_url,
                    archive_start_time, archive_timeout
            );
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::getAlreadyStartedStaging(std::vector<StagingOperation> &stagingOps)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();
        sql <<
            " UPDATE t_file "
            " SET start_time = NULL, staging_start=NULL, transfer_host=NULL, file_state='STAGING' "
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
                " SELECT f.vo_name, f.source_surl, f.staging_metadata, f.job_id, f.file_id, f.staging_start, "
                " f.bringonline_token, j.copy_pin_lifetime, j.bring_online, j.user_dn, j.cred_id, j.source_space_token "
                " FROM t_file f INNER JOIN t_job j ON (f.job_id = j.job_id) "
                " WHERE  "
                " (j.BRING_ONLINE >= 0 OR j.COPY_PIN_LIFETIME >= 0) "
                " AND f.start_time IS NOT NULL "
                " AND f.file_state = 'STARTED' "
                " AND (f.hashed_id >= :hStart AND f.hashed_id <= :hEnd)",
                soci::use(hashSegment.start), soci::use(hashSegment.end)
            );

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Recovering staging tasks with hashed_id between "
                                        << hashSegment.start << " and "
                                        << hashSegment.end << commit;

        for (soci::rowset<soci::row>::const_iterator i3 = rs3.begin(); i3 != rs3.end(); ++i3)
        {
            soci::row const& row = *i3;
            std::string vo_name = row.get<std::string>("vo_name");
            std::string source_url = row.get<std::string>("source_surl");
            std::string metadata = row.get<std::string>("staging_metadata", "");
            std::string job_id = row.get<std::string>("job_id");
            uint64_t file_id = row.get<unsigned long long>("file_id");
            int copy_pin_lifetime = row.get<int>("copy_pin_lifetime",0);
            int bring_online = row.get<int>("bring_online",0);

            time_t staging_start_time = 0;
            if (row.get_indicator("staging_start") == soci::i_ok) {
                struct tm start_tm = row.get<struct tm>("staging_start");
                staging_start_time = timegm(&start_tm);
            }

            if (copy_pin_lifetime > 0 && bring_online <= 0)
                bring_online = ServerConfig::instance().get<int>("DefaultBringOnlineTimeout");
            else if (bring_online > 0 && copy_pin_lifetime <= 0)
                copy_pin_lifetime = ServerConfig::instance().get<int>("DefaultCopyPinLifetime");

            std::string user_dn = row.get<std::string>("user_dn");
            std::string cred_id = row.get<std::string>("cred_id");
            std::string source_space_token = row.get<std::string>("source_space_token","");

            std::string bringonline_token;
            soci::indicator isNull = row.get_indicator("bringonline_token");
            if (isNull == soci::i_ok) bringonline_token = row.get<std::string>("bringonline_token");

            stagingOps.emplace_back(
                job_id, file_id, vo_name,
                user_dn, cred_id, source_url, metadata,
                copy_pin_lifetime, bring_online,
                staging_start_time,
                source_space_token,
                bringonline_token
            );
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::updateStagingStateInternal(soci::session& sql, const std::vector<MinFileStatus>& stagingOpsStatus)
{
    std::vector<TransferState> filesMsg;

    try
    {

        sql.begin();

        for (auto i = stagingOpsStatus.begin(); i < stagingOpsStatus.end(); ++i)
        {
            if (i->state == "STARTED")
            {
                sql <<
                    " UPDATE t_file "
                    " SET start_time = UTC_TIMESTAMP(),staging_start=UTC_TIMESTAMP(), staging_host=:thost, "
                    "   transfer_host=:thost, file_state='STARTED' "
                    " WHERE  "
                    "   file_id= :fileId "
                    "   AND file_state='STAGING'",
                    soci::use(hostname),
                    soci::use(hostname),
                    soci::use(i->fileId)
                    ;
            }
            else if(i->state == "FAILED")
            {
                bool shouldBeRetried = i->retry;

                if(i->retry)
                {
                    int times = 0;
                    shouldBeRetried = resetForRetryStaging(sql, i->fileId, i->jobId, i->retry, times);
                    if(shouldBeRetried)
                    {
                        if(times > 0)
                        {
                            sql << "INSERT IGNORE INTO t_file_retry_errors "
                                "    (file_id, attempt, datetime, reason) "
                                "VALUES (:fileId, :attempt, UTC_TIMESTAMP(), :reason)",
                                soci::use(i->fileId), soci::use(times), soci::use(i->reason);
                        }

                        continue;
                    }
                }

                if (!i->retry || !shouldBeRetried)
                {
                    sql <<
                        " UPDATE t_file "
                        " SET finish_time=UTC_TIMESTAMP(), staging_finished=UTC_TIMESTAMP(), reason = :reason, file_state = :fileState, dest_surl_uuid = NULL "
                        " WHERE "
                        "   file_id = :fileId "
                        "   AND file_state in ('STAGING','STARTED')",
                        soci::use(i->reason),
                        soci::use(i->state),
                        soci::use(i->fileId)
                        ;
                }
            }
            else
            {
                std::string dbState;
                std::string dbReason;
                int stage_in_only = 0;

                sql << "select count(*) from t_file where file_id=:file_id and source_surl=dest_surl",
                    soci::use(i->fileId),
                    soci::into(stage_in_only);

                if(stage_in_only == 0)  //stage-in and transfer
                {
                    dbState = i->state == "FINISHED" ? "SUBMITTED" : i->state;
                    dbReason = i->state == "FINISHED" ? std::string() : i->reason;
                }
                else //stage-in only
                {
                    dbState = i->state == "FINISHED" ? "FINISHED" : i->state;
                    dbReason = i->state == "FINISHED" ? std::string() : i->reason;

                    // Find out job type
                    Job::JobType jobType;
                    sql << "SELECT job_type FROM t_job WHERE job_id = :job_id",
                        soci::use(i->jobId),
                        soci::into(jobType);

                    // Trigger next hop after stage-in only operation
                    if (dbState == "FINISHED" && jobType == Job::kTypeMultiHop) {
                        useNextHop(sql, i->jobId);
                    }
                }

                if(dbState == "SUBMITTED")
                {
                    unsigned hashedId = getHashedId();

                    sql <<
                        " UPDATE t_file "
                        " SET hashed_id = :hashed_id, staging_finished=UTC_TIMESTAMP(), finish_time=NULL, start_time=NULL, transfer_host=NULL, reason = '', file_state = :fileState "
                        " WHERE "
                        "   file_id = :fileId "
                        "   AND file_state in ('STAGING','STARTED')",
                        soci::use(hashedId),
                        soci::use(dbState),
                        soci::use(i->fileId)
                        ;
                }
                else if(dbState == "FINISHED")
                {
                    sql <<
                        " UPDATE t_file "
                        " SET staging_finished=UTC_TIMESTAMP(), finish_time=UTC_TIMESTAMP(), reason = :reason, file_state = :fileState, dest_surl_uuid = NULL "
                        " WHERE "
                        "   file_id = :fileId "
                        "   AND file_state in ('STAGING','STARTED')",
                        soci::use(dbReason),
                        soci::use(dbState),
                        soci::use(i->fileId)
                        ;
                }
                else
                {
                    sql <<
                        " UPDATE t_file "
                        " SET staging_finished=UTC_TIMESTAMP(), finish_time=UTC_TIMESTAMP(), reason = :reason, file_state = :fileState, dest_surl_uuid = NULL "
                        " WHERE "
                        "   file_id = :fileId "
                        "   AND file_state in ('STAGING','STARTED')",
                        soci::use(dbReason),
                        soci::use(dbState),
                        soci::use(i->fileId)
                        ;
                }
            }
        }
        sql.commit();

        for (auto i = stagingOpsStatus.begin(); i < stagingOpsStatus.end(); ++i)
        {
            if(i->state == "SUBMITTED")
                updateJobTransferStatusInternal(sql, i->jobId, "ACTIVE");
            else
                updateJobTransferStatusInternal(sql, i->jobId, i->state);

            //send state message
            filesMsg = getStateOfTransferInternal(sql, i->jobId, i->fileId);
            if(!filesMsg.empty())
            {
                Producer producer(ServerConfig::instance().get<std::string>("MessagingDirectory"));
                for (auto it = filesMsg.begin(); it != filesMsg.end(); ++it)
                {
                    TransferState tmp = (*it);
                    MsgIfce::getInstance()->SendTransferStatusChange(producer, tmp);
                }
            }
            filesMsg.clear();
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


//file_id / surl 
void MySqlAPI::getArchivingFilesForCanceling(std::set< std::pair<std::string, std::string> >& files)
{
    soci::session sql(*connectionPool);
    uint64_t file_id = 0;
    std::string source_surl;
    std::string job_id;

    try
    {
        soci::rowset<soci::row> rs = (sql.prepare << " SELECT job_id, file_id, source_surl from t_file WHERE "
                "  file_state='CANCELED' and archive_finish_time is NULL "
                "  AND transfer_host = :hostname AND archive_start_time is NOT NULL ",
                soci::use(hostname));

        soci::statement stmt1 = (sql.prepare << "UPDATE t_file SET archive_finish_time = UTC_TIMESTAMP() "
                "WHERE file_id = :file_id ", soci::use(file_id, "file_id"));

        // Cancel staging files
        sql.begin();
        for (soci::rowset<soci::row>::const_iterator i2 = rs.begin(); i2 != rs.end(); ++i2)
        {
            soci::row const& row = *i2;
            job_id  = row.get<std::string>("job_id", "");
            file_id = row.get<unsigned long long>("file_id",0);
            source_surl = row.get<std::string>("source_surl","");
            files.insert({job_id, source_surl});
            stmt1.execute(true);
        }
        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


//file_id / surl / token
void MySqlAPI::getStagingFilesForCanceling(std::set< std::pair<std::string, std::string> >& files)
{
    soci::session sql(*connectionPool);
    uint64_t file_id = 0;
    std::string source_surl;
    std::string token;
    std::string job_id;

    try
    {
        soci::rowset<soci::row> rs = (sql.prepare << " SELECT job_id, file_id, source_surl, bringonline_token from t_file WHERE "
                                      "  file_state='CANCELED' and staging_finished is NULL "
                                      "  AND transfer_host = :hostname  AND staging_start is NOT NULL ",
                                      soci::use(hostname));

        soci::statement stmt1 = (sql.prepare << "UPDATE t_file SET finish_time = UTC_TIMESTAMP(), staging_finished = UTC_TIMESTAMP() "
                                 "WHERE file_id = :file_id ", soci::use(file_id, "file_id"));

        // Cancel staging files
        sql.begin();
        for (soci::rowset<soci::row>::const_iterator i2 = rs.begin(); i2 != rs.end(); ++i2)
        {
            soci::row const& row = *i2;
            job_id  = row.get<std::string>("job_id", "");
            file_id = row.get<unsigned long long>("file_id",0);
            source_surl = row.get<std::string>("source_surl","");
            token = row.get<std::string>("bringonline_token","");
            boost::tuple<std::string, uint64_t, std::string, std::string> record(job_id, file_id, source_surl, token);
            files.insert({job_id, source_surl});

            stmt1.execute(true);
        }
        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


std::list<Token> MySqlAPI::getAccessTokensWithoutRefresh()
{
    soci::session sql(*connectionPool);

    try
    {
        const soci::rowset<Token> rs = (sql.prepare <<
                                " SELECT token_id, access_token, refresh_token, issuer, scope, audience "
                                " FROM t_token WHERE refresh_token IS NULL ORDER BY NULL");

        return {rs.begin(), rs.end()};
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception");
    }
}


void MySqlAPI::storeRefreshTokens(const std::set< std::pair<std::string, std::string> >& refreshTokens)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        // Prepare statement for storing refresh token
        std::string tokenId;
        std::string refreshToken;
        soci::statement stmt = (sql.prepare <<
                                            " UPDATE t_token SET refresh_token = :refreshToken "
                                            " WHERE token_id = :tokenId",
                                soci::use(refreshToken),
                                soci::use(tokenId));

        sql.begin();
        for (const auto& pair: refreshTokens) {
            tokenId = pair.first;
            refreshToken = pair.second;
            stmt.execute(true);
        }
        sql.commit();
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception");
    }
}


void MySqlAPI::failTransfersWithFailedTokenExchange(
        const std::set<std::pair<std::string, std::string> >& failedExchanges)
{
    soci::session sql(*connectionPool);

    try
    {
        for (const auto& it: failedExchanges) {
            std::string tokenId = it.first;

            const soci::rowset<soci::row> rs = (sql.prepare <<
                                        " SELECT f.job_id, f.file_id, f.src_token_id, f.dst_token_id "
                                        " FROM t_file f "
                                        " WHERE f.file_state = 'TOKEN_PREP' AND "
                                        "       (f.src_token_id = :tokenId OR f.dst_token_id = :tokenId) "
                                        " ORDER BY NULL",
                                        soci::use(tokenId));

            for (const auto& row: rs) {
                auto job_id = row.get<std::string>("job_id", "");
                auto file_id = row.get<unsigned long long>("file_id", 0);
                auto src_token_id = row.get<std::string>("src_token_id", "");
                auto dst_token_id = row.get<std::string>("src_token_id", "");

                std::string reason = "Failed to get refresh token for ";

                if (!src_token_id.empty()) {
                    reason = "source token: " + it.second;
                } else {
                    reason = "destination token: " + it.second;
                }

                updateFileTransferStatusInternal(sql, job_id, file_id, 0, "FAILED", reason, 0, 0, 0.0, false, "");
                updateJobTransferStatusInternal(sql, job_id, "FAILED");

                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Failed 'TOKEN PREP' file:"
                                                << " job_id = " << job_id << " file_id=" << file_id
                                                << " src_token_id=" << src_token_id
                                                << " dst_token_id=" << dst_token_id
                                                << " reason=" << reason
                                                << commit;

                // Send Monitoring Transfer State messages
                Producer producer(ServerConfig::instance().get<std::string>("MessagingDirectory"));
                auto transferStates = getStateOfTransferInternal(sql, job_id, file_id);

                for (const auto &state: transferStates) {
                    MsgIfce::getInstance()->SendTransferStatusChange(producer, state);
                }
            }
        }

    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception");
    }
}


void MySqlAPI::updateTokenPrepFiles()
{
    struct TokenPrepFile {
        TokenPrepFile(uint64_t file_id, const std::string& job_id,
                      const std::string& src_token_id,
                      const std::string& dst_token_id) :
              file_id(file_id), job_id(job_id),
              src_token_id(src_token_id), dst_token_id(dst_token_id) {}

        uint64_t file_id;
        std::string job_id;
        std::string src_token_id;
        std::string dst_token_id;
    };

    soci::session sql(*connectionPool);

    try
    {
        const soci::rowset<soci::row> rs = (sql.prepare <<
                                    " SELECT f.job_id, f.file_id, f.src_token_id, f.dst_token_id "
                                    " FROM t_file f "
                                    "     INNER JOIN t_token t_src ON f.src_token_id = t_src.token_id "
                                    "     INNER JOIN t_token t_dst ON f.dst_token_id = t_dst.token_id "
                                    " WHERE f.file_state = 'TOKEN_PREP' "
                                    "     AND t_src.refresh_token IS NOT NULL "
                                    "     AND t_dst.refresh_token IS NOT NULL "
                                    " ORDER BY NULL");

        // Store all data into memory structure in order to reiterate
        // (there might be a way to reset the rowset iterator
        std::list<TokenPrepFile> tokenPrepFiles;

        for (const auto& row: rs) {
            tokenPrepFiles.emplace_back(
                    row.get<unsigned long long>("file_id", 0),
                    row.get<std::string>("job_id", ""),
                    row.get<std::string>("src_token_id", ""),
                    row.get<std::string>("dst_token_id", "")
            );
        }

        // Prepare statement for "TOKEN_PREP" --> "SUBMITTED" update
        uint64_t file_id;
        soci::statement stmt = (sql.prepare <<
                                    " UPDATE t_file SET file_state = 'SUBMITTED' "
                                    " WHERE file_id = :fileId AND file_state = 'TOKEN_PREP'",
                                soci::use(file_id));

        sql.begin();
        for (const auto& file: tokenPrepFiles) {
            file_id = file.file_id;
            stmt.execute(true);

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Updated 'TOKEN PREP' file:"
                            << " job_id = " << file.job_id << " file_id=" << file.file_id
                            << " src_token_id=" << file.src_token_id
                            << " dst_token_id=" << file.dst_token_id
                            << commit;
        }
        sql.commit();

        // Send Monitoring Transfer State messages
        Producer producer(ServerConfig::instance().get<std::string>("MessagingDirectory"));

        for (const auto& file: tokenPrepFiles) {
            auto transferStates = getStateOfTransferInternal(sql, file.job_id, file.file_id);

            for (const auto &state: transferStates) {
                MsgIfce::getInstance()->SendTransferStatusChange(producer, state);
            }
        }
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception");
    }
}


bool MySqlAPI::resetForRetryStaging(soci::session& sql, uint64_t fileId, const std::string & jobId, bool retry, int& times)
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
                soci::use(jobId),
                soci::into(nRetries, isNull),
                soci::into(vo_name),
                soci::into(retry_delay)
                ;


            if (isNull == soci::i_null || nRetries <= 0)
            {
                sql <<
                    " SELECT retry "
                    " FROM t_server_config WHERE vo_name=:vo_name LIMIT 1",
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
                soci::use(fileId), soci::use(jobId), soci::into(nRetriesTimes, isNull2);




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

                    sql << "UPDATE t_file SET retry_timestamp=:1, retry = :retry, file_state = 'STAGING', start_time=NULL, staging_start=NULL, transfer_host=NULL, log_file=NULL,"
                        " log_file_debug=NULL, throughput = 0, current_failures = 1 "
                        " WHERE  file_id = :fileId AND  job_id = :jobId AND file_state NOT IN ('FINISHED','STAGING','FAILED','CANCELED')",
                        soci::use(tTime), soci::use(nRetriesTimes+1), soci::use(fileId), soci::use(jobId);

                    willBeRetried = true;

                    times = nRetriesTimes + 1;

                    sql.commit();
                }
                else
                {
                    // update
                    time_t now = getUTC(default_retry_delay);
                    struct tm tTime;
                    gmtime_r(&now, &tTime);

                    sql.begin();

                    sql << "UPDATE t_file SET retry_timestamp=:1, retry = :retry, file_state = 'STAGING', "
                        "   staging_start=NULL, start_time=NULL, transfer_Host=NULL, "
                        "   log_file=NULL, log_file_debug=NULL, throughput = 0,  current_failures = 1 "
                        " WHERE  file_id = :fileId AND  job_id = :jobId AND file_state NOT IN ('FINISHED','STAGING','FAILED','CANCELED')",
                        soci::use(tTime), soci::use(nRetriesTimes+1), soci::use(fileId), soci::use(jobId);

                    willBeRetried = true;

                    times = nRetriesTimes + 1;

                    sql.commit();
                }
            }
        }
        catch (std::exception& e)
        {
            sql.rollback();
            throw UserError(std::string(__func__) + ": Caught exception " + e.what());
        }
        catch (...)
        {
            sql.rollback();
            throw UserError(std::string(__func__) + ": Caught exception " );
        }
    }

    return willBeRetried;
}


bool MySqlAPI::resetForRetryDelete(soci::session& sql, uint64_t fileId, const std::string & jobId, bool retry)
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
                " WHERE job_id = :jobId ",
                soci::use(jobId),
                soci::into(nRetries, isNull),
                soci::into(vo_name),
                soci::into(retry_delay)
                ;

            if (isNull == soci::i_null || nRetries <= 0)
            {
                sql <<
                    " SELECT retry "
                    " FROM t_server_config where vo_name=:vo_name LIMIT 1",
                    soci::use(vo_name), soci::into(nRetries, isNull)
                    ;
            }
            else if (isNull != soci::i_null && nRetries <= 0)
            {
                nRetries = 0;
            }

            int nRetriesTimes = 0;
            soci::indicator isNull2 = soci::i_ok;

            sql << "SELECT retry FROM t_dm WHERE file_id = :fileId AND job_id = :jobId ",
                soci::use(fileId), soci::use(jobId), soci::into(nRetriesTimes, isNull2);


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
                        " WHERE  file_id = :fileId AND  job_id = :jobId AND file_state NOT IN ('FINISHED','DELETE',"
                        "'FAILED','CANCELED')",
                        soci::use(tTime), soci::use(nRetries+1), soci::use(fileId), soci::use(jobId);

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
                        " WHERE  file_id = :fileId AND  job_id = :jobId AND file_state NOT IN ('FINISHED','SUBMITTED',"
                        "'FAILED','CANCELED')",
                        soci::use(tTime), soci::use(nRetries+1), soci::use(fileId), soci::use(jobId);

                    willBeRetried = true;

                    sql.commit();
                }
            }


        }
        catch (std::exception& e)
        {
            sql.rollback();
            throw UserError(std::string(__func__) + ": Caught exception " + e.what());
        }
        catch (...)
        {
            sql.rollback();
            throw UserError(std::string(__func__) + ": Caught exception " );
        }
    }

    return willBeRetried;
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

