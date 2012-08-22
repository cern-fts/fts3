/********************************************//**
 * Copyright @ Members of the EMI Collaboration, 2010.
 * See www.eu-emi.eu for details on the copyright holders.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 ***********************************************/

/**
 * @file OracleConnection.h
 * @brief hanlde oracle connection
 * @author Michail Salichos
 * @date 09/02/2012
 * 
 **/

#pragma once

#include <iostream>
#include <occi.h>

/**
 * OracleConnection class declaration
 **/

class OracleConnection {
public:

/**
 * OracleConnection class ctr with parameters
 **/

    OracleConnection(std::string username, std::string password, std::string connectString);

/**
 * OracleConnection class ctr
 **/
    OracleConnection() {
    }
    
/**
 * OracleConnection class dctr
 **/    
    ~OracleConnection();


/**
 * OracleConnection get resultset
 **/
    oracle::occi::ResultSet* createResultset(oracle::occi::Statement* s);
    
/**
 * OracleConnection create a statement
 **/    
    oracle::occi::Statement* createStatement(std::string sql, std::string tag);

/**
 * OracleConnection destroy a resultset
 **/    
    void destroyResultset(oracle::occi::Statement* s, oracle::occi::ResultSet* r);
    
/**
 * OracleConnection destroy a statement
 **/        
    void destroyStatement(oracle::occi::Statement* s, std::string tag);
    
/**
 * OracleConnection commit row in the database
 **/        
    void commit();
    
/**
 * OracleConnection rollback in case of an error
 **/        
    void rollback();
    
/**
 * OracleConnection return the environment used in oracle
 **/        
    oracle::occi::Environment* getEnv(){
    	return env;
	}
    
    void initConn();    
    
    void destroyConn();        
    
    bool isAlive();
    
    bool checkConn();
    
    oracle::occi::Connection* getConn(){
        if(conn)
		return conn;

	checkConn();
    	}    

private:
    oracle::occi::Environment* env;
    oracle::occi::Connection* conn;
    std::string username_;
    std::string password_;
    std::string connectString_;

};
