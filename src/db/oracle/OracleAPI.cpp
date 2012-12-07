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

using namespace FTS3_COMMON_NAMESPACE;


static double convertBtoM( double byte,  double duration) {
    return ceil((((byte / duration) / 1024) / 1024) * 100 + 0.5) / 100;
}


static int extractTimeout(std::string & str) {
    size_t found;
    found = str.find("timeout:");
    if (found != std::string::npos) {
        str = str.substr(found, str.length());
        size_t found2;
        found2 = str.find(",buffersize:");
        if (found2 != std::string::npos) {
            str = str.substr(0, found2);
            str = str.substr(8, str.length());
            return atoi(str.c_str());
        }

    }
    return 0;
}

/*
static double round(double d)
{
  return floor(d + 0.5);
}
*/

OracleAPI::OracleAPI() : conn(NULL),  conn2(NULL), conv(NULL)  {
}

OracleAPI::~OracleAPI() {
    if (conn)
        delete conn;
    if (conn2)
        delete conn2;	
    if (conv)
        delete conv;
}

void OracleAPI::init(std::string username, std::string password, std::string connectString) {
    if (!conn)
        conn = new OracleConnection(username, password, connectString);   	
    if (!conn2)
        conn2 = new OracleConnection(username, password, connectString);   		
    if (!conv)
        conv = new OracleTypeConversions();
}

