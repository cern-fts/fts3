/*
 * ConfigurationAssigner.cpp
 *
 *  Created on: Dec 10, 2012
 *      Author: simonm
 */

#include "ConfigurationAssigner.h"

#include "ws/config/Configuration.h"

#include "ws-ifce/JobStatusHandler.h"

#include <boost/assign.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

namespace fts3 {
namespace server {

using namespace fts3::ws;
using namespace fts3::common;

using namespace boost::assign;

ConfigurationAssigner::ConfigurationAssigner(TransferFiles* file) :
		file(file),
		db (DBSingleton::instance().getDBObjectInstance()),
		assign_count(0) {

}

ConfigurationAssigner::~ConfigurationAssigner() {

}

bool ConfigurationAssigner::assign() {

	string job_id = file->JOB_ID;

	// check if a configuration has already been assigned to the transfer job
	optional<int> count = db->getJobConfigCount(job_id);
	if (count) {
		// make sure nothing was deleted
		if (*count == db->countJobShareConfig(job_id)) {
			return true;
		}
		// if the configuration changed remove the assignments for the job (it will be reassigned)
		db->delJobShareConfig(job_id);
	}

	// the flag that will be returned
	bool assigned = false;

	string source = fileUrlToSeName(file->SOURCE_SURL);
	string destination = fileUrlToSeName(file->DEST_SURL);
	string vo = file->VO_NAME;

	// possible configurations for SE
	list<cfg_type> se_cfgs = list_of
			( cfg_type( share(source, destination, vo), content(true, true) ) )
			( cfg_type( share(source, Configuration::any, vo), content(true, false) ) )
			( cfg_type( share(Configuration::wildcard, Configuration::any, vo), content(true, false) ) )
			( cfg_type( share(Configuration::any, destination, vo), content(false, true) ) )
			( cfg_type( share(Configuration::any, Configuration::wildcard, vo), content(false, true) ) )
			;
	// assign configuration at SE level
	assigned |= assignShareCfg(se_cfgs);

	// get group names for source and destination SEs
	string sourceGr = db->getGroupForSe(source);
	string destinationGr = db->getGroupForSe(destination);

	// possible configuration for SE group
	list<cfg_type> gr_cfgs;
	if (!sourceGr.empty() && !destinationGr.empty())
		gr_cfgs.push_back( cfg_type( share(sourceGr, destinationGr, vo), content(true, true) ) );
	if (!sourceGr.empty())
		gr_cfgs.push_back( cfg_type( share(sourceGr, Configuration::any, vo), content(true, false) ) );
	if (!destinationGr.empty())
		gr_cfgs.push_back( cfg_type( share(Configuration::any, destinationGr, vo), content(false, true) ) );
	// assign configuration at SE group level
	assigned |= assignShareCfg(gr_cfgs);

	// set the number of assigned configurations
	db->setJobConfigCount(job_id, assign_count);

	return assigned;
}

bool ConfigurationAssigner::assignShareCfg(list<cfg_type> arg) {

	content both (false, false);

	list<cfg_type>::iterator it;
	for (it = arg.begin(); it != arg.end(); it++) {

		share s = get<SHARE>(*it);
		content c = get<CONTENT>(*it);

		// check if configuration for the given side has not been assigned already
		if ( (c.first && both.first) || (c.second && both.second) ) continue;

		string source = get<SOURCE>(s);
		string destination = get<DESTINATION>(s);
		string vo = get<VO>(s);

		// check if there is a link configuration, if no there will be no share
		// ('isTherelinkConfig' will return 'false' also if the link configuration state is 'off'
		if (!db->isThereLinkConfig(source, destination)) continue;

		// if there is a link a configuration will be assigned
		assign_count++;

		// check if there is a VO share
		scoped_ptr<ShareConfig> ptr (
				db->getShareConfig(source, destination, vo)
			);

		if (ptr.get()) {
			// assign the share configuration to transfer job
			db->addJobShareConfig(
					file->JOB_ID, ptr->source, ptr->destination, ptr->vo
				);
			// set the respective flags
			both.first |= c.first;
			both.second |= c.second;
			// if both source and destination are covert break;
			if (both.first && both.second) break;
			// otherwise continue
			continue;
		}

		// check if there is a public share
		ptr.reset(
				db->getShareConfig(source, destination, Configuration::pub)
			);

		// if not create a public share with 0 active transfer (equivalent)
		if (!ptr.get()) {
			// create the object
			ptr.reset(
					new ShareConfig
				);
			// fill in the respective values
			ptr->source = source;
			ptr->destination = destination;
			ptr->vo = Configuration::pub;
			ptr->active_transfers = 0;
			// add to DB
			db->addShareConfig(ptr.get());
		}

		// assign the share configuration to transfer job
		db->addJobShareConfig(
				file->JOB_ID, ptr->source, ptr->destination, ptr->vo
			);
		// set the respective flags
		both.first |= c.first;
		both.second |= c.second;
		// if both source and destination are covert break;
		if (both.first && both.second) break;
	}

	return both.first || both.second;
}

string ConfigurationAssigner::fileUrlToSeName(string url) {

	static const regex re(".+://([a-zA-Z0-9\\.-]+)(:\\d+)?/.+");
	smatch what;

	if (regex_match(url, what, re, match_extra)) {
		// indexes are shifted by 1 because at index 0 is the whole string
		return string(what[1]);
	} else
		return string();
}

} /* namespace server */
} /* namespace fts3 */
