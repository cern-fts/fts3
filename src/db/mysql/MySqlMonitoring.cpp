#include <error.h>
#include <mysql/soci-mysql.h>
#include "MySqlMonitoring.h"
#include "sociMonitoringConversions.h"

MySqlMonitoring::MySqlMonitoring(): poolSize(8), connectionPool(poolSize) {
    memset(&notBefore, 0, sizeof(notBefore));
}



MySqlMonitoring::~MySqlMonitoring()
{
}



void MySqlMonitoring::init(const std::string& username, const std::string& password, const std::string &connectString) {
    for (size_t i = 0; i < poolSize; ++i) {
        std::ostringstream connParams;

        connParams << connectString
                   << " user='" << username << "'"
                   << " pass='" << password << "'";

        soci::session& sql = connectionPool.at(i);
        sql.open(soci::mysql, connParams.str());
    }
}



void MySqlMonitoring::setNotBefore(time_t nb) {
    gmtime_r(&nb, &notBefore);
}



void MySqlMonitoring::getVONames(std::vector<std::string>& vos) {
    soci::session sql(connectionPool);

    try {
        soci::rowset<std::string> rs = (sql.prepare << "SELECT DISTINCT(vo_name) "
                                                       "FROM t_job "
                                                       "WHERE submit_time > :notBefore",
                                                       soci::use(notBefore));
        for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
            vos.push_back(*i);
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



void MySqlMonitoring::getSourceAndDestSEForVO(const std::string& vo,
                                              std::vector<SourceAndDestSE>& pairs) {
    soci::session sql(connectionPool);

    try {
        soci::rowset<SourceAndDestSE> rs = (sql.prepare << "SELECT DISTINCT source_se, dest_se "
                                                           "FROM t_job "
                                                           "WHERE vo_name = :vo AND submit_time > :notBefore",
                                                           soci::use(vo), soci::use(notBefore));
        for (soci::rowset<SourceAndDestSE>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
            pairs.push_back(*i);
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



unsigned MySqlMonitoring::numberOfJobsInState(const SourceAndDestSE& pair,
                                              const std::string& state) {
    soci::session sql(connectionPool);

    try {
        unsigned count = 0;
        sql << "SELECT COUNT(*) FROM t_job "
               "WHERE job_state = :state AND "
               "      source_se = :source AND "
               "      dest_se   = :dest AND "
               "      submit_time > :notBefore",
               soci::use(state),
               soci::use(pair.sourceStorageElement), soci::use(pair.destinationStorageElement),
               soci::use(notBefore), soci::into(count);
        return count;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



void MySqlMonitoring::getConfigAudit(const std::string& actionLike,
                                      std::vector<ConfigAudit>& audit) {
    soci::session sql(connectionPool);

    try {
        soci::rowset<ConfigAudit> rs = (sql.prepare << "SELECT datetime, dn, config, action "
                                                       "FROM t_config_audit "
                                                       "WHERE action LIKE :like AND datetime > :notBefore",
                                                       soci::use(actionLike), soci::use(notBefore));
        for (soci::rowset<ConfigAudit>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
            audit.push_back(*i);
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



void MySqlMonitoring::getTransferFiles(const std::string& jobId,
                                       std::vector<TransferFiles>& files) {
    soci::session sql(connectionPool);

    try {
        soci::rowset<TransferFiles> rs = (sql.prepare << "SELECT t_file.*, t_job.vo_name, t_job.overwrite_flag, "
                                                         "       t_job.user_dn, t_job.cred_id, t_job.checksum_method, "
                                                         "       t_job.source_space_token, t_job.space_token "
                                                         "FROM t_file, t_job "
                                                         "WHERE t_file.job_id = :jobId AND t_file.job_id = t_job.job_id "
                                                         "ORDER BY start_time",
                                                         soci::use(jobId));
        for (soci::rowset<TransferFiles>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
            files.push_back(*i);
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



void MySqlMonitoring::getJob(const std::string& jobId, TransferJobs& job) {
    soci::session sql(connectionPool);

    try {
        sql << "SELECT * FROM t_job WHERE job_id = :jobId",
               soci::use(jobId), soci::into(job);
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



void MySqlMonitoring::filterJobs(const std::vector<std::string>& inVos,
                                 const std::vector<std::string>& inStates,
                                 std::vector<TransferJobs>& jobs) {
    soci::session sql(connectionPool);

    try {
        size_t i;
        std::ostringstream query;
        soci::statement stmt(sql);

        query << "SELECT * FROM t_job WHERE submit_time > :notBefore ";
        stmt.exchange(soci::use(notBefore));

        if (!inVos.empty()) {
            query << "AND vo_name IN (";
            for (i = 0; i < inVos.size() - 1; ++i) {
                query << ":vo" << i << ", ";
                stmt.exchange(soci::use(inVos[i]));
            }
            query << ":vo" << i << ") ";
            stmt.exchange(soci::use(inVos[i]));
        }

        if (!inStates.empty()) {
            query << "AND job_state IN (";
            for (i = 0; i < inStates.size(); ++i) {
                query << ":state" << i << ", ";
                stmt.exchange(soci::use(inStates[i]));
            }
            query << ":state" << i << ") ";
            stmt.exchange(soci::use(inStates[i]));
        }

        query << "ORDER BY submit_time DESC, finish_time DESC, job_id DESC";

        TransferJobs job;
        stmt.exchange(soci::into(job));
        stmt.alloc();
        stmt.prepare(query.str());
        stmt.define_and_bind();

        if (stmt.execute(true)) {
            do {
                jobs.push_back(job);
            } while (stmt.fetch());
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



unsigned MySqlMonitoring::numberOfTransfersInState(const std::string& vo,
                                                   const std::vector<std::string>& state) {
    soci::session sql(connectionPool);

    try {
        unsigned count = 0;
        std::ostringstream query;
        soci::statement stmt(sql);

        query << "SELECT COUNT(*) FROM t_file, t_job WHERE "
                 "    t_job.submit_time > :notBefore AND "
                 "    t_file.job_id = t_job.job_id ";
        stmt.exchange(soci::use(notBefore));

        if (!vo.empty()) {
            query << "AND t_job.vo_name = :vo ";
            stmt.exchange(soci::use(vo));
        }

        if (!state.empty()) {
            size_t i;
            query << "AND t_file.file_state IN (";
            for (i = 0; i < state.size() - 1; ++i) {
                query << ":state" << i << ", ";
                stmt.exchange(soci::use(state[i]));
            }
            query << ":state" << i << ")";
            stmt.exchange(soci::use(state[i]));
        }

        stmt.exchange(soci::into(count));
        stmt.alloc();
        stmt.prepare(query.str());
        stmt.define_and_bind();
        stmt.execute(true);
        return count;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



void MySqlMonitoring::getUniqueReasons(std::vector<ReasonOccurrences>& reasons) {
    soci::session sql(connectionPool);

    try {
        soci::rowset<ReasonOccurrences> rs = (sql.prepare << "SELECT COUNT(*) AS count, reason "
                                                             "FROM t_file WHERE reason IS NOT NULL AND "
                                                             "                  reason != '' AND "
                                                             "                  finish_time > :notBefore "
                                                             "GROUP BY reason "
                                                             "ORDER BY reason ASC",
                                                             soci::use(notBefore));
        for (soci::rowset<ReasonOccurrences>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
            reasons.push_back(*i);
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



unsigned MySqlMonitoring::averageDurationPerSePair(const SourceAndDestSE& pair) {
    soci::session sql(connectionPool);

    try {
        unsigned avg;

        sql << "SELECT AVG(tx_duration) FROM t_file WHERE "
               "    tx_duration IS NOT NULL AND finish_time > :notBefore AND "
               "    source_surl LIKE CONCAT('%', :source, '%') AND "
               "    dest_surl LIKE CONCAT('%', :dest, '%')",
               soci::use(notBefore), soci::use(pair.sourceStorageElement),
               soci::use(pair.destinationStorageElement),
               soci::into(avg);
        return avg;
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



void MySqlMonitoring::averageThroughputPerSePair(std::vector<SePairThroughput>& avgThroughput) {
    soci::session sql(connectionPool);

    try {
        soci::rowset<SePairThroughput> rs = (sql.prepare << "SELECT t_job.source_se, t_job.dest_se, "
                                                            "       AVG(t_file.tx_duration) AS duration, AVG(t_file.throughput) AS throughput "
                                                            "FROM t_file, t_job "
                                                            "WHERE t_file.throughput IS NOT NULL AND "
                                                            "      t_file.file_state = 'FINISHED' AND "
                                                            "      t_file.job_id = t_job.job_id AND "
                                                            "      t_job.submit_time > :notBefore "
                                                            "GROUP BY t_job.source_se, t_job.dest_se",
                                                            soci::use(notBefore));
        for (soci::rowset<SePairThroughput>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
            avgThroughput.push_back(*i);
        }
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}



void MySqlMonitoring::getJobVOAndSites(const std::string& jobId, JobVOAndSites& voAndSites) {
    soci::session sql(connectionPool);

    try {
        sql << "SELECT t_job.vo_name, t_channel.source_site, t_channel.dest_site "
               "FROM t_job, t_channel "
               "WHERE t_channel.channel_name = t_job.channel_name AND "
               "      t_job.job_id = :jobId",
               soci::use(jobId), soci::into(voAndSites);
    }
    catch (std::exception& e) {
        throw Err_Custom(std::string(__func__) + ": " + e.what());
    }
}
