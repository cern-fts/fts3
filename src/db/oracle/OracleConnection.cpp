#include "OracleConnection.h"
#include "error.h"

using namespace FTS3_COMMON_NAMESPACE;

OracleConnection::OracleConnection(std::string username, std::string password, std::string connectString) {

    try {
        env = oracle::occi::Environment::createEnvironment(oracle::occi::Environment::THREADED_MUTEXED);
        if (env) {
            conn = env->createConnection(username, password, connectString);
	    conn->setStmtCacheSize(100);	    
        }
    } catch (oracle::occi::SQLException const &e) {
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
	throw Err_Custom(e.what());
    }
}

OracleConnection::~OracleConnection() {
    try {
        if (conn)
            env->terminateConnection(conn);
        if (env)
            oracle::occi::Environment::terminateEnvironment(env);
    } catch (oracle::occi::SQLException const &e) {
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));   
    }

}

oracle::occi::ResultSet* OracleConnection::createResultset(oracle::occi::Statement* s) {
   
        if (s)
            return s->executeQuery();
        else
            return NULL;
  
}

oracle::occi::Statement* OracleConnection::createStatement(std::string sql, std::string tag) {
    
        if (conn)
            s = conn->createStatement(sql, tag);
        else
            s = NULL;
	return s;	    
}

void OracleConnection::destroyResultset(oracle::occi::Statement* s, oracle::occi::ResultSet* r) {
  
        s->closeResultSet(r);
  
}

void OracleConnection::destroyStatement(oracle::occi::Statement* s, std::string tag) {
   if(tag.length() == 0)
        conn->terminateStatement(s);
   else
        conn->terminateStatement(s, tag);
   
}

void OracleConnection::commit() {
   
        this->conn->commit();
    
}

void OracleConnection::rollback() {
   
        this->conn->rollback();
    
}
