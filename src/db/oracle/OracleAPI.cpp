#include "OracleAPI.h"
#include <fstream>
#include "error.h"
#include <algorithm>
#include "definitions.h"
#include <string>
#include <signal.h>
#include <stdlib.h>

using namespace FTS3_COMMON_NAMESPACE;


double convertBtoM( double byte,  double duration) {
    return ceil((((byte / duration) / 1024) / 1024) * 100 + 0.5) / 100;
}

OracleAPI::OracleAPI() : conn(NULL), conv(NULL)  {
}

OracleAPI::~OracleAPI() {
    if (conn)
        delete conn;
    if (conv)
        delete conv;
}

void OracleAPI::init(std::string username, std::string password, std::string connectString) {
    if (!conn)
        conn = new OracleConnection(username, password, connectString);   	
    if (!conv)
        conv = new OracleTypeConversions();
}

bool OracleAPI::getInOutOfSe(const std::string & sourceSe, const std::string & destSe) {
    const std::string tagse = "getInOutOfSese";
    const std::string taggroup = "getInOutOfSegroup";
    std::string query_stmt_se = " SELECT count(*) from t_se where "
            " (t_se.name=:1 or t_se.name=:2) "
            " and t_se.state='false' ";
    std::string query_stmt_group = " SELECT count(*) from t_se_group where "
            " (t_se_group.se_name=:1 or t_se_group.se_name=:2) "
            " and t_se_group.state='false' ";

    bool processSe = true;
    bool processGroup = true;
    oracle::occi::Statement* s_se = NULL;
    oracle::occi::ResultSet* rSe = NULL;
    oracle::occi::Statement* s_group = NULL;
    oracle::occi::ResultSet* rGroup = NULL;
    try {
        s_se = conn->createStatement(query_stmt_se, tagse);
        s_se->setString(1, sourceSe);
        s_se->setString(2, destSe);
        rSe = conn->createResultset(s_se);
        if (rSe->next()) {
            int count = rSe->getInt(1);
            if (count > 0) {
                processSe = false;
                conn->destroyResultset(s_se, rSe);
                conn->destroyStatement(s_se, tagse);
                return processSe;
            }
        }

        s_group = conn->createStatement(query_stmt_group, taggroup);
        s_group->setString(1, sourceSe);
        s_group->setString(2, destSe);
        rGroup = conn->createResultset(s_group);
        if (rGroup->next()) {
            int count = rGroup->getInt(1);
            if (count > 0)
                processGroup = false;
        }
       if(conn){
        if(s_se && rSe)
        	conn->destroyResultset(s_se, rSe);
	if(s_se)	
        	conn->destroyStatement(s_se, tagse);
	if(s_group && rGroup)
        	conn->destroyResultset(s_group, rGroup);
	if(s_group)	
        	conn->destroyStatement(s_group, taggroup);
        }
        if (processSe == false || processGroup == false)
            return false;
        else
            return true;

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	if(conn){
	    if(s_se && rSe){
	        conn->destroyResultset(s_se, rSe);
	        conn->destroyStatement(s_se, tagse);
		}
	     if(s_group && rGroup){	
        	conn->destroyResultset(s_group, rGroup);
	        conn->destroyStatement(s_group, taggroup);
		}

	}
	
        return true;
    }
    return true;
}


void OracleAPI::getSubmittedJobs(std::vector<TransferJobs*>& jobs, const std::string & vos) {
    TransferJobs* tr_jobs = NULL;
    std::string tag = "getSubmittedJobs";

    std::string query_stmt(""); 
    
    if(vos.length()==0){
    query_stmt = "SELECT /* FIRST_ROWS(25) */"
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
            " AND (t_job.reuse_job='N' or t_job.reuse_job is NULL) "
            " AND t_job.job_state in ('ACTIVE', 'READY','SUBMITTED') "
	    " AND exists(SELECT NULL FROM t_file WHERE t_file.job_id = t_job.job_id AND t_file.file_state = 'SUBMITTED') "
            " AND rownum <=25  ORDER BY t_job.priority DESC"
            " , SYS_EXTRACT_UTC(t_job.submit_time)";
    }else{
    tag +="1";
    query_stmt = "SELECT /* FIRST_ROWS(25) */"
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
	    " AND t_job.VO_NAME IN " + vos +
            " AND (t_job.reuse_job='N' or t_job.reuse_job is NULL) "
            " AND t_job.job_state in ('ACTIVE', 'READY','SUBMITTED') "
	    " AND exists(SELECT NULL FROM t_file WHERE t_file.job_id = t_job.job_id AND t_file.file_state = 'SUBMITTED') "	    
            " AND rownum <=25  ORDER BY t_job.priority DESC"
            " , SYS_EXTRACT_UTC(t_job.submit_time)";    
    }	    

    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    ThreadTraits::LOCK_R lock(_mutex);
    try {
    
        if ( false == conn->checkConn() )
		return;

        s = conn->createStatement(query_stmt, tag);	
        s->setPrefetchRowCount(25);
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
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	if(conn){
	      if(s && r){
	        conn->destroyResultset(s, r);
	        conn->destroyStatement(s, tag);
		}
	}
    }

}

void OracleAPI::getSeCreditsInUse(
        int &creditsInUse,
        std::string srcSeName,
        std::string destSeName,
        std::string voName
        ) {
    std::string tag = "getSeCreditsInUse";
    std::string query_stmt =
            "SELECT COUNT(*) "
            "FROM t_job, t_file "
            "WHERE "
            "	t_job.job_id = t_file.job_id "
            "	AND t_file.file_state = 'ACTIVE' ";
    tag.append("1");
    if (!srcSeName.empty()) {
        query_stmt +=
                //"	AND t_file.source_surl like '%://" + srcSeName + "%' ";
        		"	AND t_file.source_surl like :1 ";
        tag.append("2");
    }

    if (!destSeName.empty()) {
        query_stmt +=
                //"	AND t_file.dest_surl like '%://" + destSeName + "%' ";
        		"	AND t_file.dest_surl like :2 ";
        tag.append("3");
    }

    if (srcSeName.empty() || destSeName.empty()) {
        if (!voName.empty()) {
            // vo share
             //query_stmt += "	AND t_job.vo_name = '" + voName + "' "; // vo
        	query_stmt += "	AND t_job.vo_name = :3 "; // vo
            tag.append("4");
        } else {

            std::string seName = srcSeName.empty() ? destSeName : srcSeName;
            // public share
            // voName not in those who have voshare for this SE
            query_stmt +=
                    " 	AND NOT EXISTS ( "
                    "		SELECT * "
                    "		FROM t_se_vo_share "
                    "		WHERE "
         	    "			t_se_vo_share.se_name = :4 "
                    "			AND t_se_vo_share.share_type = 'se' "
                    "			AND t_se_vo_share.share_id = '%\"share_id\":\"' || t_job.vo_name || '\"%' "
                    "	) ";
            tag.append("5");
        }
    }
    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    try {
        if ( false == conn->checkConn() )
		return;
		
        s = conn->createStatement(query_stmt, tag);
        unsigned i = 1;
        if (!srcSeName.empty()) s->setString(i++, "%://" + srcSeName + "%");
        if (!destSeName.empty()) s->setString(i++, "%://" + destSeName + "%");
        if (!voName.empty()) s->setString(i++, voName);
        if (voName.empty()) s->setString(i, srcSeName.empty() ? destSeName : srcSeName);

        r = conn->createResultset(s);
        if (r->next()) {
            creditsInUse = r->getInt(1);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, "");
    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	if(conn){
	    if(s && r)
	        conn->destroyResultset(s, r);
        	conn->destroyStatement(s, "");
	}
		
    }
}

