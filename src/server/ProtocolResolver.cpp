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

#include "common/OptimizerSample.h"

#include <vector>

#include <boost/assign/list_of.hpp>
#include <boost/scoped_ptr.hpp>

FTS3_SERVER_NAMESPACE_START

using namespace fts3::ws;
using namespace fts3::common;
using namespace boost::assign;

ProtocolResolver::ProtocolResolver(TransferFiles* file, vector< scoped_ptr<ShareConfig> >& cfgs) :
		db(DBSingleton::instance().getDBObjectInstance()),
		file(file),
		cfgs(cfgs) {

	vector< scoped_ptr<ShareConfig> >::iterator it;

	// loop over the assigned configurations
	for (it = cfgs.begin(); it != cfgs.end(); it++) {

		// get the source and destination
		string source = (*it)->source;
		string destination = (*it)->destination;
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

optional<ProtocolResolver::protocol> ProtocolResolver::getProtocolCfg(optional< pair<string, string> > link) {

	if (!link) return optional<protocol>();

	string source = (*link).first;
	string destination = (*link).second;

	scoped_ptr<LinkConfig> cfg (
			db->getLinkConfig(source, destination)
		);

	protocol ret;

	get<AUTO_PROTOCOL>(ret) = cfg->auto_protocol == Configuration::on;
	get<NOSTREAMS>(ret) = cfg->NOSTREAMS;
	get<NO_TX_ACTIVITY_TO>(ret) = cfg->NO_TX_ACTIVITY_TO;
	get<TCP_BUFFER_SIZE>(ret) = cfg->TCP_BUFFER_SIZE;
	get<URLCOPY_TX_TO>(ret) = cfg->URLCOPY_TX_TO;

	return ret;
}

optional<ProtocolResolver::protocol> ProtocolResolver::merge(optional<protocol> source, optional<protocol> destination) {

	if (!source) return destination;
	if (!destination) return source;

	protocol ret;

	get<AUTO_PROTOCOL>(ret) = get<AUTO_PROTOCOL>(*source) && get<AUTO_PROTOCOL>(*destination);

	// we care about the parameters only if the auto tuning is not enabled
	if (!get<AUTO_PROTOCOL>(ret)) {

		// for sure both source and destination were not set to auto!

		// if the source is set to auto return the destination
		if (get<AUTO_PROTOCOL>(*source)) return destination;

		// if the destination is set to auto return the source
		if (get<AUTO_PROTOCOL>(*destination)) return source;

		// neither the source or the destination were set to auto merge the protocol parameters

		get<NOSTREAMS>(ret) =
				get<NOSTREAMS>(*source) < get<NOSTREAMS>(*destination) ?
				get<NOSTREAMS>(*source) : get<NOSTREAMS>(*destination)
				;

		get<NO_TX_ACTIVITY_TO>(ret) =
				get<NO_TX_ACTIVITY_TO>(*source) < get<NO_TX_ACTIVITY_TO>(*destination) ?
				get<NO_TX_ACTIVITY_TO>(*source) : get<NO_TX_ACTIVITY_TO>(*destination)
				;

		get<TCP_BUFFER_SIZE>(ret) =
				get<TCP_BUFFER_SIZE>(*source) < get<TCP_BUFFER_SIZE>(*destination) ?
				get<TCP_BUFFER_SIZE>(*source) : get<TCP_BUFFER_SIZE>(*destination)
				;

		get<URLCOPY_TX_TO>(ret) =
				get<URLCOPY_TX_TO>(*source) < get<URLCOPY_TX_TO>(*destination) ?
				get<URLCOPY_TX_TO>(*source) : get<URLCOPY_TX_TO>(*destination)
				;
	}

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

bool ProtocolResolver::resolve() {

	// check if there's a SE pair configuration
	prot = getProtocolCfg(link[SE_PAIR]);
	
	if (prot.is_initialized()) return true;

	// check if there is a SE group pair configuration
	prot = getProtocolCfg(link[GROUP_PAIR]);
	if (prot.is_initialized()) return true;

	// get the first existing standalone source link from the list
	optional< pair<string, string> > source_link = getFirst(
			list_of (SOURCE_SE) (SOURCE_GROUP) (SOURCE_WILDCARD)
		);
	// get the first existing standalone destination link from the list
	optional< pair<string, string> > destination_link = getFirst(
			list_of (DESTINATION_SE) (DESTINATION_GROUP) (DESTINATION_WILDCARD)
		);

	// merge the configuration of the most specific standlone source and destination links
	prot = merge(
			getProtocolCfg(source_link),
			getProtocolCfg(destination_link)
		);

	if (isAuto()) {
		autotune();
	}

	return prot.is_initialized();
}

void ProtocolResolver::autotune() {

	string source = file->SOURCE_SE;
	string destination = file->DEST_SE;

	OptimizerSample opt_config;
    DBSingleton::instance().getDBObjectInstance()->initOptimizer(source, destination, 0);
    DBSingleton::instance().getDBObjectInstance()->fetchOptimizationConfig2(&opt_config, source, destination);
    get<TCP_BUFFER_SIZE>(*prot) = opt_config.getBufSize();
    get<NOSTREAMS>(*prot) = opt_config.getStreamsperFile();
    get<URLCOPY_TX_TO>(*prot) = opt_config.getTimeout();
}

bool ProtocolResolver::isAuto() {
	return get<AUTO_PROTOCOL>(*prot);
}

int ProtocolResolver::getNoStreams() {
	return get<NOSTREAMS>(*prot);
}

int ProtocolResolver::getNoTxActiveTo() {
	return get<NO_TX_ACTIVITY_TO>(*prot);
}

int ProtocolResolver::getTcpBufferSize() {
	return get<TCP_BUFFER_SIZE>(*prot);
}

int ProtocolResolver::getUrlCopyTxTo() {
	return get<URLCOPY_TX_TO>(*prot);
}

FTS3_SERVER_NAMESPACE_END
