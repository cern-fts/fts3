#include "OracleAPI.h"
#include <fstream>
#include "error.h"
#include "OracleTypeConversions.h"
#include <algorithm>

using namespace FTS3_COMMON_NAMESPACE;

OracleAPI::OracleAPI() : conn(NULL) {
}

OracleAPI::~OracleAPI() {
    if (conn)
        delete conn;
}

void OracleAPI::init(std::string username, std::string password, std::string connectString) {
    if (!conn)
        conn = new OracleConnection(username, password, connectString);
}

void OracleAPI::getSubmittedJobs(std::vector<TransferJobs*>& jobs) {
    TransferJobs* tr_jobs = NULL;
    std::vector<TransferJobs*>::iterator iter;
    const std::string tag = "getSubmittedJobs";
    const std::string updateTag = "getSubmittedJobsUpdate";
    std::string query_stmt = "SELECT "
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
            " FROM t_job, t_file"
            " WHERE t_job.job_id = t_file.job_id"
            " AND t_file.file_state = 'SUBMITTED'"
            " AND t_job.job_finished is NULL"
            " AND t_job.CANCEL_JOB is NULL"
            " AND t_file.job_finished is NULL"
            " AND rownum <= 30  ORDER BY t_job.priority DESC"
            " , SYS_EXTRACT_UTC(t_job.submit_time)";

    try {
        oracle::occi::Statement* s = conn->createStatement(query_stmt, tag);
        oracle::occi::ResultSet* r = conn->createResultset(s);
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
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

}

void OracleAPI::getSeCreditsInUse (
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
			"	AND (t_file.file_state = 'READY' OR t_file.file_state = 'ACTIVE') ";
		tag.append("1");
	if (!srcSeName.empty()) {
		query_stmt +=
//			"	AND t_file.source_surl like :1 "; // srcSeName + "/%
			"	AND t_file.source_surl like '" + srcSeName + "/%' ";
			tag.append("2");
	}

	if (!destSeName.empty()) {
		query_stmt +=
//			"	AND t_file.dest_surl like :2 "; // destSeName + "/%
			"	AND t_file.dest_surl like '" + destSeName + "/%' ";
		tag.append("3");
	}

	if (srcSeName.empty() || destSeName.empty()) {
		if (!voName.empty()) {
//			query_stmt += "	AND t_job.vo_name = :3 "; // vo
			query_stmt += "	AND t_job.vo_name = '" + voName + "' "; // vo
			tag.append("4");
		} else {
			// voName not in those who have voshare for this SE
			query_stmt +=
			" 	AND NOT EXISTS ( "
			"		SELECT * "
			"		FROM t_se_vo_share "
			"		WHERE "
//			"			t_se_vo_share.se_name = :4 "
			"			t_se_vo_share.se_name = '" + srcSeName + "' "
			"			AND t_se_vo_share.share_type = 'se' "
			"			AND t_se_vo_share.share_id = '%\"share_name\":\"' || t_job.vo_name || '\"%' "
			"	) ";
			tag.append("5");
		}
	}

    try {

        oracle::occi::Statement* s = conn->createStatement(query_stmt, "");
        oracle::occi::ResultSet* r = conn->createResultset(s);
        if(r->next()) {
        	creditsInUse = r->getInt(1);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, "");
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::getSiteCreditsInUse (
		int &creditsInUse,
		std::string srcSiteName,
		std::string destSiteName,
		std::string voName
	) {
	std::string tag = "getSiteCreditsInUse";
	std::string query_stmt =
			"SELECT COUNT(*) "
			"FROM t_job, t_file, t_se "
			"WHERE "
			"	t_job.job_id = t_file.job_id "
			"	AND (t_file.file_state = 'READY' OR t_file.file_state = 'ACTIVE') ";
			tag.append("1");
	if (!srcSiteName.empty()) {
		query_stmt +=
//			"	AND t_se.site = :1 " // the se has the information about site name
			"	AND t_se.site = '" + srcSiteName + "' " // the se has the information about site name
			"	AND t_file.source_surl like t_se.name + '/%' "; // srcSeName + "/%
			tag.append("2");
	}

	if (!destSiteName.empty()) {
		query_stmt +=
//			"	AND t_se.site = :2 " // the se has the information about site name
			"	AND t_se.site = '" + destSiteName + "' " // the se has the information about site name
			"	AND t_file.dest_surl like t_se.name + '/%' "; // destSeName + "/%
			tag.append("3");
	}

	if (srcSiteName.empty() || destSiteName.empty()) {
		if (!voName.empty()) {
			query_stmt +=
//			"	AND t_job.vo_name = :3 "; // vo
			"	AND t_job.vo_name = '" + voName + "' "; // vo
			tag.append("4");
		} else {
			// voName not in those who have voshare for this SE
			query_stmt +=
			" AND NOT EXISTS ( "
			"		SELECT * "
			"		FROM t_se_vo_share "
			"		WHERE "
			"			t_se_vo_share.se_name = t_se.name "
			"			AND t_se_vo_share.share_type = 'se' "
			"			AND t_se_vo_share.share_id = '%\"share_name\":\"' || t_job.vo_name || '\"%' "
			"	) ";
			tag.append("5");
		}
	}

    try {

        oracle::occi::Statement* s = conn->createStatement(query_stmt, "");
        oracle::occi::ResultSet* r = conn->createResultset(s);
        if(r->next()) {
        	creditsInUse = r->getInt(1);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, "");
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::updateFileStatus(TransferFiles* file, const std::string status) {
    const std::string tag = "updateFileStatus";
    std::string query =
    		"UPDATE t_file "
    		"SET file_state =:1 "
    		"WHERE file_id = :2 and file_state = 'SUBMITTED'";
    std::string query2 =
                "UPDATE t_job "
                "SET job_state =:1 "
                "WHERE job_id = :2 and job_state = 'SUBMITTED'";


    try {
        oracle::occi::Statement* s = conn->createStatement(query, "");
        s->setString(1, status);
        s->setInt(2, file->FILE_ID);
        s->executeUpdate();
	s->setSQL(query2);
        s->setString(1, status);
        s->setString(2, file->JOB_ID);
        s->executeUpdate();
        conn->commit();	
        conn->destroyStatement(s, "");

    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::updateJObStatus(std::string jobId, const std::string status) {
    const std::string tag = "updateJobStatus";
    std::string query =
                "UPDATE t_job "
                "SET job_state =:1 "
                "WHERE job_id = :2 and job_state = 'SUBMITTED'";

    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, status);
        s->setString(2, jobId);
        s->executeUpdate();
        conn->commit();
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}



void OracleAPI::getByJobId(std::vector<TransferJobs*>& jobs, std::vector<TransferFiles*>& files) {
    std::string jobAppender("");
    TransferFiles* tr_files = NULL;
    std::vector<TransferJobs*>::iterator iter;
    std::string selecttag = "getByJobId";
    std::string select = "SELECT t_file.source_surl, t_file.dest_surl, t_file.job_id, t_job.vo_name, "
                " t_file.file_id, t_job.overwrite_flag, t_job.USER_DN, t_job.CRED_ID, t_file.checksum, t_job.CHECKSUM_METHOD, t_file.file_state, t_file.logical_name, "
            " t_file.reason_class, t_file.reason, t_file.num_failures, t_file.current_failures, "
   " t_file.catalog_failures, t_file.prestage_failures, t_file.filesize, "
            " t_file.finish_time, t_file.agent_dn, t_file.internal_file_params, "
   " t_file.error_scope, t_file.error_phase "
            " FROM t_file, t_job WHERE"
   " t_file.job_id = t_job.job_id AND "
   " t_file.job_finished is NULL AND "
   " t_job.job_finished is NULL AND "
   " t_job.job_id IN(";
    try {
              	
        for (iter = jobs.begin(); iter != jobs.end(); ++iter) {
            TransferJobs* temp = (TransferJobs*) * iter;
            	std::string job_id = std::string(temp->JOB_ID);
            	jobAppender.append("'");
   		jobAppender.append(job_id);
   		jobAppender.append("',");
        }	    
	    jobAppender = jobAppender.substr(0, jobAppender.length()-1);
	    select.append(jobAppender);
	    select.append(") ORDER BY t_file.file_id DESC ");
	    		     
            oracle::occi::Statement* s = conn->createStatement(select,"");	    
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
                files.push_back(tr_files);
            }
	    
            conn->destroyResultset(s, r);
            conn->destroyStatement(s, "");
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::submitPhysical(const std::string & jobId, std::vector<src_dest_checksum_tupple> src_dest_pair, const std::string & paramFTP,
        const std::string & DN, const std::string & cred, const std::string & voName, const std::string & myProxyServer,
        const std::string & delegationID, const std::string & spaceToken, const std::string & overwrite,
        const std::string & sourceSpaceToken, const std::string &, const std::string & lanConnection, int copyPinLifeTime,
        const std::string & failNearLine, const std::string & checksumMethod) {
    /*
            Required fields
            JOB_ID 				   NOT NULL CHAR(36)
            JOB_STATE			   	   NOT NULL VARCHAR2(32)
            USER_DN				   NOT NULL VARCHAR2(1024)
     */

    std::string source;
    std::string destination;
    const std::string initial_state = "SUBMITTED";
    time_t timed = time(NULL);
    char hostname[512] = {0};
    gethostname(hostname, 512);
    const std::string currenthost = hostname; //current hostname
    const std::string tag_job_statement = "tag_job_statement";
    const std::string tag_file_statement = "tag_file_statement";
    const std::string job_statement = "INSERT INTO t_job(job_id, job_state, job_params, user_dn, user_cred, priority, vo_name,submit_time,internal_job_params,submit_host, cred_id, myproxy_server, storage_class, overwrite_flag,source_token_description,copy_pin_lifetime, lan_connection,fail_nearline, checksum_method) VALUES (:1,:2,:3,:4,:5,:6,:7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17, :18, :19)";
    const std::string file_statement = "INSERT INTO t_file (job_id, file_state, source_surl, dest_surl,checksum) VALUES (:1,:2,:3,:4,:5)";

    try {
        oracle::occi::Statement* s_job_statement = conn->createStatement(job_statement, tag_job_statement);
        s_job_statement->setString(1, jobId); //job_id
        s_job_statement->setString(2, initial_state); //job_state
        s_job_statement->setString(3, paramFTP); //job_params
        s_job_statement->setString(4, DN); //user_dn
        s_job_statement->setString(5, cred); //user_cred
        s_job_statement->setInt(6, 3); //priority
        s_job_statement->setString(7, voName); //vo_name
        s_job_statement->setTimestamp(8, OracleTypeConversions::toTimestamp(timed, conn->getEnv())); //submit_time
        s_job_statement->setString(9, ""); //internal_job_params
        s_job_statement->setString(10, currenthost); //submit_host
        s_job_statement->setString(11, delegationID); //cred_id
        s_job_statement->setString(12, myProxyServer); //myproxy_server
        s_job_statement->setString(13, spaceToken); //storage_class
        s_job_statement->setString(14, overwrite); //overwrite_flag
        s_job_statement->setString(15, sourceSpaceToken); //source_token_description
        s_job_statement->setInt(16, copyPinLifeTime); //copy_pin_lifetime
        s_job_statement->setString(17, lanConnection); //lan_connection
        s_job_statement->setString(18, failNearLine); //fail_nearline	
        if (checksumMethod.length() == 0)
            s_job_statement->setNull(19, oracle::occi::OCCICHAR);
        else
            s_job_statement->setString(19, "Y"); //checksum_method
        s_job_statement->executeUpdate();

        //now insert each src/dest pair for this job id
        std::vector<src_dest_checksum_tupple>::iterator iter;
        oracle::occi::Statement* s_file_statement = conn->createStatement(file_statement, tag_file_statement);

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
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

}

JobStatus* OracleAPI::getTransferJobStatus(std::string requestID) {

    const std::string query = "SELECT job_id, job_state, se_pair_name, user_dn, reason, submit_time, priority, vo_name, "
            "(SELECT count(*) from t_file where t_file.job_id = t_job.job_id) "
            "FROM t_job WHERE job_id = :1";
    const std::string tag = "getTransferJobStatus";

    JobStatus* js = NULL;
    try {
        js = new JobStatus();
        js->priority = 0;
        js->numFiles = 0;
        js->submitTime = (std::time_t) - 1;
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, requestID);
        oracle::occi::ResultSet* r = conn->createResultset(s);
        while (r->next()) {
            js->jobID = r->getString(1);
            js->jobStatus = r->getString(2);
            js->sePairName = r->getString(3);
            js->clientDN = r->getString(4);
            js->reason = r->getString(5);
            js->submitTime = OracleTypeConversions::toTimeT(r->getTimestamp(6));
            js->priority = r->getInt(7);
            js->voName = r->getString(8);
            js->numFiles = r->getInt(9);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
    } catch (oracle::occi::SQLException const &e) {
        if (js)
            delete js;
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
    return js;
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
    int cc = 1;
    std::string jobStatuses;

    /*this statement cannot be prepared, it's generated dynamically*/
    std::string tag = std::string("");
    std::string sel = "SELECT DISTINCT job_id, job_state, reason, submit_time, user_dn, J.vo_name,(SELECT count(*) from t_file where t_file.job_id = J.job_id), priority, cancel_job FROM t_job J ";
    //gain the benefit from the statement pooling
    std::sort(inGivenStates.begin(), inGivenStates.end());
    std::vector<std::string>::iterator foundCancel;

    if (inGivenStates.size() > 0) {
        foundCancel = std::find_if(inGivenStates.begin(), inGivenStates.end(), bind2nd(std::equal_to<std::string > (), std::string("Canceling")));
        if (foundCancel == inGivenStates.end()) { //not found
            checkForCanceling = true;
        }
    }

    if (inGivenStates.size() > 0) {
        jobStatuses = "'" + inGivenStates[0] + "'";
        for (unsigned int i = 1; i < inGivenStates.size(); i++) {
            jobStatuses += (",'" + inGivenStates[i] + "'");
        }
    }

    if (restrictToClientDN.length() > 0) {
        sel.append(" LEFT OUTER JOIN T_SE_PAIR_ACL C ON J.SE_PAIR_NAME = C.SE_PAIR_NAME LEFT OUTER JOIN t_vo_acl V ON J.vo_name = V.vo_name ");
    }

    if (inGivenStates.size() > 0) {
        sel.append(" WHERE J.job_state IN (" + jobStatuses + ") ");
    } else {
        sel.append(" WHERE J.job_state <> '0' ");
    }

    if (restrictToClientDN.length() > 0) {
        sel.append(" AND (J.user_dn = :1 OR V.principal = :2 OR C.principal = :3) ");
    }

    if (VOname.length() > 0) {
        sel.append(" AND J.vo_name = :4");
    }

    if (forDN.length() > 0) {
        sel.append(" AND J.user_dn = :5");
    }

    if (!checkForCanceling) {
        sel.append(" AND NVL(J.cancel_job,'X') <> 'Y'");
    }

    try {
        oracle::occi::Statement* s = conn->createStatement(sel, "");
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


        oracle::occi::ResultSet* r = conn->createResultset(s);
        while (r->next()) {
            std::string jid = r->getString(1);
            std::string jstate = r->getString(2);
            std::string reason = r->getString(3);
            time_t tm = OracleTypeConversions::toTimeT(r->getTimestamp(4));
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
        conn->destroyStatement(s, "");

    } catch (oracle::occi::SQLException const &e) {
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

}

/*
std::vector<FileTransferStatus> OracleAPI::getFileStatus(std::string requestID, int offset, int limit) {
        std::vector<FileTransferStatus> test;
        return test;
}


TransferJobSummary* OracleAPI::getTransferJobSummary(std::string requestID) {
        return new TransferJobSummary;
}

SePair* OracleAPI::getSEPairName(std::string sePairName) {
        return new SePair;
}

void OracleAPI::cancel(std::vector<std::string> requestIDs) {
}

void OracleAPI::addChannel(std::string channelName, std::string sourceSite, std::string destSite, std::string contact, int numberOfStreams,
        int numberOfFiles, int bandwidth, int nominalThroughput, std::string state) {
}

void OracleAPI::setJobPriority(std::string requestID, int priority) {
}

void OracleAPI::dropChannel(std::string name) {
}

void OracleAPI::setState(std::string channelName, std::string state, std::string message) {
}

std::vector<std::string> OracleAPI::listChannels() {
        std::vector<std::string> test;
        return test;
}

void OracleAPI::setNumberOfStreams(std::string channelName, int numberOfStreams, std::string message) {
}

void OracleAPI::setNumberOfFiles(std::string channelName, int numberOfFiles, std::string message) {
}

void OracleAPI::setBandwidth(std::string channelName, int utilisation, std::string message) {
}

void OracleAPI::setContact(std::string channelName, std::string contact, std::string message) {
}

void OracleAPI::setNominalThroughput(std::string channelName, int nominalThroughput, std::string message) {
}

void OracleAPI::changeStateForHeldJob(std::string jobID, std::string state) {
}

void OracleAPI::changeStateForHeldJobs(std::string channelName, std::string state) {
}

void OracleAPI::addChannelManager(std::string channelName, std::string principal) {
}

void OracleAPI::removeChannelManager(std::string channelName, std::string principal) {
}

std::vector<std::string> OracleAPI::listChannelManagers(std::string channelName) {
        std::vector<std::string> test;
        return test;
}

std::map<std::string, std::string> OracleAPI::getChannelManager(std::string channelName, std::vector<std::string> principals) {
        std::map<std::string, std::string> test;
        return test;
}

void OracleAPI::addVOManager(std::string VOName, std::string principal) {
}

void OracleAPI::removeVOManager(std::string VOName, std::string principal) {
}

std::vector<std::string> OracleAPI::listVOManagers(std::string VOName) {
        std::vector<std::string> test;
        return test;

}

std::map<std::string, std::string> OracleAPI::getVOManager(std::string VOName, std::vector<std::string> principals) {
        std::map<std::string, std::string> test;
        return test;
}

bool OracleAPI::isRequestManager(std::string requestID, std::string clientDN, std::vector<std::string> principals, bool includeOwner) {
        return true;
}

void OracleAPI::removeVOShare(std::string channelName, std::string VOName) {
}

void OracleAPI::setVOShare(std::string channelName, std::string VOName, int share) {
}

bool OracleAPI::isAgentAvailable(std::string name, std::string type) {
        return true;
}

std::string OracleAPI::getSchemaVersion() {
        return std::string("");
}

void OracleAPI::setTcpBufferSize(std::string channelName, std::string bufferSize, std::string message) {
}

void OracleAPI::setTargetDirCheck(std::string channelName, int targetDirCheck, std::string message) {
}

void OracleAPI::setUrlCopyFirstTxmarkTo(std::string channelName, int urlCopyFirstTxmarkTo, std::string message) {
}

void OracleAPI::setChannelType(std::string channelName, std::string channelType, std::string message) {
}

void OracleAPI::setBlockSize(std::string channelName, std::string blockSize, std::string message) {
}

void OracleAPI::setHttpTimeout(std::string channelName, int httpTimeout, std::string message) {
}

void OracleAPI::setTransferLogLevel(std::string channelName, std::string transferLogLevel, std::string message) {
}

void OracleAPI::setPreparingFilesRatio(std::string channelName, double preparingFilesRatio, std::string message) {
}

void OracleAPI::setUrlCopyPutTimeout(std::string channelName, int urlCopyPutTimeout, std::string message) {
}

void OracleAPI::setUrlCopyPutDoneTimeout(std::string channelName, int urlCopyPutDoneTimeout, std::string message) {
}

void OracleAPI::setUrlCopyGetTimeout(std::string channelName, int urlCopyGetTimeout, std::string message) {
}

void OracleAPI::setUrlCopyGetDoneTimeout(std::string channelName, int urlCopyGetDoneTimeout, std::string message) {
}

void OracleAPI::setUrlCopyTransferTimeout(std::string channelName, int urlCopyTransferTimeout, std::string message) {
}

void OracleAPI::setUrlCopyTransferMarkersTimeout(std::string channelName, int urlCopyTransferMarkersTimeout, std::string message) {
}

void OracleAPI::setUrlCopyNoProgressTimeout(std::string channelName, int urlCopyNoProgressTimeout, std::string message) {
}

void OracleAPI::setUrlCopyTransferTimeoutPerMB(std::string channelName, double urlCopyTransferTimeoutPerMB, std::string message) {
}

void OracleAPI::setSrmCopyDirection(std::string channelName, std::string srmCopyDirection, std::string message) {
}

void OracleAPI::setSrmCopyTimeout(std::string channelName, int srmCopyTimeout, std::string message) {
}

void OracleAPI::setSrmCopyRefreshTimeout(std::string channelName, int srmCopyRefreshTimeout, std::string message) {
}

void OracleAPI::removeVOLimit(std::string channelUpperName, std::string voName) {
}

void OracleAPI::setVOLimit(std::string channelUpperName, std::string voName, int limit) {
}
 */

/* ********************************* NEW API FTS3 *********************************/

void OracleAPI::getSe(Se* &se, std::string seName){
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


    try {
        oracle::occi::Statement* s = conn->createStatement(query_stmt, tag);
        s->setString(1, seName);
        oracle::occi::ResultSet* r = conn->createResultset(s);
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
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}


void OracleAPI::getAllSeInfoNoCritiria(std::vector<Se*>& se){
    Se* seData = NULL;
    std::vector<Se*>::iterator iter;
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
	    

    try {
        oracle::occi::Statement* s = conn->createStatement(query_stmt, tag);
        oracle::occi::ResultSet* r = conn->createResultset(s);
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
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

}
    
void OracleAPI::getAllSeConfigNoCritiria(std::vector<SeConfig*>& seConfig){
    SeConfig* seCon = NULL;
    std::vector<SeConfig*>::iterator iter;
    const std::string tag = "getAllSeConfigNoCritiria";
    std::string query_stmt = "SELECT "
            " T_SE_VO_SHARE.SE_NAME, "
            " T_SE_VO_SHARE.SHARE_ID, "
            " T_SE_VO_SHARE.SHARE_TYPE,  "
            " T_SE_VO_SHARE.SHARE_VALUE  "	    
            " FROM T_SE_VO_SHARE";


    try {
        oracle::occi::Statement* s = conn->createStatement(query_stmt, tag);
        oracle::occi::ResultSet* r = conn->createResultset(s);
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
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}
    
void OracleAPI::getAllSeAndConfigWithCritiria(std::vector<SeAndConfig*>& seAndConfig, std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE, std::string
SHARE_VALUE){
    SeAndConfig* seData = NULL;
    std::vector<SeAndConfig*>::iterator iter;
    std::string tag = "getAllSeAndConfigWithCritiria";
    std::string query_stmt = " SELECT "
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
            " t_se.GOCDB_ID, "
            " T_SE_VO_SHARE.SE_NAME, "
            " T_SE_VO_SHARE.SHARE_ID, "
            " T_SE_VO_SHARE.SHARE_TYPE,"
            " T_SE_VO_SHARE.SHARE_VALUE "	    
            " FROM t_se, T_SE_VO_SHARE where t_se.NAME = T_SE_VO_SHARE.SE_NAME ";	     
    if (SE_NAME.length() > 0){
        query_stmt.append(" and T_SE_VO_SHARE.SE_NAME ='");
		query_stmt.append(SE_NAME);
		query_stmt.append("'");
		tag.append("1");
	}
    if (SHARE_ID.length() > 0){
        query_stmt.append(" and T_SE_VO_SHARE.SHARE_ID ='");
		query_stmt.append(SHARE_ID);
		query_stmt.append("'");
		tag.append("2");
	}
    if (SHARE_TYPE.length() > 0){
        query_stmt.append(" and T_SE_VO_SHARE.SHARE_TYPE ='");
		query_stmt.append(SHARE_TYPE);
		query_stmt.append("'");
		tag.append("3");
	}
    if (SHARE_VALUE.length() > 0){
        query_stmt.append(" and T_SE_VO_SHARE.SHARE_VALUE ='");
		query_stmt.append(SHARE_VALUE);
		query_stmt.append("'");
		tag.append("4");
	}	

    try {
        oracle::occi::Statement* s = conn->createStatement(query_stmt, "");	         
        oracle::occi::ResultSet* r = conn->createResultset(s);
        while (r->next()) {
            seData = new SeAndConfig();
            seData->ENDPOINT = r->getString(1);
            seData->SE_TYPE = r->getString(2);
            seData->SITE = r->getString(3);
            seData->NAME = r->getString(4);
            seData->STATE = r->getString(5);
            seData->VERSION = r->getString(6);
            seData->HOST = r->getString(7);
            seData->SE_TRANSFER_TYPE = r->getString(8);
            seData->SE_TRANSFER_PROTOCOL = r->getString(9);
            seData->SE_CONTROL_PROTOCOL = r->getString(10);
            seData->GOCDB_ID = r->getString(11);
	    seData->SE_NAME = r->getString(12);
            seData->SHARE_ID = r->getString(13);
            seData->SHARE_TYPE = r->getString(14);
            seData->SHARE_VALUE = r->getString(15);
	    
            seAndConfig.push_back(seData);
        }
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, "");

    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

void OracleAPI::addSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
            std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID){
    std::string query = "INSERT INTO t_se (ENDPOINT, SE_TYPE, SITE, NAME, STATE, VERSION, HOST, SE_TRANSFER_TYPE, SE_TRANSFER_PROTOCOL,SE_CONTROL_PROTOCOL,GOCDB_ID) VALUES (:1,:2,:3,:4,:5,:6,:7,:8,:9,:10,:11)";
    std::string tag = "addSe";
    

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
        s->executeUpdate();
        conn->commit();
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}	    

void OracleAPI::updateSe(std::string ENDPOINT, std::string SE_TYPE, std::string SITE, std::string NAME, std::string STATE, std::string VERSION, std::string HOST,
            std::string SE_TRANSFER_TYPE, std::string SE_TRANSFER_PROTOCOL, std::string SE_CONTROL_PROTOCOL, std::string GOCDB_ID){
    int fields = 0;
    std::string query = "UPDATE T_SE SET ";
    			if(ENDPOINT.length() > 0){
	    			query.append(" ENDPOINT='");
	    			query.append(ENDPOINT);				
	    			query.append("'");				
				fields++;
				}    	    			
    			if(SE_TYPE.length() > 0){
				if(fields > 0){
				  fields = 0;	
				  query.append(", ");
				}					
	    			query.append(" SE_TYPE='");
	    			query.append(SE_TYPE);				
	    			query.append("'");						
				fields++;
				}
    			if(SITE.length() > 0){
				if(fields > 0){
				  fields = 0;	
				  query.append(", ");
				}					
	    			query.append(" SITE='");
	    			query.append(SE_TYPE);				
	    			query.append("'");						
				fields++;
				}  
    			if(STATE.length() > 0){
				if(fields > 0){
				  fields = 0;	
				  query.append(", ");
				}					
	    			query.append(" STATE='");
	    			query.append(STATE);				
	    			query.append("'");				
				fields++;
				}  
    			if(VERSION.length() > 0){
				if(fields > 0){
				  fields = 0;	
				  query.append(", ");
				}					
	    			query.append(" VERSION='");
	    			query.append(VERSION);				
	    			query.append("'");				
				fields++;
				}  
    			if(HOST.length() > 0){
				if(fields > 0){
				  fields = 0;	
				  query.append(", ");
				}					
	    			query.append(" HOST='");
	    			query.append(HOST);				
	    			query.append("'");				
				fields++;
				}
    			if(SE_TRANSFER_TYPE.length() > 0){
				if(fields > 0){
				  fields = 0;	
				  query.append(", ");
				}					
	    			query.append(" SE_TRANSFER_TYPE='");
	    			query.append(SE_TRANSFER_TYPE);				
	    			query.append("'");					
				fields++;
				}	
    			if(SE_TRANSFER_PROTOCOL.length() > 0){
				if(fields > 0){
				  fields = 0;	
				  query.append(", ");
				}					
	    			query.append(" SE_TRANSFER_PROTOCOL='");
	    			query.append(SE_TRANSFER_PROTOCOL);				
	    			query.append("'");				
				fields++;
				}							
    			if(SE_CONTROL_PROTOCOL.length() > 0){
				if(fields > 0){
				  fields = 0;	
				  query.append(", ");
				}					
	    			query.append(" SE_CONTROL_PROTOCOL='");
	    			query.append(SE_CONTROL_PROTOCOL);			       
	    			query.append("'");				
				fields++;
				}
    			if(GOCDB_ID.length() > 0){
				if(fields > 0){
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
				
    try {
        oracle::occi::Statement* s = conn->createStatement(query, ""); 

        s->executeUpdate();
        conn->commit();
        conn->destroyStatement(s, "");

    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

}	    


void OracleAPI::addSeConfig( std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE, std::string SHARE_VALUE){
    std::string query = "INSERT INTO T_SE_VO_SHARE (SE_NAME, SHARE_ID, SHARE_TYPE, SHARE_VALUE) VALUES (:1,:2,:3,:4)";
    std::string tag = "addSeConfig";

    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, SE_NAME);
        s->setString(2, SHARE_ID);
        s->setString(3, SHARE_TYPE);     
        s->setString(4, SHARE_VALUE);     	
        s->executeUpdate();
        conn->commit();
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
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
void OracleAPI::updateSeConfig(std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE, std::string SHARE_VALUE ){
    int fields = 0;
    std::string query = "UPDATE T_SE_VO_SHARE SET ";
    			/*if(SHARE_ID.length() > 0){
	    			query.append(" SHARE_ID='");
	    			query.append(SHARE_ID);			       
	    			query.append("'");					
				fields++;
				}    		    			
    			if(SE_NAME.length() > 0){
				if(fields > 0){
				  fields = 0;	
				  query.append(", ");
				}					
	    			query.append(" SE_NAME='");
	    			query.append(SE_NAME);			       
	    			query.append("'");
				fields++;				
				}
    			if(SHARE_TYPE.length() > 0){
				if(fields > 0){
				  fields = 0;	
				  query.append(", ");
				}					
	    			query.append(" SHARE_TYPE='");
	    			query.append(SHARE_TYPE);			       
	    			query.append("'");				
				}
				*/
    			if(SHARE_VALUE.length() > 0){
				if(fields > 0){
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
			


    try {
        oracle::occi::Statement* s = conn->createStatement(query, "");
        s->executeUpdate();
        conn->commit();
        conn->destroyStatement(s, "");

    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

/*
Delete a SE
REQUIRED: NAME
*/
void OracleAPI::deleteSe(std::string NAME){
    std::string query = "DELETE FROM T_SE WHERE NAME = :1";
    std::string tag = "deleteSe";

    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);
        s->setString(1, NAME);
        s->executeUpdate();
        conn->commit();
        conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}

/*
Delete the config of an SE for a specific VO
REQUIRED: SE_NAME
OPTIONAL: VO_NAME
*/
void OracleAPI::deleteSeConfig(std::string SE_NAME, std::string SHARE_ID, std::string SHARE_TYPE){
    std::string query = "DELETE FROM T_SE_VO_SHARE WHERE SE_NAME ='"+SE_NAME+"'";    
    			query.append(" AND SHARE_ID ='"+SHARE_ID+"'");	
    			query.append(" AND SHARE_TYPE ='"+SHARE_TYPE+"'");

    try {
        oracle::occi::Statement* s = conn->createStatement(query, "");
        s->executeUpdate();
        conn->commit();
        conn->destroyStatement(s, "");

    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}


void OracleAPI::updateFileTransferStatus(std::string job_id, std::string file_id, std::string transfer_status, std::string transfer_message, int process_id){
    job_id = "";
    unsigned int index = 1;
    std::string tag = "updateFileTransferStatus";
    std::stringstream query;
    query << "UPDATE t_file SET file_state=:" << 1 <<  ", REASON=:" << ++index;
		if(transfer_status.compare("FINISHED") == 0){
			query << ", FINISH_TIME=:" << ++index;
			tag.append("xx");
		}
		query << ", PID=:" << ++index;
    		query << " WHERE file_id =:" << ++index;
		query << " and (file_state='READY' or file_state='ACTIVE')";	

    try {
        oracle::occi::Statement* s = conn->createStatement(query.str(), tag);
	index = 1; //reset index
	s->setString(1, transfer_status); 
	++index;
	s->setString(index, transfer_message); 
	if(transfer_status.compare("FINISHED") == 0){
		time_t timed = time(NULL);
		++index;
		s->setTimestamp(index, OracleTypeConversions::toTimestamp(timed, conn->getEnv()));
		}
	++index;	
	s->setInt(index, process_id);
	++index;
	s->setInt(index, atoi(file_id.c_str()));	
        s->executeUpdate();
        conn->commit();	
	conn->destroyStatement(s, tag);

    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}    


void OracleAPI::updateJobTransferStatus(std::string file_id, std::string job_id, const std::string status){
    file_id = "";
    const std::string reason = "One or more files failed. Please have a look at the details for more information";
    const std::string terminal = "FINISHED";
    bool finished  = true;
    std::string state("");
    const std::string tag1 = "updateJobTransferStatus";
    const std::string tag2 = "selectJobTransferStatus";    
    const std::string tag3 = "selectJobNotFinished";        
    std::string update =
    		"UPDATE t_job "
    		"SET JOB_STATE=:1, JOB_FINISHED =:2, FINISH_TIME=:3, REASON=:4 "
    		"WHERE job_id = :5 AND JOB_STATE='ACTIVE'";	
    std::string updateJobNotFinished =
    		"UPDATE t_job "
    		"SET JOB_STATE=:1 WHERE job_id = :2 AND JOB_STATE='READY' ";				
    std::string query = "SELECT FILE_STATE FROM T_FILE WHERE JOB_ID=:1";
    std::string updateFileJobFinished =
    		"UPDATE t_file "
    		"SET JOB_FINISHED=:1 "
    		"WHERE job_id=:2 ";	    
	    
    try {
        oracle::occi::Statement* st = conn->createStatement(query, "");
	job_id = job_id.substr (0,36);
	st->setString(1,job_id);
        oracle::occi::ResultSet* r = conn->createResultset(st);
        while (r->next()) {
            	state = r->getString(1);
		if( (state.length() > 0) && (state.compare(terminal)!=0)){
			finished = false;
			break;
	     }
        }
        conn->destroyResultset(st, r);
   
    if(finished == true){
        time_t timed = time(NULL);
	st->setSQL(update);
        st->setString(1,terminal);
        st->setTimestamp(2, OracleTypeConversions::toTimestamp(timed, conn->getEnv())); 
        st->setTimestamp(3, OracleTypeConversions::toTimestamp(timed, conn->getEnv())); 	
        st->setString(4, reason);
	st->setString(5, job_id);
        st->executeUpdate();
	
	st->setSQL(updateFileJobFinished);
        st->setTimestamp(1, OracleTypeConversions::toTimestamp(timed, conn->getEnv())); 
        st->setString(2, job_id ); 	
        st->executeUpdate();	
		
        conn->commit();	  
      }
      else{     
	st->setSQL(updateJobNotFinished);
        st->setString(1,status);       
	st->setString(2, job_id);
        st->executeUpdate();
        conn->commit();	
      }
      
      conn->destroyStatement(st, "");      
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
}    


void OracleAPI::cancelJob(std::vector<std::string>& requestIDs){
	const std::string cancelReason = "Job canceled by the user";
	std::string cancelJ = "update t_job SET  JOB_STATE=:1, JOB_FINISHED =:2, FINISH_TIME=:3, REASON=:4 WHERE job_id = :5";
	std::string cancelF = "update t_file set file_state=:1, JOB_FINISHED =:2, FINISH_TIME=:3, REASON=:4 WHERE JOB_ID=:5";
	const std::string cancelJTag = "cancelJTag";
	const std::string cancelFTag = "cancelFTag";
	std::vector<std::string>::iterator iter;
	time_t timed = time(NULL);	
	
	try{
	oracle::occi::Statement* st1 = conn->createStatement(cancelJ, cancelJTag);
	oracle::occi::Statement* st2 = conn->createStatement(cancelF, cancelFTag);
	
        for (iter = requestIDs.begin(); iter != requestIDs.end(); ++iter) {
	    	std::string jobId = std::string(*iter);
		
        	st1->setString(1,"CANCELED");
        	st1->setTimestamp(2, OracleTypeConversions::toTimestamp(timed, conn->getEnv())); 
        	st1->setTimestamp(3, OracleTypeConversions::toTimestamp(timed, conn->getEnv())); 	
        	st1->setString(4, cancelReason);
		st1->setString(5, jobId);	    
            	st1->executeUpdate();
		
        	st2->setString(1,"CANCELED");
        	st2->setTimestamp(2, OracleTypeConversions::toTimestamp(timed, conn->getEnv())); 
        	st2->setTimestamp(3, OracleTypeConversions::toTimestamp(timed, conn->getEnv())); 	
        	st2->setString(4, cancelReason);
		st2->setString(5, jobId);	    		
            	st2->executeUpdate();
		conn->commit();	    
        }	   	
	
	conn->destroyStatement(st1, cancelJTag);
	conn->destroyStatement(st2, cancelFTag);
	}
	catch (oracle::occi::SQLException const &e) {
        	conn->rollback();
        	FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    	}
}

void OracleAPI::getCancelJob(std::vector<int>& requestIDs){
	const std::string tag = "getCancelJob";
	const std::string tag1 = "getCancelJobUpdateCancel";
	std::string query = "select t_file.pid, t_job.job_id from t_file, t_job where t_file.job_id=t_job.job_id and t_file.FILE_STATE='CANCELED' and t_file.PID IS NOT NULL AND t_job.CANCEL_JOB IS NULL ";
	std::string update = "update t_job SET CANCEL_JOB='Y' where job_id=:1 ";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
        oracle::occi::ResultSet* r = conn->createResultset(s);
        oracle::occi::Statement* s1 = conn->createStatement(update, tag1);
		
        while (r->next()) {           
            int pid = r->getInt(1);
	    std::string job_id = r->getString(2);
            requestIDs.push_back(pid);
	    s1->setString(1, job_id);	    		
            s1->executeUpdate();
	    conn->commit();	       	    
        }
        
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);
        conn->destroyStatement(s1, tag1);	
	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}


    /*protocol config*/
bool OracleAPI::is_se_group_member(std::string se){
	const std::string tag = "is_se_group_member";
	std::string query = "select * from t_se_group where SE_NAME=:1 ";
	bool exists = false;	
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1,se);	
        oracle::occi::ResultSet* r = conn->createResultset(s);
	
        if (r->next()) {           
            exists = true;
        }        
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);	
	return exists;
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	return exists;
    }	
return exists;
}

bool OracleAPI::is_se_protocol_exist(std::string se){
	const std::string tag = "is_se_protocol_exist";
	std::string query = "select * from t_se_protocol where SOURCE_SE=:1 and DEST_SE IS NULL ";
	bool exists = false;	
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1,se);	
        oracle::occi::ResultSet* r = conn->createResultset(s);
	
        if (r->next()) {           
            exists = true;
        }        
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);	
	return exists;
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	return exists;
    }	
return exists;
}
    
bool OracleAPI::is_se_pair_protocol_exist(std::string se1, std::string se2){
	const std::string tag = "is_se_pair_protocol_exist";
	std::string query = "select * from t_se_protocol where SOURCE_SE=:1 AND DEST_SE=:2 ";
	bool exists = false;	
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1,se1);
	s->setString(2,se2);	
        oracle::occi::ResultSet* r = conn->createResultset(s);

        if (r->next()) {           
            exists = true;
        }        
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);	
	return exists;
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	return exists;
    }	
return exists;
}

    
SeProtocolConfig* OracleAPI::get_se_protocol_config(std::string se){
	SeProtocolConfig* seProtocolConfig = NULL;
	const std::string tag = "get_se_protocol_config";
	std::string query = "select SE_ROW_ID,SE_GROUP_NAME,SOURCE_SE,DEST_SE,CONTACT,BANDWIDTH,NOSTREAMS,NOFILES, "
				"TCP_BUFFER_SIZE,NOMINAL_THROUGHPUT,SE_PAIR_STATE,LAST_ACTIVE,MESSAGE,LAST_MODIFICATION,"
				"ADMIN_DN,SE_LIMIT,BLOCKSIZE ,HTTP_TO ,TX_LOGLEVEL ,URLCOPY_PUT_TO,URLCOPY_PUTDONE_TO,URLCOPY_GET_TO,"
				"URLCOPY_GETDONE_TO,URLCOPY_TX_TO,URLCOPY_TXMARKS_TO,SRMCOPY_DIRECTION,SRMCOPY_TO,SRMCOPY_REFRESH_TO,"
				"TARGET_DIR_CHECK ,URL_COPY_FIRST_TXMARK_TO,TX_TO_PER_MB ,NO_TX_ACTIVITY_TO,PREPARING_FILES_RATIO FROM t_se_protocol where SOURCE_SE=:1 and DEST_SE IS NULL and SE_GROUP_NAME IS NULL";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1,se);	
        oracle::occi::ResultSet* r = conn->createResultset(s);
	seProtocolConfig = new SeProtocolConfig();
        while (r->next()) {    
		seProtocolConfig->NOSTREAMS = r->getInt(7);
		seProtocolConfig->URLCOPY_TX_TO =  r->getInt(24);
        }        
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);	
	return seProtocolConfig;
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	return seProtocolConfig;
    }	
return seProtocolConfig;
}
    
SeProtocolConfig* OracleAPI::get_se_pair_protocol_config(std::string se1, std::string se2){
	SeProtocolConfig* seProtocolConfig = NULL;
	const std::string tag = "get_se_protocol_config";
	std::string query = "select SE_ROW_ID,SE_GROUP_NAME,SOURCE_SE,DEST_SE,CONTACT,BANDWIDTH,NOSTREAMS,NOFILES, "
				"TCP_BUFFER_SIZE,NOMINAL_THROUGHPUT,SE_PAIR_STATE,LAST_ACTIVE,MESSAGE,LAST_MODIFICATION,"
				"ADMIN_DN,SE_LIMIT,BLOCKSIZE ,HTTP_TO ,TX_LOGLEVEL ,URLCOPY_PUT_TO,URLCOPY_PUTDONE_TO,URLCOPY_GET_TO,"
				"URLCOPY_GETDONE_TO,URLCOPY_TX_TO,URLCOPY_TXMARKS_TO,SRMCOPY_DIRECTION,SRMCOPY_TO,SRMCOPY_REFRESH_TO,"
				"TARGET_DIR_CHECK ,URL_COPY_FIRST_TXMARK_TO,TX_TO_PER_MB ,NO_TX_ACTIVITY_TO,PREPARING_FILES_RATIO FROM t_se_protocol where"
				" SOURCE_SE=:1 and DEST_SE=:2 and SE_GROUP_NAME IS NULL";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1,se1);
	s->setString(2,se2);		
        oracle::occi::ResultSet* r = conn->createResultset(s);
	
	seProtocolConfig = new SeProtocolConfig();
        while (r->next()) {    
		seProtocolConfig->NOSTREAMS = r->getInt(7);
		seProtocolConfig->URLCOPY_TX_TO =  r->getInt(24);
        }        
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);	
	return seProtocolConfig;
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	return seProtocolConfig;
    }	
return seProtocolConfig;
}
    
SeProtocolConfig* OracleAPI::get_se_group_protocol_config(std::string se){
	SeProtocolConfig* seProtocolConfig = NULL;
	const std::string tag = "get_se_group_protocol_config";
	std::string query = "select SE_ROW_ID,SE_GROUP_NAME,SOURCE_SE,DEST_SE,CONTACT,BANDWIDTH,NOSTREAMS,NOFILES, "
				"TCP_BUFFER_SIZE,NOMINAL_THROUGHPUT,SE_PAIR_STATE,LAST_ACTIVE,MESSAGE,LAST_MODIFICATION,"
				"ADMIN_DN,SE_LIMIT,BLOCKSIZE ,HTTP_TO ,TX_LOGLEVEL ,URLCOPY_PUT_TO,URLCOPY_PUTDONE_TO,URLCOPY_GET_TO,"
				"URLCOPY_GETDONE_TO,URLCOPY_TX_TO,URLCOPY_TXMARKS_TO,SRMCOPY_DIRECTION,SRMCOPY_TO,SRMCOPY_REFRESH_TO,"
				"TARGET_DIR_CHECK ,URL_COPY_FIRST_TXMARK_TO,TX_TO_PER_MB ,NO_TX_ACTIVITY_TO,PREPARING_FILES_RATIO FROM t_se_protocol where"
				" (SOURCE_SE=:1 OR DEST_SE=:2) and SE_GROUP_NAME IS NOT NULL";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1,se);
	s->setString(2,se);		
        oracle::occi::ResultSet* r = conn->createResultset(s);
	
	seProtocolConfig = new SeProtocolConfig();
        while (r->next()) {    
		seProtocolConfig->NOSTREAMS = r->getInt(7);
		seProtocolConfig->URLCOPY_TX_TO =  r->getInt(24);
        }        
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);	
	return seProtocolConfig;
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	return seProtocolConfig;
    }	
return seProtocolConfig;
}

void OracleAPI::add_se_protocol_config(SeProtocolConfig* seProtocolConfig){
	const std::string tag = "add_se_protocol_config";
	std::string query = "insert into t_se_protocol(SOURCE_SE,NOSTREAMS,URLCOPY_TX_TO) values(:1,:2,:3)";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, seProtocolConfig->SOURCE_SE);
	s->setInt(2, seProtocolConfig->NOSTREAMS);	
	s->setInt(3, seProtocolConfig->URLCOPY_TX_TO);	    			    		
        s->executeUpdate();
	conn->commit();	       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}

void OracleAPI::add_se_pair_protocol_config(SeProtocolConfig* sePairProtocolConfig){
	const std::string tag = "add_se_pair_protocol_config";
	std::string query = "insert into t_se_protocol(SOURCE_SE,DEST_SE,NOSTREAMS,URLCOPY_TX_TO) values(:1,:2,:3,:4)";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, sePairProtocolConfig->SOURCE_SE);
	s->setString(2, sePairProtocolConfig->DEST_SE);	
	s->setInt(3, sePairProtocolConfig->NOSTREAMS);	
	s->setInt(4, sePairProtocolConfig->URLCOPY_TX_TO);	    			    		
        s->executeUpdate();
	conn->commit();	       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}

void OracleAPI::add_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig){
	const std::string tag = "add_se_group_protocol_config";
	std::string query = "insert into t_se_protocol(SE_GROUP_NAME,NOSTREAMS,URLCOPY_TX_TO) values(:1,:2,:3)";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, seGroupProtocolConfig->SE_GROUP_NAME);
	s->setInt(2, seGroupProtocolConfig->NOSTREAMS);	
	s->setInt(3, seGroupProtocolConfig->URLCOPY_TX_TO);	    			    		
        s->executeUpdate();
	conn->commit();	       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}
    
