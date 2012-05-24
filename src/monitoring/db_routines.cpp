/**
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://www.eu-egee.org/partners/ for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This is singleton class to be used(if needed) for retrieving information from the database
 *  Currently, for each transfer, the following info is retrieved:
 *	-VO
 *      -source site name
 *      -dest site name
 */

#include "db_routines.h"
#include "utility_routines.h"
#include "Logger.h"

using namespace oracle::occi;

bool db_routines::instanceFlag = false;
db_routines* db_routines::single = NULL;
oracle::occi::Environment* db_routines::env = NULL;
oracle::occi::Connection* db_routines::conn = NULL;
oracle::occi::Statement* db_routines::stmt = NULL;
oracle::occi::ResultSet* db_routines::rs = NULL;
vector <std::string> db_routines::credentials;
vector <std::string> db_routines::temp;
std::string db_routines::errorMessage = std::string("");

//recovery vector in case no data is retrieved
static vector<std::string> recoveryVector(3, "");

db_routines* db_routines::getInstance() {
    if (!instanceFlag) {
        single = new db_routines();
        instanceFlag = true;
        return single;
    } else {
        return single;
    }
}

db_routines::db_routines() {
}

db_routines::~db_routines() {
    instanceFlag = false;
}

/*
Maintain a connection to the database(per channel defined in FTS)
and execute query to retrieve the vo, source site and destination site
of each transfer job
 */
vector<std::string> db_routines::getData(const char * id) {

    bool fileExists = get_mon_cfg_file();

    if ((fileExists == false) || (false == getACTIVE())){
        logger::writeLog("Check msg config file to see if the connection is active"); 
        return recoveryVector;
    }

    static volatile bool isInitialized = false;
    temp.clear();
    std::string ids = id;
    std::string vo = std::string("");
    std::string source_site = std::string("");
    std::string dest_site = std::string("");

    try {
        if (isInitialized == false) {
            if (credentials.size() == 0) {
                credentials = oracleCredentials();
                if (credentials[0].length() == 0) /*double check to ensure credentials retrieved correctly*/
                    return recoveryVector;
            }

            try{
            if (env == NULL) {
                env = oracle::occi::Environment::createEnvironment(Environment::DEFAULT);
                if (env) {
                    if (conn == NULL) {
                        conn = env->createConnection(credentials[0], credentials[1], credentials[2]);
                        conn->setStmtCacheSize(100);
                    }
                }
             }
            isInitialized = true;
	   }
	   catch (const SQLException& exc) {
	     logger::writeMsg("Cannot connect to database, check credentials");
	     isInitialized = false;
	   }
        }

        std::string sql_query = "select T_JOB.VO_NAME, T_CHANNEL.SOURCE_SITE, T_CHANNEL.DEST_SITE ";
        sql_query += "from T_JOB, T_CHANNEL where T_CHANNEL.CHANNEL_NAME = T_JOB.CHANNEL_NAME ";
        sql_query += "and T_JOB.JOB_ID='" + ids + "' and rownum < 2";

        if (conn) {
            stmt = conn->createStatement(sql_query);
            rs = stmt->executeQuery();
            while (rs->next()) {
                vo = rs->getString(1);
                source_site = rs->getString(2);
                dest_site = rs->getString(3);
            }
        }
    } catch (const SQLException& exc) {
        errorMessage = "Cannot execute query: " + exc.getMessage();
        logger::writeMsg(errorMessage);
        if(conn){
	  if (stmt) {
            stmt->closeResultSet(rs);
            conn->terminateStatement(stmt);
          }
	}  
	return recoveryVector;
    }

    if (vo.length() == 0)
        vo = "";
    if (source_site.length() == 0)
        source_site = "";
    if (dest_site.length() == 0)
        dest_site = "";

    temp.push_back(vo);
    temp.push_back(source_site);
    temp.push_back(dest_site);
    
    if(conn){
      if (stmt) {
        stmt->closeResultSet(rs);
        conn->terminateStatement(stmt);
      }
    }

    return temp;
}
