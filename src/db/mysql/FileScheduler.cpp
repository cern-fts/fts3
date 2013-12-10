/*
 * FileScheduler.cpp
 *
 *  Created on: Oct 9, 2013
 *      Author: simonm
 */

#include "FileScheduler.h"

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>

#include <error.h>

namespace fts3 {
namespace db {

using namespace fts3::common;

const std::string FileScheduler::wildcard = "*";

const std::string FileScheduler::GET_DST_IN_QUEUE =
		" SELECT dest_se AS second, COUNT(DISTINCT f.job_id, file_index) AS count "
		" FROM t_file f "
		" WHERE f.file_state = 'SUBMITTED' AND "
		"	f.source_se = :source "
		"	f.vo_name = :vo_name AND "
		"	f.wait_timestamp IS NULL AND "
		"	(f.retry_timestamp is NULL OR f.retry_timestamp < :tTime) AND "
		"	EXISTS ( "
		"		SELECT NULL FROM t_job j "
		"		WHERE j.job_id = f.job_id AND j.job_state in ('ACTIVE','READY','SUBMITTED') AND "
		"			(j.reuse_job = 'N' OR j.reuse_job IS NULL) AND j.vo_name=:vo_name "
		"		ORDER BY j.priority DESC, j.submit_time "
		"	) "
		" GROUP BY dest_se "
		;

const std::string FileScheduler::GET_SRC_IN_QUEUE =
		" SELECT source_se AS second, COUNT(DISTINCT f.job_id, file_index) AS count "
		" FROM t_file f "
		" WHERE f.file_state = 'SUBMITTED' AND "
		"	f.dest_se = :dest "
		"	f.vo_name = :vo_name AND "
		"	f.wait_timestamp IS NULL AND "
		"	(f.retry_timestamp is NULL OR f.retry_timestamp < :tTime) AND "
		"	EXISTS ( "
		"		SELECT NULL FROM t_job j "
		"		WHERE j.job_id = f.job_id AND j.job_state in ('ACTIVE','READY','SUBMITTED') AND "
		"			(j.reuse_job = 'N' OR j.reuse_job IS NULL) AND j.vo_name=:vo_name "
		"		ORDER BY j.priority DESC, j.submit_time "
		"	) "
		" GROUP BY source_se "
		;

FileScheduler::FileScheduler(soci::session& sql) :
		sql(sql),
		hashSegmentStart(hashSegmentStart),
		hashSegmentEnd(hashSegmentEnd)
{

}

FileScheduler::~FileScheduler()
{

}

std::map<std::string, int> FileScheduler::getShareCfg(std::string src, std::string dst)
{
	std::map<std::string, int> ret;

    try
        {
            soci::rowset<ShareConfig> rs = (
            		sql.prepare <<
            		" SELECT * "
            		" FROM t_share_config "
            		" WHERE source = :source AND destination = :destination",
            		soci::use(src),
            		soci::use(dst)
            );

            for (soci::rowset<ShareConfig>::const_iterator i = rs.begin(); i != rs.end(); ++i)
                {
                    ret[i->vo] = i->active_transfers;
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return ret;
}

std::map<std::string, long long> FileScheduler::getFilesInQueue(std::string se, std::string vo, const std::string& query)
{
    std::map<std::string, long long> ret;

    time_t now = time(NULL);
    struct tm tTime;
    gmtime_r(&now, &tTime);

    try
        {
    		soci::rowset<soci::row> rs = (
    				sql.prepare <<
    				query,
					soci::use(se),
					soci::use(vo),
					soci::use(tTime),
					soci::use(vo)
			 );

            soci::rowset<soci::row>::const_iterator it;
            for (it = rs.begin(); it != rs.end(); it++)
                {
					std::string dst = it->get<std::string>("second");
					long long nFiles = it->get<long long>("count");
					ret[dst] = nFiles;

                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return ret;
}

// can be optimized
int FileScheduler::getStandAloneCfg(std::string first, std::string second,  std::pair<std::string, long long> vo, const std::string& query)
{
	// number of files in the queue per first SE
	std::map<std::string, int> seconds = getFilesInQueue(first, vo.first, query);
	// number of credits for the given VO
	int ret = 0;
	// distribute the credits between destinations
	while (vo.second > 0)
		{
			int sum = 0;
			// iterate over all SEs
			std::map<std::string, int>::iterator it;
			for (it = seconds.begin(); it != seconds.end(); it++)
				{
					// check if there are still files for this destination, if no continue
					if (it->second <= 0) continue;
					// one destination has been assigned with a credit
					it->second--;
					// one credit has been assigned
					vo.second--;
					// add the remaining destination to the sum
					sum += it->second;
					// if it is the destination we care about ...
					if (second == it->first) ret++;
				}
			// if there are no more files without credits break;
			if (sum == 0) break;
		}

	return ret;
}

std::map<std::string, int> FileScheduler::merge(std::map<std::string, int>& in, std::map<std::string, int>& out)
{
	// if one is empty we do not have to care about merging
	if (in.empty()) return out;
	if (out.empty()) return in;

	std::map<std::string, int> ret;

	// TODO how to handle 'public' ???

	// iterate over 'in'
	std::map<std::string, int>::iterator it;
	for (it = in.begin(); it != in.end(); it++)
		{
			// make sure that the VO is accepted by both source and destination
			if (out.find(it->first) != out.end())
				{
					// if yes, pick the smaller value
					ret[it->first] = out[it->first] < it->second ? out[it->first] : it->second;
				}
			else
				{

				}
		}

	return ret;
}

std::map<std::string, int> FileScheduler::getStandAloneCfg(std::string src, std::string dst)
{
	// first take care of the source
	std::map<std::string, int> out;
	// get the number of credits for each VO
	std::map<std::string, int> vos  = getShareCfg(src, wildcard);
	// iterate over VOs
	std::map<std::string, int>::iterator it_vo;
	for (it_vo = vos.begin(); it_vo != vos.end(); it_vo++)
		{
			// get number of credit for the given VO
			out[it_vo->first] = getStandAloneCfg(src, dst, *it_vo, GET_DST_IN_QUEUE);
		}

	// then we take care of the destination
	std::map<std::string, int> in;
	// get the number of credits for each VO
	vos  = getShareCfg(wildcard, dst);
	// iterate over VOs
	for (it_vo = vos.begin(); it_vo != vos.end(); it_vo++)
		{
			// get number of credit for the given VO
			in[it_vo->first] = getStandAloneCfg(src, dst, *it_vo, GET_DST_IN_QUEUE);
		}

	return merge(in, out);
}

std::map<std::string, int> FileScheduler::getFilesNumPerVo(std::string src, std::string dst)
{
	// get the pair configuration for source and destination
	std::map<std::string, int> ret = getShareCfg(src, dst);
	// if pair configuration does not exist check stand alone configurations
	if (ret.empty()) ret = getStandAloneCfg(src, dst);

	return ret;
}

std::map<std::string, double> FileScheduler::getActivityShareConf(std::string vo)
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
                "	AND active = 'on'",
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

            static const boost::regex re("^\\s*\\{\\s*\"([a-zA-Z0-9\\.-]+)\"\\s*:\\s*(0\\.\\d+)\\s*\\}\\s*$");
            static const int ACTIVITY_NAME = 1;
            static const int ACTIVITY_SHARE = 2;

            for (it = tokens.begin(); it != tokens.end(); it++)
                {
                    // parse single activity share
                    std::string str = *it;

                    boost::smatch what;
                    boost::regex_match(str, what, re, boost::match_extra);

                    ret[what[ACTIVITY_NAME]] = boost::lexical_cast<double>(what[ACTIVITY_SHARE]);
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }
    return ret;
}

std::map<std::string, long long> FileScheduler::getActivitiesInQueue(std::string src, std::string dst, std::string vo)
{
    std::map<std::string, long long> ret;

    time_t now = time(NULL);
    struct tm tTime;
    gmtime_r(&now, &tTime);

    try
        {
            soci::rowset<soci::row> rs = (
				sql.prepare <<
				" SELECT file_metadata, COUNT(DISTINCT f.job_id, file_index) AS count "
				" FROM t_file f "
				" WHERE f.file_state = 'SUBMITTED' AND "
				"	f.source_se = :source AND f.dest_se = :dest AND "
				"	f.vo_name = :vo_name AND "
				"	f.wait_timestamp IS NULL AND "
				"	(f.retry_timestamp is NULL OR f.retry_timestamp < :tTime) AND "
				"	(f.hashed_id >= :hStart AND f.hashed_id <= :hEnd) AND "
				"	EXISTS ( "
				"		SELECT NULL FROM t_job j "
				"		WHERE j.job_id = f.job_id AND j.job_state in ('ACTIVE','READY','SUBMITTED') AND "
				"			(j.reuse_job = 'N' OR j.reuse_job IS NULL) AND j.vo_name=:vo_name "
				"		ORDER BY j.priority DESC, j.submit_time "
				"	) "
				" GROUP BY file_metadata ",
				soci::use(src),
				soci::use(dst),
				soci::use(vo),
				soci::use(tTime),
				soci::use(hashSegmentStart), soci::use(hashSegmentEnd),
				soci::use(vo)
			);

            soci::rowset<soci::row>::const_iterator it;
            for (it = rs.begin(); it != rs.end(); it++)
                {
                    if (it->get_indicator("file_metadata") == soci::i_null)
                        {
                            ret["default"] = it->get<long long>("count");
                        }
                    else
                        {
                            std::string activityShare = it->get<std::string>("file_metadata");
                            long long nFiles = it->get<long long>("count");
                            ret[activityShare.empty() ? "default" : activityShare] = nFiles;
                        }
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return ret;
}

std::map<std::string, int> FileScheduler::getFilesNumPerActivity(std::string src, std::string dst, std::string vo, int filesNum)
{
    std::map<std::string, int> activityFilesNum;

    try
        {
            // get activity shares configuration for given VO
            std::map<std::string, double> activityShares = getActivityShareConf(vo);

            // if there is no configuration no assigment can be made
            if (activityShares.empty()) return activityFilesNum;

            // get the activities in the queue
            std::map<std::string, long long> activitiesInQueue = getActivitiesInQueue(src, dst, vo);

            // sum of all activity shares in the queue (needed for normalization)
            double sum = 0;

            std::map<std::string, long long>::iterator it;
            for (it = activitiesInQueue.begin(); it != activitiesInQueue.end(); it++)
                {
                    sum += activityShares[it->first];
                }

            // assign slots to activities
            for (int i = 0; i < filesNum; i++)
                {
            		// if sum <= 0 there is nothing to assign
            		if (sum <= 0) break;
                    // a random number from (0, 1)
                    double r = ((double) rand() / (RAND_MAX));
                    // interval corresponding to given activity
                    double interval = 0;

                    for (it = activitiesInQueue.begin(); it != activitiesInQueue.end(); it++)
                        {
                    		// if there are no more files for this activity continue
                    		if (it->second <= 0) continue;
                    		// calculate the interval
                    		interval += activityShares[it->first] / sum;
                    		// if the slot has been assigned to the given activity ...
                    		if (r < interval)
                                {
                                    ++activityFilesNum[it->first];
                                    --it->second;
                                    // if there are no more files for the given ativity remove it from the sum
                                    if (it->second == 0) sum -= activityShares[it->first];
                                    break;
                                }
                        }
                }
        }
    catch (std::exception& e)
        {
            throw Err_Custom(std::string(__func__) + ": Caught exception " + e.what());
        }

    return activityFilesNum;
}

} /* namespace db */
} /* namespace fts3 */