void OracleAPI::getGroupCreditsInUse(
        int &creditsInUse,
        std::string srcGroupName,
        std::string destGroupName,
        std::string voName
        ) {

    std::string tag = "getSiteCreditsInUse";
    std::string query_stmt =
            "SELECT COUNT(*) "
            "FROM t_job, t_file, t_se "
            "WHERE "
            "	t_job.job_id = t_file.job_id "
            "	AND t_file.file_state = 'ACTIVE' ";
    tag.append("1");
    if (!srcGroupName.empty()) {
        query_stmt +=
                //			"	AND t_se.site = :1 " // the se has the information about site name
                "	AND t_file.source_surl like '%://' || t_se.name || '%' " // srcSeName + "/%
                "	AND t_se.name IN ( "
                "		SELECT se_name "
                "		FROM t_se_group "
                "		WHERE se_group_name = '" + srcGroupName + "' "
                "	) ";
        tag.append("2");
    }

    if (!destGroupName.empty()) {
        query_stmt +=
                //			"	AND t_se.site = :2 " // the se has the information about site name
                "	AND t_file.dest_surl like '%://' || t_se.name || '%' " // destSeName + "/%
                "	AND t_se.name IN ( "
                "		SELECT se_name "
                "		FROM t_se_group "
                "		WHERE se_group_name = '" + destGroupName + "' "
                "	) ";
        tag.append("3");
    }

    if (srcGroupName.empty() || destGroupName.empty()) {
        if (!voName.empty()) {
            query_stmt +=
                    //			"	AND t_job.vo_name = :3 "; // vo
                    "	AND t_job.vo_name = '" + voName + "' "; // vo
            tag.append("4");
        } else {

            std::string groupName = srcGroupName.empty() ? destGroupName : srcGroupName;
            // voName not in those who have voshare for this SE
            query_stmt +=
                    " AND NOT EXISTS ( "
                    "		SELECT * "
                    "		FROM t_se_vo_share "
                    "		WHERE "
                    "			t_se_vo_share.se_name = '" + groupName + "' "
                    "			AND t_se_vo_share.share_type = 'group' "
                    "			AND t_se_vo_share.share_id = '%\"share_id\":\"' || t_job.vo_name || '\"%' "
                    "	) ";
            tag.append("5");
        }
    }
    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    try {
        if ( false == conn->checkConn() )
		return;
		
        s = conn->createStatement(query_stmt, "");
        r = conn->createResultset(s);
        if (r->next()) {
            creditsInUse = r->getInt(1);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, "");
    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	if(conn){
	       if(s && r){
	        conn->destroyResultset(s, r);
        	conn->destroyStatement(s, "");
		}
	}
		
    }
}