bool OracleAPI::getInOutOfSe(const std::string & sourceSe, const std::string & destSe) {
    const std::string tagse = "getInOutOfSese";
    std::string query_stmt_se = " SELECT count(*) from t_se where "
            " (t_se.name=:1 or t_se.name=:2) "
            " and t_se.state='off' ";
    
    bool processSe = true;    
    oracle::occi::Statement* s_se = NULL;
    oracle::occi::ResultSet* rSe = NULL;
    
    try {
        s_se = conn->createStatement(query_stmt_se, tagse);
        s_se->setString(1, sourceSe);
        s_se->setString(2, destSe);
        rSe = conn->createResultset(s_se);
        if (rSe->next()) {
            int count = rSe->getInt(1);
            if (count > 0) {
                processSe = false;               
            }
        }

    conn->destroyResultset(s_se, rSe);
    conn->destroyStatement(s_se, tagse);
    
    return processSe;

    } catch (oracle::occi::SQLException const &e) {
		if(conn) {
			conn->rollback();
			if(s_se && rSe)
				conn->destroyResultset(s_se, rSe);
			if (s_se)
				conn->destroyStatement(s_se, tagse);
		}
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
    
    return processSe;
}

TransferJobs* OracleAPI::getTransferJob(std::string jobId) {

	std::string tag = "getTransferJob";
	std::string stmt =
			"SELECT "
			" 	t_job.vo_name,  "
			" 	t_job.user_dn "
			" FROM t_job"
			" WHERE t_job.job_id = :1"
			;


	oracle::occi::Statement* s = 0;
    oracle::occi::ResultSet* r = 0;

    TransferJobs* job = 0;

    ThreadTraits::LOCK_R lock(_mutex);

    try {

        if (!conn->checkConn()) return job;

		s = conn->createStatement(stmt, tag);
		s->setString(1, jobId);
		r = conn->createResultset(s);

		if (r->next()) {
			job = new TransferJobs();
			job->VO_NAME = r->getString(1);
			job->USER_DN = r->getString(2);
		}

        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {

    	if(conn) {
			conn->rollback();
			if(s && r)
				conn->destroyResultset(s, r);
			if (s)
				conn->destroyStatement(s, tag);
		}
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

	return job;
}

void OracleAPI::getSubmittedJobs(std::vector<TransferJobs*>& jobs, const std::string & vos) {
    TransferJobs* tr_jobs = NULL;
    std::string tag = "getSubmittedJobs";
    std::string tag1 = "bringdistinct";
    std::string query_stmt(""); 
    std::multimap<std::string, std::string> sePairs;
    std::string bring_distinct = " SELECT distinct t_job.source_se, t_job.dest_se  FROM t_job "
    				 " WHERE t_job.job_finished is NULL AND t_job.CANCEL_JOB is NULL "
				 " AND (t_job.reuse_job='N' or t_job.reuse_job is NULL)  "
				 " AND t_job.job_state in('ACTIVE', 'READY','SUBMITTED') and "
				 " exists(SELECT NULL FROM t_file WHERE t_file.job_id = t_job.job_id AND "
				 " t_file.file_state = 'SUBMITTED')";
    
    if(vos.length()==0){
    query_stmt = "SELECT /* FIRST_ROWS(5) */"
            " t_job.job_id, "
            " t_job.job_state, "
            " t_job.vo_name,  "
            " t_job.priority,  "
            " t_job.source, "
            " t_job.dest,  "
            " t_job.agent_dn, "
            " t_job.submit_host, "
            " t_job.source_se, "
            " t_job.dest_se, "
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
            " FROM t_job"
            " WHERE "
            " t_job.job_finished is NULL"
            " AND t_job.CANCEL_JOB is NULL"
	    " AND t_job.source_se=:1 and t_job.dest_se=:2 "
            " AND (t_job.reuse_job='N' or t_job.reuse_job is NULL) "
            " AND t_job.job_state in ('ACTIVE', 'READY','SUBMITTED') "
	    " AND exists(SELECT NULL FROM t_file WHERE t_file.job_id = t_job.job_id AND t_file.file_state = 'SUBMITTED') "
            " AND rownum <=5  ORDER BY t_job.priority DESC"
            " , SYS_EXTRACT_UTC(t_job.submit_time)";
    }else{
    tag +="1";
    query_stmt = "SELECT /* FIRST_ROWS(5) */"
            " t_job.job_id, "
            " t_job.job_state, "
            " t_job.vo_name,  "
            " t_job.priority,  "
            " t_job.source, "
            " t_job.dest,  "
            " t_job.agent_dn, "
            " t_job.submit_host, "
            " t_job.source_se, "
            " t_job.dest_se, "
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
            " FROM t_job"
            " WHERE "
            " t_job.job_finished is NULL"
            " AND t_job.CANCEL_JOB is NULL"
	    " AND t_job.source_se=:1 and t_job.dest_se=:2 "	    
	    " AND t_job.VO_NAME IN " + vos +
            " AND (t_job.reuse_job='N' or t_job.reuse_job is NULL) "
            " AND t_job.job_state in ('ACTIVE', 'READY','SUBMITTED') "
	    " AND exists(SELECT NULL FROM t_file WHERE t_file.job_id = t_job.job_id AND t_file.file_state = 'SUBMITTED') "	    
            " AND rownum <=25  ORDER BY t_job.priority DESC"
            " , SYS_EXTRACT_UTC(t_job.submit_time)";    
    }	    

    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    oracle::occi::Statement* s1 = NULL;
    oracle::occi::ResultSet* r1 = NULL;    
    ThreadTraits::LOCK_R lock(_mutex);
    try {
    
        if ( false == conn->checkConn() )
		return;

        s1 = conn->createStatement(bring_distinct, tag1);	
	r1 = conn->createResultset(s1);
        while (r1->next()) {		
		sePairs.insert(std::make_pair<std::string, std::string>(r1->getString(1),r1->getString(2)));
	}
        conn->destroyResultset(s1, r1);
        conn->destroyStatement(s1, tag1);
	s1=NULL;
	r1=NULL;

        s = conn->createStatement(query_stmt, tag);	
	s->setPrefetchRowCount(500);
       for ( std::multimap< std::string, std::string>::const_iterator iter = sePairs.begin(); iter != sePairs.end(); ++iter ){
	s->setString(1,iter->first);	
	s->setString(2,iter->second);		
	r = conn->createResultset(s);
        while (r->next()) {
            tr_jobs = new TransferJobs();
            tr_jobs->JOB_ID = r->getString(1);
            tr_jobs->JOB_STATE = r->getString(2);
            tr_jobs->VO_NAME = r->getString(3);
            tr_jobs->PRIORITY = r->getInt(4);
            tr_jobs->SOURCE = r->getString(5);
            tr_jobs->DEST = r->getString(6);
            tr_jobs->AGENT_DN = r->getString(7);
            tr_jobs->SUBMIT_HOST = r->getString(8);
            tr_jobs->SOURCE_SE = r->getString(9);
            tr_jobs->DEST_SE = r->getString(10);
            tr_jobs->USER_DN = r->getString(11);
            tr_jobs->USER_CRED = r->getString(12);
            tr_jobs->CRED_ID = r->getString(13);
            tr_jobs->SPACE_TOKEN = r->getString(14);
            tr_jobs->STORAGE_CLASS = r->getString(15);
            tr_jobs->INTERNAL_JOB_PARAMS = r->getString(16);
            tr_jobs->OVERWRITE_FLAG = r->getString(17);
            tr_jobs->SOURCE_SPACE_TOKEN = r->getString(18);
            tr_jobs->SOURCE_TOKEN_DESCRIPTION = r->getString(19);
            tr_jobs->COPY_PIN_LIFETIME = r->getInt(20);
            tr_jobs->CHECKSUM_METHOD = r->getString(21);

            //check if a SE or group must not fetch jobs because credits are set to 0 for both in/out(meaning stop processing tr jobs)
	    if(std::string(tr_jobs->SOURCE_SE).length() > 0 && std::string(tr_jobs->DEST_SE).length() > 0){
            	bool process = getInOutOfSe(tr_jobs->SOURCE_SE, tr_jobs->DEST_SE);
            	if (process == true) {
                	jobs.push_back(tr_jobs);
            	} else {
                	delete tr_jobs;
            	}
	    }
        }
        conn->destroyResultset(s, r);
        }
      conn->destroyStatement(s, tag);
      s=NULL;
    } catch (oracle::occi::SQLException const &e) {
		if(conn) {
			conn->rollback();

			if(s && r)
				conn->destroyResultset(s, r);
			if (s)
				conn->destroyStatement(s, tag);

			if(s1 && r1)
				conn->destroyResultset(s1, r1);
			if (s1)
				conn->destroyStatement(s1, tag1);
		}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

}



unsigned int OracleAPI::updateFileStatus(TransferFiles* file, const std::string status) {
    unsigned int updated = 0;    
    const std::string tag1 = "updateFileStatus1";
    const std::string tag2 = "updateFileStatus2";
    std::string query1 =
            "UPDATE t_file "
            "SET file_state =:1, start_time=:2, transferHost=:3 "
            "WHERE file_id = :4 AND FILE_STATE='SUBMITTED' ";
    std::string query2 =
            "UPDATE t_job "
            "SET job_state =:1 "
            "WHERE job_id = :2 AND JOB_STATE='SUBMITTED' ";
    char hostname[MAXHOSTNAMELEN];
    gethostname(hostname, MAXHOSTNAMELEN);
    std::string ftsHostName = std::string(hostname);	    
    oracle::occi::Statement* s1 = NULL;	    
    oracle::occi::Statement* s2 = NULL;

    
    ThreadTraits::LOCK_R lock(_mutex);

    try {
        time_t timed = time(NULL);
        s1 = conn->createStatement(query1, tag1);
        s1->setString(1, status);
        s1->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv()));	
        s1->setString(3, ftsHostName);	
        s1->setInt(4, file->FILE_ID);
        updated = s1->executeUpdate();        
        conn->commit();	
        conn->destroyStatement(s1, tag1);

        s2 = conn->createStatement(query2, tag2);
        s2->setString(1, status);
        s2->setString(2, file->JOB_ID);
        if(0 != s2->executeUpdate())
            	conn->commit();
        conn->destroyStatement(s2, tag2);
	
        return updated;
    } catch (oracle::occi::SQLException const &e) {
		if(conn) {
			conn->rollback();

			if(s1)
				conn->destroyStatement(s1, tag1);

			if(s2)
				conn->destroyStatement(s2, tag2);
		}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return 0;
    }

    return updated;
}

void OracleAPI::updateJObStatus(std::string jobId, const std::string status) {
    const std::string tag = "updateJobStatus";
    std::string query =
            "UPDATE t_job "
            "SET job_state =:1 "
            "WHERE job_id = :2 and job_state = 'SUBMITTED'";
    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s = NULL;
    try {
        s = conn->createStatement(query, tag);
        s->setString(1, status);
        s->setString(2, jobId);
        if(s->executeUpdate() != 0){
        	conn->commit();
        }
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn) {
			conn->rollback();   
			if(s)
				conn->destroyStatement(s, tag);
		}
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::getByJobId(std::vector<TransferJobs*>& jobs, std::vector<TransferFiles*>& files) {
    TransferFiles* tr_files = NULL;
    std::vector<TransferJobs*>::const_iterator iter;
    std::string selecttag = "getByJobId";
    std::string select = "SELECT t_file.source_surl, t_file.dest_surl, t_file.job_id, t_job.vo_name, "
            " t_file.file_id, t_job.overwrite_flag, t_job.USER_DN, t_job.CRED_ID, t_file.checksum, t_job.CHECKSUM_METHOD, t_job.SOURCE_SPACE_TOKEN,"
            " t_job.SPACE_TOKEN,   t_file.file_state, t_file.logical_name, "
            " t_file.reason_class, t_file.reason, t_file.num_failures, t_file.current_failures, "
            " t_file.catalog_failures, t_file.prestage_failures, t_file.filesize, "
            " t_file.finish_time, t_file.agent_dn, t_file.internal_file_params, "
            " t_file.error_scope, t_file.error_phase "
            " FROM t_file, t_job WHERE"
            " t_file.job_id = t_job.job_id AND "
            " t_file.job_finished is NULL AND "
            " t_file.file_state ='SUBMITTED' AND "
            " t_job.job_finished is NULL AND "
            " t_job.job_id=:1 ORDER BY t_file.file_id DESC ";

   
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    ThreadTraits::LOCK_R lock(_mutex);
    try {
        if ( false == conn->checkConn() )
		return;
		
        s = conn->createStatement(select, selecttag);
        s->setPrefetchRowCount(3000);
        for (iter = jobs.begin(); iter != jobs.end(); ++iter) {
            TransferJobs* temp = (TransferJobs*) * iter;
            std::string job_id = std::string(temp->JOB_ID);
            s->setString(1, job_id);
            r = conn->createResultset(s);
            while (r->next()) {
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
                files.push_back(tr_files);
            }
            conn->destroyResultset(s, r);
	    r=NULL;
        }

        conn->destroyStatement(s, selecttag);
    } catch (oracle::occi::SQLException const &e) {
		if(conn) {
			conn->rollback();
			if (r && s)
				conn->destroyResultset(s, r);
			if (s)
				conn->destroyStatement(s, selecttag);
		}

		FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::submitPhysical(const std::string & jobId, std::vector<src_dest_checksum_tupple> src_dest_pair, const std::string & paramFTP,
        const std::string & DN, const std::string & cred, const std::string & voName, const std::string & myProxyServer,
        const std::string & delegationID, const std::string & spaceToken, const std::string & overwrite,
        const std::string & sourceSpaceToken, const std::string &, const std::string & lanConnection, int copyPinLifeTime,
        const std::string & failNearLine, const std::string & checksumMethod, const std::string & reuse,
        const std::string & sourceSE, const std::string & destSe) {


    const std::string initial_state = "SUBMITTED";
    time_t timed = time(NULL);
    char hostname[512] = {0};
    gethostname(hostname, 512);
    const std::string currenthost = hostname; //current hostname
    const std::string tag_job_statement = "tag_job_statement";
    const std::string tag_file_statement = "tag_file_statement";
    const std::string job_statement = "INSERT INTO t_job(job_id, job_state, job_params, user_dn, user_cred, priority, "
            " vo_name,submit_time,internal_job_params,submit_host, cred_id, myproxy_server, SPACE_TOKEN, overwrite_flag,SOURCE_SPACE_TOKEN,copy_pin_lifetime, "
            " lan_connection,fail_nearline, checksum_method, REUSE_JOB, SOURCE_SE, DEST_SE) VALUES (:1,:2,:3,:4,:5,:6,:7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17, :18, :19, :20, :21, :22)";
    const std::string file_statement = "INSERT INTO t_file (job_id, file_state, source_surl, dest_surl,checksum) VALUES (:1,:2,:3,:4,:5)";
    ThreadTraits::LOCK_R lock(_mutex2);
    oracle::occi::Statement* s_job_statement = NULL;
    oracle::occi::Statement* s_file_statement = NULL;
    try {
        if ( false == conn2->checkConn() ){
		throw Err_Custom("Can't connect to the database");		
	}
		
        s_job_statement = conn2->createStatement(job_statement, tag_job_statement);
        s_job_statement->setString(1, jobId); //job_id
        s_job_statement->setString(2, initial_state); //job_state
        s_job_statement->setString(3, paramFTP); //job_params
        s_job_statement->setString(4, DN); //user_dn
        s_job_statement->setString(5, cred); //user_cred
        s_job_statement->setInt(6, 3); //priority
        s_job_statement->setString(7, voName); //vo_name
        s_job_statement->setTimestamp(8, conv->toTimestamp(timed, conn2->getEnv())); //submit_time
        s_job_statement->setString(9, ""); //internal_job_params
        s_job_statement->setString(10, currenthost); //submit_host
        s_job_statement->setString(11, delegationID); //cred_id
        s_job_statement->setString(12, myProxyServer); //myproxy_server
        s_job_statement->setString(13, spaceToken); //space_token
        s_job_statement->setString(14, overwrite); //overwrite_flag
        s_job_statement->setString(15, sourceSpaceToken); //source_space_token
        s_job_statement->setInt(16, copyPinLifeTime); //copy_pin_lifetime
        s_job_statement->setString(17, lanConnection); //lan_connection
        s_job_statement->setString(18, failNearLine); //fail_nearline	
        if (checksumMethod.length() == 0)
            s_job_statement->setNull(19, oracle::occi::OCCICHAR);
        else
            s_job_statement->setString(19, "Y"); //checksum_method
        if (reuse.length() == 0)
            s_job_statement->setNull(20, oracle::occi::OCCISTRING);
        else
            s_job_statement->setString(20, "Y"); //reuse session for this job
        s_job_statement->setString(21, sourceSE); //reuse session for this job
        s_job_statement->setString(22, destSe); //reuse session for this job    
        s_job_statement->executeUpdate();

        //now insert each src/dest pair for this job id
        std::vector<src_dest_checksum_tupple>::const_iterator iter;
        s_file_statement = conn2->createStatement(file_statement, tag_file_statement);		
        
	for (iter = src_dest_pair.begin(); iter != src_dest_pair.end(); ++iter) {
            s_file_statement->setString(1, jobId);
            s_file_statement->setString(2, initial_state);
            s_file_statement->setString(3, iter->source);
            s_file_statement->setString(4, iter->destination);
            s_file_statement->setString(5, iter->checksum);
            s_file_statement->executeUpdate();
        }
        conn2->commit();
        conn2->destroyStatement(s_job_statement, tag_job_statement);
        conn2->destroyStatement(s_file_statement, tag_file_statement);
    } catch (oracle::occi::SQLException const &e) {
		if(conn2) {
			conn2->rollback();

			if(s_job_statement){
				conn2->destroyStatement(s_job_statement, tag_job_statement);
			}

			if(s_file_statement){
				conn2->destroyStatement(s_file_statement, tag_file_statement);
			}
		}

		throw Err_Custom(e.what());
    }
}

void OracleAPI::getTransferJobStatus(std::string requestID, std::vector<JobStatus*>& jobs) {

    std::string query = "SELECT t_job.job_id, t_job.job_state, t_file.file_state, t_job.user_dn, t_job.reason, t_job.submit_time, t_job.priority, t_job.vo_name, "
            "(SELECT count(*) from t_file where t_file.job_id=:1) "
            "FROM t_job, t_file WHERE t_file.job_id = t_job.job_id and t_file.job_id = :2";
    const std::string tag = "getTransferJobStatus";

    JobStatus* js = NULL;
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    ThreadTraits::LOCK_R lock(_mutex2);
    try {
    
        if ( false == conn2->checkConn() ){
		throw Err_Custom("Can't connect to the database");		
	}    
    
        s = conn2->createStatement(query, tag);
        s->setString(1, requestID);
        s->setString(2, requestID);
        r = conn2->createResultset(s);
        while (r->next()) {
            js = new JobStatus();
            js->jobID = r->getString(1);
            js->jobStatus = r->getString(2);
            js->fileStatus = r->getString(3);
            js->clientDN = r->getString(4);
            js->reason = r->getString(5);
            js->submitTime = conv->toTimeT(r->getTimestamp(6));
            js->priority = r->getInt(7);
            js->voName = r->getString(8);
            js->numFiles = r->getInt(9);
            jobs.push_back(js);
        }
        conn2->destroyResultset(s, r);
        conn2->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
		if(conn2) {
			conn2->rollback();    
			if(s && r)
				conn2->destroyResultset(s, r);
			if (s)
				conn2->destroyStatement(s, tag);
		}
        throw Err_Custom(e.what());		
    }
}

/*
 * Return a list of jobs based on the status requested
 * std::vector<JobStatus*> jobs: the caller will deallocate memory JobStatus instances and clear the vector
 * std::vector<std::string> inGivenStates: order doesn't really matter, more than one states supported
 */
void OracleAPI::listRequests(std::vector<JobStatus*>& jobs, std::vector<std::string>& inGivenStates, std::string restrictToClientDN, std::string forDN, std::string VOname) {

    JobStatus* j = NULL;
    bool checkForCanceling = false;
    unsigned int cc = 1;
    std::string jobStatuses("");

    /*this statement cannot be prepared, it's generated dynamically*/
    std::string tag = "listRequests";
    std::string sel = "SELECT DISTINCT job_id, job_state, reason, submit_time, user_dn, J.vo_name,(SELECT count(*) from t_file where t_file.job_id = J.job_id), priority, cancel_job FROM t_job J ";
    //gain the benefit from the statement pooling
    std::sort(inGivenStates.begin(), inGivenStates.end());
    std::vector<std::string>::const_iterator foundCancel;

    if (inGivenStates.size() > 0) {
        foundCancel = std::find_if(inGivenStates.begin(), inGivenStates.end(), bind2nd(std::equal_to<std::string > (), std::string("Canceling")));
        if (foundCancel == inGivenStates.end()) { //not found
            checkForCanceling = true;
	    tag.append("1");
        }
	    tag.append("2");	
    }

    if (inGivenStates.size() > 0) {
        jobStatuses = "'" + inGivenStates[0] + "'";
        for (unsigned int i = 1; i < inGivenStates.size(); i++) {
            jobStatuses += (",'" + inGivenStates[i] + "'");
        }
	    tag.append("3");
    }

    if (restrictToClientDN.length() > 0) {
        sel.append(" LEFT OUTER JOIN T_SE_PAIR_ACL C ON J.SE_PAIR_NAME = C.SE_PAIR_NAME LEFT OUTER JOIN t_vo_acl V ON J.vo_name = V.vo_name ");
        tag.append("4");	
    }

    if (inGivenStates.size() > 0) {
        sel.append(" WHERE J.job_state IN (" + jobStatuses + ") ");
        tag.append(jobStatuses);
    } else {
        sel.append(" WHERE J.job_state <> '0' ");
        tag.append("6");	
    }

    if (restrictToClientDN.length() > 0) {
        sel.append(" AND (J.user_dn = :1 OR V.principal = :2 OR C.principal = :3) ");
        tag.append("7");
    }

    if (VOname.length() > 0) {
        sel.append(" AND J.vo_name = :4");
        tag.append("8");	
    }

    if (forDN.length() > 0) {
        sel.append(" AND J.user_dn = :5");
        tag.append("9");
    }

    if (!checkForCanceling) {
        sel.append(" AND NVL(J.cancel_job,'X') <> 'Y'");
        tag.append("10");	
    }

    tag.append("11");

   oracle::occi::Statement* s = NULL;
   oracle::occi::ResultSet* r = NULL;
   ThreadTraits::LOCK_R lock(_mutex2);
   
    try {
    
        if ( false == conn2->checkConn() ){
		throw Err_Custom("Can't connect to the database");		
	}        
    
        s = conn2->createStatement(sel, tag);
        if (restrictToClientDN.length() > 0) {
            s->setString(cc++, restrictToClientDN);
            s->setString(cc++, restrictToClientDN);
            s->setString(cc++, restrictToClientDN);
        }

        if (VOname.length() > 0) {
            s->setString(cc++, VOname);
        }

        if (forDN.length() > 0) {
            s->setString(cc, forDN);
        }


        r = conn2->createResultset(s);
        while (r->next()) {
            std::string jid = r->getString(1);
            std::string jstate = r->getString(2);
            std::string reason = r->getString(3);
            time_t tm = conv->toTimeT(r->getTimestamp(4));
            std::string dn = r->getString(5);
            std::string voName = r->getString(6);
            int fileCount = r->getInt(7);
            int priority = r->getInt(8);

            j = new JobStatus();
            if (j) {
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

        conn2->destroyResultset(s, r);
        conn2->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn2) {
			conn2->rollback();    

			if(s && r)
				conn2->destroyResultset(s, r);
			if (s)
				conn2->destroyStatement(s, tag);
		}
        throw Err_Custom(e.what());		
    }
}

void OracleAPI::getTransferFileStatus(std::string requestID, std::vector<FileTransferStatus*>& files) {
    std::string query = "SELECT t_file.SOURCE_SURL, t_file.DEST_SURL, t_file.file_state, t_file.reason, t_file.start_time, t_file.finish_time"
            " FROM t_file WHERE t_file.job_id = :1";
    const std::string tag = "getTransferFileStatus";

    FileTransferStatus* js = NULL;
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    ThreadTraits::LOCK_R lock(_mutex2);
    try {
    
        if ( false == conn2->checkConn() ){
		throw Err_Custom("Can't connect to the database");		
	}       
	    
        s = conn2->createStatement(query, tag);
        s->setString(1, requestID);
        r = conn2->createResultset(s);
        while (r->next()) {
            js = new FileTransferStatus();
            js->sourceSURL = r->getString(1);
            js->destSURL = r->getString(2);
            js->transferFileState = r->getString(3);
            js->reason = r->getString(4);
            js->start_time = conv->toTimeT(r->getTimestamp(5));
            js->finish_time = conv->toTimeT(r->getTimestamp(6));

            files.push_back(js);
        }
        conn2->destroyResultset(s, r);
        conn2->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
		if(conn2) {
			conn2->rollback();    

			if(s && r)
				conn2->destroyResultset(s, r);
			if (s)
				conn2->destroyStatement(s, tag);
		}
        throw Err_Custom(e.what());	
    }

}

void OracleAPI::getSe(Se* &se, std::string seName) {
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

    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    try {
        s = conn->createStatement(query_stmt, tag);
        s->setString(1, seName);
        r = conn->createResultset(s);
        if (r->next()) {
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
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn) {
			conn->rollback();
			if (s && r)
				conn->destroyResultset(s, r);
			if (s)
				conn->destroyStatement(s, tag);
		}

		FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
        std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID) {
    std::string query = "INSERT INTO t_se (ENDPOINT, SE_TYPE, SITE, NAME, STATE, VERSION, HOST, SE_TRANSFER_TYPE, SE_TRANSFER_PROTOCOL,SE_CONTROL_PROTOCOL,GOCDB_ID) VALUES (:1,:2,:3,:4,:5,:6,:7,:8,:9,:10,:11)";
    std::string tag = "addSe";

    oracle::occi::Statement* s = NULL;

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        s = conn->createStatement(query, tag);
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
        	conn->commit();
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn) {
			conn->rollback();
			if(s)
				conn->destroyStatement(s, tag);
		}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
        std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID) {
    int fields = 0;
    std::string query = "UPDATE T_SE SET ";
    if (ENDPOINT.length() > 0) {
        query.append(" ENDPOINT='");
        query.append(ENDPOINT);
        query.append("'");
        fields++;
    }
    if (SE_TYPE.length() > 0) {
        if (fields > 0) {
            fields = 0;
            query.append(", ");
        }
        query.append(" SE_TYPE='");
        query.append(SE_TYPE);
        query.append("'");
        fields++;
    }
    if (SITE.length() > 0) {
        if (fields > 0) {
            fields = 0;
            query.append(", ");
        }
        query.append(" SITE='");
        query.append(SE_TYPE);
        query.append("'");
        fields++;
    }
    if (STATE.length() > 0) {
        if (fields > 0) {
            fields = 0;
            query.append(", ");
        }
        query.append(" STATE='");
        query.append(STATE);
        query.append("'");
        fields++;
    }
    if (VERSION.length() > 0) {
        if (fields > 0) {
            fields = 0;
            query.append(", ");
        }
        query.append(" VERSION='");
        query.append(VERSION);
        query.append("'");
        fields++;
    }
    if (HOST.length() > 0) {
        if (fields > 0) {
            fields = 0;
            query.append(", ");
        }
        query.append(" HOST='");
        query.append(HOST);
        query.append("'");
        fields++;
    }
    if (SE_TRANSFER_TYPE.length() > 0) {
        if (fields > 0) {
            fields = 0;
            query.append(", ");
        }
        query.append(" SE_TRANSFER_TYPE='");
        query.append(SE_TRANSFER_TYPE);
        query.append("'");
        fields++;
    }
    if (SE_TRANSFER_PROTOCOL.length() > 0) {
        if (fields > 0) {
            fields = 0;
            query.append(", ");
        }
        query.append(" SE_TRANSFER_PROTOCOL='");
        query.append(SE_TRANSFER_PROTOCOL);
        query.append("'");
        fields++;
    }
    if (SE_CONTROL_PROTOCOL.length() > 0) {
        if (fields > 0) {
            fields = 0;
            query.append(", ");
        }
        query.append(" SE_CONTROL_PROTOCOL='");
        query.append(SE_CONTROL_PROTOCOL);
        query.append("'");
        fields++;
    }
    if (GOCDB_ID.length() > 0) {
        if (fields > 0) {
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
    oracle::occi::Statement* s = NULL;
    ThreadTraits::LOCK_R lock(_mutex);				
    try {
        s = conn->createStatement(query, "");
	
        if(s->executeUpdate()!=0)
        	conn->commit();
        conn->destroyStatement(s, "");

    } catch (oracle::occi::SQLException const &e) {
		if(conn) {
			conn->rollback();
			if(s)
				conn->destroyStatement(s, "");
		}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

}



/*
Delete a SE
REQUIRED: NAME
 */
void OracleAPI::deleteSe(std::string NAME) {
    std::string query = "DELETE FROM T_SE WHERE NAME = :1";
    std::string tag = "deleteSe";

    oracle::occi::Statement* s = NULL;

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        s = conn->createStatement(query, tag);
        s->setString(1, NAME);
        if(s->executeUpdate()!=0)
        	conn->commit();
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn) {
			conn->rollback();
			if(s)
				conn->destroyStatement(s, tag);
		}

		FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}



void OracleAPI::updateFileTransferStatus(std::string job_id, std::string file_id, std::string transfer_status, std::string transfer_message, int process_id, double
        filesize, double duration) {
    job_id = "";
    unsigned int index = 1;
    double throughput = 0;
    std::string tag = "updateFileTransferStatus";
    std::stringstream query;
    query << "UPDATE t_file SET file_state=:" << 1 << ", REASON=:" << ++index;
    if ((transfer_status.compare("FINISHED") == 0) || (transfer_status.compare("FAILED") == 0) || (transfer_status.compare("CANCELED") == 0)) {
        query << ", FINISH_TIME=:" << ++index;
        tag.append("xx");
    }
    if ((transfer_status.compare("ACTIVE") == 0)) {
        query << ", START_TIME=:" << ++index;
        tag.append("xx1");
    }
    query << ", PID=:" << ++index;
    query << ", FILESIZE=:" << ++index;
    query << ", TX_DURATION=:" << ++index;
    query << ", THROUGHPUT=:" << ++index;
    query << " WHERE file_id =:" << ++index;
    query << " and (file_state='READY' OR file_state='ACTIVE')";
    oracle::occi::Statement* s = NULL;
    ThreadTraits::LOCK_R lock(_mutex);
        
    try {
        s = conn->createStatement(query.str(), tag);
        index = 1; //reset index
        s->setString(1, transfer_status);
        ++index;
        s->setString(index, transfer_message);
        if ((transfer_status.compare("FINISHED") == 0) || (transfer_status.compare("FAILED") == 0) || (transfer_status.compare("CANCELED") == 0)) {
            time_t timed = time(NULL);
            ++index;
            s->setTimestamp(index, conv->toTimestamp(timed, conn->getEnv()));
        }
        if ((transfer_status.compare("ACTIVE") == 0)) {
            time_t timed = time(NULL);
            ++index;
            s->setTimestamp(index, conv->toTimestamp(timed, conn->getEnv()));
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
        if(filesize > 0 && duration> 0 && (transfer_status.compare("FINISHED") == 0)){
		throughput = convertBtoM(filesize, duration);	
        	s->setDouble(index, throughput);
	}else if(filesize > 0 && duration<= 0 && (transfer_status.compare("FINISHED") == 0)){
		throughput = convertBtoM(filesize, 1);	
        	s->setDouble(index, throughput);	
	}else{		
		s->setDouble(index, 0);
	}
        ++index;
        s->setInt(index, atoi(file_id.c_str()));
	
        if(s->executeUpdate()!=0)
        	conn->commit();
        conn->destroyStatement(s, tag);
	s=NULL;

    } catch (oracle::occi::SQLException const &e) {
		if(conn) {
			conn->rollback();
        	if(s)
	        	conn->destroyStatement(s, tag);
        }

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::updateJobTransferStatus(std::string file_id, std::string job_id, const std::string status) {
    file_id = "";
    std::string reason = "One or more files failed. Please have a look at the details for more information";
    const std::string terminal1 = "FINISHED";
    const std::string terminal2 = "FAILED";
    const std::string terminal4 = "FINISHEDDIRTY";
    bool finished = true;
    unsigned int updated = 0;
    int finishedDirty = 0;
    int numberOfFileInJob = 0;
    int numOfFilesInGivenState = 0;
    int failedExistInJob = 0;   
    std::string update =
            "UPDATE t_job "
            "SET JOB_STATE=:1, JOB_FINISHED =:2, FINISH_TIME=:3, REASON=:4 "
            "WHERE job_id = :5 AND JOB_STATE not in ('FINISHEDDIRTY','CANCELED','FINISHED','FAILED')";

    std::string updateJobNotFinished =
            "UPDATE t_job "
            "SET JOB_STATE=:1 WHERE job_id = :2 AND JOB_STATE not in ('FINISHEDDIRTY','CANCELED','FINISHED','FAILED') ";

    std::string query = "select Num1, Num2, Num3, Num4  from "
            "(select count(*) As Num1 from t_file where job_id=:1), "
            "(select count(*) As Num2 from t_file where job_id=:2 and file_state in ('CANCELED','FINISHED','FAILED')), "
            "(select count(*) As Num3 from t_file where job_id=:3 and file_state = 'FINISHED'), "
            "(select count(*) As Num4 from t_file where job_id=:4 and file_state in ('CANCELED','FAILED')) ";


    std::string updateFileJobFinished =
            "UPDATE t_file "
            "SET JOB_FINISHED=:1 "
            "WHERE job_id=:2 ";
    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* st = NULL;	    
    oracle::occi::ResultSet* r = NULL;
    try {
    
        if (false == conn->checkConn())
            return;    
    
        st = conn->createStatement(query, "");
        st->setString(1, job_id);
        st->setString(2, job_id);
        st->setString(3, job_id);
        st->setString(4, job_id);
        r = conn->createResultset(st);
        while (r->next()) {
            numberOfFileInJob = r->getInt(1);
            numOfFilesInGivenState = r->getInt(2);
            finishedDirty = r->getInt(3);
            failedExistInJob = r->getInt(4);
            if (numberOfFileInJob != numOfFilesInGivenState) {
                finished = false;
                break;
            }
        }
        conn->destroyResultset(st, r);
	r = NULL;

        //job finished
        if (finished == true) {
            time_t timed = time(NULL);
            //set job status
            st->setSQL(update);

            //finisheddirty
            if ((finishedDirty > 0) && (failedExistInJob > 0)) {
                st->setString(1, terminal4);
            }// finished
            else if (numberOfFileInJob == finishedDirty) {
                st->setString(1, terminal1);
                reason = std::string("");
            } else if (failedExistInJob > 0) { //failed
                st->setString(1, terminal2);
            } else { //unknown state
            }

            //set reason and timestamps for job
            st->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv()));
            st->setTimestamp(3, conv->toTimestamp(timed, conn->getEnv()));
            st->setString(4, reason);
            st->setString(5, job_id);
            updated += st->executeUpdate();

            //set timestamps for file
            st->setSQL(updateFileJobFinished);
            st->setTimestamp(1, conv->toTimestamp(timed, conn->getEnv()));
            st->setString(2, job_id);
            updated += st->executeUpdate();
            if (updated != 0)
                conn->commit();
        } else { //job not finished
            if (status.compare("ACTIVE") == 0) {
                st->setSQL(updateJobNotFinished);
                st->setString(1, status);
                st->setString(2, job_id);
                if (st->executeUpdate() != 0) {
                    conn->commit();
                }
            }
        }

        conn->destroyStatement(st, "");
        st = NULL;
        finishedDirty = 0;
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (st && r)
            	conn->destroyResultset(st, r);
            if (st)
            	conn->destroyStatement(st, "");
        }
        finishedDirty = 0;
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

void OracleAPI::cancelJob(std::vector<std::string>& requestIDs) {
    const std::string cancelReason = "Job canceled by the user";
    std::string cancelJ = "update t_job SET  JOB_STATE=:1, JOB_FINISHED =:2, FINISH_TIME=:3, REASON=:4 WHERE job_id = :5 AND job_state not in('FINISHEDDIRTY','FINISHED','FAILED')";
    std::string cancelF = "update t_file set file_state=:1, JOB_FINISHED =:2, FINISH_TIME=:3, REASON=:4 WHERE JOB_ID=:5 AND file_state not in('FINISHEDDIRTY','FINISHED','FAILED')";
    const std::string cancelJTag = "cancelJTag";
    const std::string cancelFTag = "cancelFTag";
    std::vector<std::string>::const_iterator iter;
    time_t timed = time(NULL);
    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* st1 = NULL;
    oracle::occi::Statement* st2 = NULL;
    unsigned int updated = 0;
    try {
        if (false == conn->checkConn())
            return;

        st1 = conn->createStatement(cancelJ, cancelJTag);
        st2 = conn->createStatement(cancelF, cancelFTag);

        for (iter = requestIDs.begin(); iter != requestIDs.end(); ++iter) {
            std::string jobId = std::string(*iter);

            st1->setString(1, "CANCELED");
            st1->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv()));
            st1->setTimestamp(3, conv->toTimestamp(timed, conn->getEnv()));
            st1->setString(4, cancelReason);
            st1->setString(5, jobId);
            updated += st1->executeUpdate();

            st2->setString(1, "CANCELED");
            st2->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv()));
            st2->setTimestamp(3, conv->toTimestamp(timed, conn->getEnv()));
            st2->setString(4, cancelReason);
            st2->setString(5, jobId);
            updated += st2->executeUpdate();
            if (updated != 0)
                conn->commit();
        }

        conn->destroyStatement(st1, cancelJTag);
        st1 = NULL;
        conn->destroyStatement(st2, cancelFTag);
        st2 = NULL;
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (st1)
                conn->destroyStatement(st1, cancelJTag);
            if (st2)
                conn->destroyStatement(st2, cancelFTag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

void OracleAPI::getCancelJob(std::vector<int>& requestIDs) {
    const std::string tag = "getCancelJob";
    const std::string tag1 = "getCancelJobUpdateCancel";
    char hostname[MAXHOSTNAMELEN];
    gethostname(hostname, MAXHOSTNAMELEN);        
    std::string query = " select t_file.pid, t_job.job_id from t_file, t_job where t_file.job_id=t_job.job_id and "
    			" t_file.FILE_STATE='CANCELED' and t_file.PID IS NOT NULL AND t_job.CANCEL_JOB IS NULL and t_file.TRANSFERHOST=:1 ";
    std::string update = "update t_job SET CANCEL_JOB='Y' where job_id=:1 ";

    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    oracle::occi::Statement* s1 = NULL;
    try {
        if (false == conn->checkConn())
            return;
        s = conn->createStatement(query, tag);
	s->setString(1, std::string(hostname));
        r = conn->createResultset(s);
        s1 = conn->createStatement(update, tag1);

        while (r->next()) {
            int pid = r->getInt(1);
            std::string job_id = r->getString(2);
            requestIDs.push_back(pid);
            s1->setString(1, job_id);
            if (s1->executeUpdate() != 0)
                conn->commit();
        }

        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        conn->destroyStatement(s1, tag1);

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (r && s)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag);
            if (s1)
                conn->destroyStatement(s1, tag1);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

/*t_credential API*/
void OracleAPI::insertGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs) {
    const std::string tag = "insertGrDPStorageCacheElement";
    std::string query = "INSERT INTO t_credential_cache (dlg_id, dn, cert_request, priv_key, voms_attrs) VALUES (:1, :2, empty_clob(), empty_clob(), empty_clob())";

    const std::string tag1 = "updateGrDPStorageCacheElementxxx";
    std::string query1 = "UPDATE t_credential_cache SET cert_request=:1, priv_key=:2, voms_attrs=:3 WHERE dlg_id=:4 AND dn=:5";

    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s = NULL;
    oracle::occi::Statement* s1 = NULL;
    try {

        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, dlg_id);
        s->setString(2, dn);
        s->executeUpdate();
        conn->commit();

        s1 = conn->createStatement(query1, tag1);
        s1->setString(1, cert_request);
        s1->setString(2, priv_key);
        s1->setString(3, voms_attrs);
        s1->setString(4, dlg_id);
        s1->setString(5, dn);
        s1->executeUpdate();

        conn->destroyStatement(s, tag);
        conn->destroyStatement(s1, tag1);
        s = NULL;
        s1 = NULL;
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (s)
                conn->destroyStatement(s, tag);
            if (s1)
                conn->destroyStatement(s1, tag1);
        }
        throw Err_Custom(e.what());

    }
}

void OracleAPI::updateGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs) {
    const std::string tag = "updateGrDPStorageCacheElement";
    std::string query = "UPDATE t_credential_cache SET cert_request=:1, priv_key=:2, voms_attrs=:3 WHERE dlg_id=:4 AND dn=:5";

    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s = NULL;
    try {

        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, cert_request);
        s->setString(2, priv_key);
        s->setString(3, voms_attrs);
        s->setString(4, dlg_id);
        s->setString(5, dn);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
        s = NULL;
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (s)
                conn->destroyStatement(s, tag);
        }
        throw Err_Custom(e.what());
    }
}

CredCache* OracleAPI::findGrDPStorageCacheElement(std::string delegationID, std::string dn) {
    CredCache* cred = NULL;
    const std::string tag = "findGrDPStorageCacheElement";
    std::string query = "SELECT dlg_id, dn, voms_attrs, cert_request, priv_key FROM t_credential_cache WHERE dlg_id = :1 AND dn = :2 ";

    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    try {

        if (false == conn->checkConn())
            return NULL;

        s = conn->createStatement(query, tag);
        s->setString(1, delegationID);
        s->setString(2, dn);
        r = conn->createResultset(s);

        if (r->next()) {
            cred = new CredCache();
            cred->delegationID = r->getString(1);
            cred->DN = r->getString(2);
            conv->toString(r->getClob(3), cred->vomsAttributes);
            conv->toString(r->getClob(4), cred->certificateRequest);
            conv->toString(r->getClob(5), cred->privateKey);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        return cred;
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (r && s)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return cred;
    }
    return cred;
}

void OracleAPI::deleteGrDPStorageCacheElement(std::string delegationID, std::string dn) {
    const std::string tag = "deleteGrDPStorageCacheElement";
    std::string query = "DELETE FROM t_credential_cache WHERE dlg_id = :1 AND dn = :2";

    oracle::occi::Statement* s = NULL;

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, delegationID);
        s->setString(2, dn);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
        s = NULL;
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s)
        		conn->destroyStatement(s, tag);
        }

        throw Err_Custom(e.what());
    }
}

void OracleAPI::insertGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time) {
    const std::string tag = "insertGrDPStorageElement";
    std::string query = "INSERT INTO t_credential (dlg_id, dn, termination_time, proxy, voms_attrs ) VALUES (:1, :2, :3, empty_clob(), empty_clob())";

    const std::string tag1 = "updateGrDPStorageElementxxx";
    std::string query1 = "UPDATE t_credential SET proxy = :1, voms_attrs = :2, termination_time = :3 WHERE dlg_id = :4 AND dn = :5";

    oracle::occi::Statement* s = NULL;
    oracle::occi::Statement* s1 = NULL;
    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, dlg_id);
        s->setString(2, dn);
        s->setTimestamp(3, conv->toTimestamp(termination_time, conn->getEnv()));
        s->executeUpdate();
        conn->commit();
        conn->destroyStatement(s, tag);
        s = NULL;

        s1 = conn->createStatement(query1, tag1);
        s1->setString(1, proxy);
        s1->setString(2, voms_attrs);
        s1->setTimestamp(3, conv->toTimestamp(termination_time, conn->getEnv()));
        s1->setString(4, dlg_id);
        s1->setString(5, dn);
        s1->executeUpdate();
        conn->commit();
        conn->destroyStatement(s1, tag1);
        s1 = NULL;

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (s)
                conn->destroyStatement(s, tag);
            if (s1)
                conn->destroyStatement(s1, tag1);
        }
        throw Err_Custom(e.what());
    }
}

