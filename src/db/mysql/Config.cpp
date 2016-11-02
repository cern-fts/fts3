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


std::unique_ptr<LinkConfig> MySqlAPI::getLinkConfig(const std::string &source, const std::string &destination)
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


std::unique_ptr<ShareConfig> MySqlAPI::getShareConfig(const std::string &source, const std::string &destination,
    const std::string &vo)
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


std::vector<ShareConfig> MySqlAPI::getShareConfig(const std::string &source, const std::string &destination)
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
                " FROM t_server_config WHERE vo_name IN (:vo_name, '*', NULL) "
                " ORDER BY vo_name DESC LIMIT 1",
                soci::use(vo_name), soci::into(nRetries);
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


int MySqlAPI::getMaxTimeInQueue(const std::string &voName)
{
    soci::session sql(*connectionPool);

    int maxTime = 0;
    try
    {
        soci::indicator isNull = soci::i_ok;

        sql << "SELECT max_time_queue "
               " FROM t_server_config WHERE vo_name IN (:vo, '*', NULL) "
               " ORDER BY vo_name DESC LIMIT 1",
            soci::use(voName), soci::into(maxTime, isNull);

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


int MySqlAPI::getStreamsOptimization(const std::string &voName, const std::string &sourceSe, const std::string &destSe)
{
    soci::session sql(*connectionPool);

    try
    {
        int globalConfig = 0;
        sql << "SELECT global_tcp_stream "
               " FROM t_server_config "
               " WHERE vo_name IN (:vo, '*', NULL) "
               " ORDER BY vo_name DESC LIMIT 1",
            soci::use(voName), soci::into(globalConfig);
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


int MySqlAPI::getGlobalTimeout(const std::string &voName)
{
    soci::session sql(*connectionPool);

    try
    {
        int timeout = 0;
        soci::indicator isNullTimeout = soci::i_ok;

        sql << "SELECT global_timeout FROM t_server_config "
               "WHERE vo_name IN (:vo, '*', NULL) "
               "ORDER BY vo_name DESC LIMIT 1",
               soci::use(voName), soci::into(timeout, isNullTimeout);

        return timeout;
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


int MySqlAPI::getSecPerMb(const std::string &voName)
{
    soci::session sql(*connectionPool);

    try
    {
        int seconds = 0;
        soci::indicator isNullSeconds = soci::i_ok;

        sql << "SELECT sec_per_mb FROM t_server_config "
               "WHERE vo_name IN (:vo, '*', NULL) "
               "ORDER BY vo_name DESC LIMIT 1",
               soci::use(voName), soci::into(seconds, isNullSeconds);

        return seconds;
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


bool MySqlAPI::publishUserDnInternal(soci::session& sql, const std::string &vo)
{
    std::string publish;
    soci::indicator isNullPublish = soci::i_ok;

    try
    {
        sql << "SELECT show_user_dn FROM t_server_config WHERE vo_name = :vo",
          soci::use(vo), soci::into(publish, isNullPublish);

        if (isNullPublish == soci::i_null) {
            return false;
        }
        return publish == "on";
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


bool MySqlAPI::publishUserDn(const std::string &vo)
{
    soci::session sql(*connectionPool);

    try
    {
        return publishUserDnInternal(sql, vo);
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