unsigned int OracleAPI::updateFileStatus(TransferFiles* file, const std::string status) {
    unsigned int updated = 0;    
    const std::string tag1 = "updateFileStatus1";
    const std::string tag2 = "updateFileStatus2";
    std::string query1 =
            "UPDATE t_file "
            "SET file_state =:1, start_time=:2 "
            "WHERE file_id = :3 AND FILE_STATE='SUBMITTED' ";
    std::string query2 =
            "UPDATE t_job "
            "SET job_state =:1 "
            "WHERE job_id = :2 AND JOB_STATE='SUBMITTED' ";
    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s2 = NULL;
    oracle::occi::Statement* s1 = NULL;
    try {
        time_t timed = time(NULL);
        s1 = conn->createStatement(query1, tag1);
        s1->setString(1, status);
        s1->setTimestamp(2, conv->toTimestamp(timed, conn->getEnv()));	
        s1->setInt(3, file->FILE_ID);
        updated = s1->executeUpdate();
        if (updated == 0) {
            conn->commit();
            conn->destroyStatement(s1, tag1);
            return 0;
        } else {
            conn->destroyStatement(s1, tag1);
            s2 = conn->createStatement(query2, tag2);
            s2->setString(1, status);
            s2->setString(2, file->JOB_ID);
            s2->executeUpdate();
            conn->commit();
            conn->destroyStatement(s2, tag2);
            return updated;
        }

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	if(conn){
		if(s1){
			conn->destroyStatement(s1, tag1);
		}
		if(s2){
			conn->destroyStatement(s2, tag2);
		}
	}	
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
		std::cout << "UPDATED TRUE " << std::endl;
        	conn->commit();
	}
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();   
	if(conn){
	     if(s){
        	conn->destroyStatement(s, tag);	
		}
	}
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::getByJobId(std::vector<TransferJobs*>& jobs, std::vector<TransferFiles*>& files) {
    std::string jobAppender("");
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
    ThreadTraits::LOCK_R lock(_mutex);
    try {
        if ( false == conn->checkConn() )
		return;
		
        s = conn->createStatement(select, selecttag);
        s->setPrefetchRowCount(300);
        for (iter = jobs.begin(); iter != jobs.end(); ++iter) {
            TransferJobs* temp = (TransferJobs*) * iter;
            std::string job_id = std::string(temp->JOB_ID);
            s->setString(1, job_id);
            oracle::occi::ResultSet* r = conn->createResultset(s);
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
        }

        conn->destroyStatement(s, selecttag);
    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	if(conn){
	     if(s){
		conn->destroyStatement(s, selecttag);
		}
	}
		
    }
}

void OracleAPI::submitPhysical(const std::string & jobId, std::vector<src_dest_checksum_tupple> src_dest_pair, const std::string & paramFTP,
        const std::string & DN, const std::string & cred, const std::string & voName, const std::string & myProxyServer,
        const std::string & delegationID, const std::string & spaceToken, const std::string & overwrite,
        const std::string & sourceSpaceToken, const std::string &, const std::string & lanConnection, int copyPinLifeTime,
        const std::string & failNearLine, const std::string & checksumMethod, const std::string & reuse,
        const std::string & sourceSE, const std::string & destSe) {

    std::string source;
    std::string destination;
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
    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s_job_statement = NULL;
    oracle::occi::Statement* s_file_statement = NULL;
    try {
        if ( false == conn->checkConn() ){
		throw Err_Custom("Can't connect to the database");
		return;
	}
		
        s_job_statement = conn->createStatement(job_statement, tag_job_statement);
        s_job_statement->setString(1, jobId); //job_id
        s_job_statement->setString(2, initial_state); //job_state
        s_job_statement->setString(3, paramFTP); //job_params
        s_job_statement->setString(4, DN); //user_dn
        s_job_statement->setString(5, cred); //user_cred
        s_job_statement->setInt(6, 3); //priority
        s_job_statement->setString(7, voName); //vo_name
        s_job_statement->setTimestamp(8, conv->toTimestamp(timed, conn->getEnv())); //submit_time
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
        s_file_statement = conn->createStatement(file_statement, tag_file_statement);		
        
	for (iter = src_dest_pair.begin(); iter != src_dest_pair.end(); ++iter) {
            s_file_statement->setString(1, jobId);
            s_file_statement->setString(2, initial_state);
            s_file_statement->setString(3, iter->source);
            s_file_statement->setString(4, iter->destination);
            s_file_statement->setString(5, iter->checksum);
            s_file_statement->executeUpdate();
        }
        conn->commit();
        conn->destroyStatement(s_job_statement, tag_job_statement);
        conn->destroyStatement(s_file_statement, tag_file_statement);
    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
	if(conn){
	      if(s_job_statement){
	        conn->destroyStatement(s_job_statement, tag_job_statement);
		}
		if(s_file_statement){
        	conn->destroyStatement(s_file_statement, tag_file_statement);	
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
    ThreadTraits::LOCK_R lock(_mutex);
    try {
    
        if ( false == conn->checkConn() ){
		throw Err_Custom("Can't connect to the database");
		return;
	}    
    
        s = conn->createStatement(query, tag);
        s->setString(1, requestID);
        s->setString(2, requestID);
        r = conn->createResultset(s);
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
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();    
	if(conn){
		if(s && r){
	        conn->destroyResultset(s, r);
        	conn->destroyStatement(s, tag);
		}
	}
        throw Err_Custom(e.what());		
    }
}

std::vector<std::string> OracleAPI::getSiteGroupNames() {
    std::string query = "SELECT distinct group_name FROM t_site_group";
    std::string tag = "getSiteGroupNames";
    std::vector<std::string> groups;

    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        while (r->next()) {
            std::string groupName = r->getString(1);
            groups.push_back(groupName);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
    
		if(conn)
			conn->rollback();    
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
		
    }
    return groups;
}

std::vector<std::string> OracleAPI::getSiteGroupMembers(std::string GroupName) {
    std::string query = "SELECT site_Name FROM t_site_group where group_Name = :1";
    std::string tag = "getSiteGroupMembers";
    std::vector<std::string> groups;

    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, GroupName);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        while (r->next()) {
            std::string groupName = r->getString(1);
            groups.push_back(groupName);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();    
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
		
    }
    return groups;
}

void OracleAPI::removeGroupMember(std::string groupName, std::string siteName) {
    std::string query = "DELETE FROM t_site_group WHERE group_Name = :1 AND site_Name = :2";
    std::string tag = "removeGroupMember";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, groupName);
        s->setString(2, siteName);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        conn->commit();
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
		
    }
}

void OracleAPI::addGroupMember(std::string groupName, std::string siteName) {

    std::string query = "INSERT INTO t_site_group (group_Name, site_Name) VALUES (:1,:2)";
    std::string tag = "addGroupMember";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, groupName);
        s->setString(2, siteName);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        conn->commit();
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
		
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
        tag.append("5");
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
   ThreadTraits::LOCK_R lock(_mutex);
   
    try {
    
        if ( false == conn->checkConn() ){
		throw Err_Custom("Can't connect to the database");
		return;
	}        
    
        s = conn->createStatement(sel, tag);
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


        r = conn->createResultset(s);
        while (r->next()) {
            std::string jid = r->getString(1);
            std::string jstate = r->getString(2);
            std::string reason = r->getString(3);
            time_t tm = conv->toTimeT(r->getTimestamp(4));
            std::string dn = r->getString(5);
            std::string voName = r->getString(6);
            int fileCount = r->getInt(7);
            int priority = r->getInt(8);
            std::string canceling = r->getString(9);

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

        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();    
	if(conn){
		if(s && r){
	        	conn->destroyResultset(s, r);
        		conn->destroyStatement(s, tag);	
		}
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
    ThreadTraits::LOCK_R lock(_mutex);
    try {
    
        if ( false == conn->checkConn() ){
		throw Err_Custom("Can't connect to the database");
		return;
	}       
	    
        s = conn->createStatement(query, tag);
        s->setString(1, requestID);
        r = conn->createResultset(s);
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
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();    
	if(conn){
		if(s && r){
	        	conn->destroyResultset(s, r);
	        	conn->destroyStatement(s, tag);
		}
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
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	if(conn){
	        conn->destroyResultset(s, r);
	        conn->destroyStatement(s, tag);
	}
		
    }
}

std::set<std::string> OracleAPI::getAllMatchingSeNames(std::string name) {

    std::set<std::string> result;
    const std::string tag = "getAllMatchingSeNames";
    std::string query_stmt =
            "SELECT "
            " 	t_se.NAME  "
            "FROM t_se "
            "WHERE t_se.NAME like :1";

	oracle::occi::Statement* s = NULL;
	oracle::occi::ResultSet* r = NULL;
    try {
        s = conn->createStatement(query_stmt, tag);
        s->setString(1, name);
        r = conn->createResultset(s);
        while (r->next()) {
            result.insert(
                    r->getString(1)
                    );
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	if(conn){
	        conn->destroyResultset(s, r);
        	conn->destroyStatement(s, tag);	
	}
		
    }

    return result;
}

std::set<std::string> OracleAPI::getAllMatchingSeGroupNames(std::string name) {

    std::set<std::string> result;
    const std::string tag = "getAllMatchingSeGroupNames";
    std::string query_stmt =
            "SELECT DISTINCT"
            " 	t_se_group.SE_GROUP_NAME  "
            "FROM t_se_group "
            "WHERE t_se_group.SE_GROUP_NAME like :1";

	oracle::occi::Statement* s = NULL;
	oracle::occi::ResultSet* r = NULL;
	
    try {
        s = conn->createStatement(query_stmt, tag);
        s->setString(1, name);
        r = conn->createResultset(s);
        while (r->next()) {
            result.insert(
                    r->getString(1)
                    );
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	if(conn){
	        conn->destroyResultset(s, r);
        	conn->destroyStatement(s, tag);	
	}
		
    }

    return result;
}

void OracleAPI::getAllSeInfoNoCritiria(std::vector<Se*>& se) {
    Se* seData = NULL;
    std::vector<Se*>::const_iterator iter;
    const std::string tag = "getAllSeInfoNoCritiria";
    std::string query_stmt = "SELECT "
            " t_se.ENDPOINT, "
            " t_se.SE_TYPE, "
            " t_se.SITE,  "
            " t_se.NAME,  "
            " t_se.STATE, "
            " t_se.VERSION,  "
            " t_se.HOST, "
            " t_se.SE_TRANSFER_TYPE, "
            " t_se.SE_TRANSFER_PROTOCOL, "
            " t_se.SE_CONTROL_PROTOCOL, "
            " t_se.GOCDB_ID "
            " FROM t_se";

	oracle::occi::Statement* s = NULL;
	oracle::occi::ResultSet* r = NULL;
	
    try {
        s = conn->createStatement(query_stmt, tag);
        r = conn->createResultset(s);
        while (r->next()) {
            seData = new Se();
            seData->ENDPOINT = r->getString(1);
            seData->SE_TYPE = r->getString(2);
            if (!r->isNull(3)) {
                seData->SITE = r->getString(3);
            }
            seData->NAME = r->getString(4);
            seData->STATE = r->getString(5);
            seData->VERSION = r->getString(6);
            seData->HOST = r->getString(7);
            seData->SE_TRANSFER_TYPE = r->getString(8);
            seData->SE_TRANSFER_PROTOCOL = r->getString(9);
            seData->SE_CONTROL_PROTOCOL = r->getString(10);
            seData->GOCDB_ID = r->getString(11);
            se.push_back(seData);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	if(conn){
	        conn->destroyResultset(s, r);
        	conn->destroyStatement(s, tag);	
	}
		
    }

}

void OracleAPI::getAllShareConfigNoCritiria(std::vector<SeConfig*>& seConfig) {
    SeConfig* seCon = NULL;
    std::vector<SeConfig*>::const_iterator iter;
    const std::string tag = "getAllSeConfigNoCritiria";
    std::string query_stmt = "SELECT "
            " T_SE_VO_SHARE.SE_NAME, "
            " T_SE_VO_SHARE.SHARE_ID, "
            " T_SE_VO_SHARE.SHARE_TYPE,  "
            " T_SE_VO_SHARE.SHARE_VALUE  "
            " FROM T_SE_VO_SHARE";

	oracle::occi::Statement* s = NULL;
	oracle::occi::ResultSet* r = NULL;
	
    try {
        s = conn->createStatement(query_stmt, tag);
        r = conn->createResultset(s);
        while (r->next()) {
            seCon = new SeConfig();
            seCon->SE_NAME = r->getString(1);
            seCon->SHARE_ID = r->getString(2);
            seCon->SHARE_TYPE = r->getString(3);
            seCon->SHARE_VALUE = r->getString(4);
            seConfig.push_back(seCon);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();    
    	if(conn){
	        conn->destroyResultset(s, r);
	        conn->destroyStatement(s, tag);
	}
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
		
    }
}

void OracleAPI::getAllShareAndConfigWithCritiria(std::vector<SeAndConfig*>& seAndConfig, std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE,std::string SHARE_VALUE) {
    unsigned int index = 1;
    SeAndConfig* seData = NULL;
    std::vector<SeAndConfig*>::iterator iter;
    std::string tag = "getAllSeAndConfigWithCritiria";
    std::stringstream query_stmt;
        
    query_stmt << " SELECT T_SE_VO_SHARE.SE_NAME, T_SE_VO_SHARE.SHARE_ID, T_SE_VO_SHARE.SHARE_TYPE, T_SE_VO_SHARE.SHARE_VALUE FROM T_SE_VO_SHARE ";

    bool first = true;
    if (SE_NAME.length() > 0) {

        if (first) {
            first = false;
            query_stmt << " where ";
	    tag.append("1");
        } else {
            query_stmt << " and ";
	    tag.append("2");	    
        }

        query_stmt << " T_SE_VO_SHARE.SE_NAME like :" << index;
        //query_stmt.append(SE_NAME);
        tag.append("3");
    }
    if (SHARE_ID.length() > 0) {

        if (first) {
            first = false;
            query_stmt << " where ";
	    tag.append("4");	    
        } else {
            query_stmt << " and ";
	    tag.append("5");	    
        }

        query_stmt << " T_SE_VO_SHARE.SHARE_ID like :" << ++index;
        //query_stmt.append(SHARE_ID);
        tag.append("6");
    }
    if (SHARE_TYPE.length() > 0) {

        if (first) {
            first = false;
            query_stmt << " where ";
            tag.append("7");	    
        } else {
            query_stmt << " and ";
            tag.append("8");	    
        }

        query_stmt << " T_SE_VO_SHARE.SHARE_TYPE =:" << ++index;
        //query_stmt.append(SHARE_TYPE);
        tag.append("9");
    }
    if (SHARE_VALUE.length() > 0) {

        if (first) {
            first = false;
            query_stmt << " where ";
            tag.append("10");	    
        } else {
            query_stmt << " and ";
            tag.append("11");	    
        }

        query_stmt << " T_SE_VO_SHARE.SHARE_VALUE like :" << ++index;
        //query_stmt.append(SHARE_VALUE);
        tag.append("12");
    }

        oracle::occi::Statement* s = NULL;
        oracle::occi::ResultSet* r = NULL;
	ThreadTraits::LOCK_R lock(_mutex);

    try {
        s = conn->createStatement(query_stmt.str(), tag);
	
	index = 1;
	if(SE_NAME.length() > 0)
		s->setString(index, "%"+SE_NAME+"%");
	++index;
	if(SHARE_ID.length() > 0)
		s->setString(index, "%"+SHARE_ID+"%");		
	++index;
	if(SHARE_TYPE.length() > 0)
		s->setString(index, SHARE_TYPE);				
	++index;
	if(SHARE_VALUE.length() > 0)
		s->setString(index, "%"+SHARE_VALUE+"%");						
		
        r = conn->createResultset(s);
        while (r->next()) {
            seData = new SeAndConfig();
            seData->SE_NAME = r->getString(1);
            seData->SHARE_ID = r->getString(2);
            seData->SHARE_TYPE = r->getString(3);
            seData->SHARE_VALUE = r->getString(4);

            seAndConfig.push_back(seData);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();    
    	if(conn){
	  if(s && r)
	        conn->destroyResultset(s, r);
        	conn->destroyStatement(s, tag);	
	}
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
		
    }
}

void OracleAPI::addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
        std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID) {
    std::string query = "INSERT INTO t_se (ENDPOINT, SE_TYPE, SITE, NAME, STATE, VERSION, HOST, SE_TRANSFER_TYPE, SE_TRANSFER_PROTOCOL,SE_CONTROL_PROTOCOL,GOCDB_ID) VALUES (:1,:2,:3,:4,:5,:6,:7,:8,:9,:10,:11)";
    std::string tag = "addSe";

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
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
		if(conn)
			conn->rollback();
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
    query.append(" AND SE_TRANSFER_PROTOCOL='");
    query.append(SE_TRANSFER_PROTOCOL);
    query.append("'");
    ThreadTraits::LOCK_R lock(_mutex);				
    try {
        oracle::occi::Statement* s = conn->createStatement(query, "");
	
        if(s->executeUpdate()!=0)
        	conn->commit();
        conn->destroyStatement(s, "");

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
		
    }

}

void OracleAPI::addSeConfig(std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE, std::string SHARE_VALUE) {
    std::string query = "INSERT INTO T_SE_VO_SHARE (SE_NAME, SHARE_ID, SHARE_TYPE, SHARE_VALUE) VALUES (:1,:2,:3,:4)";
    std::string tag = "addSeConfig";
    ThreadTraits::LOCK_R lock(_mutex);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, SE_NAME);
        s->setString(2, SHARE_ID);
        s->setString(3, SHARE_TYPE);
        s->setString(4, SHARE_VALUE);
        if(s->executeUpdate() != 0)
        	conn->commit();
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
		
    }

}

/*
Update the config of an SE
REQUIRED: SE_NAME / VO_NAME
OPTIONAL; the rest
set int to -1 so as NOT to be changed
 */
void OracleAPI::updateSeConfig(std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE, std::string SHARE_VALUE) {
    int fields = 0;
    std::string query = "UPDATE T_SE_VO_SHARE SET ";
    if (SHARE_VALUE.length() > 0) {
        if (fields > 0) {
            fields = 0;
            query.append(", ");
        }
        query.append(" SHARE_VALUE='");
        query.append(SHARE_VALUE);
        query.append("'");
    }
    query.append(" WHERE SHARE_ID ='");
    query.append(SHARE_ID);
    query.append("'");
    query.append(" AND ");
    query.append(" SE_NAME ='");
    query.append(SE_NAME);
    query.append("'");
    query.append(" AND SHARE_TYPE ='");
    query.append(SHARE_TYPE);
    query.append("'");

    ThreadTraits::LOCK_R lock(_mutex);

    try {
        oracle::occi::Statement* s = conn->createStatement(query, "");
        if(s->executeUpdate()!=0)
        	conn->commit();
        conn->destroyStatement(s, "");

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
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
    ThreadTraits::LOCK_R lock(_mutex);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, NAME);
        if(s->executeUpdate()!=0)
        	conn->commit();
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
		
    }
}

/*
Delete the config of an SE for a specific VO
REQUIRED: SE_NAME
OPTIONAL: VO_NAME
 */
void OracleAPI::deleteSeConfig(std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE) {
    std::string query = "DELETE FROM T_SE_VO_SHARE WHERE SE_NAME like :1 ";//'" + SE_NAME + "'";
    query.append(" AND SHARE_ID like :2 ");//'" + SHARE_ID + "'");
    query.append(" AND SHARE_TYPE = :3");//'" + SHARE_TYPE + "'");
    ThreadTraits::LOCK_R lock(_mutex);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, "");
        s->setString(1, SE_NAME);
        s->setString(2, SHARE_ID);
        s->setString(3, SHARE_TYPE);
        if(s->executeUpdate()!=0)
        	conn->commit();
        conn->destroyStatement(s, "");

    } catch (oracle::occi::SQLException const &e) {
		if(conn)
			conn->rollback();
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
		if(conn)
			conn->rollback();
        if(conn){
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
    std::string state = status;
    int finishedDirty = 0;
    int numberOfFileInJob = 0;
    int numOfFilesInGivenState = 0;
    int failedExistInJob = 0;
    const std::string tag1 = "updateJobTransferStatus";
    const std::string tag2 = "selectJobTransferStatus";
    const std::string tag3 = "selectJobNotFinished";
    std::string update =
            "UPDATE t_job "
            "SET JOB_STATE=:1, JOB_FINISHED =:2, FINISH_TIME=:3, REASON=:4 "
            "WHERE job_id = :5 AND JOB_STATE='ACTIVE'";

    std::string updateJobNotFinished =
            "UPDATE t_job "
            "SET JOB_STATE=:1 WHERE job_id = :2 AND JOB_STATE IN ('READY','ACTIVE') ";

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
                if (st->executeUpdate() != 0){   		        
                    conn->commit();
		}
            }
        }

        conn->destroyStatement(st, "");
        st = NULL;
        finishedDirty = 0;
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        if (conn && st && r) {
            conn->destroyResultset(st, r);
        }
        if (conn && st) {
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
        if (conn)
            conn->rollback();
        if (st1)
            conn->destroyStatement(st1, cancelJTag);
        if (st2)
            conn->destroyStatement(st2, cancelFTag);
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

void OracleAPI::getCancelJob(std::vector<int>& requestIDs) {
    const std::string tag = "getCancelJob";
    const std::string tag1 = "getCancelJobUpdateCancel";
    std::string query = "select t_file.pid, t_job.job_id from t_file, t_job where t_file.job_id=t_job.job_id and t_file.FILE_STATE='CANCELED' and t_file.PID IS NOT NULL AND t_job.CANCEL_JOB IS NULL ";
    std::string update = "update t_job SET CANCEL_JOB='Y' where job_id=:1 ";

    ThreadTraits::LOCK_R lock(_mutex);
    oracle::occi::Statement* s = NULL;
    oracle::occi::ResultSet* r = NULL;
    oracle::occi::Statement* s1 = NULL;
    try {
        if (false == conn->checkConn())
            return;
        s = conn->createStatement(query, tag);
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
        if (conn)
            conn->rollback();
        if (r)
            conn->destroyResultset(s, r);
        if (s)
            conn->destroyStatement(s, tag);
        if (s1)
            conn->destroyStatement(s1, tag1);
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}


/*Protocol config*/

/*check if a SE belongs to a group*/
bool OracleAPI::is_se_group_member(std::string se) {
    const std::string tag = "is_se_group_member";
    std::string query = "select SE_NAME from t_se_group where SE_NAME=:1 ";
    bool exists = false;
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, se);
        oracle::occi::ResultSet* r = conn->createResultset(s);

        if (r->next()) {
            exists = true;
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        return exists;
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return exists;
    }
    return exists;
}

bool OracleAPI::is_se_group_exist(std::string group) {
    const std::string tag = "is_se_group_exist";
    std::string query = "select SE_GROUP_NAME from t_se_group where SE_GROUP_NAME like :1 ";
    bool exists = false;
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, group);
        oracle::occi::ResultSet* r = conn->createResultset(s);

        if (r->next()) {
            exists = true;
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        return exists;
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return exists;
    }
    return exists;
}

/*check if a SE belongs to a group*/
bool OracleAPI::is_se_protocol_exist(std::string se) {
    const std::string tag = "is_se_protocol_exist";
    std::string query = "select SE_NAME from t_se_protocol where SE_NAME=:1 and se_group_name IS NULL ";
    bool exists = false;
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, se);
        oracle::occi::ResultSet* r = conn->createResultset(s);

        if (r->next()) {
            exists = true;
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        return exists;
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return exists;
    }
    return exists;
}

bool OracleAPI::is_group_protocol_exist(std::string group) {
    const std::string tag = "is_group_protocol_exist";
    std::string query = "select SE_GROUP_NAME from t_se_protocol where SE_GROUP_NAME=:1 ";
    bool exists = false;
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, group);
        oracle::occi::ResultSet* r = conn->createResultset(s);

        if (r->next()) {
            exists = true;
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        return exists;
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return exists;
    }
    return exists;

}

SeProtocolConfig* OracleAPI::get_se_protocol_config(std::string se) {
    SeProtocolConfig* seProtocolConfig = NULL;
    const std::string tag = "get_se_protocol_config";
    std::string query = "select NOSTREAMS, URLCOPY_TX_TO, TCP_BUFFER_SIZE, SE_GROUP_NAME,SE_NAME,CONTACT,BANDWIDTH,NOFILES, "
            "NOMINAL_THROUGHPUT,SE_PAIR_STATE,LAST_ACTIVE,MESSAGE,LAST_MODIFICATION,"
            "ADMIN_DN,SE_LIMIT,BLOCKSIZE ,HTTP_TO ,TX_LOGLEVEL ,URLCOPY_PUT_TO,URLCOPY_PUTDONE_TO,URLCOPY_GET_TO,"
            "URLCOPY_GETDONE_TO,URLCOPY_TXMARKS_TO,SRMCOPY_DIRECTION,SRMCOPY_TO,SRMCOPY_REFRESH_TO,"
            "TARGET_DIR_CHECK ,URL_COPY_FIRST_TXMARK_TO,TX_TO_PER_MB ,NO_TX_ACTIVITY_TO,PREPARING_FILES_RATIO FROM t_se_protocol where SE_NAME=:1 and SE_GROUP_NAME IS NULL";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, se);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        seProtocolConfig = new SeProtocolConfig();
        if (r->next()) {
            seProtocolConfig->NOSTREAMS = r->getInt(1);
            seProtocolConfig->URLCOPY_TX_TO = r->getInt(2);
            seProtocolConfig->TCP_BUFFER_SIZE = r->getInt(3);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        return seProtocolConfig;
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return seProtocolConfig;
    }
    return seProtocolConfig;
}

SeProtocolConfig* OracleAPI::get_se_group_protocol_config(std::string se) {
    SeProtocolConfig* seProtocolConfig = NULL;
    const std::string tag = "get_se_group_protocol_config";
    std::string query = "select NOSTREAMS, URLCOPY_TX_TO, TCP_BUFFER_SIZE, SE_GROUP_NAME,SE_NAME,CONTACT,BANDWIDTH,NOFILES, "
            " NOMINAL_THROUGHPUT,SE_PAIR_STATE,LAST_ACTIVE,MESSAGE,LAST_MODIFICATION,"
            "ADMIN_DN,SE_LIMIT,BLOCKSIZE ,HTTP_TO ,TX_LOGLEVEL ,URLCOPY_PUT_TO,URLCOPY_PUTDONE_TO,URLCOPY_GET_TO,"
            "URLCOPY_GETDONE_TO,URLCOPY_TXMARKS_TO,SRMCOPY_DIRECTION,SRMCOPY_TO,SRMCOPY_REFRESH_TO,"
            "TARGET_DIR_CHECK ,URL_COPY_FIRST_TXMARK_TO,TX_TO_PER_MB ,NO_TX_ACTIVITY_TO,PREPARING_FILES_RATIO FROM t_se_protocol where"
            " SE_GROUP_NAME=:1";

    std::string group = get_group_name(se);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, group);
        oracle::occi::ResultSet* r = conn->createResultset(s);

        seProtocolConfig = new SeProtocolConfig();
        if (r->next()) {
            seProtocolConfig->NOSTREAMS = r->getInt(1);
            seProtocolConfig->URLCOPY_TX_TO = r->getInt(2);
            seProtocolConfig->TCP_BUFFER_SIZE = r->getInt(3);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        return seProtocolConfig;
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return seProtocolConfig;
    }
    return seProtocolConfig;
}

