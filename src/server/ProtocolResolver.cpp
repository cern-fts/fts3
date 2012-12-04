/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use soap file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implcfgied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * ProtocolResolver.cpp
 *
 *  Created on: Dec 3, 2012
 *      Author: Michal Simon
 */

#include "ProtocolResolver.h"

#include "ws/config/Configuration.h"

#include <vector>

#include <boost/tuple/tuple.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/assign/list_of.hpp>

FTS3_SERVER_NAMESPACE_START

using namespace fts3::ws;
using namespace boost::assign;

ProtocolResolver::ProtocolResolver(string job_id) :	db(DBSingleton::instance().getDBObjectInstance()) {

	vector< tuple<string, string, string> > cfgs = db->getJobShareConfig(job_id);
	vector< tuple<string, string, string> >::iterator it;

	// loop over the assigned configurations
	for (it = cfgs.begin(); it != cfgs.end(); it++) {

		// get the source and destination
		string source = get<SOURCE>(*it);
		string destination = get<DESTINATION>(*it);
		// create source-destination pair
		pair<string, string> entry = pair<string, string>(source, destination);

		// check if it is default configuration for destination SE
		if (destination == Configuration::wildcard && source == Configuration::any) {
			link[DESTINATION_WILDCARD] = entry;
			continue;
		}
		// check if it is default configuration for source SE
		if (source == Configuration::wildcard && destination == Configuration::any) {
			link[SOURCE_WILDCARD] = entry;
			continue;
		}

		// check if we are dealing with groups or SEs
		if (isGr(source) || isGr(destination)) {
			// check if it's standalone group configuration of the destination
			if (destination != Configuration::any && source == Configuration::any) {
				link[DESTINATION_GROUP] = entry;
				continue;
			}
			// check if it's standalone group configuration of the source
			if (source != Configuration::any && destination == Configuration::any) {
				link[SOURCE_GROUP] = entry;
				continue;
			}
			// if it's neither of the above it has to be a pair
			link[GROUP_PAIR] = entry;

		} else {
			// check if it's standalone SE configuration of the destination
			if (destination != Configuration::any && source == Configuration::any) {
				link[DESTINATION_SE] = entry;
				continue;
			}
			// check if it's standalone SE configuration of the source
			if (source != Configuration::any && destination == Configuration::any) {
				link[SOURCE_SE] = entry;
				continue;
			}
			// if it's neither of the above it has to be a pair
			link[SE_PAIR] = entry;
		}
	}
}

ProtocolResolver::~ProtocolResolver() {

}

bool ProtocolResolver::isGr(string name) {
	return db->checkGroupExists(name);
}

SeProtocolConfig* ProtocolResolver::getProtocolCfg(optional< pair<string, string> > link) {

	if (!link) return 0;

	string source = (*link).first;
	string destination = (*link).second;

	scoped_ptr<LinkConfig> cfg (
			db->getLinkConfig(source, destination)
		);

	SeProtocolConfig* protocol = new SeProtocolConfig;
	protocol->NOSTREAMS = cfg->NOSTREAMS;
	protocol->NO_TX_ACTIVITY_TO = cfg->NO_TX_ACTIVITY_TO;
	protocol->TCP_BUFFER_SIZE = cfg->TCP_BUFFER_SIZE;
	protocol->URLCOPY_TX_TO = cfg->URLCOPY_TX_TO;

	return protocol;
}

SeProtocolConfig* ProtocolResolver::merge(SeProtocolConfig* source_ptr, SeProtocolConfig* destination_ptr) {

	if (!source_ptr) return destination_ptr;
	if (!destination_ptr) return source_ptr;

	scoped_ptr<SeProtocolConfig> source (source_ptr);
	scoped_ptr<SeProtocolConfig> destination (destination_ptr);

	SeProtocolConfig* ret = new SeProtocolConfig;

	ret->NOSTREAMS =
			source->NOSTREAMS < destination->NOSTREAMS ?
			source->NOSTREAMS : destination->NOSTREAMS
			;

	ret->NO_TX_ACTIVITY_TO =
			source->NO_TX_ACTIVITY_TO < destination->NO_TX_ACTIVITY_TO ?
			source->NO_TX_ACTIVITY_TO : destination->NO_TX_ACTIVITY_TO
			;

	ret->TCP_BUFFER_SIZE =
			source->TCP_BUFFER_SIZE < destination->TCP_BUFFER_SIZE ?
			source->TCP_BUFFER_SIZE : destination->TCP_BUFFER_SIZE
			;

	ret->URLCOPY_TX_TO =
			source->URLCOPY_TX_TO < destination->URLCOPY_TX_TO ?
			source->URLCOPY_TX_TO : destination->URLCOPY_TX_TO
			;

	return ret;
}

optional< pair<string, string> > ProtocolResolver::getFirst(list<LinkType> l) {
	// look for the first link
	list<LinkType>::iterator it;
	for (it = l.begin(); it != l.end(); it++) {
		// return the first existing link
		if (link[*it]) return link[*it];
	}
	// if nothing was found return empty link
	return optional< pair<string, string> >();
}

SeProtocolConfig* ProtocolResolver::resolve() {

	// check if there's a SE pair configuration
	SeProtocolConfig* ret = getProtocolCfg(link[SE_PAIR]);
	if (ret) return ret;

	// check if there is a SE group pair configuration
	ret = getProtocolCfg(link[GROUP_PAIR]);
	if (ret) return ret;

	// get the first existing standalone source link from the list
	optional< pair<string, string> > source_link = getFirst(
			list_of (SOURCE_SE) (SOURCE_GROUP) (SOURCE_WILDCARD)
		);
	// get the first existing standalone destination link from the list
	optional< pair<string, string> > destination_link = getFirst(
			list_of (DESTINATION_SE) (DESTINATION_GROUP) (DESTINATION_WILDCARD)
		);

	// merge the configuration of the most specific standlone source and destination links
	ret = merge(
			getProtocolCfg(source_link),
			getProtocolCfg(destination_link)
		);

	return ret;
}

FTS3_SERVER_NAMESPACE_END