void OracleAPI::delete_se_protocol_config(SeProtocolConfig* seProtocolConfig){
	const std::string tag = "delete_se_protocol_config";
	std::string query = "delete from t_se_protocol where SOURCE_SE=:1 and DEST_SE IS NULL and SE_GROUP_NAME IS NULL ";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, seProtocolConfig->SOURCE_SE);				    		
        s->executeUpdate();
	conn->commit();	       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}

void OracleAPI::delete_se_pair_protocol_config(SeProtocolConfig* sePairProtocolConfig){
	const std::string tag = "delete_se_pair_protocol_config";
	std::string query = "delete from t_se_protocol where SOURCE_SE=:1 and DEST_SE=:2 ";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, sePairProtocolConfig->SOURCE_SE);
	s->setString(2, sePairProtocolConfig->DEST_SE);	    			    		
        s->executeUpdate();
	conn->commit();	       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}

void OracleAPI::delete_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig){
	const std::string tag = "delete_se_group_protocol_config";
	std::string query = "delete from t_se_protocol where SE_GROUP_NAME=:1";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, seGroupProtocolConfig->SE_GROUP_NAME);    			    		
        s->executeUpdate();
	conn->commit();	       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}





void OracleAPI::update_se_protocol_config(SeProtocolConfig* seProtocolConfig){
}