SeProtocolConfig* OracleAPI::get_group_protocol_config(std::string group) {
    SeProtocolConfig* seProtocolConfig = NULL;
    const std::string tag = "get_se_group_protocol_config";
    std::string query = "select NOSTREAMS, URLCOPY_TX_TO, SE_GROUP_NAME,SE_NAME,CONTACT,BANDWIDTH,NOFILES, "
            "TCP_BUFFER_SIZE,NOMINAL_THROUGHPUT,SE_PAIR_STATE,LAST_ACTIVE,MESSAGE,LAST_MODIFICATION,"
            "ADMIN_DN,SE_LIMIT,BLOCKSIZE ,HTTP_TO ,TX_LOGLEVEL ,URLCOPY_PUT_TO,URLCOPY_PUTDONE_TO,URLCOPY_GET_TO,"
            "URLCOPY_GETDONE_TO,URLCOPY_TXMARKS_TO,SRMCOPY_DIRECTION,SRMCOPY_TO,SRMCOPY_REFRESH_TO,"
            "TARGET_DIR_CHECK ,URL_COPY_FIRST_TXMARK_TO,TX_TO_PER_MB ,NO_TX_ACTIVITY_TO,PREPARING_FILES_RATIO FROM t_se_protocol where"
            " SE_GROUP_NAME=:1";

    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, group);
        oracle::occi::ResultSet* r = conn->createResultset(s);

        seProtocolConfig = new SeProtocolConfig();
        if (r->next()) {
            seProtocolConfig->NOSTREAMS = r->getInt(1);
            seProtocolConfig->URLCOPY_TX_TO = r->getInt(2);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
    return seProtocolConfig;
}

