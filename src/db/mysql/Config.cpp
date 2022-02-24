/*
 * Copyright (c) CERN 2013-2017
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

#include <boost/logic/tribool.hpp>
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
        soci::indicator levelIndicator = soci::i_ok;

        sql << "SELECT MAX(debug_level) FROM t_se WHERE storage IN (:source, :destination)",
            soci::use(sourceStorage), soci::use(destStorage), soci::into(level, levelIndicator);

        if (levelIndicator == soci::i_ok) {
            return level;
        }

        sql << "SELECT debug_level FROM t_se WHERE storage = '*'", soci::into(level, levelIndicator);
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
}


std::unique_ptr<LinkConfig> MySqlAPI::getLinkConfig(const std::string &source, const std::string &destination)
{
    soci::session sql(*connectionPool);

    try
    {
        std::unique_ptr<LinkConfig> config(new LinkConfig);

        sql << "SELECT source_se, dest_se, min_active, max_active, optimizer_mode, tcp_buffer_size, nostreams "
               "FROM t_link_config WHERE source = :source AND destination = :dest",
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
                " FROM t_server_config WHERE vo_name IN (:vo_name, '*') OR vo_name IS NULL "
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
            Job::JobType mreplica;
            sql << "select job_type from t_job where job_id=:job_id", soci::use(jobId), soci::into(mreplica);
            if(mreplica == Job::kTypeMultipleReplica || mreplica == Job::kTypeMultiHop) {
                nRetries = 0;
            }
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


int MySqlAPI::getRetryTimes(const std::string & jobId, uint64_t fileId)
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
               " FROM t_server_config WHERE vo_name IN (:vo, '*') OR vo_name IS NULL "
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


OptimizerMode getOptimizerModeInner(soci::session &sql, const std::string &source, const std::string &dest)
{
    try {
        OptimizerMode mode = kOptimizerDisabled;

        sql <<
            "SELECT optimizer_mode FROM ("
            "   SELECT optimizer_mode FROM t_link_config WHERE source_se = :source AND dest_se = :dest UNION "
            "   SELECT optimizer_mode FROM t_link_config WHERE source_se = :source AND dest_se = '*' UNION "
            "   SELECT optimizer_mode FROM t_link_config WHERE source_se = '*' AND dest_se = :dest UNION "
            "   SELECT optimizer_mode FROM t_link_config WHERE source_se = '*' AND dest_se = '*' UNION "
            "   SELECT 1 FROM dual "
            ") AS o LIMIT 1",
            soci::use(source, "source"), soci::use(dest, "dest"),
            soci::into(mode);

        return mode;
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
    std::string servicename = "fts_server";

    try
    {
        soci::indicator isNull = soci::i_ok;

        sql << "SELECT drain FROM t_hosts WHERE hostname = :hostname and service_name= :servicename",soci::use(hostname), soci::use(servicename), soci::into(drain, isNull);

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


boost::tribool MySqlAPI::isProtocolUDT(const std::string &source, const std::string &dest)
{
    soci::session sql(*connectionPool);

    try {
        boost::logic::tribool srcEnabled(boost::indeterminate);
        boost::logic::tribool dstEnabled(boost::indeterminate);
        boost::logic::tribool starEnabled(boost::indeterminate);
        sql << "SELECT udt FROM t_se WHERE storage = :source", soci::use(source), soci::into(srcEnabled);
        sql << "SELECT udt FROM t_se WHERE storage = :dest", soci::use(dest), soci::into(dstEnabled);
        sql << "SELECT udt FROM t_se WHERE storage = '*'", soci::into(starEnabled);

        // Fallback if both are undefined
        if (boost::indeterminate(srcEnabled) && boost::indeterminate(dstEnabled)) {
            return starEnabled;
        }
        // If only one is undefined, the other decides
        else if (boost::indeterminate(srcEnabled)) {
            return dstEnabled;
        } else if (boost::indeterminate(dstEnabled)) {
            return srcEnabled;
        }
        // Both defined, need to agree
        return srcEnabled && dstEnabled;
    }
    catch (std::exception &e) {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...) {
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


boost::tribool MySqlAPI::isProtocolIPv6(const std::string &source, const std::string &dest)
{
    soci::session sql(*connectionPool);

    try {
        boost::logic::tribool srcEnabled(boost::indeterminate);
        boost::logic::tribool dstEnabled(boost::indeterminate);
        boost::logic::tribool starEnabled(boost::indeterminate);
        sql << "SELECT ipv6 FROM t_se WHERE storage = :source", soci::use(source), soci::into(srcEnabled);
        sql << "SELECT ipv6 FROM t_se WHERE storage = :dest", soci::use(dest), soci::into(dstEnabled);
        sql << "SELECT ipv6 FROM t_se WHERE storage = '*'", soci::into(starEnabled);

        // Fallback if both are undefined
        if (boost::indeterminate(srcEnabled) && boost::indeterminate(dstEnabled)) {
            return starEnabled;
        }
        // If only one is undefined, the other decides
        else if (boost::indeterminate(srcEnabled)) {
            return dstEnabled;
        } else if (boost::indeterminate(dstEnabled)) {
            return srcEnabled;
        }
        // Both defined, need to agree
        return srcEnabled && dstEnabled;
    }
    catch (std::exception &e) {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...) {
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


boost::tribool MySqlAPI::getEvictionFlag(const std::string &source)
{
    soci::session sql(*connectionPool);

    try {
        boost::logic::tribool evictionEnabled(boost::indeterminate);
        sql << "SELECT eviction FROM t_se WHERE storage = :source", soci::use(source), soci::into(evictionEnabled);

        if (boost::indeterminate(evictionEnabled)) {
            return false;
        } else {
            return evictionEnabled;
        }
    }
    catch (std::exception &e) {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...) {
        throw UserError(std::string(__func__) + ": Caught exception ");
    }
}


int MySqlAPI::getStreamsOptimization(const std::string &sourceSe, const std::string &destSe)
{
    soci::session sql(*connectionPool);

    try
    {
        int streams = 0;
        soci::indicator ind;

        sql <<
        "SELECT nostreams FROM ("
        "   SELECT nostreams FROM t_link_config WHERE source_se = :source AND dest_se = :dest AND nostreams IS NOT NULL UNION "
        "   SELECT nostreams FROM t_link_config WHERE source_se = :source AND dest_se = '*' AND nostreams IS NOT NULL UNION "
        "   SELECT nostreams FROM t_link_config WHERE source_se = '*' AND dest_se = :dest AND nostreams IS NOT NULL UNION "
        "   SELECT nostreams FROM t_link_config WHERE source_se = '*' AND dest_se = '*' AND nostreams IS NOT NULL UNION "
        "   SELECT nostreams FROM t_optimizer WHERE source_se = :source AND dest_se = :dest"
        ") AS cfg LIMIT 1",
        soci::use(sourceSe, "source"), soci::use(destSe, "dest"),
        soci::into(streams, ind);

        if (ind == soci::i_null) {
            streams = 0;
        }

        return streams;
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


bool MySqlAPI::getDisableDelegationFlag(const std::string &sourceSe, const std::string &destSe)
{
    soci::session sql(*connectionPool);

    try
    {
        std::string disable_delegation;
        soci::indicator ind = soci::i_ok;

        sql <<
            "SELECT no_delegation FROM ("
            "   SELECT no_delegation FROM t_link_config WHERE source_se = :source AND dest_se = :dest AND no_delegation IS NOT NULL UNION "
            "   SELECT no_delegation FROM t_link_config WHERE source_se = :source AND dest_se = '*' AND no_delegation IS NOT NULL UNION "
            "   SELECT no_delegation FROM t_link_config WHERE source_se = '*' AND dest_se = :dest AND no_delegation IS NOT NULL UNION "
            "   SELECT no_delegation FROM t_link_config WHERE source_se = '*' AND dest_se = '*' AND no_delegation IS NOT NULL"
            ") AS dlg LIMIT 1",
            soci::use(sourceSe, "source"), soci::use(destSe, "dest"),
            soci::into(disable_delegation, ind);

        if (ind == soci::i_null) {
            return false;
        } else {
            return disable_delegation == "on";
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
        throw UserError(std::string(__func__) + ": Caught exception");
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
               "WHERE vo_name IN (:vo, '*') OR vo_name IS NULL "
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
               "WHERE vo_name IN (:vo, '*') OR vo_name IS NULL "
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


bool MySqlAPI::getDisableStreamingFlag(const std::string& voName)
{
    soci::session sql(*connectionPool);

    try
    {
        std::string disable_streaming;
        soci::indicator ind = soci::i_ok;

        sql <<
            "SELECT no_streaming FROM t_server_config "
            "WHERE vo_name IN (:vo, '*') OR vo_name IS NULL "
            "ORDER BY vo_name DESC LIMIT 1",
            soci::use(voName), soci::into(disable_streaming, ind);

        if (ind == soci::i_null) {
            return false;
        } else {
            return disable_streaming == "on";
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
        throw UserError(std::string(__func__) + ": Caught exception");
    }
}


bool MySqlAPI::getCloudStorageCredentials(const std::string& user_dn,
    const std::string& vo, const std::string& cloud_name, CloudStorageAuth& auth)
{
    soci::session sql(*connectionPool);

    try
    {
        sql <<
            " SELECT * "
            " FROM t_cloudStorage cs "
            " JOIN t_cloudStorageUser cu ON cs.cloudStorage_name = cu.cloudStorage_name "
            " WHERE"
            "   cs.cloudStorage_name=:cs_name AND ("
            "       (cu.user_dn=:user_dn AND cu.vo_name=:vo) OR "
            "       (cu.user_dn='*' AND cu.vo_name=:vo) OR "
            "       (cu.user_dn=:user_dn AND (cu.vo_name IN ('*', '') OR cu.vo_name IS NULL))"
            "   )",
            soci::use(cloud_name, "cs_name"), soci::use(user_dn, "user_dn"), soci::use(vo, "vo"),
            soci::into(auth);
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


StorageConfig MySqlAPI::getStorageConfig(const std::string &storage)
{
    soci::session sql(*connectionPool);
    StorageConfig seConfig, seStarConfig;

    try
    {
        sql << "SELECT * FROM t_se WHERE storage = :storage", soci::use(storage), soci::into(seConfig);
        sql << "SELECT * FROM t_se WHERE storage = '*'", soci::into(seStarConfig);
        seConfig.merge(seStarConfig);
    }
    catch (std::exception& e)
    {
        throw UserError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...)
    {
        throw UserError(std::string(__func__) + ": Caught exception " );
    }

    return seConfig;
}