void OracleAPI::update_se_pair_protocol_config(SeProtocolConfig* sePairProtocolConfig){
}

void OracleAPI::update_se_group_protocol_config(SeProtocolConfig* seGroupProtocolConfig){
	int index = 1;
	const std::string tag = "update_se_group_protocol_config";
        std::stringstream query;
    	query << "UPDATE t_se_protocol SET ";
		if( (seGroupProtocolConfig->CONTACT).length() > 0){
			query << " CONTACT =:" << index;
			++index;
		}
		if( seGroupProtocolConfig->NOSTREAMS > -1){
			query << ", NOSTREAMS =:" << index;
			++index;
		}
		if( seGroupProtocolConfig->TCP_BUFFER_SIZE > -1){
			query << ", TCP_BUFFER_SIZE =:" << index;
			++index;
		}
		if( seGroupProtocolConfig->BLOCKSIZE > -1){
			query << ", BLOCKSIZE =:" << index;
			++index;
		}
		if( seGroupProtocolConfig->BLOCKSIZE > -1){
			query << ", BLOCKSIZE =:" << index;
			++index;
		}
    		query << " WHERE se_group_name =:" << index;
    try {
        oracle::occi::Statement* s = conn->createStatement(query.str(), tag);		   			    		
        s->executeUpdate();
	conn->commit();	       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}
 

SeProtocolConfig* OracleAPI::getProtocol(std::string se1, std::string se2){
	bool is_group = is_se_group_member(se2);
	if(is_group){
		return get_se_group_protocol_config(se2);
	}else{
		bool is_pair = is_se_pair_protocol_exist(se1, se2);
		if(is_pair){
			return get_se_pair_protocol_config(se1, se2);
		}else{
			bool is_se = is_se_protocol_exist(se2);
			if(is_se)
				return get_se_protocol_config(se2);
		}
	}
	return NULL;
}

    /*se group operations*/
void OracleAPI::add_se_to_group(std::string se, std::string group){
	const std::string tag = "add_se_to_group";
	std::string query = "insert into t_se_group(se_group_name, se_name) values(:1,:2)";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, group);
	s->setString(2, se);	    			    		
        s->executeUpdate();
	conn->commit();	       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}