void OracleAPI::updateGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time) {
    const std::string tag = "updateGrDPStorageElement";
    std::string query = "UPDATE t_credential SET proxy = :1, voms_attrs = :2, termination_time = :3 WHERE dlg_id = :4 AND dn = :5";

    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s = NULL;
    try {

        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, proxy);
        s->setString(2, voms_attrs);
        s->setTimestamp(3, conv->toTimestamp(termination_time, conn->getEnv()));
        s->setString(4, dlg_id);
        s->setString(5, dn);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
        s = NULL;
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (s)
                conn->destroyStatement(s, tag);
        }
        throw Err_Custom(e.what());

    }
}

Cred* OracleAPI::findGrDPStorageElement(std::string delegationID, std::string dn) {
    Cred* cred = NULL;
    const std::string tag = "findGrDPStorageElement";
    std::string query = "SELECT dlg_id, dn, voms_attrs, proxy, termination_time FROM t_credential WHERE dlg_id = :1 AND dn = :2 ";

    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    try {

        if (false == conn->checkConn())
            return NULL;

        s = conn->createStatement(query, tag);
        s->setString(1, delegationID);
        s->setString(2, dn);
        r = conn->createResultset(s);

        if (r->next()) {
            cred = new Cred();
            cred->delegationID = r->getString(1);
            cred->DN = r->getString(2);
            conv->toString(r->getClob(3), cred->vomsAttributes);
            conv->toString(r->getClob(4), cred->proxy);
            cred->termination_time = conv->toTimeT(r->getTimestamp(5));
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        return cred;
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (s && r)
            	conn->destroyResultset(s, r);
            if (s)
            	conn->destroyStatement(s, tag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return cred;
    }
    return cred;
}

void OracleAPI::deleteGrDPStorageElement(std::string delegationID, std::string dn) {
    const std::string tag = "deleteGrDPStorageElement";
    std::string query = "DELETE FROM t_credential WHERE dlg_id = :1 AND dn = :2";

    oracle::occi::Statement* s = NULL;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, delegationID);
        s->setString(2, dn);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
        s = NULL;
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s)
        		conn->destroyStatement(s, tag);
        }
        throw Err_Custom(e.what());
    }
}

