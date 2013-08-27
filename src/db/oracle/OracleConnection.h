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
#include "logger.h"
#include "error.h"

using namespace FTS3_COMMON_NAMESPACE;
using namespace oracle::occi;


class ConnectionDeallocator
{
public:
    ConnectionDeallocator(oracle::occi::StatelessConnectionPool* pool): pool(pool) {}

    void operator () (oracle::occi::Connection* p)
    {
        if (pool && p)
            pool->releaseConnection(p);
    }

private:
    oracle::occi::StatelessConnectionPool* pool;
};
typedef boost::shared_ptr<oracle::occi::Connection> SafeConnection;


class StatementDeallocator
{
public:
    StatementDeallocator(SafeConnection& conn, const std::string& tag):
        conn(conn), tag(tag) {};

    void operator () (oracle::occi::Statement* s)
    {
        if (conn && s)
            {
                if (tag.length() == 0)
                    conn->terminateStatement(s);
                else
                    conn->terminateStatement(s, tag);
            }
    }

private:
    SafeConnection conn;
    std::string tag;
};
typedef boost::shared_ptr<oracle::occi::Statement> SafeStatement;


class ResultSetDeallocator
{
public:
    ResultSetDeallocator(SafeStatement& stmt): stmt(stmt) {}

    void operator () (oracle::occi::ResultSet* rs)
    {
        if (stmt && rs)
            stmt->closeResultSet(rs);
    }
private:
    SafeStatement stmt;
};
typedef boost::shared_ptr<oracle::occi::ResultSet> SafeResultSet;


/**
 * OracleConnection class declaration
 **/

class OracleConnection
{
public:

    /**
     * OracleConnection class ctr with parameters
     **/

    OracleConnection(const std::string username, const  std::string password, const std::string connectString, int pooledConn);

    /**
     * OracleConnection class ctr
     **/
    OracleConnection() {}

    /**
     * OracleConnection class dctr
     **/
    virtual ~OracleConnection();


    /**
     * OracleConnection get resultset
     **/
    SafeResultSet createResultset(SafeStatement& stmt, SafeConnection&);

    /**
     * OracleConnection create a statement
     **/
    SafeStatement createStatement(std::string sql, std::string tag, SafeConnection& conn);

    /**
     * OracleConnection commit row in the database
     **/
    void commit(SafeConnection& conn);

    /**
     * OracleConnection rollback in case of an error
     **/
    void rollback(SafeConnection& conn);

    SafeConnection getPooledConnection();

    /**
     * OracleConnection return the environment used in oracle
     **/
    inline oracle::occi::Environment* getEnv()
    {
        return env;
    }

    /* For backwards compatibility */
    void destroyResultset(SafeStatement&, SafeResultSet& r)
    {
        r.reset();
    }

    void destroyStatement(SafeStatement& s, const std::string&, SafeConnection&)
    {
        s.reset();
    }

    void releasePooledConnection(SafeConnection& c)
    {
        c.reset();
    }

private:
    oracle::occi::Environment* env;
    oracle::occi::StatelessConnectionPool *scPool;
    std::string username_;
    std::string password_;
    std::string connectString_;
    int maxConn;  //The maximum number of connections that can be opened the pool;
    int minConn;  //The number of connections initially created in a pool.
    int incrConn; //The number of connections by which to increment the pool if all open connections are busy
};
