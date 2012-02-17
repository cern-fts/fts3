#include "OracleConnection.h"
#include "Logger.h"

OracleConnection::OracleConnection(std::string username, std::string password, std::string connectString) : conn(NULL), env(NULL) {

    try {
        env = oracle::occi::Environment::createEnvironment(oracle::occi::Environment::THREADED_MUTEXED);
        if (env) {
            conn = env->createConnection(username, password, connectString);
	    conn->setStmtCacheSize(500);
        }
    } catch (oracle::occi::SQLException const &e) {
        Logger::instance().error(e.what());        
    }
}

OracleConnection::~OracleConnection() {
    try {
        if (conn)
            env->terminateConnection(conn);
        if (env)
            oracle::occi::Environment::terminateEnvironment(env);
    } catch (oracle::occi::SQLException const &e) {
        Logger::instance().error(e.what());
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
    
}

void OracleConnection::destroyResultset(oracle::occi::Statement* s, oracle::occi::ResultSet* r) {
  
      if(s && r)
        s->closeResultSet(r);
  
}

void OracleConnection::destroyStatement(oracle::occi::Statement* s, std::string tag) {
   
        conn->terminateStatement(s, tag);
   
}

void OracleConnection::commit() {
   
        this->conn->commit();
    
}

void OracleConnection::rollback() {
   
        this->conn->rollback();
    
}
