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
#include "OracleMonitoring.h"
#include "OracleConnection.h"
#include "OracleTypeConversions.h"



OracleMonitoring::OracleMonitoring(): conn(NULL), conv(NULL) {
}


OracleMonitoring::~OracleMonitoring()
{
    if (conn) delete conn;
    if (conv) delete conv;
}


void OracleMonitoring::init(const std::string& username, const std::string& password, const std::string &connectString, int pooledConn) {
    if (!conn)
        conn = new OracleConnection(username, password, connectString, pooledConn);
    if (!conv)
        conv = new OracleTypeConversions();

    if (conn && conv)
        notBefore = conv->toTimestamp(0, conn->getEnv());
}



void OracleMonitoring::setNotBefore(time_t nb) {
    notBefore = conv->toTimestamp(nb, conn->getEnv());
}



void OracleMonitoring::getVONames(std::vector<std::string>& vos) {
    const std::string tag   = "getVONames";
    const std::string query = "SELECT DISTINCT(VO_NAME)\
                               FROM T_JOB\
                               WHERE SUBMIT_TIME > :notBefore";
			       
    oracle::occi::Connection* pooledConnection = NULL;
    pooledConnection = conn->getPooledConnection();    			       


    oracle::occi::Statement* s = conn->createStatement(query, tag, pooledConnection);
    s->setTimestamp(1, notBefore);

    oracle::occi::ResultSet* r = conn->createResultset(s, pooledConnection);

    while (r->next()) {
        vos.push_back(r->getString(1));
    }

    conn->destroyResultset(s, r);
    conn->destroyStatement(s, tag, pooledConnection);
    conn->releasePooledConnection(pooledConnection);                    
}



void OracleMonitoring::getSourceAndDestSEForVO(const std::string& vo,
                                               std::vector<SourceAndDestSE>& pairs) {
    const std::string tag   = "getSourceAndDestSEForVO";
    const std::string query = "SELECT DISTINCT SOURCE_SE, DEST_SE\
                               FROM T_JOB\
                               WHERE VO_NAME = :vo AND SUBMIT_TIME > :notBefore";
    oracle::occi::Connection* pooledConnection = NULL;
    pooledConnection = conn->getPooledConnection();    


    oracle::occi::Statement* s = conn->createStatement(query, tag, pooledConnection);
    s->setString(1, vo);
    s->setTimestamp(2, notBefore);

    oracle::occi::ResultSet* r = conn->createResultset(s, pooledConnection);

    while (r->next()) {
        SourceAndDestSE pair;
        pair.sourceStorageElement      = r->getString(1);
        pair.destinationStorageElement = r->getString(2);
        pairs.push_back(pair);
    }

    conn->destroyResultset(s, r);
    conn->destroyStatement(s, tag, pooledConnection);
    conn->releasePooledConnection(pooledConnection);                    
}



unsigned OracleMonitoring::numberOfJobsInState(const SourceAndDestSE& pair,
                                                 const std::string& state) {
    const std::string tag   = "numberOfJobsWithState";
    const std::string query = "SELECT COUNT(*) FROM T_JOB\
                               WHERE JOB_STATE = :state AND\
                                     SOURCE_SE = :source AND\
                                     DEST_SE   = :dest AND\
                                     SUBMIT_TIME > :notBefore";
    oracle::occi::Connection* pooledConnection = NULL;
    pooledConnection = conn->getPooledConnection();    


    oracle::occi::Statement* s = conn->createStatement(query, tag, pooledConnection);
    s->setString(1, state);
    s->setString(2, pair.sourceStorageElement);
    s->setString(3, pair.destinationStorageElement);
    s->setTimestamp(4, notBefore);

    oracle::occi::ResultSet* r = conn->createResultset(s, pooledConnection);

    unsigned count = 0;
    if (r->next())
        count = r->getNumber(1);

    conn->destroyResultset(s, r);
    conn->destroyStatement(s, tag, pooledConnection);
    conn->releasePooledConnection(pooledConnection);                    

    return count;
}



