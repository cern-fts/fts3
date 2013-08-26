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

#include "OracleConnection.h"


int taf_callback(Environment*, Connection*, void*, Connection::FailOverType foType, Connection::FailOverEventType foEvent)
{
    switch (foEvent)
        {
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

    switch (foType)
        {
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
    if (foEvent == Connection::FO_END)
        {
            return 0;
        }
    return 0;
}


OracleConnection::OracleConnection(const std::string username,const  std::string password, const std::string connectString, int pooledConn) : env(NULL), scPool(NULL), username_(username),
    password_(password), connectString_(connectString), maxConn(20), minConn(15), incrConn(1)
{

    try
        {
            if(pooledConn==1)
                {
                    maxConn = pooledConn;
                    minConn = pooledConn;
                    incrConn = 0;
                }
            else if(pooledConn == 2)
                {
                    maxConn = pooledConn;
                    minConn = pooledConn;
                    incrConn = 0;
                }
            else
                {
                    maxConn = pooledConn;
                    minConn = pooledConn/2;
                    incrConn = 1;
                }

            env = oracle::occi::Environment::createEnvironment(oracle::occi::Environment::THREADED_MUTEXED);
            if (env)
                {
                    // Create a homogenous connection pool
                    scPool = env->createStatelessConnectionPool
                             (username, password, connectString, maxConn,
                              minConn, incrConn, oracle::occi::StatelessConnectionPool::HOMOGENEOUS);
                    scPool->setStmtCacheSize(500);
                }
        }
    catch (oracle::occi::SQLException const &e)
        {
            throw Err_Custom(e.what());
        }
    catch (...)
        {
            throw Err_Custom("Unknown oracle exception");
        }
}

OracleConnection::~OracleConnection()
{
    // Destroy the connection pool
    env->terminateStatelessConnectionPool (scPool);
    oracle::occi::Environment::terminateEnvironment (env);
}

SafeConnection OracleConnection::getPooledConnection()
{
    try
        {
            if (scPool)
                {
                    oracle::occi::Connection* tempConn = NULL;
                    tempConn = scPool->getConnection();
                    tempConn->setTAFNotify(taf_callback, NULL);
                    tempConn->setStmtCacheSize(500);
                    return SafeConnection(tempConn, ConnectionDeallocator(scPool));
                }
        }
    catch (const oracle::occi::SQLException& e)
        {
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch (...)
        {
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    return SafeConnection();
}

SafeResultSet OracleConnection::createResultset(SafeStatement& s, SafeConnection&)
{
    if (s)
        return SafeResultSet(s->executeQuery(), ResultSetDeallocator(s));
    else
        return SafeResultSet();

}

SafeStatement OracleConnection::createStatement(std::string sql, std::string tag, SafeConnection& conn)
{

    if (conn)
        {
            oracle::occi::Statement* s = NULL;
            // Check if the statement is already cached
            if (true == conn->isCached("", tag))
                {
                    s = conn->createStatement("", tag);
                    s->setAutoCommit(false);
                    return SafeStatement(s, StatementDeallocator(conn, tag));
                }
            else
                {
                    s = conn->createStatement(sql, tag);
                    s->setAutoCommit(false);
                    return SafeStatement(s, StatementDeallocator(conn, tag));
                }
        }
    return SafeStatement();
}



void OracleConnection::commit(SafeConnection&)
{
    //if (conn)
    //conn->commit();

}

void OracleConnection::rollback(SafeConnection& conn)
{
    if (conn)
        conn->rollback();
}