bool OracleAPI::getDebugMode(std::string source_hostname, std::string destin_hostname) {
    std::string tag = "getDebugMode";
    std::string query;
    bool debug = false;
    query = "SELECT source_se, dest_se, debug FROM t_debug WHERE source_se = :1 AND (dest_se = :2 or dest_se is null)";

    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (false == conn->checkConn())
            return false;

        s = conn->createStatement(query, tag);
        s->setString(1, source_hostname);
        s->setString(2, destin_hostname);
        r = conn->createResultset(s);

        if (r->next()) {
            debug = std::string(r->getString(3)).compare("on") == 0 ? true : false;
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        return debug;
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return debug;
    }
    return debug;

}

void OracleAPI::setDebugMode(std::string source_hostname, std::string destin_hostname, std::string mode) {
    std::string tag1 = "setDebugModeDelete";
    std::string tag2 = "setDebugModeInsert";
    std::string query1("");
    std::string query2("");
    unsigned int updated = 0;

    if (destin_hostname.length() == 0) {
        query1 = "delete from t_debug where source_se=:1 and dest_se is null";
        query2 = "insert into t_debug(source_se, debug) values(:1,:2)";
        tag1 += "1";
        tag2 += "1";
    } else {
        query1 = "delete from t_debug where source_se=:1 and dest_se =:2";
        query2 = "insert into t_debug(source_se,dest_se,debug) values(:1,:2,:3)";
    }

    oracle::occi::Statement* s1 = NULL;
    oracle::occi::Statement* s2 = NULL;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (false == conn->checkConn())
            return;

        s1 = conn->createStatement(query1, tag1);
        s2 = conn->createStatement(query2, tag2);
        if (destin_hostname.length() == 0) {
            s1->setString(1, source_hostname);
            s2->setString(1, source_hostname);
            s2->setString(2, mode);
        } else {
            s1->setString(1, source_hostname);
            s1->setString(2, destin_hostname);
            s2->setString(1, source_hostname);
            s2->setString(2, destin_hostname);
            s2->setString(3, mode);
        }
        updated += s1->executeUpdate();
        updated += s2->executeUpdate();
        if (updated != 0)
            conn->commit();
        conn->destroyStatement(s1, tag1);
        conn->destroyStatement(s2, tag2);
        s1 = NULL;
        s2 = NULL;
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s1)
        		conn->destroyStatement(s1, tag1);
        	if(s2)
        		conn->destroyStatement(s2, tag2);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::getSubmittedJobsReuse(std::vector<TransferJobs*>& jobs, const std::string & vos) {
    TransferJobs* tr_jobs = NULL;
    std::string tag = "getSubmittedJobsReuse";
    std::string query_stmt("");

    if (vos.length() == 0) {
        query_stmt = "SELECT /* FIRST_ROWS(1) */ "
                " t_job.job_id, "
                " t_job.job_state, "
                " t_job.vo_name,  "
                " t_job.priority,  "
                " t_job.source, "
                " t_job.dest,  "
                " t_job.agent_dn, "
                " t_job.submit_host, "
                " t_job.source_se, "
                " t_job.dest_se, "
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
                " AND t_job.job_state in ('ACTIVE', 'READY','SUBMITTED') "
                " AND ROWNUM <=1 "
                " ORDER BY t_job.priority DESC"
                " , SYS_EXTRACT_UTC(t_job.submit_time)";
    } else {
        tag += "1";
        query_stmt = "SELECT /* FIRST_ROWS(1) */ "
                " t_job.job_id, "
                " t_job.job_state, "
                " t_job.vo_name,  "
                " t_job.priority,  "
                " t_job.source, "
                " t_job.dest,  "
                " t_job.agent_dn, "
                " t_job.submit_host, "
                " t_job.source_se, "
                " t_job.dest_se, "
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
                " AND t_job.VO_NAME IN " + vos +
                " AND t_job.job_state in ('ACTIVE', 'READY','SUBMITTED') "
                " AND ROWNUM <=1 "
                " ORDER BY t_job.priority DESC"
                " , SYS_EXTRACT_UTC(t_job.submit_time)";
    }

    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    ThreadTraits::LOCK_R lock(_mutex);
    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query_stmt, tag);
        s->setPrefetchRowCount(1);
        r = conn->createResultset(s);
        while (r->next()) {
            tr_jobs = new TransferJobs();
            tr_jobs->JOB_ID = r->getString(1);
            tr_jobs->JOB_STATE = r->getString(2);
            tr_jobs->VO_NAME = r->getString(3);
            tr_jobs->PRIORITY = r->getInt(4);
            tr_jobs->SOURCE = r->getString(5);
            tr_jobs->DEST = r->getString(6);
            tr_jobs->AGENT_DN = r->getString(7);
            tr_jobs->SUBMIT_HOST = r->getString(8);
            tr_jobs->SOURCE_SE = r->getString(9);
            tr_jobs->DEST_SE = r->getString(10);
            tr_jobs->USER_DN = r->getString(11);
            tr_jobs->USER_CRED = r->getString(12);
            tr_jobs->CRED_ID = r->getString(13);
            tr_jobs->SPACE_TOKEN = r->getString(14);
            tr_jobs->STORAGE_CLASS = r->getString(15);
            tr_jobs->INTERNAL_JOB_PARAMS = r->getString(16);
            tr_jobs->OVERWRITE_FLAG = r->getString(17);
            tr_jobs->SOURCE_SPACE_TOKEN = r->getString(18);
            tr_jobs->SOURCE_TOKEN_DESCRIPTION = r->getString(19);
            tr_jobs->COPY_PIN_LIFETIME = r->getInt(20);
            tr_jobs->CHECKSUM_METHOD = r->getString(21);

            //check if a SE or group must not fetch jobs because credits are set to 0 for both in/out(meaning stop processing tr jobs)
            if (std::string(tr_jobs->SOURCE_SE).length() > 0 && std::string(tr_jobs->DEST_SE).length() > 0) {
                bool process = getInOutOfSe(tr_jobs->SOURCE_SE, tr_jobs->DEST_SE);
                if (process == true) {
                    jobs.push_back(tr_jobs);
                } else {
                    delete tr_jobs;
                }
            }

        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }

}