void OracleAPI::remove_se_from_group(std::string se, std::string group){
	const std::string tag = "remove_se_from_group";
	std::string query = "delete from t_se_group where se_name=:1 and se_group_name=:2";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, se);
	s->setString(2, group);	    			    		
        s->executeUpdate();
	conn->commit();	       	           
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}

void OracleAPI::delete_group(std::string group){
	const std::string tag = "delete_group";
	std::string query = "delete from t_se_group where se_group_name=:1";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, group);	    		
        s->executeUpdate();
	conn->commit();	       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}


    /*t_credential API*/
void OracleAPI::insertGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs){
	const std::string tag = "insertGrDPStorageCacheElement";
	std::string query = "INSERT INTO t_credential_cache (dlg_id, dn, cert_request, priv_key, voms_attrs) VALUES (:1, :2, empty_clob(), empty_clob(), empty_clob())";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, dlg_id);
	s->setString(2, dn);	    			    		
        s->executeUpdate();
	conn->commit();	       	    
	updateGrDPStorageCacheElement(dlg_id, dn, cert_request, priv_key, voms_attrs);
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}

void OracleAPI::updateGrDPStorageCacheElement(std::string dlg_id, std::string dn, std::string cert_request, std::string priv_key, std::string voms_attrs){
	const std::string tag = "updateGrDPStorageCacheElement";
	std::string query = "UPDATE t_credential_cache SET cert_request = :1, priv_key = :2, voms_attrs = :3 WHERE dlg_id = :4 AND dn = :5";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, cert_request);
	s->setString(2, priv_key);			
	s->setString(3, voms_attrs); 
	s->setString(4, dlg_id);
	s->setString(5, dn);	    			    		
        s->executeUpdate();
	conn->commit();	       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}


