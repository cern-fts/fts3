/*
 *	Copyright notice:
 *	Copyright � Members of the EMI Collaboration, 2010.
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
 * GSoapContextAdapter.h
 *
 * ConfigurationAssigner.cpp
 *
 *  Created on: Dec 10, 2012
 *      Author: simonm
 */

#include "ConfigurationAssigner.h"

#include "ws/config/Configuration.h"

#include "common/JobStatusHandler.h"

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

void ConfigurationAssigner::assign(vector< scoped_ptr<ShareConfig> >& out) {

	string source = file->SOURCE_SE;
	string destination = file->DEST_SE;
	string vo = file->VO_NAME;

	// possible configurations for SE
	list<cfg_type> se_cfgs = list_of
			( cfg_type( share(source, destination, vo), content(true, true) ) )
			( cfg_type( share(source, Configuration::any, vo), content(true, false) ) )
			( cfg_type( share(Configuration::wildcard, Configuration::any, vo), content(true, false) ) )
			( cfg_type( share(Configuration::any, destination, vo), content(false, true) ) )
			( cfg_type( share(Configuration::any, Configuration::wildcard, vo), content(false, true) ) )
			;

	assignShareCfg(se_cfgs, out);

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

	assignShareCfg(gr_cfgs, out);
}

void ConfigurationAssigner::assignShareCfg(list<cfg_type> arg, vector< scoped_ptr<ShareConfig> >& out) {

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

		// check if there is a VO share
		scoped_ptr<ShareConfig> ptr (
				db->getShareConfig(source, destination, vo)
			);

		if (ptr.get()) {
			// if it is a auto-share don't assign a configuration
			if (ptr->active_transfers == auto_share) continue;
			// assign the share configuration to transfer job
// TODO			out.push_back(ptr);
			// a configuration has been assigned
			assign_count++;
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
			// add to out
// TODO			out.push_back(ptr);
		}

		// if it is a auto-share don't assign a configuration
		if (ptr->active_transfers == auto_share) continue;
		// assign the share configuration to transfer job
// TODO		out.push_back(ptr);
		// a configuration has been assigned
		assign_count++;
		// set the respective flags
		both.first |= c.first;
		both.second |= c.second;
		// if both source and destination are covert break;
		if (both.first && both.second) break;
	}
}

} /* namespace server */
} /* namespace fts3 */
