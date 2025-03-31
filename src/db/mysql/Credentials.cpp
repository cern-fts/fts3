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


std::pair<std::string, bool> MySqlAPI::findToken(const std::string& tokenId)
{
    soci::session sql(*connectionPool);

    try {
        std::string token;
        bool unmanagedVal;
        soci::indicator unmanagedNull = soci::i_ok;

        sql << "SELECT access_token, unmanaged "
               "FROM t_token "
               "WHERE token_id = :tokenID",
                soci::use(tokenId),
                soci::into(token),
                soci::into(unmanagedVal, unmanagedNull);

        if (sql.got_data()) {
            bool unmanaged = (unmanagedNull != soci::i_null) ? unmanagedVal : false;
            return {token, unmanaged};
        }
    } catch (std::exception& e) {
        throw UserError(std::string(__func__) + ": Caught exception " +  e.what());
    } catch (...) {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return {"", false};
}
