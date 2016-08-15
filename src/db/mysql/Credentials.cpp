/*
 * Copyright (c) CERN 2013-2016
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MySqlAPI.h"
#include "sociConversions.h"
#include "common/Exceptions.h"
#include "db/generic/DbUtils.h"

#include <soci/mysql/soci-mysql.h>

using namespace fts3::common;
using namespace db;


bool MySqlAPI::insertCredentialCache(const std::string& delegationId,
    const std::string& userDn, const std::string& certRequest,
    const std::string& privateKey, const std::string& vomsAttrs)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();
        sql << "INSERT INTO t_credential_cache "
            "    (dlg_id, dn, cert_request, priv_key, voms_attrs) VALUES "
            "    (:dlgId, :dn, :certRequest, :privKey, :vomsAttrs)",
            soci::use(delegationId), soci::use(userDn), soci::use(certRequest),
            soci::use(privateKey), soci::use(vomsAttrs);
        sql.commit();
    }
    catch (soci::mysql_soci_error const &e)
    {
        sql.rollback();
        unsigned int err_code = e.err_num_;

        // the magic '1062' is the error code of
        // Duplicate entry 'XXX' for key 'PRIMARY'
        if (err_code == 1062) return false;

        throw UserError(std::string(__func__) + ": Caught exception " +  e.what());
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " +  e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    return true;
}


boost::optional<UserCredentialCache> MySqlAPI::findCredentialCache(
    const std::string& delegationId, const std::string& userDn)
{
    soci::session sql(*connectionPool);

    try
    {
        UserCredentialCache cred;
        sql << "SELECT dlg_id, dn, voms_attrs, cert_request, priv_key "
            "FROM t_credential_cache "
            "WHERE dlg_id = :dlgId and dn = :dn",
            soci::use(delegationId), soci::use(userDn), soci::into(cred);
        if (sql.got_data())
        {
            return cred;
        }
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " +  e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    return boost::optional<UserCredentialCache>();
}


void MySqlAPI::deleteCredentialCache(const std::string& delegationId, const std::string& userDn)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();
        sql << "DELETE FROM t_credential_cache WHERE dlg_id = :dlgId AND dn = :dn",
            soci::use(delegationId), soci::use(userDn);
        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " +  e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::insertCredential(const std::string& delegationId,
    const std::string& userDn, const std::string& proxy, const std::string& vomsAttrs,
    time_t terminationTime)
{
    soci::session sql(*connectionPool);

    try
    {
        struct tm tTime;
        gmtime_r(&terminationTime, &tTime);

        sql.begin();
        sql << "INSERT INTO t_credential "
            "    (dlg_id, dn, termination_time, proxy, voms_attrs) VALUES "
            "    (:dlgId, :dn, :terminationTime, :proxy, :vomsAttrs)",
            soci::use(delegationId), soci::use(userDn), soci::use(tTime),
            soci::use(proxy), soci::use(vomsAttrs);
        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " +  e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::updateCredential(const std::string& delegationId,
    const std::string& dn, const std::string& proxy, const std::string& vomsAttrs,
    time_t terminationTime)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();
        struct tm tTime;
        gmtime_r(&terminationTime, &tTime);

        sql << "UPDATE t_credential SET "
            "    proxy = :proxy, "
            "    voms_attrs = :vomsAttrs, "
            "    termination_time = :terminationTime "
            "WHERE dlg_id = :dlgId AND dn = :dn",
            soci::use(proxy), soci::use(vomsAttrs), soci::use(tTime),
            soci::use(delegationId), soci::use(dn);

        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


boost::optional<UserCredential> MySqlAPI::findCredential(
    const std::string& delegationId, const std::string& userDn)
{
    soci::session sql(*connectionPool);

    try
    {
        UserCredential cred;
        sql << "SELECT dlg_id, dn, voms_attrs, proxy, termination_time "
            "FROM t_credential "
            "WHERE dlg_id = :dlgId AND dn =:dn",
            soci::use(delegationId), soci::use(userDn),
            soci::into(cred);

        if (sql.got_data())
        {
            return cred;
        }
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " +  e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return boost::optional<UserCredential>();
}


void MySqlAPI::deleteCredential(const std::string& delegationId, const std::string& userDn)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();
        sql << "DELETE FROM t_credential WHERE dlg_id = :dlgId AND dn = :dn",
            soci::use(delegationId), soci::use(userDn);
        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " +  e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


bool MySqlAPI::isCredentialExpired(const std::string & dlg_id, const std::string & dn)
{
    soci::session sql(*connectionPool);

    bool expired = true;
    try
    {
        struct tm termTime;

        sql << "SELECT termination_time FROM t_credential WHERE dlg_id = :dlgId AND dn = :dn",
            soci::use(dlg_id), soci::use(dn), soci::into(termTime);

        if (sql.got_data())
        {
            time_t termTimestamp = timegm(&termTime);
            time_t now = getUTC(0);
            expired = (difftime(termTimestamp, now) <= 0);
        }
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return expired;
}
