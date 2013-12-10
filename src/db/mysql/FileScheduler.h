/*
 * FileScheduler.h
 *
 *  Created on: Oct 9, 2013
 *      Author: simonm
 */

#ifndef FILESCHEDULER_H_
#define FILESCHEDULER_H_

namespace fts3 {
namespace db {

#include <map>
#include <string>

#include <mysql/soci-mysql.h>
#include <mysql/mysql.h>

#include "MySqlAPI.h"


class FileScheduler
{

public:

	FileScheduler(soci::session& sql);
	virtual ~FileScheduler();

	std::map<std::string, int> getShareCfg(std::string src, std::string dst);
	std::map<std::string, long long> getFilesInQueue(std::string se, std::string vo, const std::string& query);
	int getStandAloneCfg(std::string first, std::string second, std::pair<std::string, long long> vo, const std::string& query);
	std::map<std::string, int> getStandAloneCfg(std::string src, std::string dst);
	std::map<std::string, int> merge(std::map<std::string, int>& in, std::map<std::string, int>& out);
	std::map<std::string, int> getFilesNumPerVo(std::string src, std::string dst);


	std::map<std::string, double> getActivityShareConf(std::string vo);
	std::map<std::string, long long> getActivitiesInQueue(std::string src, std::string dst, std::string vo);
	std::map<std::string, int> getFilesNumPerActivity(std::string src, std::string dst, std::string vo, int filesNum);

private:
	soci::session& sql;

    unsigned& hashSegmentStart;
    unsigned& hashSegmentEnd;

    static const std::string wildcard;

    static const std::string GET_DST_IN_QUEUE;
    static const std::string GET_SRC_IN_QUEUE;

};

} /* namespace db */
} /* namespace fts3 */
#endif /* FILESCHEDULER_H_ */