void OracleAPI::auditConfiguration(const std::string & dn, const std::string & config, const std::string & action) {
    const std::string tag = "auditConfiguration";
    std::string query = "INSERT INTO t_config_audit (when, dn, config, action ) VALUES (:1, :2, :3, :4)";

    oracle::occi::Statement* s = NULL;

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        time_t timed = time(NULL);
        s = conn->createStatement(query, tag);
        s->setTimestamp(1, conv->toTimestamp(timed, conn->getEnv()));
        s->setString(2, dn);
        s->setString(3, config);
        s->setString(4, action);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s)
        		conn->destroyStatement(s, tag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

/*custom optimization stuff*/

void OracleAPI::fetchOptimizationConfig2(OptimizerSample* ops, const std::string & source_hostname, const std::string & destin_hostname) {
    const std::string tag = "fetchOptimizationConfig2XXX";
    const std::string tag1 = "fetchOptimizationConfig2YYY";
    const std::string tag2 = "fetchOptimizationConfig2YYYXXX";
    const std::string tag3 = "fetchOptimizationConfig2ZZZZ";
    const std::string tagMid = "midRangeTimeout";
    int foundNoThrouput = 0;
    int foundNoRecords = 0;
    int timeoutRecords = 0;

    std::string query_stmt_throuput = " select /* FIRST_ROWS(1) */ nostreams, timeout, buffer "
            " from "
            " (select throughput, active, nostreams, timeout, buffer "
            " from t_optimize "
            " where "
            " source_se = :1 and dest_se=:2 "
            " order by abs(((select count(*) from  t_file, t_job where t_file.file_state='ACTIVE' "
            " and  t_job.job_id = t_file.job_id and t_job.source_se=:3 and "
            " t_job.dest_se=:4))-active), abs(500-throughput) desc) where rownum=1 ";

    std::string query_stmt_throuput2 = " SELECT  nostreams, timeout, buffer "
            "  FROM t_optimize WHERE  "
            "  source_se = :1 and dest_se=:2 "
            "  and throughput is NULL and file_id=0 and rownum=1 ";

    std::string query_stmt_throuput1 = "  SELECT count(*) "
            " FROM t_optimize WHERE  "
            " throughput is NULL and source_se = :1 and dest_se=:2 and file_id=0 ";

    std::string query_stmt_throuput3 = " select count(*) from t_optimize where source_se = :1 and dest_se=:2";

    std::string midRangeTimeout = " select count(*) from t_file,t_job where "
            " t_job.job_id=t_file.job_id and t_file.file_state='FAILED' "
            " and t_file.reason like '%operation timeout%' and t_job.source_se=:1 "
            " and t_job.dest_se=:2 and rownum<=3 "
            " and (t_file.finish_time > (CURRENT_TIMESTAMP - interval '30' minute)) "
            " order by  SYS_EXTRACT_UTC(t_file.finish_time) desc";

    oracle::occi::Statement* s3 = NULL;
    oracle::occi::ResultSet* r3 = NULL;
    oracle::occi::Statement* s1 = NULL;
    oracle::occi::ResultSet* r1 = NULL;
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    oracle::occi::Statement* sMid = NULL;
    oracle::occi::ResultSet* rMid = NULL;
    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        /*check the last 3 records between src/dest which failed with timeout*/
        sMid = conn->createStatement(midRangeTimeout, tagMid);
        sMid->setString(1, source_hostname);
        sMid->setString(2, destin_hostname);
        rMid = conn->createResultset(sMid);
        if (rMid->next()) {
            timeoutRecords = rMid->getInt(1);
        }
        conn->destroyResultset(sMid, rMid);
        conn->destroyStatement(sMid, tagMid);
        sMid = NULL;
        rMid = NULL;

        s3 = conn->createStatement(query_stmt_throuput3, tag3);
        s3->setString(1, source_hostname);
        s3->setString(2, destin_hostname);
        r3 = conn->createResultset(s3);
        if (r3->next()) {
            foundNoRecords = r3->getInt(1);
        }
        conn->destroyResultset(s3, r3);
        conn->destroyStatement(s3, tag3);
        s3 = NULL;
        r3 = NULL;

        s1 = conn->createStatement(query_stmt_throuput1, tag1);
        s1->setString(1, source_hostname);
        s1->setString(2, destin_hostname);
        r1 = conn->createResultset(s1);
        if (r1->next()) {
            foundNoThrouput = r1->getInt(1);
        }
        conn->destroyResultset(s1, r1);
        conn->destroyStatement(s1, tag1);
        s1 = NULL;
        r1 = NULL;


        if (foundNoRecords > 0 && foundNoThrouput == 0) { //ALL records for this SE/DEST are having throughput
            s = conn->createStatement(query_stmt_throuput, tag);
            s->setString(1, source_hostname);
            s->setString(2, destin_hostname);
            s->setString(3, source_hostname);
            s->setString(4, destin_hostname);
            //s->setPrefetchRowCount(1);
            r = conn->createResultset(s);
            if (r->next()) {
                ops->streamsperfile = r->getInt(1);
                ops->timeout = r->getInt(2);
                ops->bufsize = r->getInt(3);
                ops->file_id = 1;
            } else {
                if (timeoutRecords == 0) {
                    ops->streamsperfile = DEFAULT_NOSTREAMS;
                    ops->timeout = DEFAULT_TIMEOUT;
                    ops->bufsize = DEFAULT_BUFFSIZE;
                    ops->file_id = 0;
                } else {
                    ops->streamsperfile = DEFAULT_NOSTREAMS;
                    ops->timeout = MID_TIMEOUT;
                    ops->bufsize = DEFAULT_BUFFSIZE;
                    ops->file_id = 0;
                }
            }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag);
            s = NULL;
            r = NULL;
        } else if (foundNoRecords > 0 && foundNoThrouput > 0) { //found records in the DB but are some without throughput 
            s = conn->createStatement(query_stmt_throuput2, tag2);
            s->setString(1, source_hostname);
            s->setString(2, destin_hostname);
            //s->setPrefetchRowCount(1);
            r = conn->createResultset(s);
            if (r->next()) {
                ops->streamsperfile = r->getInt(1);
                ops->timeout = r->getInt(2);
                ops->bufsize = r->getInt(3);
                ops->file_id = 1; //sampled, not being picked again              
            } else {
                if (timeoutRecords == 0) {
                    ops->streamsperfile = DEFAULT_NOSTREAMS;
                    ops->timeout = DEFAULT_TIMEOUT;
                    ops->bufsize = DEFAULT_BUFFSIZE;
                    ops->file_id = 0;
                } else {
                    ops->streamsperfile = DEFAULT_NOSTREAMS;
                    ops->timeout = MID_TIMEOUT;
                    ops->bufsize = DEFAULT_BUFFSIZE;
                    ops->file_id = 0;
                }
            }
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, tag2);
            s = NULL;
            r = NULL;

        } else {
            if (timeoutRecords == 0) {
                ops->streamsperfile = DEFAULT_NOSTREAMS;
                ops->timeout = DEFAULT_TIMEOUT;
                ops->bufsize = DEFAULT_BUFFSIZE;
                ops->file_id = 0;
            } else {
                ops->streamsperfile = DEFAULT_NOSTREAMS;
                ops->timeout = MID_TIMEOUT;
                ops->bufsize = DEFAULT_BUFFSIZE;
                ops->file_id = 0;
            }
        }

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

            if (s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3);

            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1);

            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag2);

            if (sMid && rMid)
                conn->destroyResultset(sMid, rMid);
            if (sMid)
                conn->destroyStatement(sMid, tagMid);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