CredCache* OracleAPI::findGrDPStorageCacheElement(std::string delegationID, std::string dn){
	CredCache* cred = NULL;
	const std::string tag = "findGrDPStorageCacheElement";
	std::string query = "SELECT dlg_id, dn, voms_attrs, cert_request, priv_key FROM t_credential_cache WHERE dlg_id = :1 AND dn = :2";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1,delegationID);
	s->setString(2,dn);		
        oracle::occi::ResultSet* r = conn->createResultset(s);
		
        if(r->next()) {    
		cred = new CredCache();
  		cred->delegationID = r->getString(1);
		cred->DN = r->getString(2);
		OracleTypeConversions::toString(r->getClob(3), cred->vomsAttributes);
		OracleTypeConversions::toString(r->getClob(4), cred->certificateRequest);
		OracleTypeConversions::toString(r->getClob(5), cred->privateKey);
        }        
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);	
	return cred;
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	return cred;
    }	
return cred;
}




void OracleAPI::deleteGrDPStorageCacheElement(std::string delegationID, std::string dn){
	const std::string tag = "deleteGrDPStorageCacheElement";
	std::string query = "DELETE FROM t_credential_cache WHERE dlg_id = :1 AND dn = :2";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, delegationID);	    		
	s->setString(2, dn);	    			
        s->executeUpdate();
	conn->commit();	       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}
    