void OracleMonitoring::getConfigAudit(const std::string& actionLike,
                                      std::vector<ConfigAudit>& audit) {
    const std::string tag   = "getConfigAudit";
    const std::string query = "SELECT WHEN, DN, CONFIG, ACTION\
                               FROM T_CONFIG_AUDIT\
                               WHERE ACTION LIKE :like AND\
                                     WHEN > :notBefore";
    oracle::occi::Connection* pooledConnection = NULL;
    pooledConnection = conn->getPooledConnection();    


    oracle::occi::Statement* s = conn->createStatement(query, tag, pooledConnection);
    s->setString(1, actionLike);
    s->setTimestamp(2, notBefore);

    oracle::occi::ResultSet* r = conn->createResultset(s, pooledConnection);
    while (r->next()) {
        ConfigAudit item;
        item.when = conv->toTimeT(s->getTimestamp(1));
        item.userDN = s->getString(2);
        item.config = s->getString(3);
        item.action = s->getString(4);
        audit.push_back(item);
    }

    conn->destroyResultset(s, r);
    conn->destroyStatement(s, tag, pooledConnection);
    conn->releasePooledConnection(pooledConnection);                    
}



void OracleMonitoring::getTransferFiles(const std::string& jobId,
                                        std::vector<TransferFiles>& files) {
    const std::string tag   = "getTransferFiles";
    const std::string query = "SELECT INTERNAL_FILE_PARAMS, JOB_ID, SOURCE_SURL, DEST_SURL,\
                                     FILE_STATE, FILESIZE, REASON, START_TIME, FINISH_TIME,\
                                     THROUGHPUT, CHECKSUM, TX_DURATION\
                               FROM T_FILE\
                               WHERE JOB_ID = :jobId\
                               ORDER BY SYS_EXTRACT_UTC(t_file.start_time)";
    oracle::occi::Connection* pooledConnection = NULL;
    pooledConnection = conn->getPooledConnection();    


    oracle::occi::Statement* s = conn->createStatement(query, tag, pooledConnection);
    s->setString(1, jobId);

    oracle::occi::ResultSet* r = conn->createResultset(s, pooledConnection);
    while (r->next()) {
        TransferFiles tf;
        tf.INTERNAL_FILE_PARAMS = r->getString(1);
        tf.JOB_ID = r->getString(2);
        tf.SOURCE_SURL = r->getString(3);
        tf.DEST_SURL = r->getString(4);
        tf.FILE_STATE = r->getString(5);
        tf.FILESIZE = r->getString(6);
        tf.REASON = r->getString(7);
        //tf.START_TIME = r->getString(8);
        tf.FINISH_TIME = r->getString(9);
        //tf.THROUGHPUT = r->getNumber(10);
        tf.CHECKSUM = r->getString(11);
        //tf.TX_DURATION = r->getNumber(12);
        files.push_back(tf);
    }

    conn->destroyResultset(s, r);
    conn->destroyStatement(s, tag, pooledConnection);
    conn->releasePooledConnection(pooledConnection);                    
}



void OracleMonitoring::getJob(const std::string& jobId, TransferJobs& job) {
    const std::string tag   = "getJob";
    const std::string query = "SELECT USER_DN, JOB_ID, VO_NAME,\
                                     JOB_STATE, SUBMIT_TIME, FINISH_TIME, REASON,\
                                     SOURCE_SPACE_TOKEN, SPACE_TOKEN\
                               FROM T_JOB\
                               WHERE JOB_ID = :jobId";
    oracle::occi::Connection* pooledConnection = NULL;
    pooledConnection = conn->getPooledConnection();    


    oracle::occi::Statement* s = conn->createStatement(query, tag, pooledConnection);
    s->setString(1, jobId);

    oracle::occi::ResultSet* r = conn->createResultset(s, pooledConnection);
    if (r->next()) {
        job.USER_DN = r->getString(1);
        job.JOB_ID = r->getString(2);
        job.VO_NAME = r->getString(3);
        job.JOB_STATE = r->getString(6);
        job.SUBMIT_TIME = conv->toTimeT(r->getTimestamp(7));
        job.FINISH_TIME = conv->toTimeT(r->getTimestamp(8));
        job.REASON = r->getString(9);
    }

    conn->destroyResultset(s, r);
    conn->destroyStatement(s, tag, pooledConnection);
    conn->releasePooledConnection(pooledConnection);                    
}