bool OracleAPI::add_se_protocol_config(SeProtocolConfig* seProtocolConfig) {
    const std::string tag = "add_se_protocol_config";
    std::string query = "insert into t_se_protocol(SE_NAME,NOSTREAMS,URLCOPY_TX_TO) values(:1,:2,:3)";
    ThreadTraits::LOCK_R lock(_mutex);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, seProtocolConfig->SE_NAME);
        s->setInt(2, seProtocolConfig->NOSTREAMS);
        s->setInt(3, seProtocolConfig->URLCOPY_TX_TO);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
        return true;
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return false;
    }
    return true;
}

bool OracleAPI::add_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig) {
    const std::string tag = "add_se_group_protocol_config";
    std::string query = "insert into t_se_protocol(SE_GROUP_NAME,NOSTREAMS,URLCOPY_TX_TO) values(:1,:2,:3)";

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, seGroupProtocolConfig->SE_GROUP_NAME);
        s->setInt(2, seGroupProtocolConfig->NOSTREAMS);
        s->setInt(3, seGroupProtocolConfig->URLCOPY_TX_TO);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
        return true;
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return false;
    }

    return true;
}

void OracleAPI::delete_se_protocol_config(SeProtocolConfig* seProtocolConfig) {
    const std::string tag = "delete_se_protocol_config";
    std::string query = "delete from t_se_protocol where SE_NAME like :1 and SE_GROUP_NAME IS NULL ";

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, seProtocolConfig->SE_NAME);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

