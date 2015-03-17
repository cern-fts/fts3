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

#include <error.h>
#include <oracle/soci-oracle.h>
#include "OracleMonitoring.h"
#include "sociMonitoringConversions.h"

OracleMonitoring::OracleMonitoring(): poolSize(2), connectionPool(NULL)
{
    memset(&notBefore, 0, sizeof(notBefore));
}



OracleMonitoring::~OracleMonitoring()
{
}



void OracleMonitoring::init(const std::string& username, const std::string& password, const std::string &connectString, int pooledConn)
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



void OracleMonitoring::setNotBefore(time_t nb)
{
    gmtime_r(&nb, &notBefore);
}



void OracleMonitoring::getVONames(std::vector<std::string>& vos)
{
    soci::session sql(*connectionPool);

    try
        {
            struct message_sanity msg;
            msg.msgCron = true;
            CleanUpSanityChecks temp(this, sql, msg);
            if(!temp.getCleanUpSanityCheck())
                return;

            soci::rowset<std::string> rs = (sql.prepare << "SELECT DISTINCT(vo_name) "
                                            "FROM t_job "
                                            "WHERE job_finished is NULL ");
            for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    vos.push_back(*i);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
        }
}



void OracleMonitoring::getSourceAndDestSEForVO(const std::string& vo,
        std::vector<SourceAndDestSE>& pairs)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::rowset<SourceAndDestSE> rs = (sql.prepare << "SELECT DISTINCT source_se, dest_se "
                                                "FROM t_job "
                                                "WHERE vo_name = :vo AND "
                                                "      job_finished is NULL ",
                                                soci::use(vo) );
            for (soci::rowset<SourceAndDestSE>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    pairs.push_back(*i);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
        }
}



unsigned OracleMonitoring::numberOfJobsInState(const SourceAndDestSE& pair,
        const std::string& state)
{
    soci::session sql(*connectionPool);

    try
        {
            unsigned count = 0;
            sql << "SELECT COUNT(*) FROM t_job "
                "WHERE job_state = :state AND "
                "      source_se = :source AND "
                "      dest_se   = :dest AND "
                "      (job_finished > :notBefore OR job_finished IS NULL)",
                soci::use(state),
                soci::use(pair.sourceStorageElement), soci::use(pair.destinationStorageElement),
                soci::use(notBefore), soci::into(count);
            return count;
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
        }
}



void OracleMonitoring::getConfigAudit(const std::string& actionLike,
                                      std::vector<ConfigAudit>& audit)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::rowset<ConfigAudit> rs = (sql.prepare << "SELECT datetime, dn, config, action "
                                            "FROM t_config_audit "
                                            "WHERE action LIKE :like AND datetime > :notBefore",
                                            soci::use(actionLike), soci::use(notBefore));
            for (soci::rowset<ConfigAudit>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    audit.push_back(*i);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
        }
}



void OracleMonitoring::getTransferFiles(const std::string& jobId,
                                        std::vector<TransferFiles>& files)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::rowset<TransferFiles> rs = (sql.prepare << "SELECT t_file.*, t_job.vo_name, t_job.overwrite_flag, "
                                              "       t_job.user_dn, t_job.cred_id, t_job.checksum_method, "
                                              "       t_job.source_space_token, t_job.space_token "
                                              "FROM t_file, t_job "
                                              "WHERE t_file.job_id = :jobId AND t_file.job_id = t_job.job_id "
                                              "ORDER BY start_time",
                                              soci::use(jobId));
            for (soci::rowset<TransferFiles>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    files.push_back(*i);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
        }
}



void OracleMonitoring::getJob(const std::string& jobId, TransferJobs& job)
{
    soci::session sql(*connectionPool);

    try
        {
            sql << "SELECT * FROM t_job WHERE job_id = :jobId",
                soci::use(jobId), soci::into(job);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
        }
}