void OracleMonitoring::filterJobs(const std::vector<std::string>& inVos,
                                  const std::vector<std::string>& inStates,
                                  std::vector<TransferJobs>& jobs) {
    std::ostringstream tag;
    std::ostringstream query;
    size_t i, j;
    oracle::occi::Connection* pooledConnection = NULL; 
    pooledConnection = conn->getPooledConnection();       

    tag << "filterJobs" << inVos.size() << "-" << inStates.size();

    query << "SELECT USER_DN, JOB_ID, VO_NAME, JOB_STATE, "
             "       SUBMIT_TIME, FINISH_TIME, SOURCE_SPACE_TOKEN, SPACE_TOKEN "
             "FROM T_JOB "
             "WHERE SUBMIT_TIME > :notBefore";

    if (!inVos.empty()) {
        query << " AND VO_NAME IN (";
        for (i = 1; i < inVos.size(); ++i)
            query << ":vo" << i << ", ";
        query << ":vo" << i << ")";
    }

    if (!inStates.empty()) {
        query << " AND JOB_STATE IN (";
        for (i = 1; i < inStates.size(); ++i)
            query << ":state" << i << ", ";
        query << ":state" << i << ")";
    }

    query << " ORDER BY SUBMIT_TIME DESC, FINISH_TIME DESC, JOB_ID DESC";



    oracle::occi::Statement* s = conn->createStatement(query.str(), tag.str(), pooledConnection);
    s->setTimestamp(1, notBefore);
    for (i = 0; i < inVos.size(); ++i)
        s->setString((unsigned int)i + 2, inVos[i]);
    for (j = 0; j < inStates.size(); ++j)
        s->setString((unsigned int)i + (unsigned int)j + 2, inStates[j]);

    oracle::occi::ResultSet* r = conn->createResultset(s, pooledConnection);
    while (r->next()) {
        TransferJobs job;
        job.USER_DN = r->getString(1);
        job.JOB_ID  = r->getString(2);
        job.VO_NAME = r->getString(3);
        job.JOB_STATE = r->getString(6);
        job.SUBMIT_TIME = conv->toTimeT(r->getTimestamp(7));
        job.FINISH_TIME = conv->toTimeT(r->getTimestamp(8));
        job.SOURCE_SPACE_TOKEN = r->getString(9);
        job.SPACE_TOKEN = r->getString(10);
        jobs.push_back(job);
    }

    conn->destroyResultset(s, r);
    conn->destroyStatement(s, tag.str(), pooledConnection);
    conn->releasePooledConnection(pooledConnection);                    
}



unsigned OracleMonitoring::numberOfTransfersInState(const std::string& vo,
                                                    const std::vector<std::string>& state) {
    std::ostringstream tag;
    std::ostringstream query;
    size_t i;
    oracle::occi::Connection* pooledConnection = NULL;  
    pooledConnection = conn->getPooledConnection();      

    if (state.size() == 0)
        return 0;

    tag << "numberOfTransfersInState" << state.size() << vo.empty();
    query << "SELECT COUNT(*) FROM T_FILE, T_JOB "
             "WHERE  T_JOB.SUBMIT_TIME > :notBefore AND "
             "T_FILE.JOB_ID = T_JOB.JOB_ID AND "
             "T_FILE.FILE_STATE IN (";
    for (i = 1; i < state.size(); ++i)
        query << ":state" << i << ", ";
    query << ":state" << i << ") ";

    if (!vo.empty())
        query << " AND T_JOB.VO_NAME = :vo";



    oracle::occi::Statement* s = conn->createStatement(query.str(), tag.str(), pooledConnection);
    s->setTimestamp(1, notBefore);

    for (i = 0; i < state.size(); ++i)
        s->setString((unsigned int)i + 2, state[i]);

    if (!vo.empty())
        s->setString((unsigned int)i + 2, vo);

    oracle::occi::ResultSet* r = conn->createResultset(s, pooledConnection);
    unsigned count = 0;
    if (r->next())
        count = r->getNumber(1);

    conn->destroyResultset(s, r);
    conn->destroyStatement(s, tag.str(), pooledConnection);
    conn->releasePooledConnection(pooledConnection);                    

    return count;
}