void OracleAPI::delete_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig) {
    const std::string tag = "delete_se_group_protocol_config";
    std::string query = "delete from t_se_protocol where SE_GROUP_NAME like :1";

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, seGroupProtocolConfig->SE_GROUP_NAME);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

/*
void OracleAPI::update_se_pair_protocol_config(SeProtocolConfig* sePairProtocolConfig){
}
 */


void OracleAPI::update_se_protocol_config(SeProtocolConfig* seProtocolConfig) {
    unsigned int index = 1;
    std::string tag = "update_se_group_protocol_config";
    std::stringstream query;
    query << "UPDATE t_se_protocol SET ";
    if (seProtocolConfig->NOSTREAMS > 0) {
        query << " NOSTREAMS = :" << index;
        ++index;
        tag += "1";
    }
    if (seProtocolConfig->URLCOPY_TX_TO > 0) {
        if (index == 2) query << ",";
        query << " URLCOPY_TX_TO = :" << index;
        ++index;
        tag += "2";
    }
    query << " WHERE SE_NAME = :" << index;

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        index = 1; //reset index
        oracle::occi::Statement* s = conn->createStatement(query.str(), tag);
        if (seProtocolConfig->NOSTREAMS > 0) {
            s->setInt(index, seProtocolConfig->NOSTREAMS);
            ++index;
        }
        if (seProtocolConfig->URLCOPY_TX_TO > 0) {
            s->setInt(index, seProtocolConfig->URLCOPY_TX_TO);
            ++index;
        }
        s->setString(index, seProtocolConfig->SE_NAME);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

void OracleAPI::update_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig) {
    unsigned int index = 1;
    std::string tag = "update_se_group_protocol_config";
    std::stringstream query;
    query << "UPDATE t_se_protocol SET ";
    if (seGroupProtocolConfig->NOSTREAMS > 0) {
        query << " NOSTREAMS = :" << index;
        ++index;
        tag += "1";
    }
    if (seGroupProtocolConfig->URLCOPY_TX_TO > 0) {
        if (index == 2) query << ",";
        query << " URLCOPY_TX_TO = :" << index;
        ++index;
        tag += "2";
    }
    query << " WHERE se_group_name = :" << index;

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        index = 1; //reset index
        oracle::occi::Statement* s = conn->createStatement(query.str(), tag);
        if (seGroupProtocolConfig->NOSTREAMS > 0) {
            s->setInt(index, seGroupProtocolConfig->NOSTREAMS);
            ++index;
        }
        if (seGroupProtocolConfig->URLCOPY_TX_TO > 0) {
            s->setInt(index, seGroupProtocolConfig->URLCOPY_TX_TO);
            ++index;
        }
        s->setString(index, seGroupProtocolConfig->SE_GROUP_NAME);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

SeProtocolConfig* OracleAPI::getProtocol(std::string, std::string se2) {

    ThreadTraits::LOCK_R lock(_mutex);
    bool is_dest_group = is_se_group_member(se2);

    if (is_dest_group) {
        return get_se_group_protocol_config(se2);
    } else {
        bool is_se = is_se_protocol_exist(se2);
        if (is_se) {
            return get_se_protocol_config(se2);
        }
    }

    return NULL;
}

/*se group operations*/
void OracleAPI::add_se_to_group(std::string se, std::string group) {
    const std::string tag = "add_se_to_group";
    std::string query = "insert into t_se_group(se_group_name, se_name) values(:1,:2)";

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, group);
        s->setString(2, se);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

void OracleAPI::remove_se_from_group(std::string se, std::string group) {
    const std::string tag = "remove_se_from_group";
    std::string query = "delete from t_se_group where se_name=:1 and se_group_name=:2";

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, se);
        s->setString(2, group);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

void OracleAPI::delete_group(std::string group) {
    const std::string tag = "delete_group";
    std::string query = "delete from t_se_group where se_group_name like :1";

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, group);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

std::string OracleAPI::get_group_name(std::string se) {
    const std::string tag = "get_group_name";
    std::string group("");
    std::string query = "select SE_GROUP_NAME from t_se_group where SE_NAME=:1";
    ThreadTraits::LOCK_R lock(_mutex);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, se);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        if (r->next()) {
            group = r->getString(1);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        return group;
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

        return group;
    }
    return group;

}