void OracleMonitoring::filterJobs(const std::vector<std::string>& inVos,
                                  const std::vector<std::string>& inStates,
                                  std::vector<TransferJobs>& jobs)
{
    soci::session sql(*connectionPool);

    try
        {
            size_t i;
            std::ostringstream query;
            soci::statement stmt(sql);

            query << "SELECT * FROM t_job WHERE (finish_time > :notBefore OR finish_time IS NULL) ";
            stmt.exchange(soci::use(notBefore));

            if (!inVos.empty())
                {
                    query << "AND vo_name IN (";
                    for (i = 0; i < inVos.size() - 1; ++i)
                        {
                            query << ":vo" << i << ", ";
                            stmt.exchange(soci::use(inVos[i]));
                        }
                    query << ":vo" << i << ") ";
                    stmt.exchange(soci::use(inVos[i]));
                }

            if (!inStates.empty())
                {
                    query << "AND job_state IN (";
                    for (i = 0; i < inStates.size() - 1; ++i)
                        {
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

            if (stmt.execute(true))
                {
                    do
                        {
                            jobs.push_back(job);
                        }
                    while (stmt.fetch());
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
        }
}



unsigned OracleMonitoring::numberOfTransfersInState(const std::string& vo,
        const std::vector<std::string>& state)
{
    soci::session sql(*connectionPool);

    try
        {
            unsigned count = 0;
            std::ostringstream query;
            soci::statement stmt(sql);

            if (!vo.empty())
                {
                    query << "SELECT COUNT(*) FROM t_file WHERE "
                          " vo_name = :vo ";
                    stmt.exchange(soci::use(vo));
                }
            else
                {
                    query << " SELECT COUNT(*) FROM t_file WHERE "
                          "    (job_finished_time > :notBefore OR job_finished IS NULL) ";
                    stmt.exchange(soci::use(notBefore));
                }

            if (!state.empty())
                {
                    size_t i;
                    query << "AND t_file.file_state IN (";
                    for (i = 0; i < state.size() - 1; ++i)
                        {
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
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
        }
}



unsigned  OracleMonitoring::numberOfTransfersInState(const std::string& vo,
        const SourceAndDestSE& pair,
        const std::vector<std::string>& state)
{
    soci::session sql(*connectionPool);

    try
        {
            unsigned count = 0;
            std::ostringstream query;
            soci::statement stmt(sql);

            query << "SELECT COUNT(*) FROM t_file WHERE "
                  "   source_se = :src AND dest_se = :dest ";

            stmt.exchange(soci::use(pair.sourceStorageElement));
            stmt.exchange(soci::use(pair.destinationStorageElement));

            if (!vo.empty())
                {
                    query << " AND vo_name = :vo ";
                    stmt.exchange(soci::use(vo));
                }

            if (!state.empty())
                {
                    size_t i;
                    query << "AND file_state IN (";
                    for (i = 0; i < state.size() - 1; ++i)
                        {
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
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
        }
}



void OracleMonitoring::getUniqueReasons(std::vector<ReasonOccurrences>& reasons)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::rowset<ReasonOccurrences> rs = (sql.prepare << "SELECT COUNT(*) AS count, reason "
                                                  "FROM t_file WHERE reason IS NOT NULL AND "
                                                  "                  reason != '' AND "
                                                  "                  finish_time > :notBefore "
                                                  "GROUP BY reason "
                                                  "ORDER BY count DESC",
                                                  soci::use(notBefore));
            for (soci::rowset<ReasonOccurrences>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    reasons.push_back(*i);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
        }
}



unsigned OracleMonitoring::averageDurationPerSePair(const SourceAndDestSE& pair)
{
    soci::session sql(*connectionPool);

    try
        {
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
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
        }
}



void OracleMonitoring::averageThroughputPerSePair(std::vector<SePairThroughput>& avgThroughput)
{
    soci::session sql(*connectionPool);

    try
        {
            soci::rowset<SePairThroughput> rs = (sql.prepare << "SELECT t_job.source_se, t_job.dest_se, "
                                                 "       AVG(t_file.tx_duration) AS duration, AVG(t_file.throughput) AS throughput "
                                                 "FROM t_file, t_job "
                                                 "WHERE t_file.throughput IS NOT NULL AND "
                                                 "      t_file.file_state = 'FINISHED' AND "
                                                 "      t_file.job_id = t_job.job_id AND "
                                                 "      t_job.finish_time > :notBefore "
                                                 "GROUP BY t_job.source_se, t_job.dest_se",
                                                 soci::use(notBefore));
            for (soci::rowset<SePairThroughput>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    avgThroughput.push_back(*i);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
        }
}



void OracleMonitoring::getJobVOAndSites(const std::string& jobId, JobVOAndSites& voAndSites)
{
    soci::session sql(*connectionPool);

    try
        {
            sql << "SELECT t_job.vo_name, t_channel.source_site, t_channel.dest_site "
                "FROM t_job, t_channel "
                "WHERE t_channel.channel_name = t_job.channel_name AND "
                "      t_job.job_id = :jobId",
                soci::use(jobId), soci::into(voAndSites);
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": " + e.what());
        }
}


bool OracleMonitoring::assignSanityRuns(soci::session& sql, struct message_sanity &msg)
{

    long long rows = 0;

    try
        {
            if(msg.msgCron)
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

    return false;
}


void OracleMonitoring::resetSanityRuns(soci::session& sql, struct message_sanity &msg)
{
    try
        {
            sql.begin();
            if(msg.msgCron)
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
}

