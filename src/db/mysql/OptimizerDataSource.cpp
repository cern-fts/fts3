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

#include "server/services/optimizer/Optimizer.h"


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

    // Function reads in all values from the t_se table, which specify
    //   - inbound and outbound throughput from every SE
    //   - inbound and outbound maximum number of connections from every SE.
    // Additionally, the instantaneous throughput is also computed. 
    // Returns: A map from SE name (string) --> StorageState (both limits and actual throughput values).
    void getStorageStates(std::map<std::string, StorageState> *result) {

        // First read in global default values:
        int outActiveGlobal = 0, inActiveGlobal = 0;
        double outTputGlobal = 0.0, inTputGlobal = 0.0;
        soci::indicator nullOutActive, nullInActive;
        soci::indicator nullOutTput, nullInTput;

        sql <<
            "SELECT inbound_max_active, inbound_max_throughput, "
            "outbound_max_active, outbound_max_throughput "
            "FROM t_se WHERE storage = '*' ",
            soci::into(inActiveGlobal, nullInActive), soci::into(inTputGlobal, nullInTput),
            soci::into(outActiveGlobal, nullOutActive), soci::into(outTputGlobal, nullOutTput);

        (*result)["*"] = StorageState(inActiveGlobal, inTputGlobal, outActiveGlobal, outTputGlobal);

        // We then fill in the table for every SE
        soci::rowset<soci::row> rs = (sql.prepare <<
            "SELECT DISTINCT storage, inbound_max_active, inbound_max_throughput, "
            "outbound_max_active, outbound_max_throughput "
            "FROM t_se WHERE storage != '*'"
        );

        soci::indicator ind;
        for (auto i = rs.begin(); i != rs.end(); ++i) { //For each row in the table, load all values into an SEState object and store in the map "result"
            std::string se = i->get<std::string>("storage"); //indexed with the name of the storage element

            StorageState SEState;

            SEState.outboundMaxActive = i->get<int>("outbound_max_active", ind);
            if (ind == soci::i_null) {
                SEState.outboundMaxActive = 0;
                if (nullOutActive != soci::i_null) {
                    SEState.outboundMaxActive = outActiveGlobal;
                }
            }

            SEState.outboundMaxThroughput = i->get<double>("outbound_max_throughput", ind);
            if (ind == soci::i_null) {
                SEState.outboundMaxThroughput = 0;
                if (nullOutTput != soci::i_null) {
                    SEState.outboundMaxThroughput = outTputGlobal;
                }
            }

            SEState.inboundMaxActive = i->get<int>("inbound_max_active");
            if (ind == soci::i_null) {
                SEState.inboundMaxActive = 0;
                if (nullInActive != soci::i_null) {
                    SEState.inboundMaxActive = inActiveGlobal;
                }
            }

            SEState.inboundMaxThroughput = i->get<double>("inbound_max_throughput");
            if (ind == soci::i_null) {
                SEState.inboundMaxThroughput = 0;
                if (nullInTput != soci::i_null) {
                    SEState.inboundMaxThroughput = inTputGlobal;
                }
            }            

            // Queries database to get current instantaneous throughput value.
            if (SEState.outboundMaxThroughput > 0) {
                SEState.asSourceThroughputInst = getThroughputAsSourceInst(se);
            }
            if (SEState.inboundMaxThroughput > 0) { 
                SEState.asDestThroughputInst = getThroughputAsDestinationInst(se);                
            }

            (*result)[se] = SEState;
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "inbound max throughput for " << se 
                                             << ": " << SEState.inboundMaxThroughput << commit;
        }
    }    


    // Function reads in all values from the t_netlink_config and t_netlink_stat table which specify
    //   - throughput of every link 
    //   - maximum number of connections of every link (IN PROGRESS)
    // Additionally, the instantaneous throughput is also computed. 
    // Returns: A map from SE name (string) --> LinkState (both limits and actual throughput values).
    void getNetLinkStates(std::map<std::string, NetLinkState> *result) {

        // Netlink limits
        soci::indicator nullCapacity;

        // First read in global default values:
        int ActiveGlobal = 0;
        double TputGlobal = 0.0;
        soci::indicator nullActive;
        soci::indicator nullTput;

        sql <<
            "SELECT DISTINCT nc.max_active, nc.max_throughput, ns.capacity "
            "FROM t_netlink_stat ns "
            "JOIN t_netlink_config nc ON ns.head_ip = nc.head_ip AND ns.tail_ip = nc.tail_ip "
            "WHERE ns.netlink_id != '*'", 
            soci::into(ActiveGlobal, nullActive), soci::into(TputGlobal, nullTput);

        (*result)["*"] = NetLinkState(ActiveGlobal, TputGlobal);

        // We then fill in the table for every link
        soci::rowset<soci::row> rs = (sql.prepare <<
            "SELECT DISTINCT ns.netlink_id, nc.min_active, nc.max_active, nc.max_throughput, ns.capacity "
            "FROM t_netlink_stat ns "
            "JOIN t_netlink_config nc ON ns.head_ip = nc.head_ip AND ns.tail_ip = nc.tail_ip "
            "WHERE ns.netlink_id != '*'"
        );

        soci::indicator ind;
        for (auto i = rs.begin(); i != rs.end(); ++i) { //For each row in the table, load all values into a linkState object and store in the map "result"
            std::string link = i->get<std::string>("netlink_id"); //indexed with the name of the link

            NetLinkState netLinkState;

            netLinkState.minActive = i->get<int>("min_active", ind);
            if (ind == soci::i_null) {
                netLinkState.minActive = 0;
            }

            netLinkState.maxActive = i->get<int>("max_active", ind);
            if (ind == soci::i_null) {
                netLinkState.maxActive = ActiveGlobal; // todo 
            }

            netLinkState.maxThroughput = i->get<double>("max_throughput", ind);
            if (ind == soci::i_null) {
                // if t_netlink_config 'max_throughput' is not set, try t_netlink_stat 'capacity' which is observed by ALTO
                netLinkState.maxThroughput = i->get<double>("capacity", nullCapacity);
                if (nullCapacity == soci::i_null) {
                    // if capacity is null, set to 0  
                    netLinkState.maxThroughput = TputGlobal; //todo 
                }
            }

            // Queries database to get current instantaneous throughput value.
            if (netLinkState.maxThroughput > 0) {
                netLinkState.throughputInst = getThroughputOverNetLinkInst(link);
            }

            (*result)[link] = netLinkState;
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "inbound max throughput for " << link
                                            << ": " << netLinkState.maxThroughput << commit;
        }
    }  

    OptimizerMode getOptimizerMode(const std::string &source, const std::string &dest) {
        return getOptimizerModeInner(sql, source, dest);
    }


    // Writes to range object for a specific pair.
    void getPairLimits(const Pair &pair, Range *range) {
        
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

        int64_t totalBytes = 0;
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

            // Not finish information
            if (endtm.tm_year <= 0) {
                periodInWindow = now - std::max(start, windowStart);
                long duration = now - start;
                if (duration > 0) {
                    bytesInWindow = double(transferred / duration) * periodInWindow;
                }
            }
            // Finished
            else {
                periodInWindow = end - std::max(start, windowStart);
                long duration = end - start;
                if (duration > 0 && filesize > 0) {
                    bytesInWindow = double(filesize / duration) * periodInWindow;
                }
                else if (duration <= 0) {
                    bytesInWindow = filesize;
                }
            }

            totalBytes += bytesInWindow;
            if (filesize > 0) {
                filesizes.push_back(filesize);
            }
        }

        *throughput = totalBytes / interval.total_seconds();
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Throughput for " << pair << ": " << *throughput << commit;

        // Statistics on the file size
        if (!filesizes.empty()) {
            for (auto i = filesizes.begin(); i != filesizes.end(); ++i) {
                *filesizeAvg += *i;
            }
            *filesizeAvg /= filesizes.size();

            double deviations = 0.0;
            for (auto i = filesizes.begin(); i != filesizes.end(); ++i) {
                deviations += pow(*filesizeAvg - *i, 2);

            }
            *filesizeStdDev = sqrt(deviations / filesizes.size());
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

        return avgDuration;
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

    std::list<std::string> getNetLinks(const Pair &pair) {
        std::list<std::string> netLinks;

        soci::rowset<std::string> rs = (sql.prepare <<
            "SELECT ns.netlink_id "
            "FROM t_netlink_config nc "
            "JOIN t_netlink_stat ns ON nc.head_ip = ns.head_ip AND nc.tail_ip = ns.tail_ip "
            "JOIN t_netlink_trace nt ON nt.netlink = ns.netlink_id "
            "WHERE nt.source_se = :source AND nt.dest_se = :dest",
            soci::use(pair.source), soci::use(pair.destination)
        );

        for (const std::string &netLink : rs) {
            netLinks.push_back(netLink);
        }

        return netLinks;
    }

    double getThroughputAsSourceInst(const std::string &se) {
        // Estimation of total outbound throughput.
        double throughput = 0;
        soci::indicator isNull;

        sql <<
            "SELECT SUM(throughput) FROM t_file "
            "WHERE source_se= :name AND file_state='ACTIVE' AND throughput IS NOT NULL",
            soci::use(se), soci::into(throughput, isNull);
        
        return throughput; //Returns value in MB/s
    }    

    double getThroughputAsDestinationInst(const std::string &se) {
        // Estimation of total inbound throughput.
        double throughput = 0;
        soci::indicator isNull;

        sql << "SELECT SUM(throughput) FROM t_file "
               "WHERE dest_se= :name AND file_state='ACTIVE' AND throughput IS NOT NULL",
            soci::use(se), soci::into(throughput, isNull);

        return throughput;
    }

    double getThroughputOverNetLinkInst(const std::string &netLink) {
        double throughput = 0;
        soci::indicator isNull;

        sql << "SELECT SUM(f.throughput) FROM t_file f, t_netlink_trace nt "
               "WHERE f.source_se = nt.source_se AND f.dest_se = nt.dest_se AND nt.netlink = :netlink "
               " AND f.file_state = 'ACTIVE' AND f.throughput IS NOT NULL ",
            soci::use(netLink), soci::into(throughput, isNull);

        return throughput;
    }

    // Stores value in both the snapshot database t_optimizer as well as t_optimizer_evolution
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