std::vector<std::string> OracleAPI::get_group_names() {
    const std::string tag = "get_group_names";
    std::vector<std::string> group;
    std::string query = "select distinct SE_GROUP_NAME from t_se_group";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        while (r->next()) {
            group.push_back(r->getString(1));
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
    return group;
}

std::vector<std::string> OracleAPI::get_group_members(std::string name) {
    const std::string tag = "get_group_members";
    std::vector<std::string> members;
    std::string query = "select SE_NAME from t_se_group where SE_GROUP_NAME=:1";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, name);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        while (r->next()) {
            members.push_back(r->getString(1));
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
    return members;
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
        if (conn)
            conn->rollback();
        if (conn) {
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
        if (conn)
            conn->rollback();
        if (conn) {
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
        if (conn)
            conn->rollback();
        if (conn) {
            if (r)
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

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        if (false == conn->checkConn())
            return;

        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, delegationID);
        s->setString(2, dn);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
        s = NULL;
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
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
        if (conn)
            conn->rollback();
        if (conn && s && s1) {
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
        if (conn)
            conn->rollback();
        if (conn) {
            if (s) {
                conn->destroyStatement(s, tag);
            }
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
        if (conn)
            conn->rollback();
        if (conn) {
            conn->destroyResultset(s, r);
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

    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (false == conn->checkConn())
            return;

        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, delegationID);
        s->setString(2, dn);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
        s = NULL;
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
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
        if (conn)
            conn->rollback();
        if (conn) {
            if (s && r) {
                conn->destroyResultset(s, r);
                conn->destroyStatement(s, tag);
            }
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
    ThreadTraits::LOCK_R lock(_mutex);
    try {

        if (false == conn->checkConn())
            return;

        oracle::occi::Statement* s1 = conn->createStatement(query1, tag1);
        oracle::occi::Statement* s2 = conn->createStatement(query2, tag2);
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
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

void OracleAPI::getSubmittedJobsReuse(std::vector<TransferJobs*>& jobs, const std::string & vos) {
    TransferJobs* tr_jobs = NULL;
    std::vector<TransferJobs*>::const_iterator iter;
    std::string tag = "getSubmittedJobsReuse";
    std::string query_stmt("");

    if(vos.length()==0){
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
    }else{
    tag+="1";
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
            jobs.push_back(tr_jobs);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        if (conn) {
            if (s && r) {
                conn->destroyResultset(s, r);
                conn->destroyStatement(s, tag);
            }
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }

}

void OracleAPI::auditConfiguration(const std::string & dn, const std::string & config, const std::string & action) {
    const std::string tag = "auditConfiguration";
    std::string query = "INSERT INTO t_config_audit (when, dn, config, action ) VALUES (:1, :2, :3, :4)";

    ThreadTraits::LOCK_R lock(_mutex);
    try {
        time_t timed = time(NULL);
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setTimestamp(1, conv->toTimestamp(timed, conn->getEnv()));
        s->setString(2, dn);
        s->setString(3, config);
        s->setString(4, action);
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

void OracleAPI::setGroupOrSeState(const std::string & se, const std::string & group, const std::string & state) {
    std::string tag = "setGroupOrSeState";
    std::string query("");

    if (se.length() > 0) {
        query = "update t_se set state=:1 where name=:2";
    } else {
        query = "update t_se_group set state=:1 where se_group_name=:2";
        tag += "1";
    }
    ThreadTraits::LOCK_R lock(_mutex);
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        if (se.length() > 0) {
            s->setString(1, state);
            s->setString(2, se);
        } else {
            s->setString(1, state);
            s->setString(2, group);
        }
        if (s->executeUpdate() != 0)
            conn->commit();
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

/*custom optimization stuff*/

void OracleAPI::fetchOptimizationConfig2(OptimizerSample* ops, const std::string & source_hostname, const std::string & destin_hostname) {
    const std::string tag = "fetchOptimizationConfig2XXX";
    const std::string tag1 = "fetchOptimizationConfig2YYY";
    const std::string tag2 = "fetchOptimizationConfig2YYYXXX";
    const std::string tag3 = "fetchOptimizationConfig2ZZZZ";
    const std::string tag4 = "111fetchOptimizationConfig2ZZZZ";
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
        if (conn)
            conn->rollback();
        if (conn) {
            if (s3 && r3) {
                conn->destroyResultset(s3, r3);
                conn->destroyStatement(s3, tag3);
            }
            if (s1 && r1) {
                conn->destroyResultset(s1, r1);
                conn->destroyStatement(s1, tag1);
            }
            if (s && r) {
                conn->destroyResultset(s, r);
                conn->destroyStatement(s, tag2);
            }
            if (sMid && rMid) {
                conn->destroyResultset(sMid, rMid);
                conn->destroyStatement(sMid, tagMid);
            }
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));

    }
}

void OracleAPI::updateOptimizer(std::string, double filesize, int timeInSecs, int nostreams, int timeout, int buffersize, std::string source_hostname, std::string destin_hostname) {
    const std::string tag1 = "updateOptimizer1";
    const std::string tag2 = "updateOptimizer2";
    const std::string tag3 = "updateOptimizer3";
    double throughput = 0;
    bool activeExists = false;
    int active = 0;

    std::string query1 = " select active from t_optimize where "
            " active=(select count(*) from  t_file, t_job where t_file.file_state='ACTIVE' "
            " and t_job.job_id = t_file.job_id and t_job.source_se=:1 and t_job.dest_se=:2)"
            " and nostreams = :3 and timeout=:4 and buffer=:5 and source_se=:6 and dest_se=:7 ";

    std::string query2 = "UPDATE t_optimize SET filesize = :1, throughput = :2, active=:3, when=:4, timeout=:5 "
            " WHERE nostreams = :6 and timeout=:7 and buffer=:8 and source_se=:9 and dest_se=:10 ";
    std::string query3 = "select count(*) from t_optimize where source_se=:1 and dest_se=:2";
    oracle::occi::Statement* s1 = NULL;
    oracle::occi::ResultSet* r1 = NULL;
    oracle::occi::Statement* s2 = NULL;
    oracle::occi::Statement* s3 = NULL;
    oracle::occi::ResultSet* r3 = NULL;
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
            int count = 0;
            s3 = conn->createStatement(query3, tag3);
            s3->setString(1, source_hostname);
            s3->setString(2, destin_hostname);
            r3 = conn->createResultset(s3);
            if (r3->next()) {
                count = r3->getInt(1);
            }
            conn->destroyResultset(s3, r3);
            conn->destroyStatement(s3, tag3);

            if (count < 400) {
                if (timeInSecs <= DEFAULT_TIMEOUT) {
                    timeout = DEFAULT_TIMEOUT;
                }
                addOptimizer(now, throughput, source_hostname, destin_hostname, 1, nostreams, timeout, buffersize, active);
            }
        }
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        if (conn) {
            if (s1 && r1) {
                conn->destroyResultset(s1, r1);
                conn->destroyStatement(s1, tag1);
            }
            if (s2) {
                conn->destroyStatement(s2, tag2);
            }
            if (s3 && r3) {
                conn->destroyResultset(s3, r3);
                conn->destroyStatement(s3, tag3);
            }
        }
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::addOptimizer(time_t when, double throughput, const std::string & source_hostname, const std::string & destin_hostname, int file_id, int nostreams, int timeout, int buffersize, int) {
    const std::string tag = "addOptimizer";
    const std::string tag1 = "addOptimizer1";
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
        if (conn)
            conn->rollback();
        if (conn) {
            if (s) {
                conn->destroyStatement(s, tag);
            }
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
        if (conn)
            conn->rollback();
        if (conn) {
            if (s1 && r1) {
                conn->destroyResultset(s1, r1);
                conn->destroyStatement(s1, tag1);
            }
            if (s) {
                conn->destroyStatement(s, tag);
            }
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
        if (conn)
            conn->rollback();
        if (conn) {
            if (s && r) {
                conn->destroyResultset(s, r);
                conn->destroyStatement(s, tag);
            }
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
    bool allowed = true;
    double actThr = 0;
    int act = 0;
    int maxDest = 0;
    int noFailedPerSePair = 0;
    std::string query_stmt1 = " select count(*) from  t_file, t_job where t_file.file_state='ACTIVE' and t_job.job_id = t_file.job_id and t_job.source_se=:1 and t_job.dest_se=:2 ";


    std::string query_stmt2 = " select avg(throughput) from t_optimize  "
            " where (active=(select count(*) from  t_file, t_job where t_file.file_state='ACTIVE' "
            " and  t_job.job_id = t_file.job_id and t_job.source_se=:1 and t_job.dest_se=:2) or "
            " active=(select max(active) from t_optimize where active < (select count(*) from  t_file, t_job where "
            " (t_file.file_state='ACTIVE' OR t_file.file_state='READY') and  t_job.job_id = t_file.job_id and t_job.source_se=:3 and t_job.dest_se=:4 "
            " )))  and  source_se=:5  and dest_se=:6 and t_optimize.when  is not null order by "
            " SYS_EXTRACT_UTC(when) desc";

    std::string query_stmt3 = " select count(*) from  t_file, t_job where t_file.file_state='ACTIVE' and t_job.job_id = t_file.job_id and t_job.dest_se=:1 ";

    std::string query_stmt4 = " select file_state  from (select file_state  from t_file, t_job where  "
            " t_job.job_id = t_file.job_id and t_job.source_se=:1 and t_job.dest_se=:2 and t_file.FINISH_TIME "
            " is not null  order by SYS_EXTRACT_UTC(t_file.FINISH_TIME) desc) where rownum <10 ";

    oracle::occi::Statement* s1 = NULL;
    oracle::occi::ResultSet* r1 = NULL;
    oracle::occi::Statement* s2 = NULL;
    oracle::occi::ResultSet* r2 = NULL;
    oracle::occi::Statement* s3 = NULL;
    oracle::occi::ResultSet* r3 = NULL;
    oracle::occi::Statement* s4 = NULL;
    oracle::occi::ResultSet* r4 = NULL;
    ThreadTraits::LOCK_R lock(_mutex);

    try {

        if (false == conn->checkConn())
            return false;
        s3 = conn->createStatement(query_stmt3, tag3);
        s3->setString(1, destin_hostname);
        s3->setPrefetchRowCount(1);
        r3 = conn->createResultset(s3);
        if (r3->next()) {
            maxDest = r3->getInt(1);
        }
        conn->destroyResultset(s3, r3);
        conn->destroyStatement(s3, tag3);
        s3 = NULL;
        r3 = NULL;
        s4 = conn->createStatement(query_stmt4, tag4);
        s4->setString(1, source_hostname);
        s4->setString(2, destin_hostname);
        s4->setPrefetchRowCount(3);
        r4 = conn->createResultset(s4);
        while (r4->next()) {
            noFailedPerSePair += std::string(r4->getString(1)).compare("FAILED") == 0 ? 1 : 0;
        }

        conn->destroyResultset(s4, r4);
        conn->destroyStatement(s4, tag4);
        s4 = NULL;
        r4 = NULL;
        s1 = conn->createStatement(query_stmt1, tag1);
        s1->setString(1, source_hostname);
        s1->setString(2, destin_hostname);
        s1->setPrefetchRowCount(1);
        r1 = conn->createResultset(s1);
        if (r1->next()) { //active tr's
            act = r1->getInt(1);
        }

        conn->destroyResultset(s1, r1);
        conn->destroyStatement(s1, tag1);
        s1 = NULL;
        r1 = NULL;
        s2 = conn->createStatement(query_stmt2, tag2);
        s2->setString(1, source_hostname);
        s2->setString(2, destin_hostname);
        s2->setString(3, source_hostname);
        s2->setString(4, destin_hostname);
        s2->setString(5, source_hostname);
        s2->setString(6, destin_hostname);
        s2->setPrefetchRowCount(1);
        r2 = conn->createResultset(s2);
        if (r2->next()) { //found records in t_optimize
            actThr = r2->getInt(1); //throughput                   
        }

        conn->destroyResultset(s2, r2);
        conn->destroyStatement(s2, tag2);
        s2 = NULL;
        r2 = NULL;
        if (noFailedPerSePair == 0) { //no failures in the last 3 transfers
            if (act == 0 && maxDest <= 30) {
                allowed = true;
                return allowed;
            } else if (act > 0 && maxDest <= 30 && act <= 20) {
                allowed = true;
                return allowed;
            } else if (actThr > 1 && maxDest <= 30 && act <= 20) {
                allowed = true;
                return allowed;
            } else if (actThr < 1 && maxDest > 30 && act <= 20) {
                allowed = true;
                return allowed;
            } else if (actThr < 1 && maxDest > 30 && act > 20) {
                allowed = false;
                return allowed;
            } else if (actThr < 1 && maxDest > 5 && act > 5) {
                allowed = false;
                return allowed;
            }
            else {
                allowed = true;
                return allowed;
            }
        } else { //failures
            if (noFailedPerSePair > 4 && act > 2) {
                allowed = false;
                return allowed;
            } else if (noFailedPerSePair < 4 && act > 5) {
                allowed = false;
                return allowed;
            }
            else if (actThr > 1 && maxDest <= 20 && act < 6) {
                allowed = true;
                return allowed;
            } else if (actThr < 1 && maxDest <= 20 && act < 4) {
                allowed = true;
                return allowed;
            } else if (actThr > 1 && maxDest > 10 && act > 4) {
                allowed = false;
                return allowed;
            } else if (actThr < 1 && maxDest > 10 && act > 5) {
                allowed = false;
                return allowed;
            } else {
                allowed = true;
                return allowed;
            }
        }
        return allowed;
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        if (conn) {
            if (s1 && r1) {
                conn->destroyResultset(s1, r1);
                conn->destroyStatement(s1, tag1);
            }
            if (s2 && r2) {
                conn->destroyResultset(s2, r2);
                conn->destroyStatement(s2, tag2);
            }
            if (s3 && r3) {
                conn->destroyResultset(s3, r3);
                conn->destroyStatement(s3, tag3);
            }
            if (s4 && r4) {
                conn->destroyResultset(s4, r4);
                conn->destroyStatement(s4, tag4);
            }
        }
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
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        if (conn) {
            if (s) {
                conn->destroyStatement(s, tag);
            }
            if (s1) {
                conn->destroyStatement(s1, tag1);
            }
        }
    }
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

/* Extract timeout from string: nostreams:8,timeout:3600,buffersize:9437184 */

/* REUSE CASE ???*/
void OracleAPI::forceFailTransfers() {
    std::string tag = "forceFailTransfers";
    std::string query = "select job_id, file_id, START_TIME ,PID, INTERNAL_FILE_PARAMS from t_file where file_state='ACTIVE' and pid is not null";
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
    bool found_r1 = false;
    bool found_r2 = false;
    _mutex.lock();
    try {
        if (false == conn->checkConn()) {
            _mutex.unlock();
            return;
        }

        s = conn->createStatement(query, tag);
        r = conn->createResultset(s);
        while (r->next()) {
            found_r1 = true;
            job_id = r->getString(1);
            file_id = r->getInt(2);
            start_time = conv->toTimeT(r->getTimestamp(3));
            pid = r->getInt(4);
            internalParams = r->getString(5);
            timeout = extractTimeout(internalParams);
            time_t lifetime = std::time(NULL);
            diff = difftime(lifetime, start_time);
            if (timeout != 0 && diff > (timeout + 1000)) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Killing pid:" << pid << ", jobid:" << job_id << ", fileid:" << file_id << " because it was stalled" << commit;
                std::stringstream ss;
                ss << file_id;
                found_r2 = true;
                _mutex.unlock();
                updateFileTransferStatus(job_id, ss.str(), transfer_status, transfer_message, pid, 0, 0);
                updateJobTransferStatus(ss.str(), job_id, status);
            }
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        s = NULL;
        r = NULL;

        if (found_r1 && !found_r2)
            _mutex.unlock();
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        if (conn) {
            if (s && r) {
                conn->destroyResultset(s, r);
            }
            if (s) {
                conn->destroyStatement(s, tag);
            }
        }
    }
    _mutex.unlock();
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
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        if (conn) {
            if (s4) {
                conn->destroyStatement(s4, tag4);
            }
            if (s5) {
                conn->destroyStatement(s5, tag5);
            }
            if (s6) {
                conn->destroyStatement(s6, tag6);
            }
        }
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
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        if (conn) {
            if (s && r) {
                conn->destroyResultset(s, r);
                conn->destroyStatement(s, tag);
            }
        }
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
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        if (conn) {
            if (s) {
                conn->destroyStatement(s, tag);
            }
        }
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
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        if (conn) {
            if (s) {
                conn->destroyStatement(s, tag);
            }
        }
    }
}

void OracleAPI::revertToSubmitted(){
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
	    std::cout << "-----> DIFF = " << diff << std::endl;
            if (diff > 15000) { /*~5h*/
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "The transfer with file id " << file_id << " seems to be stalled, restart it" << commit;
                s1 = conn->createStatement(query1, tag1);
		s1->setInt(1,file_id);	
        	if(s1->executeUpdate()!=0){		
        		conn->commit();
		}
        	conn->destroyStatement(s1, tag1);		
		s1=NULL;
		if(reuseFlag.compare("Y")==0){
			s3 = conn->createStatement(query3, tag3);
			s3->setString(1,job_id);	
			if(s3->executeUpdate()!=0){		
        			conn->commit();
			}
        		conn->destroyStatement(s3, tag3);		
			s3=NULL;						
		}
            }
        }
        conn->destroyResultset(s2, r2);
        conn->destroyStatement(s2, tag2);	        
    } catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::revertToSubmittedTerminate(){
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
	if(s1->executeUpdate()!=0){		
        		conn->commit();
	}	
	conn->destroyStatement(s1, tag1);
	
        s2 = conn->createStatement(query2, tag2);
	if(s2->executeUpdate()!=0){		
        		conn->commit();
	}	
	conn->destroyStatement(s2, tag2);	
    }catch (oracle::occi::SQLException const &e) {
        if (conn)
            conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}


bool OracleAPI::configExists(const std::string & src, const std::string & dest, const std::string & vo){
	const std::string tag = "configExists";
	return false;
}


// the class factories
extern "C" GenericDbIfce* create() {
    return new OracleAPI;
}

extern "C" void destroy(GenericDbIfce* p) {
    if (p)
        delete p;
}