void OracleAPI::updateOptimizer(std::string, double filesize, int timeInSecs, int nostreams, int timeout, int buffersize, std::string source_hostname, std::string destin_hostname) {
    const std::string tag1 = "updateOptimizer1";
    const std::string tag2 = "updateOptimizer2";
    double throughput = 0;
    bool activeExists = false;
    int active = 0;

    std::string query1 = " select active from t_optimize where "
            " active=(select count(*) from  t_file, t_job where t_file.file_state='ACTIVE' "
            " and t_job.job_id = t_file.job_id and t_job.source_se=:1 and t_job.dest_se=:2)"
            " and nostreams = :3 and timeout=:4 and buffer=:5 and source_se=:6 and dest_se=:7 ";

    std::string query2 = "UPDATE t_optimize SET filesize = :1, throughput = :2, active=:3, when=:4, timeout=:5 "
            " WHERE nostreams = :6 and timeout=:7 and buffer=:8 and source_se=:9 and dest_se=:10 ";

    oracle::occi::Statement* s1 = NULL;
    oracle::occi::ResultSet* r1 = NULL;
    oracle::occi::Statement* s2 = NULL;
    ThreadTraits::LOCK_R lock(_mutex);
    try {


        if (false == conn->checkConn())
            return;

        time_t now = std::time(NULL);
        s1 = conn->createStatement(query1, tag1);
        s1->setString(1, source_hostname);
        s1->setString(2, destin_hostname);
        s1->setInt(3, nostreams);
        s1->setInt(4, timeout);
        s1->setInt(5, buffersize);
        s1->setString(6, source_hostname);
        s1->setString(7, destin_hostname);
        r1 = conn->createResultset(s1);
        if (r1->next()) {
            activeExists = true;
            active = r1->getInt(1);
        }
        conn->destroyResultset(s1, r1);
        conn->destroyStatement(s1, tag1);

        if (filesize > 0 && timeInSecs > 0)
            throughput = convertBtoM(filesize, timeInSecs);
        else
            throughput = convertBtoM(filesize, 1);
        if (filesize <= 0)
            filesize = 0;
        if (nostreams <= 0)
            nostreams = DEFAULT_NOSTREAMS;
        if (buffersize <= 0)
            buffersize = 0;
        if (activeExists) { //update
            s2 = conn->createStatement(query2, tag2);
            s2->setDouble(1, filesize);
            s2->setDouble(2, throughput);
            s2->setInt(3, active);
            s2->setTimestamp(4, conv->toTimestamp(now, conn->getEnv()));
            if (timeInSecs <= DEFAULT_TIMEOUT)
                s2->setInt(5, DEFAULT_TIMEOUT);
            else
                s2->setInt(5, timeout);
            s2->setInt(6, nostreams);
            s2->setInt(7, timeout);
            s2->setInt(8, buffersize);
            s2->setString(9, source_hostname);
            s2->setString(10, destin_hostname);
            if (s2->executeUpdate() != 0)
                conn->commit();
            conn->destroyStatement(s2, tag2);
        } else { //insert new            
            if (timeInSecs <= DEFAULT_TIMEOUT) {
                timeout = DEFAULT_TIMEOUT;
            }
            addOptimizer(std::time(NULL), throughput, source_hostname, destin_hostname, 1, nostreams, timeout, buffersize, active);
        }
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1);

            if (s2)
                conn->destroyStatement(s2, tag2);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::addOptimizer(time_t when, double throughput, const std::string & source_hostname, const std::string & destin_hostname, int file_id, int nostreams, int timeout, int buffersize, int) {
    const std::string tag = "addOptimizer";
    std::string query = "insert into "
            " t_optimize(file_id, source_se, dest_se, nostreams, timeout, active, buffer, throughput, when) "
            " values(:1,:2,:3,:4,:5,(select count(*) from  t_file, t_job where t_file.file_state='ACTIVE' and t_job.job_id = t_file.job_id and "
            " t_job.source_se=:6 and t_job.dest_se=:7),:8,:9,:10) ";

    oracle::occi::Statement* s = NULL;
    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setInt(1, file_id);
        s->setString(2, source_hostname);
        s->setString(3, destin_hostname);
        s->setInt(4, nostreams);
        s->setInt(5, timeout);
        s->setString(6, source_hostname);
        s->setString(7, destin_hostname);
        s->setInt(8, buffersize);
        s->setDouble(9, throughput);
        s->setTimestamp(10, conv->toTimestamp(when, conn->getEnv()));
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (s)
                conn->destroyStatement(s, tag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

void OracleAPI::initOptimizer(const std::string & source_hostname, const std::string & destin_hostname, int) {
    const std::string tag = "initOptimizer";
    const std::string tag1 = "initOptimizer2";
    std::string query = "insert into t_optimize(source_se, dest_se,timeout,nostreams,buffer, file_id ) values(:1,:2,:3,:4,:5,0) ";
    std::string query1 = "select count(*) from t_optimize where source_se=:1 and dest_se=:2";
    int foundRecords = 0;


    oracle::occi::Statement* s1 = NULL;
    oracle::occi::ResultSet* r1 = NULL;
    oracle::occi::Statement* s = NULL;
    ThreadTraits::LOCK_R lock(_mutex);
    try {
        if (false == conn->checkConn())
            return;

        s1 = conn->createStatement(query1, tag1);
        s1->setString(1, source_hostname);
        s1->setString(2, destin_hostname);
        r1 = conn->createResultset(s1);
        if (r1->next()) {
            foundRecords = r1->getInt(1);
        }
        conn->destroyResultset(s1, r1);
        conn->destroyStatement(s1, tag1);
        r1 = NULL;
        s1 = NULL;


        if (foundRecords == 0) {
            s = conn->createStatement(query, tag);

            for (unsigned register int x = 0; x < timeoutslen; x++) {
                for (unsigned register int y = 0; y < nostreamslen; y++) {
                    for (unsigned register int z = 0; z < buffsizeslen; z++) {
                        s->setString(1, source_hostname);
                        s->setString(2, destin_hostname);
                        s->setInt(3, timeouts[x]);
                        s->setInt(4, nostreams[y]);
                        s->setInt(5, buffsizes[z]);
                        s->executeUpdate();
                    }
                }
            }
            conn->commit();
            conn->destroyStatement(s, tag);
            s = NULL;
        }
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1);

            if (s)
                conn->destroyStatement(s, tag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

bool OracleAPI::isCredentialExpired(const std::string & dlg_id, const std::string & dn) {

    bool valid = false;
    const std::string tag = "isCredentialExpired";
    std::string query = "SELECT termination_time from t_credential where dlg_id=:1 and dn=:2";
    double dif;

    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    try {

        if (false == conn->checkConn())
            return false;

        s = conn->createStatement(query, tag);
        s->setString(1, dlg_id);
        s->setString(2, dn);
        r = conn->createResultset(s);

        if (r->next()) {
            time_t lifetime = std::time(NULL);
            time_t term_time = conv->toTimeT(r->getTimestamp(1));
            dif = difftime(term_time, lifetime);

            if (dif > 0)
                valid = true;

        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        return valid;
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return valid;
    }
    return valid;
}

bool OracleAPI::isTrAllowed(const std::string & source_hostname, const std::string & destin_hostname) {
    const std::string tag1 = "isTrAllowed1";
    const std::string tag2 = "isTrAllowed2";
    const std::string tag3 = "isTrAllowed3";
    const std::string tag4 = "isTrAllowed4";
    const std::string tag5 = "isTrAllowed5"; 
    const std::string tag6 = "isTrAllowed6";     
    bool allowed = true;
    int act = 0;
    int maxDest = 0;
    int maxSource = 0;   
    double numberOfFinished = 0;
    double numberOfFailed = 0;    
    double ratioSuccessFailure = 0;
    double numberOfFinishedAll = 0;
    double numberOfFailedAll = 0;    
  
    std::string query_stmt1 = " select count(*) from  t_file, t_job where t_file.file_state='ACTIVE' and t_job.job_id = t_file.job_id and t_job.source_se=:1 ";
	    
    std::string query_stmt2 = " select count(*) from  t_file, t_job where t_file.file_state='ACTIVE' and t_job.job_id = t_file.job_id and t_job.dest_se=:1";
    
    std::string query_stmt3 = " select file_state from  (select file_state from t_file, t_job where t_job.job_id = t_file.job_id and t_job.source_se=:1 and t_job.dest_se=:2 "
    			      " and file_state in ('FAILED','FINISHED') and (t_file.FINISH_TIME > (CURRENT_TIMESTAMP - interval '1' hour)) order by "
			      " SYS_EXTRACT_UTC(t_file.FINISH_TIME) desc) WHERE ROWNUM < 20 ";

    std::string query_stmt4 = " select count(*) from  t_file, t_job where t_job.job_id = t_file.job_id and t_job.source_se=:1 and t_job.dest_se=:2 "
    			      " and file_state = 'ACTIVE' ";
			      
    std::string query_stmt5 = " select count(*) from  t_file, t_job where t_job.job_id = t_file.job_id and t_job.source_se=:1 and t_job.dest_se=:2 "
    			      " and file_state = 'FINISHED' and (t_file.FINISH_TIME > (CURRENT_TIMESTAMP - interval '1' hour))";
			      
    std::string query_stmt6 = " select count(*) from  t_file, t_job where t_job.job_id = t_file.job_id and t_job.source_se=:1 and t_job.dest_se=:2 "
    			      " and file_state = 'FAILED' and (t_file.FINISH_TIME > (CURRENT_TIMESTAMP - interval '1' hour))";			      

    oracle::occi::Statement* s1 = NULL;
    oracle::occi::ResultSet* r1 = NULL;   
    oracle::occi::Statement* s2 = NULL;
    oracle::occi::ResultSet* r2 = NULL;   
    oracle::occi::Statement* s3 = NULL;
    oracle::occi::ResultSet* r3 = NULL;          
    oracle::occi::Statement* s4 = NULL;
    oracle::occi::ResultSet* r4 = NULL; 
    oracle::occi::Statement* s5 = NULL;
    oracle::occi::ResultSet* r5 = NULL;        
    oracle::occi::Statement* s6 = NULL;
    oracle::occi::ResultSet* r6 = NULL;    

    ThreadTraits::LOCK_R lock(_mutex);

    try {

        if (false == conn->checkConn())
            return false;	
	    	    
	s1 = conn->createStatement(query_stmt1, tag1);
        s1->setString(1, source_hostname);
        s1->setPrefetchRowCount(1);
        r1 = conn->createResultset(s1);
        if (r1->next()) {
            maxSource = r1->getInt(1);
        }
        conn->destroyResultset(s1, r1);
        conn->destroyStatement(s1, tag1);
        s1 = NULL;
        r1 = NULL;	    
	    
  	s2 = conn->createStatement(query_stmt2, tag2);
        s2->setString(1, destin_hostname);
        s2->setPrefetchRowCount(1);
        r2 = conn->createResultset(s2);
        if (r2->next()) {
            maxDest = r2->getInt(1);
        }
        conn->destroyResultset(s2, r2);
        conn->destroyStatement(s2, tag2);
        s2 = NULL;
        r2 = NULL;	    
	    
   	s3 = conn->createStatement(query_stmt3, tag3);
        s3->setString(1, source_hostname);
	s3->setString(2, destin_hostname);	
        s3->setPrefetchRowCount(20);
        r3 = conn->createResultset(s3);
	
        while (r3->next()) {
	    std::string fileState = r3->getString(1);
	    if(fileState.compare("FAILED")==0)
		    numberOfFailed += 1.0;
	    
    	    if(fileState.compare("FINISHED")==0)
		    numberOfFinished += 1.0;		        	    	    
        }
        conn->destroyResultset(s3, r3);
        conn->destroyStatement(s3, tag3);
        s3 = NULL;
        r3 = NULL;	
	
  	s4 = conn->createStatement(query_stmt4, tag4);
        s4->setString(1, source_hostname);
	s4->setString(2, destin_hostname);
        s4->setPrefetchRowCount(1);
        r4 = conn->createResultset(s4);
        if (r4->next()) {
            act = r4->getInt(1);
        }
        conn->destroyResultset(s4, r4);
        conn->destroyStatement(s4, tag4);
        s4 = NULL;
        r4 = NULL;	  
	
  	s5 = conn->createStatement(query_stmt5, tag5);
        s5->setString(1, source_hostname);
	s5->setString(2, destin_hostname);
        s5->setPrefetchRowCount(1);
        r5 = conn->createResultset(s5);
        if (r5->next()) {
            numberOfFinishedAll = r5->getInt(1);	    
        }
        conn->destroyResultset(s5, r5);
        conn->destroyStatement(s5, tag5);
        s5 = NULL;
        r5 = NULL;
	
  	s6 = conn->createStatement(query_stmt6, tag6);
        s6->setString(1, source_hostname);
	s6->setString(2, destin_hostname);
        s6->setPrefetchRowCount(1);
        r6 = conn->createResultset(s6);
        if (r6->next()) {
            numberOfFailedAll = r6->getInt(1);	    
        }
        conn->destroyResultset(s6, r6);
        conn->destroyStatement(s6, tag6);
        s6 = NULL;
        r6 = NULL;		  	  	
	  	    	    
	if(numberOfFinished>0){		
		ratioSuccessFailure = numberOfFinished/(numberOfFinished + numberOfFailed) * (100.0/1.0);
	}
	else{		
		ratioSuccessFailure = 0;
	}
		
	allowed = optimizerObject.transferStart(numberOfFinished,numberOfFailed,source_hostname, destin_hostname, act, maxSource, maxDest,
	ratioSuccessFailure,numberOfFinishedAll, numberOfFailedAll);
        return allowed;
	
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

            if (s1 && r1)
                conn->destroyResultset(s1, r1);
            if (s1)
                conn->destroyStatement(s1, tag1);

            if (s2 && r2)
                conn->destroyResultset(s1, r1);
            if (s2)
                conn->destroyStatement(s1, tag1);

            if (s3 && r3)
                conn->destroyResultset(s3, r3);
            if (s3)
                conn->destroyStatement(s3, tag3);

            if (s4 && r4)
                conn->destroyResultset(s4, r4);
            if (s4)
                conn->destroyStatement(s4, tag4);

            if (s5 && r5)
                conn->destroyResultset(s5, r5);
            if (s5)
                conn->destroyStatement(s5, tag5);

            if (s6 && r6)
                conn->destroyResultset(s6, r6);
            if (s6)
                conn->destroyStatement(s6, tag6);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        return allowed;
    }
    return allowed;
}

void OracleAPI::setAllowedNoOptimize(const std::string & job_id, int file_id, const std::string & params) {
    const std::string tag = "setAllowedNoOptimize";
    const std::string tag1 = "setAllowedNoOptimize1";
    std::string query_stmt = "update t_file set INTERNAL_FILE_PARAMS=:1 where file_id=:2 and job_id=:3";
    std::string query_stmt1 = "update t_file set INTERNAL_FILE_PARAMS=:1 where job_id=:2";
    oracle::occi::Statement* s = NULL;
    oracle::occi::Statement* s1 = NULL;
    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (false == conn->checkConn())
            return;

        if (file_id == 0) {
            s1 = conn->createStatement(query_stmt1, tag1);
            s1->setString(1, params);
            s1->setString(2, job_id);
            if (s1->executeUpdate() != 0)
                conn->commit();
            conn->destroyStatement(s1, tag1);
            s1 = NULL;
        } else {
            s = conn->createStatement(query_stmt, tag);
            s->setString(1, params);
            s->setInt(2, file_id);
            s->setString(3, job_id);
            if (s->executeUpdate() != 0)
                conn->commit();
            conn->destroyStatement(s, tag);
            s = NULL;
        }
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

            if (s)
                conn->destroyStatement(s, tag);

            if (s1)
                conn->destroyStatement(s1, tag1);
        }

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}


/* REUSE CASE*/
void OracleAPI::forceFailTransfers() {
    std::string tag = "forceFailTransfers";
    char hostname[MAXHOSTNAMELEN];
    gethostname(hostname, MAXHOSTNAMELEN);    
    std::string vmHostname("");
    std::string query = "select job_id, file_id, START_TIME ,PID, INTERNAL_FILE_PARAMS, TRANSFERHOST from t_file where file_state='ACTIVE' and pid is not null";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    std::string job_id("");
    int file_id = 0;
    time_t start_time;
    int pid = 0;
    std::string internalParams("");
    int timeout = 0;
    const std::string transfer_status = "FAILED";
    const std::string transfer_message = "Transfer has been forced-killed because it was stalled";
    const std::string status = "FAILED";
    double diff = 0;
    try {
        if (false == conn->checkConn()) {
            return;
        }

        s = conn->createStatement(query, tag);
        r = conn->createResultset(s);
        while (r->next()) {
            job_id = r->getString(1);
            file_id = r->getInt(2);
            start_time = conv->toTimeT(r->getTimestamp(3));
            pid = r->getInt(4);
            internalParams = r->getString(5);
            timeout = extractTimeout(internalParams);
            time_t lifetime = std::time(NULL);
	    vmHostname = r->getString(6);
            diff = difftime(lifetime, start_time);
            if (timeout != 0 && diff > (timeout + 1000)) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Killing pid:" << pid << ", jobid:" << job_id << ", fileid:" << file_id << " because it was stalled" << commit;
                std::stringstream ss;
                ss << file_id;
		if(vmHostname.compare(std::string(hostname))==0)
                	kill(pid, SIGUSR1);
                updateFileTransferStatus(job_id, ss.str(), transfer_status, transfer_message, pid, 0, 0);
                updateJobTransferStatus(ss.str(), job_id, status);
            }
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        s = NULL;
        r = NULL;

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::setAllowed(const std::string & job_id, int file_id, const std::string & source_se, const std::string & dest, int nostreams, int timeout, int buffersize) {

    const std::string tag4 = "setAllowed";
    const std::string tag5 = "setAllowed111";
    const std::string tag6 = "setAllowed222";
    std::string query_stmt_throuput4 = "update t_optimize set file_id=1 where nostreams=:1 and buffer=:2 and timeout=:3 and source_se=:4 and dest_se=:5";
    std::string query_stmt_throuput5 = "update t_file set INTERNAL_FILE_PARAMS=:1 where file_id=:2 and job_id=:3";
    std::string query_stmt_throuput6 = "update t_file set INTERNAL_FILE_PARAMS=:1 where job_id=:2";
    oracle::occi::Statement* s4 = NULL;
    oracle::occi::Statement* s5 = NULL;
    oracle::occi::Statement* s6 = NULL;
    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (false == conn->checkConn())
            return;

        s4 = conn->createStatement(query_stmt_throuput4, tag4);
        s4->setInt(1, nostreams);
        s4->setInt(2, buffersize);
        s4->setInt(3, timeout);
        s4->setString(4, source_se);
        s4->setString(5, dest);
        if (s4->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s4, tag4);
        s4 = NULL;

        std::stringstream params;
        params << "nostreams:" << nostreams << ",timeout:" << timeout << ",buffersize:" << buffersize;
        if (file_id != -1) {
            s5 = conn->createStatement(query_stmt_throuput5, tag5);
            s5->setString(1, params.str());
            s5->setInt(2, file_id);
            s5->setString(3, job_id);
            if (s5->executeUpdate() != 0)
                conn->commit();
            conn->destroyStatement(s5, tag5);
            s5 = NULL;
        } else {
            s6 = conn->createStatement(query_stmt_throuput6, tag6);
            s6->setString(1, params.str());
            s6->setString(2, job_id);
            if (s6->executeUpdate() != 0)
                conn->commit();
            conn->destroyStatement(s6, tag6);
            s6 = NULL;
        }
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

            if (s4)
                conn->destroyStatement(s4, tag4);
            if (s5)
                conn->destroyStatement(s5, tag5);
            if (s6)
                conn->destroyStatement(s6, tag6);
        }

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::terminateReuseProcess(const std::string & jobId) {
    const std::string tag = "terminateReuseProcess";
    const std::string tag1 = "terminateReuseProcess11";
    std::string query = "select REUSE_JOB from t_job where job_id=:1 and REUSE_JOB is not null";
    std::string update = "update t_file set file_state='FAILED' where job_id=:1 and file_state != 'FINISHED' ";
    oracle::occi::Statement* s = NULL;
    oracle::occi::Statement* s1 = NULL;
    oracle::occi::ResultSet* r = NULL;
    unsigned int updated = 0;
    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, jobId);
        r = conn->createResultset(s);
        if (r->next()) {
            s1 = conn->createStatement(update, tag1);
            s1->setString(1, jobId);
            updated += s1->executeUpdate();
            conn->destroyStatement(s1, tag1);
        }
        if (updated != 0)
            conn->commit();
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        s = NULL;
        r = NULL;

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

            if (s && r)
                conn->destroyResultset(s, r);
            if (s)
                conn->destroyStatement(s, tag);

            if (s1)
            	conn->destroyStatement(s1, tag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::setPid(const std::string & jobId, const std::string & fileId, int pid) {
    const std::string tag = "setPid";
    std::string query = "update t_file set pid=:1 where job_id=:2 and file_id=:3";
    oracle::occi::Statement* s = NULL;
    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setInt(1, pid);
        s->setString(2, jobId);
        s->setInt(3, atoi(fileId.c_str()));
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
        s = NULL;

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

            if (s)
                conn->destroyStatement(s, tag);
        }

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::setPidV(int pid, std::map<int, std::string>& pids) {
    const std::string tag = "setPidV";
    std::string query = "update t_file set pid=:1 where job_id=:2 and file_id=:3";
    oracle::occi::Statement* s = NULL;
    std::map<int, std::string>::const_iterator iter;
    unsigned int updated = 0;
    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        for (iter = pids.begin(); iter != pids.end(); ++iter) {
            s->setInt(1, pid);
            s->setString(2, (*iter).second);
            s->setInt(3, (*iter).first);
            updated += s->executeUpdate();
        }
        if (updated != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
        s = NULL;

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (s)
                conn->destroyStatement(s, tag);
        }

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::revertToSubmitted() {
    const std::string tag1 = "revertToSubmitted";
    const std::string tag2 = "revertToSubmitted1";
    const std::string tag3 = "revertToSubmittedReused";
    std::string query1 = " update t_file set file_state='SUBMITTED' where file_state='READY' and FINISH_TIME is NULL and JOB_FINISHED is NULL and file_id=:1";
    std::string query2 = " select t_file.start_time, t_file.file_id, t_file.job_id, t_job.REUSE_JOB from t_file,t_job where t_file.file_state"
            "='READY' and t_file.FINISH_TIME is NULL "
            " and t_file.JOB_FINISHED is NULL and t_file.job_id=t_job.job_id";
    std::string query3 = "update t_job T1 set T1.job_state='SUBMITTED' where T1.job_state in('READY','ACTIVE') and "
            " T1.FINISH_TIME is NULL and T1.JOB_FINISHED is NULL and T1.REUSE_JOB='Y' and T1.job_id "
            " IN (SELECT T2.job_id FROM t_file T2 WHERE T2.job_id = T1.job_id and T2.file_state='READY') and T1.job_id=:1 ";
    oracle::occi::Statement* s1 = NULL;
    oracle::occi::Statement* s2 = NULL;
    oracle::occi::ResultSet* r2 = NULL;
    oracle::occi::Statement* s3 = NULL;
    double diff = 0;
    int file_id = 0;
    std::string job_id("");
    std::string reuseFlag("");
    time_t start_time;
    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s2 = conn->createStatement(query2, tag2);
        r2 = conn->createResultset(s2);
        while (r2->next()) {
            start_time = conv->toTimeT(r2->getTimestamp(1));
            file_id = r2->getInt(2);
            job_id = r2->getString(3);
            reuseFlag = r2->getString(4);
            time_t current_time = std::time(NULL);
            diff = difftime(current_time, start_time);
            if (diff > 15000) { /*~5h*/
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "The transfer with file id " << file_id << " seems to be stalled, restart it" << commit;
                s1 = conn->createStatement(query1, tag1);
                s1->setInt(1, file_id);
                if (s1->executeUpdate() != 0) {
                    conn->commit();
                }
                conn->destroyStatement(s1, tag1);
                s1 = NULL;
                if (reuseFlag.compare("Y") == 0) {
                    s3 = conn->createStatement(query3, tag3);
                    s3->setString(1, job_id);
                    if (s3->executeUpdate() != 0) {
                        conn->commit();
                    }
                    conn->destroyStatement(s3, tag3);
                    s3 = NULL;
                }
            }
        }
        conn->destroyResultset(s2, r2);
        conn->destroyStatement(s2, tag2);
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

        	if(s1)
        		conn->destroyStatement(s1, tag1);

        	if(s2 && r2)
        		conn->destroyResultset(s2, r2);
        	if (s2)
        		conn->destroyStatement(s2, tag2);

        	if(s3)
        		conn->destroyStatement(s3, tag3);
        }

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::revertToSubmittedTerminate() {
    const std::string tag1 = "revertToSubmittedTerminate1";
    const std::string tag2 = "revertToSubmittedTerminate2";
    std::string query1 = "update t_file set file_state='SUBMITTED' where file_state='READY' and FINISH_TIME is NULL and JOB_FINISHED is NULL";
    std::string query2 = "update t_job T1 set T1.job_state='SUBMITTED' where T1.job_state in('READY','ACTIVE') and "
            " T1.FINISH_TIME is NULL and T1.JOB_FINISHED is NULL and T1.REUSE_JOB='Y' and T1.job_id "
            " IN (SELECT T2.job_id FROM t_file T2 WHERE T2.job_id = T1.job_id and T2.file_state='READY') ";
    oracle::occi::Statement* s1 = NULL;
    oracle::occi::Statement* s2 = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s1 = conn->createStatement(query1, tag1);
        if (s1->executeUpdate() != 0) {
            conn->commit();
        }
        conn->destroyStatement(s1, tag1);

        s2 = conn->createStatement(query2, tag2);
        if (s2->executeUpdate() != 0) {
            conn->commit();
        }
        conn->destroyStatement(s2, tag2);
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

        	if(s1)
        		conn->destroyStatement(s1, tag1);
        	if(s2)
        		conn->destroyStatement(s2, tag2);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::backup() {
    std::string query1 = "insert into t_job_backup select * from t_job where job_state IN ('FINISHED', 'FAILED', 'CANCELED', 'FINISHEDDIRTY') and job_finished < (systimestamp - interval '7' DAY )";
    std::string query2 = "insert into t_file_backup select * from t_file  where job_id IN (select job_id from t_job_backup)";
    std::string query3 = "delete from t_file where file_id in (select file_id from t_file_backup)";
    std::string query4 = "delete from t_job where job_id in (select job_id from t_job_backup)";
    oracle::occi::Statement* s1 = NULL;
    oracle::occi::Statement* s2 = NULL;
    oracle::occi::Statement* s3 = NULL;
    oracle::occi::Statement* s4 = NULL;

    try {
        if (false == conn->checkConn())
            return;

        s1 = conn->createStatement(query1, "");
        s1->executeUpdate();
        conn->commit();
        s2 = conn->createStatement(query2, "");
        s2->executeUpdate();
        conn->commit();
        s3 = conn->createStatement(query3, "");
        s3->executeUpdate();
        conn->commit();
        s4 = conn->createStatement(query4, "");
        s4->executeUpdate();
        conn->commit();

        conn->destroyStatement(s1, "");
        conn->destroyStatement(s2, "");
        conn->destroyStatement(s3, "");
        conn->destroyStatement(s4, "");

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

        	if(s1)
        		conn->destroyStatement(s1, "");
        	if(s2)
        		conn->destroyStatement(s2, "");
        	if(s3)
        		conn->destroyStatement(s3, "");
        	if(s4)
        		conn->destroyStatement(s2, "");
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::forkFailedRevertState(const std::string & jobId, int fileId) {
    const std::string tag = "forkFailedRevertState";
    std::string query = "update t_file set file_state='SUBMITTED' where file_id=:1 and job_id=:2 and file_state not in ('FINISHED','FAILED','CANCELED')";
    oracle::occi::Statement* stmt = 0;

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        if (false == conn->checkConn())
            return;

        stmt = conn->createStatement(query, tag);
        stmt->setInt(1, fileId);
        stmt->setString(2, jobId);
        if (stmt->executeUpdate() != 0) {
            conn->commit();
        }
        conn->destroyStatement(stmt, tag);

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(stmt)
        		conn->destroyStatement(stmt, tag);
        }

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::forkFailedRevertStateV(std::map<int, std::string>& pids) {
    const std::string tag = "forkFailedRevertStateV";
    std::string query = "update t_file set file_state='SUBMITTED' where file_id=:1 and job_id=:2 and file_state not in ('FINISHED','FAILED','CANCELED')";
    oracle::occi::Statement* s = NULL;
    std::map<int, std::string>::const_iterator iter;
    unsigned int updated = 0;
    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        for (iter = pids.begin(); iter != pids.end(); ++iter) {
            s->setInt(1, (*iter).first);
            s->setString(2, (*iter).second);
            updated += s->executeUpdate();
        }
        if (updated != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
        s = NULL;

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (s)
                conn->destroyStatement(s, tag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::retryFromDead(std::map<int, std::string>& pids) {
    const std::string tag = "retryFromDead";
    std::string query = "update t_file set file_state='SUBMITTED' where file_id=:1 and job_id=:2 and file_state not in ('FINISHED','FAILED','CANCELED')";
    oracle::occi::Statement* s = NULL;
    std::map<int, std::string>::const_iterator iter;
    unsigned int updated = 0;
    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        for (iter = pids.begin(); iter != pids.end(); ++iter) {
            s->setInt(1, (*iter).first);
            s->setString(2, (*iter).second);
            updated += s->executeUpdate();
        }
        if (updated != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
        s = NULL;

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
            if (s)
                conn->destroyStatement(s, tag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::blacklistSe(std::string se, std::string msg, std::string adm_dn) {

    std::string query = "INSERT INTO t_bad_ses (se, message, addition_time, admin_dn) VALUES (:1, :2, :3, :4)";
    std::string tag = "blacklistSe";

    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (false == conn->checkConn()) return;

        time_t timed = time(NULL);

        s = conn->createStatement(query, tag);
        s->setString(1, se);
        s->setString(2, msg);
        s->setTimestamp(3, conv->toTimestamp(timed, conn->getEnv()));
        s->setString(4, adm_dn);
        r = conn->createResultset(s);
        conn->commit();
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::blacklistDn(std::string dn, std::string msg, std::string adm_dn) {

    std::string query = "INSERT INTO t_bad_dns (dn, message, addition_time, admin_dn) VALUES (:1, :2, :3, :4)";
    std::string tag = "blacklistSe";

    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (false == conn->checkConn()) return;

        time_t timed = time(NULL);

        s = conn->createStatement(query, tag);
        s->setString(1, dn);
        s->setString(2, msg);
        s->setTimestamp(3, conv->toTimestamp(timed, conn->getEnv()));
        s->setString(4, adm_dn);
        r = conn->createResultset(s);
        conn->commit();
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::unblacklistSe(std::string se) {

    std::string query = "DELETE FROM t_bad_ses WHERE se = :1";
    std::string tag = "unblacklistSe";

    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (false == conn->checkConn()) return;

        s = conn->createStatement(query, tag);
        s->setString(1, se);
        r = conn->createResultset(s);
        conn->commit();
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::unblacklistDn(std::string dn) {

    std::string query = "DELETE FROM t_bad_dns WHERE dn = :1";
    std::string tag = "unblacklistSe";

    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (false == conn->checkConn()) return;

        s = conn->createStatement(query, tag);
        s->setString(1, dn);
        r = conn->createResultset(s);
        conn->commit();
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

bool OracleAPI::isSeBlacklisted(std::string se) {

    std::string tag = "isSeBlacklisted";
    std::string stmt = "SELECT * FROM t_bad_ses WHERE se = :1";

    oracle::occi::Statement* s = 0;
    oracle::occi::ResultSet* r = 0;

    bool ret = false;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (!conn->checkConn()) return ret;

        s = conn->createStatement(stmt, tag);
        s->setString(1, se);
        r = conn->createResultset(s);

        ret = r->next();

        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {

        if (conn) {
            conn->rollback();

        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

    return ret;
}

bool OracleAPI::isDnBlacklisted(std::string dn) {

    std::string tag = "isSeBlacklisted";
    std::string stmt = "SELECT * FROM t_bad_dns WHERE dn = :1";

    oracle::occi::Statement* s = 0;
    oracle::occi::ResultSet* r = 0;

    bool ret = false;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (!conn->checkConn()) return ret;

        s = conn->createStatement(stmt, tag);
        s->setString(1, dn);
        r = conn->createResultset(s);

        ret = r->next();

        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {

        if (conn) {
            conn->rollback();

        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

    return ret;
}

/********* section for the new config API **********/
bool OracleAPI::isFileReadyState(int fileID) {
    const std::string tag = "isFileReadyState";
    bool ready = false;
    std::string query = "select file_state from t_file where file_id=:1";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return false;

        s = conn->createStatement(query, tag);
        s->setInt(1, fileID);
        r = conn->createResultset(s);
        if (r->next()) {
            std::string state = r->getString(1);
            if (state.compare("READY") == 0)
                ready = true;
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        return ready;
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
    return ready;
}

bool OracleAPI::checkGroupExists(const std::string & groupName) {
    const std::string tag = "checkGroupExists";
    std::string query = "select groupName from t_group_members where groupName=:1";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    bool groupExist = false;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return false;

        s = conn->createStatement(query, tag);
        s->setString(1, groupName);
        r = conn->createResultset(s);
        if (r->next()) {
            groupExist = true;
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        return groupExist;
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

    return groupExist;
}

//t_group_members

void OracleAPI::getGroupMembers(const std::string & groupName, std::vector<std::string>& groupMembers) {
    const std::string tag = "getGroupMembers";
    std::string query = "select member from t_group_members where groupName=:1";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, groupName);
        r = conn->createResultset(s);
        while (r->next()) {
            std::string member = r->getString(1);
            groupMembers.push_back(member);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

std::string OracleAPI::getGroupForSe(const std::string se) {
    const std::string tag = "getGroupForSe";
    std::string query = "select groupName from t_group_members where member=:1";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    std::string ret;

    try {
        if (false == conn->checkConn())
            return ret;

        s = conn->createStatement(query, tag);
        s->setString(1, se);
        r = conn->createResultset(s);
        if (r->next()) {
            ret = r->getString(1);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();

        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

    return ret;
}


void OracleAPI::addMemberToGroup(const std::string & groupName, std::vector<std::string>& groupMembers){
	std::string tag="addMemberToGroup";
	std::string query = "insert into t_group_members(member, groupName) values(:1, :2)";
	oracle::occi::Statement* s = NULL;
	std::vector<std::string>::const_iterator iter;
	
	ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        for (iter = groupMembers.begin(); iter != groupMembers.end(); ++iter) {
            s->setString(1, std::string(*iter));
            s->setString(2, groupName);
            s->executeUpdate();
        }
        conn->commit();
        conn->destroyStatement(s, tag);
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s)
        		conn->destroyStatement(s, tag);
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::deleteMembersFromGroup(const std::string & groupName, std::vector<std::string>& groupMembers) {
    std::string tag = "deleteMembersFromGroup";
    std::string query = "delete from t_group_members where groupName=:1 and member=:2";
    oracle::occi::Statement* s = NULL;
    std::vector<std::string>::const_iterator iter;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        for (iter = groupMembers.begin(); iter != groupMembers.end(); ++iter) {
            s->setString(1, groupName);
            s->setString(2, std::string(*iter));
            s->executeUpdate();
        }
        conn->commit();
        conn->destroyStatement(s, tag);
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
          	if(s)
          		conn->destroyStatement(s, tag);
        }

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::addLinkConfig(LinkConfig* cfg) {
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
    		"	no_tx_activity_to"
    		") values(:1,:2,:3,:4,:5,:6,:7,:8)";
    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s = NULL;
    try {

        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, cfg->source);
        s->setString(2, cfg->destination);
        s->setString(3, cfg->state);
        s->setString(4, cfg->symbolic_name);
        s->setInt(5, cfg->NOSTREAMS);
        s->setInt(6, cfg->TCP_BUFFER_SIZE);
        s->setInt(7, cfg->URLCOPY_TX_TO);
        s->setInt(8, cfg->NO_TX_ACTIVITY_TO);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s)
        		conn->destroyStatement(s, tag);
        }

		throw Err_Custom(e.what());
    }
}

void OracleAPI::updateLinkConfig(LinkConfig* cfg) {
    std::string tag = "updateLinkConfig";
    std::string query =
    		"update t_link_config "
    		"set state=:1,symbolicName=:2,NOSTREAMS=:3,tcp_buffer_size=:4,URLCOPY_TX_TO=:5,no_tx_activity_to=:6 "
    		"where source=:7 and destination=:8";
    oracle::occi::Statement* s = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, cfg->state);
        s->setString(2, cfg->symbolic_name);
        s->setInt(3, cfg->NOSTREAMS);
        s->setInt(4, cfg->TCP_BUFFER_SIZE);
        s->setInt(5, cfg->URLCOPY_TX_TO);
        s->setInt(6, cfg->NO_TX_ACTIVITY_TO);
        s->setString(7, cfg->source);
        s->setString(8, cfg->destination);
        s->executeUpdate();
        conn->commit();
        conn->destroyStatement(s, tag);
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s)
        		conn->destroyStatement(s, tag);
        }
        throw Err_Custom(e.what());
    }
}

void OracleAPI::deleteLinkConfig(std::string source, std::string destination) {
    const std::string tag = "deleteLinkConfig";
    std::string query = "delete from t_link_config where source=:1 and destination=:2";
    oracle::occi::Statement* s = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, source);
        s->setString(2, destination);
        s->executeUpdate();
        conn->commit();
        conn->destroyStatement(s, tag);
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s)
        		conn->destroyStatement(s, tag);
        }

        throw Err_Custom(e.what());
    }
}

LinkConfig* OracleAPI::getLinkConfig(std::string source, std::string destination) {
    std::string tag = "getLinkConfig";
    std::string query =
    		"select source,destination,state,symbolicName,nostreams, tcp_buffer_size, urlcopy_tx_to, no_tx_activity_to "
    		"from t_link_config where source=:1 and destination=:2";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    LinkConfig* cfg = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return NULL;

        s = conn->createStatement(query, tag);
        s->setString(1, source);
        s->setString(2, destination);
        r = conn->createResultset(s);
        if (r->next()) {
            cfg = new LinkConfig();
            cfg->source = r->getString(1);
            cfg->destination = r->getString(2);
            cfg->state = r->getString(3);
            cfg->symbolic_name = r->getString(4);
            cfg->NOSTREAMS = r->getInt(5);
            cfg->TCP_BUFFER_SIZE = r->getInt(6);
            cfg->URLCOPY_TX_TO = r->getInt(7);
            cfg->NO_TX_ACTIVITY_TO = r->getInt(8);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

        return cfg;
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
        }

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

    return cfg;
}

bool OracleAPI::isThereLinkConfig(std::string source, std::string destination) {

    std::string tag = "isThereLinkConfig";
    std::string query =
    		"select count(*) "
    		"from t_link_config "
    		"where state='on' and source=:1 and destination=:2 ";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    bool ret = false;

    try {
        if (false == conn->checkConn())
            return NULL;

        s = conn->createStatement(query, tag);
        s->setString(1, source);
        s->setString(2, destination);
        r = conn->createResultset(s);
        if (r->next()) {
        	ret = r->getInt(1) > 0;
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

        return ret;
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
        }
        throw Err_Custom(e.what());
    }

    return ret;
}

std::pair<std::string, std::string>* OracleAPI::getSourceAndDestination(std::string symbolic_name) {

    std::string tag = "getSourceAndDestination";
    std::string query =
    		"select source,destination "
    		"from t_link_config where symbolicName=:1";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    std::pair<std::string, std::string>* ret = 0;;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return NULL;

        s = conn->createStatement(query, tag);
        s->setString(1, symbolic_name);
        r = conn->createResultset(s);
        if (r->next()) {
        	ret = new std::pair<std::string, std::string>();
        	ret->first = r->getString(1);
        	ret->second = r->getString(2);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

        return ret;
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
        }

        throw Err_Custom(e.what());
    }

    return ret;
}

bool OracleAPI::isGrInPair(std::string group) {

    std::string tag = "isGrInPair";
    std::string query =
    		"select * from t_link_config "
    		"where (source=:1 and destination<>'*') or (source<>'*' and destination=:1)";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    bool ret = false;

    try {
        if (false == conn->checkConn())
            return NULL;

        s = conn->createStatement(query, tag);
        s->setString(1, group);
        r = conn->createResultset(s);
        if (r->next()) {
        	ret = true;
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

        return ret;
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
        }

        throw Err_Custom(e.what());
    }

    // if the exception was thrown don't allow to remove group
	return true;
}

void OracleAPI::addShareConfig(ShareConfig* cfg) {
    const std::string tag = "addShareConfig";
    std::string query =
    		"insert into t_share_config("
    		"	source,"
    		"	destination,"
    		"	vo,"
    		"	active"
    		") values(:1,:2,:3,:4)";
    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s = NULL;
    try {

        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, cfg->source);
        s->setString(2, cfg->destination);
        s->setString(3, cfg->vo);
        s->setInt(4, cfg->active_transfers);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s)
        		conn->destroyStatement(s, tag);
        }
        throw Err_Custom(e.what());
    }
}
void OracleAPI::updateShareConfig(ShareConfig* cfg) {
	std::string tag = "updateShareConfig";
	std::string query =
			"update t_share_config "
			"set active=:1 "
			"where source=:2 and destination=:3 and vo=:4";
	oracle::occi::Statement* s = NULL;

	ThreadTraits::LOCK_R lock(_mutex);

	try {
		if (false == conn->checkConn())
			return;

		s = conn->createStatement(query, tag);
		s->setInt(1, cfg->active_transfers);
		s->setString(2, cfg->source);
		s->setString(3, cfg->destination);
		s->setString(4, cfg->vo);
		s->executeUpdate();
		conn->commit();
		conn->destroyStatement(s, tag);
	}    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s)
        		conn->destroyStatement(s, tag);
        }

		throw Err_Custom(e.what());
	}
}

void OracleAPI::deleteShareConfig(std::string source, std::string destination, std::string vo) {
    const std::string tag = "deleteShareConfig";
    std::string query = "delete from t_share_config where source=:1 and destination=:2 and vo=:3";
    oracle::occi::Statement* s = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, source);
        s->setString(2, destination);
        s->setString(3, vo);
        s->executeUpdate();
        conn->commit();
        conn->destroyStatement(s, tag);
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s)
        		conn->destroyStatement(s, tag);
        }

        throw Err_Custom(e.what());
    }
}

void OracleAPI::deleteShareConfig(std::string source, std::string destination) {
    const std::string tag = "deleteShareConfig2";
    std::string query = "delete from t_share_config where source=:1 and destination=:2";
    oracle::occi::Statement* s = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, source);
        s->setString(2, destination);
        s->executeUpdate();
        conn->commit();
        conn->destroyStatement(s, tag);
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s)
        		conn->destroyStatement(s, tag);
        }

        throw Err_Custom(e.what());
    }
}

ShareConfig* OracleAPI::getShareConfig(std::string source, std::string destination, std::string vo) {
    std::string tag = "getShareConfig";
    std::string query =
    		"select source,destination,vo,active "
    		"from t_share_config where source=:1 and destination=:2 and vo=:3";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    ShareConfig* cfg = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return NULL;

        s = conn->createStatement(query, tag);
        s->setString(1, source);
        s->setString(2, destination);
        s->setString(3, vo);
        r = conn->createResultset(s);
        if (r->next()) {
            cfg = new ShareConfig();
            cfg->source = r->getString(1);
            cfg->destination = r->getString(2);
            cfg->vo = r->getString(3);
            cfg->active_transfers = r->getInt(4);
         }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

        return cfg;
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
        }

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

    return cfg;
}

std::vector<ShareConfig*> OracleAPI::getShareConfig(std::string source, std::string destination) {
    std::string tag = "getShareConfig2";
    std::string query =
    		"select source,destination,vo,active "
    		"from t_share_config where source=:1 and destination=:2";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    std::vector<ShareConfig*> ret;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return ret;

        s = conn->createStatement(query, tag);
        s->setString(1, source);
        s->setString(2, destination);
        r = conn->createResultset(s);
        while (r->next()) {
        	ShareConfig* cfg = new ShareConfig();
            cfg->source = r->getString(1);
            cfg->destination = r->getString(2);
            cfg->vo = r->getString(3);
            cfg->active_transfers = r->getInt(4);
            ret.push_back(cfg);
         }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

        return ret;
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
        }

        throw Err_Custom(e.what());
    }

    return ret;
}

void OracleAPI::submitHost(const std::string & jobId) {
    char hostname[MAXHOSTNAMELEN];
    gethostname(hostname, MAXHOSTNAMELEN);
    std::string tag = "submitHost";
    std::string query = "update t_job set submitHost=:1 where job_id=:2";
    oracle::occi::Statement* s = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return;

        s = conn->createStatement(query, tag);
        s->setString(1, std::string(hostname));
        s->setString(2, jobId);
        s->executeUpdate();
        conn->commit();
        conn->destroyStatement(s, tag);
    }    catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if (s)
        		conn->destroyStatement(s, tag);
        }

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

std::string OracleAPI::transferHost(int fileId) {
    std::string hostname("");
    std::string tag = "transferHost";
    std::string query = "select transferHost from t_file where file_id=:1";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return std::string("");

        s = conn->createStatement(query, tag);
        s->setInt(1, fileId);
	r = conn->createResultset(s);
	if (r->next()) {
		hostname = r->getString(1);
	}
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
    }catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
    
    return hostname;
}