void OracleAPI::insertGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time){
	const std::string tag = "insertGrDPStorageElement";
	std::string query = "INSERT INTO t_credential (dlg_id, dn, termination_time, proxy, voms_attrs ) VALUES (:1, :2, :3, empty_clob(), empty_clob())";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, dlg_id);
	s->setString(2, dn);	    		
	s->setTimestamp(3, OracleTypeConversions::toTimestamp(termination_time, conn->getEnv())); 		    		
        s->executeUpdate();
	conn->commit();	
	updateGrDPStorageElement(dlg_id, dn, proxy, voms_attrs, termination_time);       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}

void OracleAPI::updateGrDPStorageElement(std::string dlg_id, std::string dn, std::string proxy, std::string voms_attrs, time_t termination_time){
	const std::string tag = "updateGrDPStorageElement";
	std::string query = "UPDATE t_credential SET proxy = :1, voms_attrs = :2, termination_time = :3 WHERE dlg_id = :4 AND dn = :5";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, proxy);
	s->setString(2, voms_attrs);			
	s->setTimestamp(3, OracleTypeConversions::toTimestamp(termination_time, conn->getEnv())); 
	s->setString(4, dlg_id);
	s->setString(5, dn);	    			    		
        s->executeUpdate();
	conn->commit();	       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}