unsigned OracleMonitoring::numberOfTransfersInState(const std::string& vo,
                                                    const SourceAndDestSE& pair,
                                                    const std::vector<std::string>& state)
{
    std::ostringstream tag;
    std::ostringstream query;
    size_t i;
    oracle::occi::Connection* pooledConnection = NULL; 
    pooledConnection = conn->getPooledConnection();       

    if (state.size() == 0)
        return 0;

    tag << "numberOfTransfersInStatePair" << state.size();
    query << "SELECT COUNT(*) FROM T_FILE, T_JOB "
             "WHERE  T_JOB.SUBMIT_TIME > :notBefore AND "
             "T_FILE.JOB_ID = T_JOB.JOB_ID AND "
             "T_FILE.FILE_STATE IN (";
    for (i = 1; i < state.size(); ++i)
        query << ":state" << i << ", ";
    query << ":state" << i << ") ";

    query << " AND T_JOB.SOURCE_SE = :src AND T_JOB.DEST_SE = :dest ";
    if (!vo.empty())
        query << "AND T_JOB.VO_NAME = :vo";



    oracle::occi::Statement* s = conn->createStatement(query.str(), tag.str(), pooledConnection);
    s->setTimestamp(1, notBefore);

    for (i = 0; i < state.size(); ++i)
        s->setString((unsigned int)i + 2, state[i]);

    s->setString((unsigned int)i + 2, pair.sourceStorageElement);
    s->setString((unsigned int)i + 3, pair.destinationStorageElement);

    if (!vo.empty())
        s->setString((unsigned int)i + 4, vo);

    oracle::occi::ResultSet* r = conn->createResultset(s, pooledConnection);
    unsigned count = 0;
    if (r->next())
        count = r->getNumber(1);

    conn->destroyResultset(s, r);
    conn->destroyStatement(s, tag.str(), pooledConnection);
    conn->releasePooledConnection(pooledConnection);                    

    return count;
}



void OracleMonitoring::getUniqueReasons(std::vector<ReasonOccurrences>& reasons) {
    const std::string tag   = "getUniqueReasons";
    const std::string query = "SELECT COUNT(*), REASON\
                               FROM T_FILE\
                               WHERE REASON IS NOT NULL AND\
                                     FINISH_TIME > :notBefore\
                               GROUP BY REASON\
                               ORDER BY REASON ASC";
    oracle::occi::Connection* pooledConnection = NULL;
    pooledConnection = conn->getPooledConnection();    


    oracle::occi::Statement* s = conn->createStatement(query, tag, pooledConnection);
    s->setTimestamp(1, notBefore);

    oracle::occi::ResultSet* r = conn->createResultset(s, pooledConnection);
    while (r->next()) {
        ReasonOccurrences reason;
        reason.count = r->getNumber(1);
        reason.reason = r->getString(2);
        reasons.push_back(reason);
    }

    conn->destroyResultset(s, r);
    conn->destroyStatement(s, tag, pooledConnection);
}



