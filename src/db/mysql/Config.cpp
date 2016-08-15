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

#include "common/definitions.h"
#include "common/Exceptions.h"

#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>

using namespace fts3::common;


std::map<std::string, double> MySqlAPI::getActivityShareConf(soci::session& sql, std::string vo)
{

    std::map<std::string, double> ret;

    soci::indicator isNull = soci::i_ok;
    std::string activity_share_str;
    try
    {

        sql <<
            " SELECT activity_share "
                " FROM t_activity_share_config "
                " WHERE vo = :vo "
                "   AND active = 'on'",
            soci::use(vo),
            soci::into(activity_share_str, isNull)
            ;

        if (isNull == soci::i_null || activity_share_str.empty()) return ret;

        // remove the opening '[' and closing ']'
        activity_share_str = activity_share_str.substr(1, activity_share_str.size() - 2);

        // iterate over activity shares
        boost::char_separator<char> sep(",");
        boost::tokenizer< boost::char_separator<char> > tokens(activity_share_str, sep);
        boost::tokenizer< boost::char_separator<char> >::iterator it;

        static const boost::regex re("^\\s*\\{\\s*\"([ a-zA-Z0-9\\._-]+)\"\\s*:\\s*((0\\.)?\\d+)\\s*\\}\\s*$");
        static const int ACTIVITY_NAME = 1;
        static const int ACTIVITY_SHARE = 2;

        for (it = tokens.begin(); it != tokens.end(); it++)
        {
            // parse single activity share
            std::string str = *it;

            boost::smatch what;
            boost::regex_match(str, what, re, boost::match_extra);

            std::string activity_name(what[ACTIVITY_NAME]);
            boost::algorithm::to_lower(activity_name);
            ret[activity_name] = boost::lexical_cast<double>(what[ACTIVITY_SHARE]);
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
    return ret;
}


std::vector<std::string> MySqlAPI::getAllActivityShareConf()
{
    soci::session sql(*connectionPool);

    std::vector<std::string> ret;

    try
    {
        soci::rowset<soci::row> rs = (
            sql.prepare <<
                        " SELECT vo "
                            " FROM t_activity_share_config "
        );

        soci::rowset<soci::row>::const_iterator it;
        for (it = rs.begin(); it != rs.end(); it++)
        {
            ret.push_back(it->get<std::string>("vo"));
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

    return ret;
}


boost::optional<StorageElement> MySqlAPI::getStorageElement(const std::string& seName)
{
    soci::session sql(*connectionPool);

    try
    {
        StorageElement se;
        sql << "SELECT * FROM t_se WHERE name = :name",
            soci::use(seName), soci::into(se);

        if (sql.got_data())
        {
            return se;
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

    return boost::optional<StorageElement>();
}


void MySqlAPI::addStorageElement(const std::string& seName, const std::string& state)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();
        sql << "INSERT INTO t_se (name, state) VALUES "
            "                 (:name, :state)",
            soci::use(seName), soci::use(state);
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
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


void MySqlAPI::updateStorageElement(const std::string& seName, const std::string& state)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();
        sql << "UPDATE t_se SET STATE = :state WHERE name = :name",
            soci::use(state), soci::use(seName);
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
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


unsigned MySqlAPI::getDebugLevel(const std::string& sourceStorage, const std::string& destStorage)
{
    soci::session sql(*connectionPool);

    try
    {
        unsigned level = 0;

        if(sourceStorage.empty() || destStorage.empty())
            return 0;

        soci::statement stmt =
            (sql.prepare
                << " SELECT debug_level "
                    " FROM t_debug "
                    " WHERE source_se = :source OR dest_se= :dest_se and debug_level is NOT NULL LIMIT 1 ", soci::use(
                sourceStorage), soci::use(destStorage), soci::into(
                level));
        stmt.execute(true);

        if(sql.got_data())
            return level;
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " +  e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return 0;
}


void MySqlAPI::setDebugLevel(const std::string& sourceStorage, const std::string& destStorage, unsigned level)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();
        if (!sourceStorage.empty())
        {
            sql << "DELETE FROM t_debug WHERE source_se = :source AND dest_se IS NULL",
                soci::use(sourceStorage);
            sql << "INSERT INTO t_debug (source_se, debug_level) VALUES (:source, :level)",
                soci::use(sourceStorage), soci::use(level);
        }
        if (!destStorage.empty())
        {
            sql << "DELETE FROM t_debug WHERE source_se IS NULL AND dest_se = :dest",
                soci::use(destStorage);
            sql << "INSERT INTO t_debug (dest_se, debug_level) VALUES (:dest, :level)",
                soci::use(destStorage), soci::use(level);
        }
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


void MySqlAPI::auditConfiguration(const std::string & dn, const std::string & config, const std::string & action)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();
        sql << "INSERT INTO t_config_audit (datetime, dn, config, action ) VALUES "
            "                           (UTC_TIMESTAMP(), :dn, :config, :action)",
            soci::use(dn), soci::use(config), soci::use(action);
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


void MySqlAPI::blacklistSe(const std::string& storage, const std::string& voName,
    const std::string& status, int timeout,
    const std::string& msg, const std::string& adminDn)
{
    soci::session sql(*connectionPool);

    try
    {
        int count = 0;

        sql <<
            " SELECT COUNT(*) FROM t_bad_ses WHERE se = :se ",
            soci::use(storage),
            soci::into(count)
            ;

        sql.begin();

        if (count)
        {
            sql << " UPDATE t_bad_ses SET "
                "   addition_time = UTC_TIMESTAMP(), "
                "   admin_dn = :admin, "
                "   vo = :vo, "
                "   status = :status, "
                "   wait_timeout = :timeout "
                " WHERE se = :se ",
                soci::use(adminDn),
                soci::use(voName),
                soci::use(status),
                soci::use(timeout),
                soci::use(storage)
                ;
        }
        else
        {

            sql << "INSERT INTO t_bad_ses (se, message, addition_time, admin_dn, vo, status, wait_timeout) "
                "               VALUES (:se, :message, UTC_TIMESTAMP(), :admin, :vo, :status, :timeout)",
                soci::use(storage), soci::use(msg), soci::use(adminDn), soci::use(voName), soci::use(status), soci::use(timeout);
        }

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


void MySqlAPI::blacklistDn(const std::string& userDn, const std::string& msg,
    const std::string& adminDn)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();
        sql << "INSERT IGNORE INTO t_bad_dns (dn, message, addition_time, admin_dn) "
            "               VALUES (:dn, :message, UTC_TIMESTAMP(), :admin)",
            soci::use(userDn), soci::use(msg), soci::use(adminDn);
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


void MySqlAPI::unblacklistSe(const std::string& storage)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        // delete the entry from DB
        sql << "DELETE FROM t_bad_ses WHERE se = :se",
            soci::use(storage);

        // set to null pending fields in respective files
        sql <<
            " UPDATE t_file f SET f.wait_timestamp = NULL, f.wait_timeout = NULL "
                " WHERE (f.source_se = :src OR f.dest_se = :dest) "
                "   AND f.file_state IN ('ACTIVE','SUBMITTED') and f.job_finished is NULL "
                "   AND NOT EXISTS ( "
                "       SELECT NULL "
                "       FROM t_bad_ses "
                "       WHERE (se = f.source_se OR se = f.dest_se)  AND (status = 'WAIT' OR status = 'WAIT_AS')"
                "   )",
            soci::use(storage),
            soci::use(storage);

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


void MySqlAPI::unblacklistDn(const std::string& userDn)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        // delete the entry from DB
        sql << "DELETE FROM t_bad_dns WHERE dn = :dn",
            soci::use(userDn);


        // set to null pending fields in respective files
        sql <<
            " UPDATE t_file f, t_job j SET f.wait_timestamp = NULL, f.wait_timeout = NULL "
                " WHERE f.job_id = j.job_id AND j.user_dn = :dn "
                "   AND f.file_state IN ('ACTIVE','SUBMITTED') and f.job_finished is NULL "
                "   AND NOT EXISTS ( "
                "       SELECT NULL "
                "       FROM t_bad_dns "
                "       WHERE dn = j.user_dn AND (status = 'WAIT' OR status = 'WAIT_AS') "
                "   )",
            soci::use(userDn);

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


bool MySqlAPI::allowSubmit(const std::string& storage, const std::string& voName)
{
    soci::session sql(*connectionPool);

    try
    {
        int count;
        sql << "SELECT COUNT(*) FROM t_bad_ses "
            "WHERE se = :se AND status != 'WAIT_AS' AND (vo IS NULL OR vo='' OR vo = :vo)",
            soci::use(storage), soci::use(voName), soci::into(count);
        return count == 0;
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


boost::optional<int> MySqlAPI::getTimeoutForSe(const std::string& storage)
{
    soci::session sql(*connectionPool);

    boost::optional<int> ret;

    try
    {
        soci::indicator isNull = soci::i_ok;
        int tmp = 0;

        sql <<
            " SELECT wait_timeout FROM t_bad_ses WHERE se = :se ",
            soci::use(storage),
            soci::into(tmp, isNull)
            ;

        if (sql.got_data())
        {
            if (isNull == soci::i_ok) ret = tmp;
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

    return ret;
}


bool MySqlAPI::isDnBlacklisted(const std::string& userDn)
{
    soci::session sql(*connectionPool);

    bool blacklisted = false;
    try
    {
        sql << "SELECT dn FROM t_bad_dns WHERE dn = :dn", soci::use(userDn);
        blacklisted = sql.got_data();
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return blacklisted;
}


/*
    we need to check if a member already belongs to another group
    true: it is member of another group
    false: it is not member of another group
 */
bool MySqlAPI::checkIfSeIsMemberOfAnotherGroup(const std::string & member)
{
    soci::session sql(*connectionPool);

    bool isMember = false;
    try
    {
        std::string group;
        sql << "SELECT groupName FROM t_group_members WHERE member = :member",
            soci::use(member), soci::into(group);

        isMember = sql.got_data();
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return isMember;
}


bool MySqlAPI::checkGroupExists(const std::string & groupName)
{
    soci::session sql(*connectionPool);

    bool exists = false;
    try
    {
        std::string grp;
        sql << "SELECT groupName FROM t_group_members WHERE groupName = :group",
            soci::use(groupName), soci::into(grp);

        exists = sql.got_data();
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return exists;
}


void MySqlAPI::getGroupMembers(const std::string & groupName, std::vector<std::string>& groupMembers)
{
    soci::session sql(*connectionPool);

    try
    {
        soci::rowset<std::string> rs = (sql.prepare << "SELECT member FROM t_group_members "
            "WHERE groupName = :group",
            soci::use(groupName));
        for (soci::rowset<std::string>::const_iterator i = rs.begin();
             i != rs.end(); ++i)
            groupMembers.push_back(*i);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


std::string MySqlAPI::getGroupForSe(const std::string se)
{
    soci::session sql(*connectionPool);

    std::string group;
    try
    {
        sql << "SELECT groupName FROM t_group_members "
            "WHERE member = :member",
            soci::use(se), soci::into(group);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return group;
}


void MySqlAPI::addMemberToGroup(const std::string & groupName,
    const std::vector<std::string>& groupMembers)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        std::string member;
        soci::statement stmt = (sql.prepare << "INSERT INTO t_group_members(member, groupName) "
            "                    VALUES (:member, :group)",
            soci::use(member), soci::use(groupName));
        for (std::vector<std::string>::const_iterator i = groupMembers.begin();
             i != groupMembers.end(); ++i)
        {
            member = *i;
            stmt.execute(true);
        }


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


void MySqlAPI::deleteMembersFromGroup(const std::string & groupName,
    const std::vector<std::string>& groupMembers)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        std::string member;
        soci::statement stmt = (sql.prepare << "DELETE FROM t_group_members "
            "WHERE groupName = :group AND member = :member",
            soci::use(groupName), soci::use(member));
        for (std::vector<std::string>::const_iterator i = groupMembers.begin();
             i != groupMembers.end(); ++i)
        {
            member = *i;
            stmt.execute(true);
        }
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


void MySqlAPI::addLinkConfig(const LinkConfig& cfg)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        sql << "INSERT INTO t_link_config (source, destination, state, symbolicName, "
            "                          nostreams, tcp_buffer_size, urlcopy_tx_to, no_tx_activity_to, auto_tuning)"
            "                  VALUES (:src, :dest, :state, :sname, :nstreams, :tcp, :txto, :txactivity, :auto_tuning)",
            soci::use(cfg.source), soci::use(cfg.destination),
            soci::use(cfg.state), soci::use(cfg.symbolicName),
            soci::use(cfg.numberOfStreams), soci::use(cfg.tcpBufferSize),
            soci::use(cfg.transferTimeout), soci::use(cfg.transferTimeout),
            soci::use(cfg.autoTuning);


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


void MySqlAPI::updateLinkConfig(const LinkConfig& cfg)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        sql << "UPDATE t_link_config SET "
            "  state = :state, symbolicName = :sname, "
            "  nostreams = :nostreams, tcp_buffer_size = :tcp, "
            "  urlcopy_tx_to = :txto, no_tx_activity_to = 0, auto_tuning = :auto_tuning "
            "WHERE source = :source AND destination = :dest",
            soci::use(cfg.state), soci::use(cfg.symbolicName),
            soci::use(cfg.numberOfStreams), soci::use(cfg.tcpBufferSize),
            soci::use(cfg.transferTimeout),
            soci::use(cfg.autoTuning),
            soci::use(cfg.source), soci::use(cfg.destination);

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


void MySqlAPI::deleteLinkConfig(std::string source, std::string destination)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        sql << "DELETE FROM t_link_config WHERE source = :source AND destination = :destination",
            soci::use(source), soci::use(destination);

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


std::unique_ptr<LinkConfig> MySqlAPI::getLinkConfig(std::string source, std::string destination)
{
    soci::session sql(*connectionPool);

    try
    {
        std::unique_ptr<LinkConfig> config(new LinkConfig);

        sql << "SELECT * FROM t_link_config WHERE source = :source AND destination = :dest",
            soci::use(source), soci::use(destination),
            soci::into(*config);

        if (sql.got_data())
            return config;
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return std::unique_ptr<LinkConfig>();
}


std::unique_ptr<std::pair<std::string, std::string>> MySqlAPI::getSourceAndDestination(std::string symbolic_name)
{
    soci::session sql(*connectionPool);

    try
    {
        std::string source, destination;
        sql << "SELECT source, destination FROM t_link_config WHERE symbolicName = :sname",
            soci::use(symbolic_name), soci::into(source), soci::into(destination);
        if (sql.got_data()) {
            return std::unique_ptr<std::pair<std::string, std::string>>(new std::pair<std::string, std::string>(source, destination));
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
    return std::unique_ptr<std::pair<std::string, std::string>>();
}


bool MySqlAPI::isGrInPair(std::string group)
{
    soci::session sql(*connectionPool);

    bool inPair = false;
    try
    {
        sql << "SELECT symbolicName FROM t_link_config WHERE "
            "  ((source = :group AND destination <> '*') OR "
            "  (source <> '*' AND destination = :group))",
            soci::use(group, "group");
        inPair = sql.got_data();
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return inPair;
}


void MySqlAPI::addShareConfig(const ShareConfig& cfg)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        sql << "INSERT IGNORE INTO t_share_config (source, destination, vo, active) "
            "                    VALUES (:source, :destination, :vo, :active)",
            soci::use(cfg.source), soci::use(cfg.destination), soci::use(cfg.vo),
            soci::use(cfg.activeTransfers);

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


void MySqlAPI::updateShareConfig(const ShareConfig& cfg)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        sql << "UPDATE t_share_config SET "
            "  active = :active "
            "WHERE source = :source AND destination = :dest AND vo = :vo",
            soci::use(cfg.activeTransfers),
            soci::use(cfg.source), soci::use(cfg.destination), soci::use(cfg.vo);

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


void MySqlAPI::deleteShareConfig(std::string source, std::string destination, std::string vo)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        sql << "DELETE FROM t_share_config WHERE source = :source AND destination = :dest AND vo = :vo",
            soci::use(source), soci::use(destination), soci::use(vo);

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


void MySqlAPI::deleteShareConfig(std::string source, std::string destination)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        sql << "DELETE FROM t_share_config WHERE source = :source AND destination = :dest",
            soci::use(destination), soci::use(source);

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


std::unique_ptr<ShareConfig> MySqlAPI::getShareConfig(std::string source, std::string destination, std::string vo)
{
    soci::session sql(*connectionPool);

    try
    {
        std::unique_ptr<ShareConfig> config(new ShareConfig);
        sql << "SELECT * FROM t_share_config WHERE "
            "  source = :source AND destination = :dest AND vo = :vo",
            soci::use(source), soci::use(destination), soci::use(vo),
            soci::into(*config);
        if (sql.got_data())
            return config;
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return std::unique_ptr<ShareConfig>();
}


std::vector<ShareConfig> MySqlAPI::getShareConfig(std::string source, std::string destination)
{
    soci::session sql(*connectionPool);

    std::vector<ShareConfig> cfg;
    try
    {
        soci::rowset<ShareConfig> rs = (sql.prepare << "SELECT * FROM t_share_config WHERE "
            "  source = :source AND destination = :dest",
            soci::use(source), soci::use(destination));
        for (soci::rowset<ShareConfig>::const_iterator i = rs.begin();
             i != rs.end(); ++i)
        {
            cfg.push_back(*i);
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
    return cfg;
}


void MySqlAPI::addActivityConfig(std::string vo, std::string shares, bool active)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        const std::string act = active ? "on" : "off";

        sql << "INSERT INTO t_activity_share_config (vo, activity_share, active) "
            "                    VALUES (:vo, :share, :active)",
            soci::use(vo),
            soci::use(shares),
            soci::use(act)
            ;

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


void MySqlAPI::updateActivityConfig(std::string vo, std::string shares, bool active)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        const std::string act = active ? "on" : "off";

        sql <<
            " UPDATE t_activity_share_config "
                " SET activity_share = :share, active = :active "
                " WHERE vo = :vo",
            soci::use(shares),
            soci::use(act),
            soci::use(vo)
            ;

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


void MySqlAPI::deleteActivityConfig(std::string vo)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        sql << "DELETE FROM t_activity_share_config WHERE vo = :vo ",
            soci::use(vo);

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


bool MySqlAPI::isActivityConfigActive(std::string vo)
{
    soci::session sql(*connectionPool);

    std::string active;
    soci::indicator isNull = soci::i_ok;

    try
    {
        sql << "SELECT active FROM t_activity_share_config WHERE vo = :vo ",
            soci::use(vo),
            soci::into(active, isNull)
            ;
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return active == "on";
}


std::map< std::string, double > MySqlAPI::getActivityConfig(std::string vo)
{
    soci::session sql(*connectionPool);
    try
    {
        return getActivityShareConf(sql, vo);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


void MySqlAPI::setPriority(std::string job_id, int priority)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        sql << "UPDATE t_job SET "
            "  priority = :priority "
            "WHERE job_id = :jobId",
            soci::use(priority), soci::use(job_id);

        sql.commit();
    }
    catch (std::exception& e)
    {
        sql.rollback();
        try
        {
            sleep(TIME_TO_SLEEP_BETWEEN_TRANSACTION_RETRIES);
            sql.begin();

            sql << "UPDATE t_job SET "
                "  priority = :priority "
                "WHERE job_id = :jobId",
                soci::use(priority), soci::use(job_id);

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
    catch (...)
    {
        try
        {
            sleep(TIME_TO_SLEEP_BETWEEN_TRANSACTION_RETRIES);
            sql.begin();

            sql << "UPDATE t_job SET "
                "  priority = :priority "
                "WHERE job_id = :jobId",
                soci::use(priority), soci::use(job_id);

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
}


void MySqlAPI::setSeProtocol(std::string protocol, std::string se, std::string state)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        int count = 0;

        sql <<
            " SELECT count(auto_number) "
                " FROM t_optimize "
                " WHERE source_se = :se ",
            soci::use(se), soci::into(count)
            ;

        if (count)
        {
            sql <<
                " UPDATE t_optimize "
                    " SET " << protocol << " = :state "
                    " WHERE source_se = :se ",
                soci::use(state),
                soci::use(se)
                ;
        }
        else
        {
            sql <<
                " INSERT INTO t_optimize (source_se, " << protocol << ", file_id) "
                    " VALUES (:se, :state, 0)",
                soci::use(se),
                soci::use(state)
                ;

        }

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


void MySqlAPI::setRetry(int retry, const std::string & vo_name)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        int count = 0;

        sql <<
            " SELECT count(vo_name) "
                " FROM t_server_config "
                " WHERE vo_name = :vo_name ",
            soci::use(vo_name), soci::into(count)
            ;

        if (count)
        {
            sql <<
                " UPDATE t_server_config "
                    " SET retry = :retry "
                    " WHERE vo_name = :vo_name ",
                soci::use(retry),
                soci::use(vo_name)
                ;
        }
        else
        {
            sql <<
                "INSERT INTO t_server_config (retry, vo_name) "
                    "VALUES(:retry, :vo_name)",
                soci::use(retry),
                soci::use(vo_name);
            ;

        }

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


void MySqlAPI::setShowUserDn(bool show)
{
    soci::session sql(*connectionPool);
    int count = 0;

    try
    {
        sql << "SELECT count(*) FROM t_server_config WHERE vo_name IS NULL or vo_name = '*'", soci::into(count);
        if (!count)
        {
            sql.begin();
            sql << "INSERT INTO t_server_config (show_user_dn, vo_name) VALUES (:show, '*')",
                soci::use(std::string(show ? "on" : "off"));
            sql.commit();

        }
        else
        {
            sql.begin();
            sql << "UPDATE t_server_config SET show_user_dn = :show WHERE show_user_dn IS NOT NULL",
                soci::use(std::string(show ? "on" : "off"));
            sql.commit();
        }
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


int MySqlAPI::getRetry(const std::string & jobId)
{
    soci::session sql(*connectionPool);

    int nRetries = 0;
    soci::indicator isNull = soci::i_ok;
    std::string vo_name;

    try
    {
        sql <<
            " SELECT retry, vo_name "
                " FROM t_job "
                " WHERE job_id = :jobId ",
            soci::use(jobId),
            soci::into(nRetries, isNull),
            soci::into(vo_name)
            ;

        if (isNull == soci::i_null || nRetries == 0)
        {
            sql <<
                " SELECT retry "
                    " FROM t_server_config where vo_name=:vo_name LIMIT 1",
                soci::use(vo_name), soci::into(nRetries)
                ;
        }
        else if (nRetries <= 0)
        {
            nRetries = -1;
        }


        //do not retry multiple replica jobs
        if(nRetries > 0)
        {
            std::string mreplica;
            sql << "select reuse_job from t_job where job_id=:job_id", soci::use(jobId), soci::into(mreplica);
            if(mreplica == "R" || mreplica == "H")
                nRetries = 0;
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
    return nRetries;
}


int MySqlAPI::getRetryTimes(const std::string & jobId, int fileId)
{
    soci::session sql(*connectionPool);

    int nRetries = 0;
    soci::indicator isNull = soci::i_ok;

    try
    {
        sql << "SELECT retry FROM t_file WHERE file_id = :fileId AND job_id = :jobId ",
            soci::use(fileId), soci::use(jobId), soci::into(nRetries, isNull);

        if(isNull == soci::i_null)
            return 0;
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return nRetries;
}


int MySqlAPI::getMaxTimeInQueue()
{
    soci::session sql(*connectionPool);

    int maxTime = 0;
    try
    {
        soci::indicator isNull = soci::i_ok;

        sql << "SELECT max_time_queue FROM t_server_config WHERE vo_name IS NULL or vo_name = '*' LIMIT 1",
            soci::into(maxTime, isNull);

        //just in case soci it is reseting the value to NULL
        if(isNull != soci::i_null && maxTime > 0)
            return maxTime;
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return maxTime;
}


void MySqlAPI::setMaxTimeInQueue(int afterXHours)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        sql << "UPDATE t_server_config SET max_time_queue = :maxTime",
            soci::use(afterXHours);

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


void MySqlAPI::setGlobalLimits(const int* maxActivePerLink, const int* maxActivePerSe)
{
    soci::session sql(*connectionPool);
    int existsLink = 0;
    int existsSe = 0;

    try
    {
        sql << "SELECT max_per_link FROM t_server_config WHERE (vo_name IS NULL or vo_name = '*') and max_per_link is not NULL", soci::into(existsLink);

        sql.begin();

        if (maxActivePerLink)
        {
            if(sql.got_data())
            {
                sql << "UPDATE t_server_config SET max_per_link = :maxLink where max_per_link=:existsLink",
                    soci::use(*maxActivePerLink), soci::use(existsLink);
            }
            else
            {
                sql << "INSERT into t_server_config(max_per_link, vo_name) VALUES(:maxLink, '*')",
                    soci::use(*maxActivePerLink);
            }
        }


        sql << "SELECT max_per_se FROM t_server_config WHERE (vo_name IS NULL or vo_name = '*') and max_per_se is not NULL", soci::into(existsSe);

        if (maxActivePerSe)
        {
            if(sql.got_data())
            {
                sql << "UPDATE t_server_config SET max_per_se = :maxSe where max_per_se=:existsSe",
                    soci::use(*maxActivePerSe), soci::use(existsSe);
            }
            else
            {
                sql << "INSERT into t_server_config(max_per_se, vo_name) VALUES(:maxSe, '*')",
                    soci::use(*maxActivePerSe);
            }
        }

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


void MySqlAPI::authorize(bool add, const std::string& op, const std::string& dn)
{
    soci::session sql(*connectionPool);
    try
    {
        if (add)
        {
            sql << "INSERT IGNORE INTO t_authz_dn (operation, dn) VALUES (:op, :dn)",
                soci::use(op), soci::use(dn);
        }
        else
        {
            sql << "DELETE FROM t_authz_dn WHERE operation = :op AND dn = :dn",
                soci::use(op), soci::use(dn);
        }
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


std::vector< std::pair<std::string, std::string> > MySqlAPI::getPairsForSe(std::string se)
{
    soci::session sql(*connectionPool);

    std::vector< std::pair<std::string, std::string> > ret;

    try
    {
        soci::rowset<soci::row> rs = (
            sql.prepare <<
                        " select source, destination "
                            " from t_link_config "
                            " where (source = :source and destination <> '*') "
                            "  or (source <> '*' and destination = :dest) ",
                soci::use(se),
                soci::use(se)
        );

        for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
        {
            ret.push_back(
                make_pair(i->get<std::string>("source"), i->get<std::string>("destination"))
            );
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

    return ret;
}


std::vector<std::string> MySqlAPI::getAllStandAlloneCfgs()
{

    soci::session sql(*connectionPool);

    std::vector<std::string> ret;

    try
    {
        soci::rowset<std::string> rs = (
            sql.prepare <<
                        " select source "
                            " from t_link_config "
                            " where destination = '*' and auto_tuning <> 'all' "
        );

        for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
        {
            ret.push_back(*i);
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

    return ret;
}


std::vector< std::pair<std::string, std::string> > MySqlAPI::getAllPairCfgs()
{

    soci::session sql(*connectionPool);

    std::vector< std::pair<std::string, std::string> > ret;

    try
    {
        soci::rowset<soci::row> rs = (sql.prepare << "select source, destination from t_link_config where source <> '*' and destination <> '*'");

        for (soci::rowset<soci::row>::const_iterator i = rs.begin(); i != rs.end(); ++i)
        {
            soci::row const& row = *i;
            ret.push_back(
                std::make_pair(row.get<std::string>("source"), row.get<std::string>("destination"))
            );
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

    return ret;
}


void MySqlAPI::setMaxStageOp(const std::string& se, const std::string& vo, int val, const std::string & opt)
{
    soci::session sql(*connectionPool);

    try
    {
        int exist = 0;

        sql <<
            " SELECT COUNT(*) "
                " FROM t_stage_req "
                " WHERE vo_name = :vo AND host = :se AND operation = :opt ",
            soci::use(vo),
            soci::use(se),
            soci::use(opt),
            soci::into(exist)
            ;

        sql.begin();

        // if the record already exist ...
        if (exist)
        {
            // update
            sql <<
                " UPDATE t_stage_req "
                    " SET concurrent_ops = :value "
                    " WHERE vo_name = :vo AND host = :se AND operation = :opt ",
                soci::use(val),
                soci::use(vo),
                soci::use(se),
                soci::use(opt)
                ;
        }
        else
        {
            // otherwise insert
            sql <<
                " INSERT "
                    " INTO t_stage_req (host, vo_name, concurrent_ops, operation) "
                    " VALUES (:se, :vo, :value, :opt)",
                soci::use(se),
                soci::use(vo),
                soci::use(val),
                soci::use(opt)
                ;
        }

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

bool MySqlAPI::isShareOnly(std::string se)
{

    soci::session sql(*connectionPool);

    bool ret = true;
    try
    {
        sql <<
            " select symbolicName from t_link_config "
                " where source = :source and destination = '*' and auto_tuning = 'all'",
            soci::use(se)
            ;

        ret = sql.got_data();

    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return ret;
}


std::vector<std::string> MySqlAPI::getAllShareOnlyCfgs()
{

    soci::session sql(*connectionPool);

    std::vector<std::string> ret;

    try
    {
        soci::rowset<std::string> rs = (
            sql.prepare <<
                        " select source "
                            " from t_link_config "
                            " where destination = '*' and auto_tuning = 'all' "
        );

        for (soci::rowset<std::string>::const_iterator i = rs.begin(); i != rs.end(); ++i)
        {
            ret.push_back(*i);
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

    return ret;
}


bool MySqlAPI::hasStandAloneSeCfgAssigned(int file_id, std::string vo)
{
    soci::session sql(*connectionPool);

    int count = 0;

    try
    {
        sql <<
            " select count(*) "
                " from t_file_share_config fc "
                " where fc.file_id = :id "
                "   and fc.vo = :vo "
                "   and ((fc.source <> '(*)' and fc.destination = '*') or (fc.source = '*' and fc.destination <> '(*)')) "
                "   and not exists ( "
                "       select null "
                "       from t_group_members g "
                "       where (g.groupName = fc.source or g.groupName = fc.destination) "
                "   ) ",
            soci::use(file_id),
            soci::use(vo),
            soci::into(count)
            ;
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return count > 0;
}


bool MySqlAPI::hasPairSeCfgAssigned(int file_id, std::string vo)
{
    soci::session sql(*connectionPool);

    int count = 0;

    try
    {
        sql <<
            " select count(*) "
                " from t_file_share_config fc "
                " where fc.file_id = :id "
                "   and fc.vo = :vo "
                "   and (fc.source <> '(*)' and fc.source <> '*' and fc.destination <> '*' and fc.destination <> '(*)') "
                "   and not exists ( "
                "       select null "
                "       from t_group_members g "
                "       where (g.groupName = fc.source or g.groupName = fc.destination) "
                "   ) ",
            soci::use(file_id),
            soci::use(vo),
            soci::into(count)
            ;
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return count > 0;
}


bool MySqlAPI::hasPairGrCfgAssigned(int file_id, std::string vo)
{
    soci::session sql(*connectionPool);

    int count = 0;

    try
    {
        sql <<
            " select count(*) "
                " from t_file_share_config fc "
                " where fc.file_id = :id "
                "   and fc.vo = :vo "
                "   and (fc.source <> '(*)' and fc.source <> '*' and fc.destination <> '*' and fc.destination <> '(*)') "
                "   and exists ( "
                "       select null "
                "       from t_group_members g "
                "       where (g.groupName = fc.source or g.groupName = fc.destination) "
                "   ) ",
            soci::use(file_id),
            soci::use(vo),
            soci::into(count)
            ;
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return count > 0;
}


void MySqlAPI::setOptimizerMode(int mode)
{

    soci::session sql(*connectionPool);
    int _mode = 0;

    try
    {
        sql << "select count(*) from t_optimize_mode", soci::into(_mode);
        if (_mode == 0)
        {
            sql.begin();

            sql << "INSERT INTO t_optimize_mode (mode_opt) VALUES (:mode) ON DUPLICATE KEY UPDATE mode_opt=:mode",
                soci::use(mode),
                soci::use(mode)
                ;

            sql.commit();

        }
        else
        {
            sql.begin();

            sql << "update t_optimize_mode set mode_opt = :mode",
                soci::use(mode)
                ;

            sql.commit();
        }
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


int getOptimizerMode(soci::session &sql)
{
    try {
        soci::indicator ind = soci::i_ok;
        int mode = 0;

        sql <<
            " select mode_opt "
                " from t_optimize_mode LIMIT 1",
            soci::into(mode, ind);

        if (ind == soci::i_ok) {
            if (mode <= 0)
                return 1;
            else
                return mode;
        }
        return 1;
    }
    catch (std::exception &e) {
        throw UserError(std::string(__func__) + ": Caught mode exception " + e.what());
    }
    catch (...) {
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


bool MySqlAPI::getDrain()
{
    soci::session sql(*connectionPool);

    try
    {
        return getDrainInternal(sql);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
}


bool MySqlAPI::getDrainInternal(soci::session& sql)
{

    int drain = 0;

    try
    {
        soci::indicator isNull = soci::i_ok;

        sql << "SELECT drain FROM t_hosts WHERE hostname = :hostname",soci::use(hostname), soci::into(drain, isNull);

        if(isNull == soci::i_null || drain == 0)
            return false;

        return true;
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return true;
}


void MySqlAPI::setDrain(bool drain)
{

    soci::session sql(*connectionPool);
    unsigned index=0, count1=0, start=0, end=0;
    std::string service_name = "fts_server";

    try
    {
        if(drain == true)
        {
            sql.begin();
            sql << " update t_hosts set drain=1 where hostname = :hostname ",soci::use(hostname);
            sql.commit();
        }
        else
        {
            //update heartbeat first to avoid overlapping of hash range when moving out of draining mode
            updateHeartBeatInternal(sql, &index, &count1, &start, &end, service_name);
            sleep(2);

            sql.begin();
            sql << " update t_hosts set drain=0 where hostname = :hostname ",soci::use(hostname);
            sql.commit();
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


std::string MySqlAPI::getBandwidthLimit()
{
    soci::session sql(*connectionPool);

    std::string result;

    try
    {
        std::string source_hostname;
        std::string destination_hostname;
        result = getBandwidthLimitInternal(sql, source_hostname, destination_hostname);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    return result;
}


std::string MySqlAPI::getBandwidthLimitInternal(soci::session& sql, const std::string & source_hostname, const std::string & destination_hostname)
{

    std::ostringstream result;

    try
    {
        if(!source_hostname.empty())
        {
            double bandwidth = 0;
            soci::indicator isNullBandwidth = soci::i_ok;
            sql << " select throughput from t_optimize where source_se = :source_se and throughput is not NULL ",
                soci::use(source_hostname), soci::into(bandwidth, isNullBandwidth);

            if(sql.got_data() && bandwidth > 0)
                result << "Source endpoint: " << source_hostname << "   Bandwidth restriction: " << bandwidth << " MB/s\n";
        }
        else if (!destination_hostname.empty())
        {
            double bandwidth = 0;
            soci::indicator isNullBandwidth = soci::i_ok;
            sql << " select throughput from t_optimize where dest_se = :dest_se  and throughput is not NULL ",
                soci::use(destination_hostname), soci::into(bandwidth, isNullBandwidth);

            if(sql.got_data() && bandwidth > 0)
                result << "Destination endpoint: " << destination_hostname << "   Bandwidth restriction: " << bandwidth << " MB/s\n";
        }
        else
        {
            soci::rowset<soci::row> rs = (
                sql.prepare <<
                            " SELECT source_se, dest_se, throughput from t_optimize "
                                " WHERE  "
                                " throughput is not NULL "
            );

            soci::rowset<soci::row>::const_iterator it;
            for (it = rs.begin(); it != rs.end(); ++it)
            {
                std::string source_se = it->get<std::string>("source_se","");
                std::string dest_se = it->get<std::string>("dest_se","");
                double bandwidth = it->get<double>("throughput");

                if(!source_se.empty() && bandwidth > 0)
                {
                    result << "Source endpoint: " << source_se << "   Bandwidth restriction: " << bandwidth << " MB/s\n";
                }
                if(!dest_se.empty() && bandwidth > 0)
                {
                    result << "Destination endpoint: " << dest_se   << "   Bandwidth restriction: " << bandwidth << " MB/s\n";
                }
            }
        }
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    return result.str();
}


void MySqlAPI::setBandwidthLimit(const std::string & source_hostname, const std::string & destination_hostname, int bandwidthLimit)
{

    soci::session sql(*connectionPool);

    try
    {
        long long int countSource = 0;
        long long int countDest = 0;

        if(!source_hostname.empty())
        {
            sql << "select count(*) from t_optimize where source_se=:source_se  and throughput is not NULL ",
                soci::use(source_hostname), soci::into(countSource);

            if(countSource == 0 && bandwidthLimit > 0)
            {
                sql.begin();

                sql << " insert into t_optimize(throughput, source_se) values(:throughput, :source_se) ",
                    soci::use(bandwidthLimit), soci::use(source_hostname);

                sql.commit();
            }
            else if (countSource > 0)
            {
                if(bandwidthLimit == -1)
                {
                    sql.begin();
                    sql << "update t_optimize set throughput=NULL where source_se=:source_se ",
                        soci::use(source_hostname);
                    sql.commit();
                }
                else
                {
                    sql.begin();
                    sql << "update t_optimize set throughput=:throughput where source_se=:source_se and throughput is not NULL ",
                        soci::use(bandwidthLimit), soci::use(source_hostname);
                    sql.commit();
                }
            }
        }

        if(!destination_hostname.empty())
        {
            sql << "select count(*) from t_optimize where dest_se=:dest_se and throughput is not NULL ",
                soci::use(destination_hostname), soci::into(countDest);

            if(countDest == 0 && bandwidthLimit > 0)
            {
                sql.begin();

                sql << " insert into t_optimize(throughput, dest_se) values(:throughput, :dest_se) ",
                    soci::use(bandwidthLimit), soci::use(destination_hostname);

                sql.commit();
            }
            else if (countDest > 0)
            {
                if(bandwidthLimit == -1)
                {
                    sql.begin();
                    sql << "update t_optimize set throughput=NULL where dest_se=:dest_se ",
                        soci::use(destination_hostname);
                    sql.commit();
                }
                else
                {
                    sql.begin();
                    sql << "update t_optimize set throughput=:throughput where dest_se=:dest_se  and throughput is not NULL ",
                        soci::use(bandwidthLimit), soci::use(destination_hostname);
                    sql.commit();
                }

            }
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


bool MySqlAPI::isProtocolUDT(const std::string & source_hostname, const std::string & destination_hostname)
{
    soci::session sql(*connectionPool);

    try
    {
        soci::indicator isNullUDT = soci::i_ok;
        std::string udt;

        sql << " select udt from t_optimize where (source_se = :source_se OR source_se = :dest_se) and udt is not NULL ",
            soci::use(source_hostname), soci::use(destination_hostname), soci::into(udt, isNullUDT);

        if(sql.got_data() && udt == "on")
            return true;

    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    return false;
}


bool MySqlAPI::isProtocolIPv6(const std::string & source_hostname, const std::string & destination_hostname)
{
    soci::session sql(*connectionPool);

    try
    {
        soci::indicator isNullUDT = soci::i_ok;
        std::string ipv6;

        sql << " select ipv6 from t_optimize where (source_se = :source_se OR source_se = :dest_se) and ipv6 is not NULL ",
            soci::use(source_hostname), soci::use(destination_hostname), soci::into(ipv6, isNullUDT);

        if(sql.got_data() && ipv6 == "on")
            return true;

    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    return false;
}


int MySqlAPI::getStreamsOptimizationInternal(soci::session &sql, const std::string &sourceSe,
    const std::string &destSe)
{
    try
    {
        int globalConfig = 0;
        sql << "SELECT global_tcp_stream "
            " FROM t_server_config "
            " WHERE (vo_name IS NULL OR vo_name = '*') AND "
            "    global_tcp_stream > 0",
            soci::into(globalConfig);
        if(sql.got_data() && globalConfig > 0) {
            return globalConfig;
        }

        int streams = 0;
        soci::indicator isNullStreams = soci::i_ok;
        sql << " SELECT nostreams FROM t_optimize_streams "
            " WHERE source_se = :source_se AND dest_se = :dest_se",
            soci::use(sourceSe), soci::use(destSe),
            soci::into(streams, isNullStreams);

        if (isNullStreams == soci::i_ok) {
            return streams;
        }
        else {
            return DEFAULT_NOSTREAMS;
        }
    }
    catch (std::exception& e)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        sql.rollback();
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


int MySqlAPI::getStreamsOptimization(const std::string & source_hostname, const std::string & destination_hostname)
{
    soci::session sql(*connectionPool);

    try
    {
        return getStreamsOptimizationInternal(sql, source_hostname, destination_hostname);
    }
    catch (std::exception& e)
    {
        try
        {
            sleep(1);
            return getStreamsOptimizationInternal(sql, source_hostname, destination_hostname);
        }
        catch (std::exception& e)
        {
            throw UserError(std::string(__func__) + ": Caught exception " + e.what());
        }
        catch (...)
        {
            throw UserError(std::string(__func__) + ": Caught exception ");
        }
    }
    catch (...)
    {
        try
        {
            sleep(1);
            return getStreamsOptimizationInternal(sql, source_hostname, destination_hostname);
        }
        catch (std::exception& e)
        {
            throw UserError(std::string(__func__) + ": Caught exception " + e.what());
        }
        catch (...)
        {
            throw UserError(std::string(__func__) + ": Caught exception ");
        }
    }

}


int MySqlAPI::getGlobalTimeout()
{
    soci::session sql(*connectionPool);
    int timeout = 0;

    try
    {
        soci::indicator isNullTimeout = soci::i_ok;

        sql << "SELECT global_timeout FROM t_server_config WHERE vo_name IS NULL OR vo_name = '*'", soci::into(timeout, isNullTimeout);

        if(sql.got_data() && timeout > 0)
        {
            return timeout;
        }
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    return timeout;

}


void MySqlAPI::setGlobalTimeout(int timeout)
{
    soci::session sql(*connectionPool);
    int timeoutLocal = 0;
    soci::indicator isNullTimeout = soci::i_ok;

    try
    {
        sql << "SELECT global_timeout FROM t_server_config WHERE vo_name IS NULL or vo_name = '*'", soci::into(timeoutLocal, isNullTimeout);
        if (!sql.got_data())
        {
            sql.begin();

            sql << "INSERT INTO t_server_config (global_timeout, vo_name) VALUES (:timeout, '*') ",
                soci::use(timeout);

            sql.commit();

        }
        else
        {
            sql.begin();

            sql << "UPDATE t_server_config SET global_timeout = :timeout",
                soci::use(timeout);

            sql.commit();
        }
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


int MySqlAPI::getSecPerMb()
{
    soci::session sql(*connectionPool);
    int seconds = 0;

    try
    {
        soci::indicator isNullSeconds = soci::i_ok;

        sql << "SELECT sec_per_mb FROM t_server_config WHERE vo_name IS NULL OR vo_name = '*'", soci::into(seconds, isNullSeconds);

        if(sql.got_data() && seconds > 0)
        {
            return seconds;
        }
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception ");
    }

    return seconds;

}


void MySqlAPI::setSecPerMb(int seconds)
{
    soci::session sql(*connectionPool);
    int secondsLocal = 0;
    soci::indicator isNullSeconds = soci::i_ok;

    try
    {
        sql << "SELECT sec_per_mb FROM t_server_config WHERE vo_name IS NULL OR vo_name = '*'", soci::into(secondsLocal, isNullSeconds);
        if (!sql.got_data())
        {
            sql.begin();

            sql << "INSERT INTO t_server_config (sec_per_mb, vo_name) VALUES (:seconds, '*') ",
                soci::use(seconds);

            sql.commit();

        }
        else
        {
            sql.begin();

            sql << "UPDATE t_server_config SET sec_per_mb = :seconds",
                soci::use(seconds);

            sql.commit();
        }
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


void MySqlAPI::setSourceMaxActive(const std::string & source_hostname, int maxActive)
{
    soci::session sql(*connectionPool);
    std::string source_se;
    soci::indicator isNullSourceSe = soci::i_ok;

    try
    {
        sql << "select source_se from t_optimize where source_se = :source_se and active is not NULL", soci::use(source_hostname), soci::into(source_se, isNullSourceSe);

        if (!sql.got_data())
        {
            if (maxActive > 0)
            {
                sql.begin();

                sql << "INSERT INTO t_optimize (source_se, active, file_id) VALUES (:source_se, :active, 0)  ",
                    soci::use(source_hostname), soci::use(maxActive);

                sql.commit();
            }
        }
        else
        {
            sql.begin();

            if (maxActive == -1)
            {
                sql << "update t_optimize set active = NULL where source_se = :source_se ",
                    soci::use(source_hostname);
            }
            else
            {
                sql << "update t_optimize set active = :active where source_se = :source_se and active is not NULL ",
                    soci::use(maxActive), soci::use(source_hostname);
            }
            sql.commit();
        }
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


void MySqlAPI::setDestMaxActive(const std::string & destination_hostname, int maxActive)
{
    soci::session sql(*connectionPool);
    std::string dest_se;
    soci::indicator isNullDestSe = soci::i_ok;

    try
    {
        sql << "select dest_se from t_optimize where dest_se = :dest_se and active is not NULL", soci::use(destination_hostname), soci::into(dest_se, isNullDestSe);

        if (!sql.got_data())
        {
            if (maxActive > 0)
            {
                sql.begin();

                sql << "INSERT INTO t_optimize (dest_se, active, file_id) VALUES (:dest_se, :active, 0)  ",
                    soci::use(destination_hostname), soci::use(maxActive);

                sql.commit();
            }
        }
        else
        {
            sql.begin();

            if (maxActive == -1)
            {
                sql << "update t_optimize set active = NULL where dest_se = :dest_se ",
                    soci::use(destination_hostname);
            }
            else
            {
                sql << "update t_optimize set active = :active where dest_se = :dest_se  and active is not NULL ",
                    soci::use(maxActive), soci::use(destination_hostname);
            }
            sql.commit();
        }
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


void MySqlAPI::setFixActive(const std::string & source, const std::string & destination, int active)
{
    soci::session sql(*connectionPool);
    try
    {
        sql.begin();
        if (active > 0)
        {
            sql <<
                "INSERT INTO t_optimize_active (source_se, dest_se, active, fixed, ema, datetime,"
                    "    min_active, max_active) "
                    "VALUES (:source, :dest, :active, 'on', 0, UTC_TIMESTAMP(), :active, :active) "
                    "ON DUPLICATE KEY UPDATE "
                    "   active = :active, fixed = 'on', datetime = UTC_TIMESTAMP(), "
                    "   min_active = :active, max_active = :active",
                soci::use(source, "source"), soci::use(destination, "dest"), soci::use(active, "active");
        }
        else
        {
            sql << "INSERT INTO t_optimize_active (source_se, dest_se, active, fixed, ema, datetime) "
                "VALUES (:source, :dest, 2, 'off', 0, UTC_TIMESTAMP()) "
                "ON DUPLICATE KEY UPDATE "
                "  fixed = 'off', datetime = UTC_TIMESTAMP()",
                soci::use(source, "source"), soci::use(destination, "dest");
        }
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


int MySqlAPI::getBufferOptimization()
{
    soci::session sql(*connectionPool);

    try
    {
        return getOptimizerMode(sql);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " +  e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return 1; //default level
}


bool MySqlAPI::getCloudStorageCredentials(const std::string& user_dn,
    const std::string& vo, const std::string& cloud_name, CloudStorageAuth& auth)
{
    soci::session sql(*connectionPool);

    try
    {
        sql << "SELECT * "
            "FROM t_cloudStorage cs, t_cloudStorageUser cu "
            "WHERE (cu.user_dn=:user_dn OR cu.vo_name=:vo) AND cs.cloudStorage_name=:cs_name AND cs.cloudStorage_name = cu.cloudStorage_name",
            soci::use(user_dn), soci::use(vo), soci::use(cloud_name), soci::into(auth);
        if (!sql.got_data())
            return false;
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }
    return true;
}


void MySqlAPI::setCloudStorageCredentials(std::string const &dn, std::string const &vo, std::string const &storage,
    std::string const &accessKey, std::string const &secretKey)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        int count = 0;

        sql <<
            " SELECT count(*) "
                " FROM t_cloudStorageUser "
                " WHERE user_dn = :dn and vo_name = :vo and cloudStorage_name = :storage ",
            soci::use(dn),
            soci::use(vo),
            soci::use(storage),
            soci::into(count)
            ;

        if (count)
        {
            sql <<
                " UPDATE t_cloudStorageUser "
                    " SET access_token = :accessKey, access_token_secret = :secretKey "
                    " WHERE user_dn = :dn and vo_name = :vo and cloudStorage_name = :storage ",
                soci::use(accessKey),
                soci::use(secretKey),
                soci::use(dn),
                soci::use(vo),
                soci::use(storage)
                ;
        }
        else
        {
            // first make sure that the corresponding object in t_cloudStorage exists
            sql <<
                " INSERT INTO t_cloudStorage (cloudStorage_name) "
                    " SELECT * FROM (SELECT :storage) AS tmp "
                    " WHERE NOT EXISTS ( "
                    "   SELECT NULL FROM t_cloudStorage WHERE cloudStorage_name = :storage "
                    " ) "
                    " LIMIT 1",
                soci::use(storage),
                soci::use(storage)
                ;
            // then add the record
            sql <<
                "INSERT INTO t_cloudStorageUser (user_dn, vo_name, cloudStorage_name, access_token, access_token_secret) "
                    "VALUES(:dn, :vo, :storage, :accessKey, :secretKey)",
                soci::use(dn),
                soci::use(vo),
                soci::use(storage),
                soci::use(accessKey),
                soci::use(secretKey)
                ;

        }

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


void MySqlAPI::setCloudStorage(std::string const & storage, std::string const & appKey, std::string const & appSecret, std::string const & apiUrl)
{
    soci::session sql(*connectionPool);

    try
    {
        sql.begin();

        sql <<
            " INSERT INTO t_cloudStorage (cloudStorage_name, app_key, app_secret, service_api_url) "
                " VALUES (:storage, :appKey, :appSecret, :apiUrl) "
                " ON DUPLICATE KEY UPDATE "
                " app_key = :appKey, app_secret = :appSecret, service_api_url = :apiUrl",
            soci::use(storage),
            soci::use(appKey),
            soci::use(appSecret),
            soci::use(apiUrl),
            soci::use(appKey),
            soci::use(appSecret),
            soci::use(apiUrl)
            ;

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


bool MySqlAPI::getUserDnVisibleInternal(soci::session& sql)
{
    std::string show_user_dn;
    soci::indicator isNullShow = soci::i_ok;

    try
    {
        sql << "select show_user_dn from t_server_config", soci::into(show_user_dn, isNullShow);

        if (isNullShow == soci::i_null)
        {
            return true;
        }
        else if(show_user_dn == "on")
        {
            return true;
        }
        else if(show_user_dn == "off")
        {
            return false;
        }
        else
        {
            return true;
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

    return true;
}


bool MySqlAPI::getUserDnVisible()
{
    soci::session sql(*connectionPool);

    try
    {
        return getUserDnVisibleInternal(sql);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return true;
}
