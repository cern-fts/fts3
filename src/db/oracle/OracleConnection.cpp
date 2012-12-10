#include "OracleConnection.h"
#include "logger.h"
#include "error.h"

using namespace FTS3_COMMON_NAMESPACE;

OracleConnection::OracleConnection(std::string username, std::string password, std::string connectString): env(NULL), conn(NULL),username_(username),
password_(password),  connectString_(connectString){

    try {
        env = oracle::occi::Environment::createEnvironment(oracle::occi::Environment::THREADED_MUTEXED);
        if (env) {
            conn = env->createConnection(username, password, connectString);
	    conn->setStmtCacheSize(300);	    
        }
    }catch (oracle::occi::SQLException const &e) {
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	//throw Err_Custom(e.what());
    }catch(...){
        //throw Err_Custom("Unknown exception");
    }
}

void OracleConnection::initConn(){
    // Check Preconditions
    if(NULL != conn){	
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom("DB Connection object already exists"));
    }
    try{
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Initializing DB connection"));	
        // Initialize Connection Object
	if(env){
        	conn = env->createConnection(username_, password_, connectString_);
        	// SetStatement Cache Size
        	conn->setStmtCacheSize(300);
	}
    } catch(const oracle::occi::SQLException& e){
        conn = NULL;
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }catch(...){
        conn = NULL;
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
    }
}
    
void OracleConnection::destroyConn(){
    // Check Preconditions
    if(NULL == conn){
        // Connection Already Deleted
        return;
    }
    try{
        // Finalize the Connection Object
	if(env){
        	env->terminateConnection(conn);
	}
    } catch(const oracle::occi::SQLException& e){
		FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));	
    }catch(...){
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
    }

    // Connection    
    conn = NULL;
}
    
bool OracleConnection::isAlive(){
	bool result = false;
	if(NULL == conn){	
		FTS3_COMMON_EXCEPTION_THROW(Err_Custom("No Connection to DB established"));
		return false;
	}
   try{
        const std::string tag = "isAlive";
        // Create Statement
        oracle::occi::Statement* stmt = conn->createStatement("SELECT SYSDATE FROM DUAL", tag);
        // Execute a Simple Query
        stmt->execute();
        conn->terminateStatement(stmt, tag);
        // Connection is still Alive
        result = true;
    } catch(const oracle::occi::SQLException& e){           
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        result = false;
    } catch(...){           
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unresolved database exception raised"));
        result = false;
    }     
    return result;	
}


bool OracleConnection::checkConn(){
    if(NULL == env){	
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Cannot Create DAO Context since the Environment is not initialized"));
        env = oracle::occi::Environment::createEnvironment(oracle::occi::Environment::THREADED_MUTEXED);
    }
    // Check the Oracle DAO context Object
        if(false == isAlive()){
            // Dispose Context
            destroyConn();
            sleep(5);
            // Initialize Connection
            initConn();
            
            // Check if now the connection is alive
            if(false == isAlive()){
		FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Invalid DAO Context after reinitialization"));
                // Dispose Context
                destroyConn();
                return false;
            }
        }
	return true;
}

OracleConnection::~OracleConnection() {
    try {
        if (env){
	   if(conn){
             env->terminateConnection(conn);
	   }
	   oracle::occi::Environment::terminateEnvironment(env);
        }
    } catch (oracle::occi::SQLException const &e) {
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));   
    } catch(...){           
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unresolved database exception raised during shutdown"));        
    }        

}

oracle::occi::ResultSet* OracleConnection::createResultset(oracle::occi::Statement* s) {   
        if (s)
            return s->executeQuery();
        else
            return NULL;
  
}

oracle::occi::Statement* OracleConnection::createStatement(std::string sql, std::string tag) {
            
    if(conn){
    oracle::occi::Statement* s = NULL;
   // Check if the statement is already cached
    if(true == conn->isCached("",tag)){
    	s = conn->createStatement("", tag);
	s->setAutoCommit(false);
	return s;
    }else{
        s = conn->createStatement(sql, tag);
	s->setAutoCommit(false);	
	return s;
    }
     
  return s;	
   }
   return NULL;    
}

void OracleConnection::destroyResultset(oracle::occi::Statement* s, oracle::occi::ResultSet* r) {
    if(s){
    	if(r){ 
        	s->closeResultSet(r);
	}
   }
   r=NULL;
}

void OracleConnection::destroyStatement(oracle::occi::Statement* s, std::string tag) {

 if(conn){
   if(tag.length() == 0){
     if(s){
        conn->terminateStatement(s);
	}
   }
   else{
   	if(s){
		if(conn)
        		conn->terminateStatement(s, tag);
	}
   }
 }
 s=NULL;	
}

void OracleConnection::commit() {
      if(conn)
        this->conn->commit();
    
}

void OracleConnection::rollback() {
      if(conn)
        this->conn->rollback();
    
}
