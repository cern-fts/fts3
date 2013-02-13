#include "OracleConnection.h"


int taf_callback(Environment*, Connection*, void*, Connection::FailOverType foType, Connection::FailOverEventType foEvent) {    
    switch (foEvent) {
        case Connection::FO_END:
            break;
        case Connection::FO_ABORT:
            break;
        case Connection::FO_REAUTH:
            break;
        case Connection::FO_BEGIN:
            break;
        case Connection::FO_ERROR:
            break;
        default:
            break;
    }

    switch (foType) {
        case Connection::FO_NONE:
            break;
        case Connection::FO_SESSION:
            break;
        case Connection::FO_SELECT:
            break;
        default:
            break;
    }
    if (foEvent == Connection::FO_ERROR)
        return FO_RETRY;
    if (foEvent == Connection::FO_END) {
        return 0;
    }
    return 0;
}

OracleConnection::OracleConnection(const std::string username,const  std::string password, const std::string connectString, int pooledConn) : env(NULL), scPool(NULL), username_(username),
password_(password), connectString_(connectString), maxConn(20), minConn(15), incrConn(1) {

    try {        
	if(pooledConn==1){
		maxConn = pooledConn;
		minConn = pooledConn;
		incrConn = 0;
	}else if(pooledConn == 2){
		maxConn = pooledConn;
		minConn = pooledConn;
		incrConn = 0;				
	}else{
		maxConn = pooledConn;
		minConn = pooledConn/2;
		incrConn = 1;		
	}
	
        env = oracle::occi::Environment::createEnvironment(oracle::occi::Environment::THREADED_MUTEXED);
        if (env) {
            // Create a homogenous connection pool  
            scPool = env->createStatelessConnectionPool
                    (username, password, connectString, maxConn,
                    minConn, incrConn, oracle::occi::StatelessConnectionPool::HOMOGENEOUS);
            scPool->setStmtCacheSize(500);
        }
    } catch (oracle::occi::SQLException const &e) {
        throw Err_Custom(e.what());
    } catch (...) {
        throw Err_Custom("Unknown oracle exception");
    }
}

OracleConnection::~OracleConnection(){
    // Destroy the connection pool  
    env->terminateStatelessConnectionPool (scPool);      
    oracle::occi::Environment::terminateEnvironment (env);  
}

oracle::occi::Connection* OracleConnection::getPooledConnection() {
    oracle::occi::Connection* tempConn = NULL;
    try {
        if (scPool) {
            tempConn = scPool->getConnection();
            tempConn->setTAFNotify(taf_callback, NULL);
            tempConn->setStmtCacheSize(100);
        }
    } catch (const oracle::occi::SQLException& e) {
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    } catch (...) {
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
    }
    return tempConn;
}

void OracleConnection::releasePooledConnection(oracle::occi::Connection *conn) {
    if (conn)
        if (scPool)
            scPool->releaseConnection(conn);
}

oracle::occi::ResultSet* OracleConnection::createResultset(oracle::occi::Statement* s, oracle::occi::Connection*) {
    if (s)
        return s->executeQuery();
    else
        return NULL;

}

oracle::occi::Statement* OracleConnection::createStatement(std::string sql, std::string tag, oracle::occi::Connection* conn) {

    if (conn) {
        oracle::occi::Statement* s = NULL;
        // Check if the statement is already cached
        if (true == conn->isCached("", tag)) {
            s = conn->createStatement("", tag);
            s->setAutoCommit(true);
            return s;
        } else {
            s = conn->createStatement(sql, tag);
            s->setAutoCommit(true);
            return s;
        }

        return s;
    }
    return NULL;
}

void OracleConnection::destroyResultset(oracle::occi::Statement* s, oracle::occi::ResultSet* r) {
    if (s) {
        if (r) {
            s->closeResultSet(r);
        }
    }
    r = NULL;
}

void OracleConnection::destroyStatement(oracle::occi::Statement* s, std::string tag, oracle::occi::Connection* conn) {
    if (conn) {
        if (tag.length() == 0) {
            if (s) {
                conn->terminateStatement(s);
            }
        } else {
            if (s) {
                if (conn)
                    conn->terminateStatement(s, tag);
            }
        }
    }
    s = NULL;
}

void OracleConnection::commit(oracle::occi::Connection*) {
    //if (conn)
        //conn->commit();

}

void OracleConnection::rollback(oracle::occi::Connection* conn) {
    if (conn)
        conn->rollback();

}