unsigned OracleMonitoring::averageDurationPerSePair(const SourceAndDestSE& pair) {
    const std::string tag   = "averageDurationPerSePair";
    const std::string query = "SELECT AVG(TX_DURATION)\
                               FROM T_FILE\
                               WHERE TX_DURATION IS NOT NULL AND\
                                     FINISH_TIME > :notBefore AND\
                                     SOURCE_SURL LIKE CONCAT('%', :source, '%') AND\
                                     DEST_SURL LIKE CONCAT('%', :dest, '%')";
    oracle::occi::Connection* pooledConnection = NULL;
    pooledConnection = conn->getPooledConnection();    


    oracle::occi::Statement* s = conn->createStatement(query, tag, pooledConnection);
    s->setTimestamp(1, notBefore);
    s->setString(2, pair.sourceStorageElement);
    s->setString(3, pair.destinationStorageElement);

    oracle::occi::ResultSet* r = conn->createResultset(s, pooledConnection);
    unsigned duration = 0;
    if (r->next())
        duration = r->getNumber(0);

    conn->destroyResultset(s, r);
    conn->destroyStatement(s, tag, pooledConnection);
    conn->releasePooledConnection(pooledConnection);                    

    return duration;
}



void OracleMonitoring::averageThroughputPerSePair(std::vector<SePairThroughput>& avgThroughput) {
    const std::string tag   = "averageThroughputPerSePair";
    const std::string query = "SELECT T_JOB.SOURCE_SE, T_JOB.DEST_SE, AVG(TX_DURATION), AVG(THROUGHPUT)\
                               FROM T_FILE, T_JOB\
                               WHERE T_FILE.THROUGHPUT IS NOT NULL AND\
                                     T_FILE.FILE_STATE = 'FINISHED' AND\
                                     T_FILE.JOB_ID = T_JOB.JOB_ID AND\
                                     T_JOB.SUBMIT_TIME > :notBefore\
                               GROUP BY T_JOB.SOURCE_SE, T_JOB.DEST_SE";
    oracle::occi::Connection* pooledConnection = NULL;
    pooledConnection = conn->getPooledConnection();        


    oracle::occi::Statement* s = conn->createStatement(query, tag, pooledConnection);
    s->setTimestamp(1, notBefore);

    oracle::occi::ResultSet* r = conn->createResultset(s, pooledConnection);
    while (r->next()) {
        SePairThroughput pairThroughput;
        pairThroughput.storageElements.sourceStorageElement = r->getString(1);
        pairThroughput.storageElements.destinationStorageElement = r->getString(2);
        pairThroughput.duration = r->getNumber(3);
        pairThroughput.averageThroughput = r->getNumber(4);

        avgThroughput.push_back(pairThroughput);
    }

    conn->destroyResultset(s, r);
    conn->destroyStatement(s, tag, pooledConnection);
    conn->releasePooledConnection(pooledConnection);                    
}



void OracleMonitoring::getJobVOAndSites(const std::string& jobId, JobVOAndSites& voAndSites) {
    const std::string tag   = "getJobVOAndSites";
    const std::string query = "SELECT T_JOB.VO_NAME, T_CHANNEL.SOURCE_SITE, T_CHANNEL.DEST_SITE\
                               FROM T_JOB, T_CHANNEL\
                               WHERE T_CHANNEL.CHANNEL_NAME = T_JOB.CHANNEL_NAME AND\
                                     T_JOB.JOB_ID = :jobId";
    oracle::occi::Connection* pooledConnection = NULL;
    pooledConnection = conn->getPooledConnection();        


    oracle::occi::Statement* s = conn->createStatement(query, tag, pooledConnection);
    s->setString(1, jobId);

    oracle::occi::ResultSet* r = conn->createResultset(s, pooledConnection);
    if (r->next()) {
        voAndSites.vo = r->getString(1);
        voAndSites.sourceSite = r->getString(2);
        voAndSites.destinationSite = r->getString(3);
    }

    conn->destroyResultset(s, r);
    conn->destroyStatement(s, tag, pooledConnection);
    conn->releasePooledConnection(pooledConnection);                    
}