/*for session reuse check only*/
bool OracleAPI::isFileReadyStateV(std::map<int, std::string>& fileIds) {
    const std::string tag = "isFileReadyStateV";
    std::string query = "select file_state from t_file where file_id=:1";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    std::string ready("");
    bool isReady = false;

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return false;

        s = conn->createStatement(query, tag);       
        s->setInt(1, fileIds.begin()->first);
        r = conn->createResultset(s);
        if (r->next()) {
        	ready = r->getString(1);
                if (ready.compare("READY") == 0) {
                    isReady = true;
                }
            }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        s = NULL;
        return isReady;
    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
    return isReady;
}

std::string OracleAPI::transferHostV(std::map<int, std::string>& fileIds) {   
    const std::string tag = "transferHostV";
    std::string host("");
    std::string query = "select transferHost from t_file where file_id=:1";
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;   

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        if (false == conn->checkConn())
            return std::string("");

        s = conn->createStatement(query, tag);
        s->setInt(1, fileIds.begin()->first);
	r = conn->createResultset(s);
	if(r->next()){
		host = r->getString(1);
	}
	conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        s = NULL;

    } catch (oracle::occi::SQLException const &e) {
        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
    
    return host;
}


bool OracleAPI::checkIfSeIsMemberOfAnotherGroup(const std::string & member) {
    std::string tag = "checkIfSeIsMemberOfAnotherGroup";
    std::string stmt = "select groupName from t_group_members where member=:1 ";

    oracle::occi::Statement* s = 0;
    oracle::occi::ResultSet* r = 0;
    bool ret = false;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (!conn->checkConn()) return ret;

        s = conn->createStatement(stmt, tag);
        s->setString(1, member);
        r = conn->createResultset(s);

        if (r->next()) {
            ret = true;
        }

        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {

        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

    return ret;
}


void OracleAPI::addJobShareConfig(std::string job_id, std::string source, std::string destination, std::string vo) {
	   const std::string tag = "addJobShareConfig";
	    std::string query =
	    		"insert into t_job_share_config("
	    		"	job_id,"
	    		"	source,"
	    		"	destination,"
	    		"	vo"
	    		") values(:1,:2,:3,:4)";
	    ThreadTraits::LOCK_R lock(_mutex);
	    oracle::occi::Statement* s = NULL;
	    try {

	        if (false == conn->checkConn())
	            return;

	        s = conn->createStatement(query, tag);
	        s->setString(1, job_id);
	        s->setString(2, source);
	        s->setString(3, destination);
	        s->setString(4, vo);
	        if (s->executeUpdate() != 0)
	            conn->commit();
	        conn->destroyStatement(s, tag);

	    } catch (oracle::occi::SQLException const &e) {
	        if (conn) {
	            conn->rollback();
	        	if(s)
	        		conn->destroyStatement(s, tag);
	        }
	        throw Err_Custom(e.what());
	    }
}

std::vector< boost::tuple<std::string, std::string, std::string> > OracleAPI::getJobShareConfig(std::string job_id) {

    std::string tag = "getJobShareConfig";
    std::string query = " select source, destination, vo from t_job_share_config where job_id=:1 ";

    oracle::occi::Statement* s = 0;
    oracle::occi::ResultSet* r = 0;

    std::vector< boost::tuple<std::string, std::string, std::string> > ret;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (!conn->checkConn()) return ret;

        s = conn->createStatement(query, tag);
        s->setString(1, job_id);
        r = conn->createResultset(s);

        while (r->next()) {
        	boost::tuple<std::string, std::string, std::string> tmp;
        	boost::get<0>(tmp) = r->getString(1);
        	boost::get<1>(tmp) = r->getString(2);
        	boost::get<2>(tmp) = r->getString(3);
        	ret.push_back(tmp);
        }

        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {

        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

    return ret;
}

bool OracleAPI::isThereJobShareConfig(std::string job_id) {

    std::string tag = "isThereJobShareConfig";
    std::string query = " select count(*) from t_job_share_config where job_id=:1 ";

    oracle::occi::Statement* s = 0;
    oracle::occi::ResultSet* r = 0;

    bool ret = false;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (!conn->checkConn()) return ret;

        s = conn->createStatement(query, tag);
        s->setString(1, job_id);
        r = conn->createResultset(s);

        if (r->next()) {
        	ret = r->getInt(1) > 0;
        }

        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {

        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

    return ret;
}

int OracleAPI::countActiveTransfers(std::string source, std::string destination, std::string vo) {

    std::string tag = "countSeActiveTransfers";
    std::string query =
    		"select count(*) "
    		"from t_file f, t_job_share_config c "
    		"where "
    		"	f.file_state = 'ACTIVE' and "
    		"	f.job_id = c.job_id and "
    		"	c.source = :1 and "
    		"	c.destination = :2 and "
    		"	c.vo = :3"
    		;

    oracle::occi::Statement* s = 0;
    oracle::occi::ResultSet* r = 0;

    int ret = 0;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (!conn->checkConn()) return ret;

        s = conn->createStatement(query, tag);
        s->setString(1, source);
        s->setString(2, destination);
        s->setString(3, vo);
        r = conn->createResultset(s);

        if (r->next()) {
        	ret = r->getInt(1);
        }

        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {

        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

    return ret;
}

int OracleAPI::countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo) {

    std::string tag = "countActiveOutboundTransfersUsingDefaultCfg";
    std::string query =
    		"select count(*) "
    		"from t_file f, t_job j, t_job_share_config c "
    		"where "
    		"	f.file_state = 'ACTIVE' and "
    		"	f.job_id = j.job_id and "
    		"	j.source_se = :1 and "
    		"	j.job_id = c.job_id and "
    		"	c.source = '(*)' and "
    		"	c.destination = '*' and "
    		"	c.vo = :2"
    		;

    oracle::occi::Statement* s = 0;
    oracle::occi::ResultSet* r = 0;

    int ret = 0;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (!conn->checkConn()) return ret;

        s = conn->createStatement(query, tag);
        s->setString(1, se);
        s->setString(2, vo);
        r = conn->createResultset(s);

        if (r->next()) {
        	ret = r->getInt(1);
        }

        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {

        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

    return ret;
}

int OracleAPI::countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo) {

    std::string tag = "countActiveInboundTransfersUsingDefaultCfg";
    std::string query =
    		"select count(*) "
    		"from t_file f, t_job j, t_job_share_config c "
    		"where "
    		"	f.file_state = 'ACTIVE' and "
    		"	f.job_id = j.job_id and "
    		"	j.dest_se = :1 and "
    		"	j.job_id = c.job_id and "
    		"	c.source = '*' and "
    		"	c.destination = '(*)' and "
    		"	c.vo = :2"
    		;

    oracle::occi::Statement* s = 0;
    oracle::occi::ResultSet* r = 0;

    int ret = 0;

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (!conn->checkConn()) return ret;

        s = conn->createStatement(query, tag);
        s->setString(1, se);
        s->setString(2, vo);
        r = conn->createResultset(s);

        if (r->next()) {
        	ret = r->getInt(1);
        }

        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {

        if (conn) {
            conn->rollback();
        	if(s && r)
        		conn->destroyResultset(s, r);
        	if (s)
        		conn->destroyStatement(s, tag);
       	}

        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

    return ret;
}


// the class factories

extern "C" GenericDbIfce* create() {
    return new OracleAPI;
}

extern "C" void destroy(GenericDbIfce* p) {
    if (p)
        delete p;
}

extern "C" MonitoringDbIfce* create_monitoring() {
    return new OracleMonitoring;
}

extern "C" void destroy_monitoring(MonitoringDbIfce* p) {
    if (p)
        delete p;
}
