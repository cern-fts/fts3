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

#include <numeric>
#include "MySqlAPI.h"
#include "db/generic/DbUtils.h"
#include "common/Exceptions.h"
#include "common/Logger.h"
#include "sociConversions.h"

using namespace db;
using namespace fts3;
using namespace fts3::common;
using namespace fts3::optimizer;

// Set the new number of actives
static void setNewOptimizerValue(soci::session &sql,
    const Pair &pair, int optimizerDecision, double ema)
{
    sql.begin();
    sql <<
        "INSERT INTO t_optimizer (source_se, dest_se, active, ema, datetime) "
        "VALUES (:source, :dest, :active, :ema, UTC_TIMESTAMP()) "
        "ON DUPLICATE KEY UPDATE "
        "   active = :active, ema = :ema, datetime = UTC_TIMESTAMP()",
        soci::use(pair.source, "source"), soci::use(pair.destination, "dest"),
        soci::use(optimizerDecision, "active"), soci::use(ema, "ema");
    sql.commit();
}

// Insert the optimizer decision into the historical table, so we can follow
// the progress
static void updateOptimizerEvolution(soci::session &sql,
    const Pair &pair, int active, int diff, const std::string &rationale, const PairState &newState)
{
    try {
        sql.begin();
        sql << " INSERT INTO t_optimizer_evolution "
            " (datetime, source_se, dest_se, "
            "  ema, active, throughput, success, "
            "  filesize_avg, filesize_stddev, "
            "  actual_active, queue_size, "
            "  rationale, diff) "
            " VALUES "
            " (UTC_TIMESTAMP(), :source, :dest, "
            "  :ema, :active, :throughput, :success, "
            "  :filesize_avg, :filesize_stddev, "
            "  :actual_active, :queue_size, "
            "  :rationale, :diff)",
            soci::use(pair.source), soci::use(pair.destination),
            soci::use(newState.ema), soci::use(active), soci::use(newState.throughput), soci::use(newState.successRate),
            soci::use(newState.filesizeAvg), soci::use(newState.filesizeStdDev),
            soci::use(newState.activeCount), soci::use(newState.queueSize),
            soci::use(rationale), soci::use(diff);
        sql.commit();
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

    sql << "SELECT count(*) FROM t_file "
    "WHERE source_se = :source AND dest_se = :dest_se AND file_state = :state",
    soci::use(pair.source), soci::use(pair.destination), soci::use(state), soci::into(count);

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
            "SELECT DISTINCT source_se, dest_se "
            "FROM t_file "
            "WHERE file_state IN ('ACTIVE', 'SUBMITTED') "
            "GROUP BY source_se, dest_se, file_state "
            "ORDER BY NULL"
        );

        for (auto i = rs.begin(); i != rs.end(); ++i) {
            result.push_back(Pair(i->get<std::string>("source_se"), i->get<std::string>("dest_se")));
        }

        return result;
    }


    OptimizerMode getOptimizerMode(const std::string &source, const std::string &dest) {
        return getOptimizerModeInner(sql, source, dest);
    }

    void getPairLimits(const Pair &pair, Range *range, StorageLimits *limits) {
        soci::indicator nullIndicator;

        limits->source = limits->destination = 0;
        limits->throughputSource = 0;
        limits->throughputDestination = 0;

        // Storage limits
        sql <<
            "SELECT outbound_max_throughput, outbound_max_active FROM ("
            "   SELECT outbound_max_throughput, outbound_max_active FROM t_se WHERE storage = :source UNION "
            "   SELECT outbound_max_throughput, outbound_max_active FROM t_se WHERE storage = '*' "
            ") AS se LIMIT 1",
            soci::use(pair.source),
            soci::into(limits->throughputSource, nullIndicator), soci::into(limits->source, nullIndicator);

        sql <<
            "SELECT inbound_max_throughput, inbound_max_active FROM ("
            "   SELECT inbound_max_throughput, inbound_max_active FROM t_se WHERE storage = :dest UNION "
            "   SELECT inbound_max_throughput, inbound_max_active FROM t_se WHERE storage = '*' "
            ") AS se LIMIT 1",
        soci::use(pair.destination),
        soci::into(limits->throughputDestination, nullIndicator), soci::into(limits->destination, nullIndicator);

        // Link working range
        soci::indicator isNullMin, isNullMax;
        sql <<
            "SELECT configured, min_active, max_active FROM ("
            "   SELECT 1 AS configured, min_active, max_active FROM t_link_config WHERE source_se = :source AND dest_se = :dest UNION "
            "   SELECT 1 AS configured, min_active, max_active FROM t_link_config WHERE source_se = :source AND dest_se = '*' UNION "
            "   SELECT 1 AS configured, min_active, max_active FROM t_link_config WHERE source_se = '*' AND dest_se = :dest UNION "
            "   SELECT 0 AS configured, min_active, max_active FROM t_link_config WHERE source_se = '*' AND dest_se = '*' "
            ") AS lc LIMIT 1",
            soci::use(pair.source, "source"), soci::use(pair.destination, "dest"),
            soci::into(range->specific), soci::into(range->min, isNullMin), soci::into(range->max, isNullMax);

        if (isNullMin == soci::i_null || isNullMax == soci::i_null) {
            range->min = range->max = 0;
        }
    }

    int getOptimizerValue(const Pair &pair) {
        soci::indicator isCurrentNull;
        int currentActive = 0;

        sql << "SELECT active FROM t_optimizer "
            "WHERE source_se = :source AND dest_se = :dest_se",
            soci::use(pair.source),soci::use(pair.destination),
            soci::into(currentActive, isCurrentNull);

        if (isCurrentNull == soci::i_null) {
            currentActive = 0;
        }
        return currentActive;
    }

    void getThroughputInfo(const Pair &pair, const boost::posix_time::time_duration &interval,
        double *throughput, double *filesizeAvg, double *filesizeStdDev)
    {
        static struct tm nulltm = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        *throughput = *filesizeAvg = *filesizeStdDev = 0;

        time_t now = time(NULL);
        time_t windowStart = now - interval.total_seconds();

        soci::rowset<soci::row> transfers = (sql.prepare <<
        "SELECT start_time, finish_time, transferred, filesize "
        " FROM t_file "
        " WHERE "
        "   source_se = :sourceSe AND dest_se = :destSe AND file_state = 'ACTIVE' "
        "UNION ALL "
        "SELECT start_time, finish_time, transferred, filesize "
        " FROM t_file USE INDEX(idx_finish_time)"
        " WHERE "
        "   source_se = :sourceSe AND dest_se = :destSe "
        "   AND file_state IN ('FINISHED', 'ARCHIVING') AND finish_time >= (UTC_TIMESTAMP() - INTERVAL :interval SECOND)",
        soci::use(pair.source, "sourceSe"), soci::use(pair.destination, "destSe"),
        soci::use(interval.total_seconds(), "interval"));

        double totalBytes = 0;
        std::vector<int64_t> filesizes;

        for (auto j = transfers.begin(); j != transfers.end(); ++j) {
            auto transferred = j->get<long long>("transferred", 0.0);
            auto filesize = j->get<long long>("filesize", 0.0);
            auto starttm = j->get<struct tm>("start_time");
            auto endtm = j->get<struct tm>("finish_time", nulltm);

            time_t start = timegm(&starttm);
            time_t end = timegm(&endtm);
            time_t periodInWindow = 0;
            double bytesInWindow = 0;

            // Note: the calculations here disregard the variable types and
            //       resort to using double in most places

            // Not finish information
            if (endtm.tm_year <= 0) {
                periodInWindow = now - std::max(start, windowStart);
                long duration = now - start;
                if (duration > 0) {
                    bytesInWindow = double(transferred / duration) * (double) periodInWindow;
                }
            }
            // Finished
            else {
                periodInWindow = end - std::max(start, windowStart);
                long duration = end - start;
                if (duration > 0 && filesize > 0) {
                    bytesInWindow = double(filesize / duration) * (double) periodInWindow;
                }
                else if (duration <= 0) {
                    bytesInWindow = (double) filesize;
                }
            }

            totalBytes += bytesInWindow;
            if (filesize > 0) {
                filesizes.push_back(filesize);
            }
        }

        *throughput = totalBytes / interval.total_seconds();
        // Statistics on the file size
        if (!filesizes.empty()) {
            for (auto& filesize: filesizes) {
                *filesizeAvg += (double) filesize;
            }
            *filesizeAvg /= (double) filesizes.size();

            double deviations = 0.0;
            for (auto& filesize: filesizes) {
                deviations += pow(*filesizeAvg - (double) filesize, 2);

            }
            *filesizeStdDev = sqrt(deviations / (double) filesizes.size());
        }
    }

    time_t getAverageDuration(const Pair &pair, const boost::posix_time::time_duration &interval) {
        double avgDuration = 0.0;
        soci::indicator isNullAvg = soci::i_ok;

        sql << "SELECT AVG(tx_duration) FROM t_file USE INDEX(idx_finish_time)"
            " WHERE source_se = :source AND dest_se = :dest AND file_state IN ('FINISHED', 'ARCHIVING') AND "
            "   tx_duration > 0 AND tx_duration IS NOT NULL AND "
            "   finish_time > (UTC_TIMESTAMP() - INTERVAL :interval SECOND) LIMIT 1",
            soci::use(pair.source), soci::use(pair.destination), soci::use(interval.total_seconds()),
            soci::into(avgDuration, isNullAvg);

        return static_cast<time_t>(avgDuration);
    }

    double getSuccessRateForPair(const Pair &pair, const boost::posix_time::time_duration &interval,
        int *retryCount) {
        soci::rowset<soci::row> rs = (sql.prepare <<
            "SELECT file_state, retry, current_failures AS recoverable FROM t_file USE INDEX(idx_finish_time)"
            " WHERE "
            "      source_se = :source AND dest_se = :dst AND "
            "      finish_time > (UTC_TIMESTAMP() - interval :calculateTimeFrame SECOND) AND "
            "file_state <> 'NOT_USED' ",
            soci::use(pair.source), soci::use(pair.destination), soci::use(interval.total_seconds())
        );

        int nFailedLastHour = 0;
        int nFinishedLastHour = 0;

        // we need to exclude non-recoverable errors so as not to count as failures and affect efficiency
        *retryCount = 0;
        for (auto i = rs.begin(); i != rs.end(); ++i)
        {
            const int retryNum = i->get<int>("retry", 0);
            const bool isRecoverable = i->get<bool>("recoverable", false);
            const std::string state = i->get<std::string>("file_state", "");

            // Recoverable FAILED
            if (state == "FAILED" && isRecoverable) {
                ++nFailedLastHour;
            }
            // Submitted, with a retry set
            else if (state == "SUBMITTED" && retryNum) {
                ++nFailedLastHour;
                *retryCount += retryNum;
            }
            // FINISHED
            else if (state == "FINISHED" || state == "ARCHIVING") {
                ++nFinishedLastHour;
            }
        }

        // Round up efficiency
        int nTotal = nFinishedLastHour + nFailedLastHour;
        if (nTotal > 0) {
            return ceil((nFinishedLastHour * 100.0) / nTotal);
        }
        // If there are no terminal, use 100% success rate rather than 0 to avoid
        // the optimizer stepping back
        else {
            return 100.0;
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

    void storeOptimizerDecision(const Pair &pair, int activeDecision,
        const PairState &newState, int diff, const std::string &rationale) {

        setNewOptimizerValue(sql, pair, activeDecision, newState.ema);
        updateOptimizerEvolution(sql, pair, activeDecision, diff, rationale, newState);
    }

    void storeOptimizerStreams(const Pair &pair, int streams) {
        sql.begin();

        sql << "UPDATE t_optimizer "
               "SET nostreams = :nostreams, datetime = UTC_TIMESTAMP() "
               "WHERE source_se = :source AND dest_se = :dest",
            soci::use(pair.source, "source"), soci::use(pair.destination, "dest"),
            soci::use(streams, "nostreams");

        sql.commit();
    }
};


OptimizerDataSource *MySqlAPI::getOptimizerDataSource()
{
    return new MySqlOptimizerDataSource(connectionPool);
}
