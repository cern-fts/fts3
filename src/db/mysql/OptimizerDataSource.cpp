/*
 * Copyright (c) CERN 2013-2016
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

#include "MySqlAPI.h"
#include "db/generic/DbUtils.h"
#include "common/Exceptions.h"
#include "common/Logger.h"

using namespace db;
using namespace fts3;
using namespace fts3::common;
using namespace fts3::optimizer;

// Set the new number of actives
static void setNewOptimizerValue(soci::session &sql,
    const Pair &pair, int optimizerDecision, double ema)
{
    sql.begin();
    sql << "UPDATE t_optimize_active SET active = :active, ema = :ema, datetime = UTC_TIMESTAMP() "
        "WHERE source_se = :source AND dest_se = :dest ",
        soci::use(optimizerDecision), soci::use(ema), soci::use(pair.source), soci::use(pair.destination);
    sql.commit();
}

// Insert the optimizer decision into the historical table, so we can follow
// the progress
static void updateOptimizerEvolution(soci::session &sql,
    const Pair &pair,
    int active, double throughput, double successRate, double bandwidthLimit,
    int actualActive, int queueSize,
    int diff, const std::string &rationale)
{
    try {
        if (throughput > 0 && successRate > 0) {
            sql.begin();
            sql << " INSERT INTO t_optimizer_evolution "
                " (datetime, source_se, dest_se, "
                "  throughput_limit, active, throughput, success, "
                "  actual_active, queue_size, "
                "  rationale, diff) "
                " VALUES "
                " (UTC_TIMESTAMP(), :source, :dest, "
                "  :thr_limit, :active, :throughput, :success, "
                "  :actual_active, :queue_size, "
                "  :rationale, :diff)",
                soci::use(pair.source), soci::use(pair.destination),
                soci::use(bandwidthLimit), soci::use(active), soci::use(throughput), soci::use(successRate),
                soci::use(actualActive), soci::use(queueSize),
                soci::use(rationale), soci::use(diff);
            sql.commit();
        }
    }
    catch (std::exception &e) {
        sql.rollback();
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Could not update the optimizer evolution: " << e.what() << commit;
    }
    catch (...) {
        sql.rollback();
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Could not update the optimizer evolution: unknown reason" << commit;
    }
}


// Count how many files are in the given state for the given pair
// Only non terminal!
static int getCountInState(soci::session &sql, const Pair &pair, const std::string &state)
{
    int count = 0;

    if (pair.source.empty()) {
        sql << "SELECT count(*) FROM t_file "
            "WHERE dest_se = :dest_se AND file_state = :state AND job_finished IS NULL",
            soci::use(pair.destination), soci::use(state), soci::into(count);
    }
    else if (pair.destination.empty()) {
        sql << "SELECT count(*) FROM t_file "
            "WHERE source_se = :source AND file_state = :state AND job_finished IS NULL",
            soci::use(pair.source), soci::use(state), soci::into(count);
    }
    else {
        sql << "SELECT count(*) FROM t_file "
            "WHERE source_se = :source AND dest_se = :dest_se AND file_state = :state AND job_finished IS NULL",
            soci::use(pair.source), soci::use(pair.destination), soci::use(state), soci::into(count);
    }

    return count;
}


class MySqlOptimizerDataSource: public OptimizerDataSource {
private:
    soci::session sql;

public:
    MySqlOptimizerDataSource(soci::connection_pool* connectionPool): sql(*connectionPool)
    {
    }

    ~MySqlOptimizerDataSource() {
    }

    std::list<Pair> getActivePairs(void) {
        std::list<Pair> result;

        soci::rowset<soci::row> rs = (sql.prepare <<
            "SELECT DISTINCT o.source_se, o.dest_se "
            "FROM t_optimize_active o "
            " INNER JOIN t_file f ON (o.source_se = f.source_se) "
            " WHERE o.dest_se = f.dest_se AND "
            "     f.file_state IN ('ACTIVE','SUBMITTED') AND f.job_finished IS NULL");

        for (auto i = rs.begin(); i != rs.end(); ++i) {
            result.push_back(Pair(i->get<std::string>("source_se"), i->get<std::string>("dest_se")));
        }

        return result;
    }


    int getOptimizerMode(void) {
        extern int getOptimizerMode(soci::session &sql);
        return getOptimizerMode(sql);
    }

    bool isRetryEnabled(void) {
        int retryValue = 0;
        soci::indicator isRetryNull = soci::i_ok;
        sql << " SELECT retry FROM t_server_config",
            soci::into(retryValue, isRetryNull);
        return isRetryNull != soci::i_null && retryValue;
    }

    int getGlobalStorageLimit(void) {
        int maxPerSe = 0;
        soci::indicator isSeNull;

        sql << "SELECT max_per_se "
            "FROM t_server_config "
            "WHERE max_per_se > 0 AND"
            "   vo_name IS NULL OR vo_name = '*'",
            soci::into(maxPerSe, isSeNull);

        if (maxPerSe < 0 || isSeNull != soci::i_ok) {
            maxPerSe = 0;
        }

        return maxPerSe;
    }

    int getGlobalLinkLimit(void) {
        int linkLimit = 0;
        soci::indicator isLinkNull;

        sql << "SELECT max_per_link "
            "FROM t_server_config "
            "WHERE max_per_link > 0 AND"
            "   vo_name IS NULL OR vo_name = '*'",
            soci::into(linkLimit, isLinkNull);

        if (linkLimit < 0 || isLinkNull != soci::i_ok) {
            linkLimit = 0;
        }

        return linkLimit;
    }

    void getPairLimits(const Pair &pair, Range *range, Limits *limits) {
        soci::indicator nullIndicator;

        limits->link = limits->source = limits->destination = 0;
        limits->throughputSource = 0;
        limits->throughputDestination = 0;

        sql << "SELECT active, throughput FROM t_optimize WHERE source_se = :source_se AND active IS NOT NULL",
            soci::use(pair.source),
            soci::into(limits->source), soci::into(limits->throughputSource, nullIndicator);

        sql << "SELECT active, throughput FROM t_optimize WHERE dest_se = :dest_se AND active IS NOT NULL",
            soci::use(pair.destination),
            soci::into(limits->destination), soci::into(limits->throughputDestination, nullIndicator);

        unsigned storedActive = 0;
        std::string activeFixedFlagStr;
        soci::indicator isNullFixedFlag, isNullMin, isNullMax, isNullActive;

        sql << "SELECT fixed, active, min_active, max_active FROM t_optimize_active "
            "WHERE source_se = :source AND dest_se = :dest",
            soci::use(pair.source), soci::use(pair.destination),
            soci::into(activeFixedFlagStr, isNullFixedFlag),
            soci::into(storedActive, isNullActive),
            soci::into(range->min, isNullMin),
            soci::into(range->max, isNullMax);

        if (isNullFixedFlag != soci::i_null && activeFixedFlagStr == "on") {
            if (isNullMin == soci::i_null) {
                range->min = storedActive;
            }
            if (isNullMax == soci::i_null) {
                range->max = storedActive;
            }
        }
        else {
            range->min = range->max = 0;
        }
    }

    int getOptimizerValue(const Pair &pair) {
        soci::indicator isCurrentNull;
        int currentActive;

        sql << "SELECT active FROM t_optimize_active "
            "WHERE source_se = :source AND dest_se = :dest_se LIMIT 1",
            soci::use(pair.source),soci::use(pair.destination),
            soci::into(currentActive, isCurrentNull);

        if (isCurrentNull == soci::i_null) {
            currentActive = 0;
        }
        return currentActive;
    }

    double getWeightedThroughput(const Pair &pair, const boost::posix_time::time_duration &interval) {
        soci::rowset<soci::row> rsSizeAndThroughput = (sql.prepare <<
            "SELECT filesize, throughput "
            " FROM t_file "
            " WHERE source_se = :source AND dest_se = :dest AND "
            "       file_state  in ('ACTIVE','FINISHED') AND throughput > 0 AND "
            "       filesize > 0 AND "
            "       (job_finished IS NULL OR job_finished >= (UTC_TIMESTAMP() - INTERVAL :interval SECOND))",
        soci::use(pair.source),soci::use(pair.destination), soci::use(interval.total_seconds()));

        double accumulator = 0;
        double totalSize = 0;

        for (auto j = rsSizeAndThroughput.begin(); j != rsSizeAndThroughput.end(); ++j) {
            double filesize = j->get<double>("filesize", 0.0);
            totalSize += filesize;
            accumulator += (j->get<double>("throughput", 0.0) * filesize);
        }

        if (totalSize > 0) {
            accumulator /= totalSize;
        }

        return accumulator;
    }

    time_t getAverageDuration(const Pair &pair, const boost::posix_time::time_duration &interval) {
        double avgDuration = 0.0;
        soci::indicator isNullAvg = soci::i_ok;

        sql << "SELECT AVG(tx_duration) FROM t_file "
            " WHERE source_se = :source AND dest_se = :dest AND file_state = 'FINISHED' AND "
            "   tx_duration > 0 AND tx_duration IS NOT NULL AND "
            "   job_finished > (UTC_TIMESTAMP() - INTERVAL :interval SECOND) LIMIT 1",
            soci::use(pair.source), soci::use(pair.destination), soci::use(interval.total_seconds()),
            soci::into(avgDuration, isNullAvg);

        return avgDuration;
    }

    double getSuccessRateForPair(const Pair &pair, const boost::posix_time::time_duration &interval,
        int *retryCount) {
        soci::rowset<soci::row> rs = (isRetryEnabled() > 0) ?
        (
            sql.prepare << "SELECT file_state, retry, current_failures, reason FROM t_file "
            "WHERE "
            "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
            "      ( "
            "          (t_file.job_finished is NULL and current_failures > 0) OR "
            "          (t_file.job_finished > (UTC_TIMESTAMP() - interval :calculateTimeFrame SECOND)) "
            "      ) AND "
            "      file_state IN ('FAILED','FINISHED','SUBMITTED') ",
            soci::use(pair.source), soci::use(pair.destination), soci::use(interval.total_seconds())
        )
        :
        (
            sql.prepare << "SELECT file_state, retry, current_failures, reason FROM t_file "
            " WHERE "
            "      t_file.source_se = :source AND t_file.dest_se = :dst AND "
            "      t_file.job_finished > (UTC_TIMESTAMP() - interval :calculateTimeFrame SECOND) AND "
            "file_state <> 'NOT_USED' ",
            soci::use(pair.source), soci::use(pair.destination), soci::use(interval.total_seconds())
        );

        int nFailedLastHour = 0;
        int nFinishedLastHour = 0;

        // we need to exclude non-recoverable errors so as not to count as failures and affect efficiency
        *retryCount = 0;
        for (auto i = rs.begin(); i != rs.end(); ++i)
        {
            int retryNum = i->get<int>("retry", 0);
            int currentFailures = i->get<int>("current_failures", 0);

            std::string state = i->get<std::string>("file_state", "");
            std::string reason = i->get<std::string>("reason", "");

            //we do not want BringOnline errors to affect transfer success rate, exclude them
            bool isBringOnlineError = (reason.find("BringOnline") != std::string::npos) ||
                                      (reason.find("bring-online") != std::string::npos);

            if(state.compare("FAILED") == 0 && isBringOnlineError)
            {
                //do nothing, it's a non recoverable error so do not consider it
            }
            else if(state.compare("FAILED") == 0 && currentFailures == 0)
            {
                //do nothing, it's a non recoverable error so do not consider it
            }
            else if ( (state.compare("FAILED") == 0 ||  state.compare("SUBMITTED") == 0) && retryNum > 0)
            {
                nFailedLastHour+=1.0;
                *retryCount += retryNum;
            }
            else if(state.compare("FAILED") == 0 && currentFailures == 1)
            {
                nFailedLastHour+=1.0;
            }
            else if (state.compare("FINISHED") == 0)
            {
                nFinishedLastHour+=1.0;
            }
        }

        // round up efficiency
        if (nFinishedLastHour > 0) {
            return ceil((nFinishedLastHour * 100.0) / (nFinishedLastHour + nFailedLastHour));
        }
        else {
            return 0.0;
        }
    }

    int getActive(const Pair &pair) {
        return getCountInState(sql, pair, "ACTIVE");
    }

    int getSubmitted(const Pair &pair) {
        return getCountInState(sql, pair, "SUBMITTED");
    }

    double getThroughputAsSource(const std::string &se) {
        double throughput = 0;
        soci::indicator isNull;

        sql <<
            "SELECT SUM(throughput) FROM t_file "
            "WHERE source_se= :name AND file_state='ACTIVE' AND throughput IS NOT NULL",
            soci::use(se), soci::into(throughput, isNull);

        return throughput;
    }

    double getThroughputAsDestination(const std::string &se) {
        double throughput = 0;
        soci::indicator isNull;

        sql << "SELECT SUM(throughput) FROM t_file "
               "WHERE dest_se= :name AND file_state='ACTIVE' AND throughput IS NOT NULL",
            soci::use(se), soci::into(throughput, isNull);

        return throughput;
    }

    void storeOptimizerDecision(const Pair &pair, int activeDecision, double bandwidthLimit,
        const PairState &newState, int diff, const std::string &rationale) {

        setNewOptimizerValue(sql, pair, activeDecision, newState.ema);
        updateOptimizerEvolution(sql, pair, activeDecision, newState.ema, newState.successRate,
            bandwidthLimit, newState.activeCount, newState.queueSize, diff, rationale);
    }

    void storeOptimizerStreams(const Pair &pair, int streams) {
        sql.begin();

        sql << "INSERT INTO t_optimize_streams (source_se, dest_se, nostreams, datetime) "
               " VALUES(:source, :dest, :nostreams, UTC_TIMESTAMP()) "
               " ON DUPLICATE KEY"
               " UPDATE "
               "    nostreams = :nostreams, datetime = UTC_TIMESTAMP()",
            soci::use(pair.source, "source"), soci::use(pair.destination, "dest"),
            soci::use(streams, "nostreams");

        sql.commit();
    }
};


OptimizerDataSource *MySqlAPI::getOptimizerDataSource()
{
    return new MySqlOptimizerDataSource(connectionPool);
}