Cred* OracleAPI::findGrDPStorageElement(std::string delegationID, std::string dn){
	Cred* cred = NULL;
	const std::string tag = "findGrDPStorageElement";
	std::string query = "SELECT dlg_id, dn, voms_attrs, proxy, termination_time FROM t_credential WHERE dlg_id = :1 AND dn = :2";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1,delegationID);
	s->setString(2,dn);		
        oracle::occi::ResultSet* r = conn->createResultset(s);
		
        if(r->next()) {    
		cred = new Cred();
		cred->delegationID = r->getString(1) ;
		cred->DN = r->getString(2) ;
		OracleTypeConversions::toString(r->getClob(3), cred->vomsAttributes);
		OracleTypeConversions::toString(r->getClob(4), cred->proxy);
		cred->termination_time = OracleTypeConversions::toTimeT(r->getTimestamp(5));						
        }        
        conn->destroyResultset(s, r);
        conn->destroyStatement(s, tag);	
	return cred;
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	return cred;
    }	
return cred;
}

void OracleAPI::deleteGrDPStorageElement(std::string delegationID, std::string dn){
	const std::string tag = "deleteGrDPStorageElement";
	std::string query = "DELETE FROM t_credential WHERE dlg_id = :1 AND dn = :2";
    try {
        oracle::occi::Statement* s = conn->createStatement(query, tag);	
	s->setString(1, delegationID);	    		
	s->setString(2, dn);	    			
        s->executeUpdate();
	conn->commit();	       	    
        conn->destroyStatement(s, tag);	
    } catch (oracle::occi::SQLException const &e) {
        conn->rollback();
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }	
}




// the class factories
extern "C" GenericDbIfce* create() {
    return new OracleAPI;
}

extern "C" void destroy(GenericDbIfce* p) {
    delete p;
}
