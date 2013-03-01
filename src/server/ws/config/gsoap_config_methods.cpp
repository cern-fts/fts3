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
 */

#include "ws-ifce/gsoap/gsoap_stubs.h"

#include "ConfigurationHandler.h"
#include "ws/AuthorizationManager.h"
#include "ws/CGsiAdapter.h"

#include "db/generic/SingleDbInstance.h"

#include "common/logger.h"
#include "common/error.h"

#include "DrainMode.h"

#include <vector>
#include <string>
#include <set>
#include <exception>
#include <utility>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace db;
using namespace fts3::common;
using namespace boost;
using namespace boost::algorithm;
using namespace fts3::ws;


int fts3::implcfg__setConfiguration(soap* soap, config__Configuration *_configuration, struct implcfg__setConfigurationResponse &response) {

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'setConfiguration' request" << commit;

	vector<string>& cfgs = _configuration->cfg;
	vector<string>::iterator it;

	try {
		CGsiAdapter cgsi(soap);
		string dn = cgsi.getClientDn();

		ConfigurationHandler handler (dn);
		for(it = cfgs.begin(); it < cfgs.end(); it++) {
			handler.parse(*it);
			// authorize each configuration operation separately for each se/se group
			AuthorizationManager::getInstance().authorize(
					soap,
					AuthorizationManager::CONFIG,
					AuthorizationManager::dummy
				);
			handler.add();
		}
	} catch(std::exception& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "A std::exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "ConfigurationException");
		return SOAP_FAULT;

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "ConfigurationException");
		return SOAP_FAULT;
	}

    return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int fts3::implcfg__getConfiguration(soap* soap, string vo, string name, string source, string destination, implcfg__getConfigurationResponse & response) {

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getConfiguration' request" << commit;

	response.configuration = soap_new_config__Configuration(soap, -1);
	try {
		AuthorizationManager::getInstance().authorize(
				soap,
				AuthorizationManager::CONFIG,
				AuthorizationManager::dummy
			);

		CGsiAdapter cgsi(soap);
		string dn = cgsi.getClientDn();

		bool all = source.empty() && destination.empty();
		bool standalone = !source.empty() && destination.empty();
		bool pair = !source.empty() && !destination.empty();
		bool symbolic_name = !name.empty();

//		if (symbolic_name && (standalone || pair) ) {
//			throw Err_Custom("Either a stand alone configuration or pair configuration or symbolic name may be specified for the query!");
//		}

		ConfigurationHandler handler (dn);
		if (all) {
			response.configuration->cfg = handler.get();
		} else if (standalone) {
			response.configuration->cfg = handler.get(source);
		} else if (pair) {
			response.configuration->cfg = handler.getPair(source, destination);
		} else if (symbolic_name) {
			response.configuration->cfg = handler.getPair(name);
		} else {
			throw Err_Custom("Wrongly specified parameters, either both the source and destination have to be specified or the configuration name!");
		}

	} catch(std::exception& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "A std::exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "ConfigurationException");
		return SOAP_FAULT;

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "ConfigurationException");
		return SOAP_FAULT;
	}

    return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int fts3::implcfg__delConfiguration(soap* soap, config__Configuration *_configuration, struct implcfg__delConfigurationResponse &_param_11) {

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'delConfiguration' request" << commit;

	vector<string>& cfgs = _configuration->cfg;
	vector<string>::iterator it;

	try {
		CGsiAdapter cgsi(soap);
		string dn = cgsi.getClientDn();

		ConfigurationHandler handler (dn);
		for(it = cfgs.begin(); it < cfgs.end(); it++) {
			handler.parse(*it);
			// authorize each configuration operation separately for each se/se group
			AuthorizationManager::getInstance().authorize(
					soap,
					AuthorizationManager::CONFIG,
					AuthorizationManager::dummy
				);
			handler.del();
		}

	} catch(std::exception& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "A std::exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "ConfigurationException");
		return SOAP_FAULT;

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "ConfigurationException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int fts3::implcfg__doDrain(soap* soap, bool drain, struct implcfg__doDrainResponse &_param_13) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Turning " << (drain ? "on" : "off") << " the drain mode" << commit;
	DrainMode::getInstance() = drain;
	return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int fts3::implcfg__setRetry(soap* ctx, int retry, implcfg__setRetryResponse& _resp) {

	try {
		// authorize
		AuthorizationManager::getInstance().authorize(
				ctx,
				AuthorizationManager::CONFIG,
				AuthorizationManager::dummy
			);

		// get user dn
		CGsiAdapter cgsi(ctx);
		string dn = cgsi.getClientDn();

		// prepare the command for audit
		stringstream cmd;
		cmd << "fts-config-set --retry " << retry;

		// audit the operation
		DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd.str(), "retry");

		// set the number of retries
		DBSingleton::instance().getDBObjectInstance()->setRetry(retry);

		// log it
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << dn << " had set the retry value to " << retry << commit;

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(ctx, ex.what(), "TransferException");

		return SOAP_FAULT;
	} catch (...) {
	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the number of retries cannot be set"  << commit;
	    return SOAP_FAULT;
	}

	return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int fts3::implcfg__setQueueTimeout(soap* ctx, unsigned int timeout, implcfg__setQueueTimeoutResponse& resp) {

	try {
		// authorize
		AuthorizationManager::getInstance().authorize(
				ctx,
				AuthorizationManager::CONFIG,
				AuthorizationManager::dummy
			);

		// get user dn
		CGsiAdapter cgsi(ctx);
		string dn = cgsi.getClientDn();

		// prepare the command for audit
		stringstream cmd;
		cmd << "fts-config-set --queue-timeout " << timeout;

		// audit the operation
		DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd.str(), "queue-timeout");

		// set the number of retries
		DBSingleton::instance().getDBObjectInstance()->setMaxTimeInQueue(timeout);

		// log it
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << dn << " had set the maximum timeout in the queue to " << timeout << commit;

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(ctx, ex.what(), "TransferException");

		return SOAP_FAULT;
	} catch (...) {
	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the number of retries cannot be set"  << commit;
	    return SOAP_FAULT;
	}

	return SOAP_OK;
}

int fts3::implcfg__setBringOnline(soap* ctx, config__BringOnline *bring_online, implcfg__setBringOnlineResponse &resp) {

	try {

		// authorize
		AuthorizationManager::getInstance().authorize(
				ctx,
				AuthorizationManager::CONFIG,
				AuthorizationManager::dummy
			);

		// get user VO
		CGsiAdapter cgsi(ctx);
		string vo = cgsi.getClientVo();

		vector<config__BringOnlinePair*>& v = bring_online->boElem;
		vector<config__BringOnlinePair*>::iterator it;

		for (it = v.begin(); it != v.end(); it++) {

			config__BringOnlinePair* pair = *it;

			DBSingleton::instance().getDBObjectInstance()->setMaxStageOp(
					pair->first,
					vo,
					pair->second
				);
		}

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(ctx, ex.what(), "TransferException");

		return SOAP_FAULT;
	} catch (...) {
	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the number of retries cannot be set"  << commit;
	    return SOAP_FAULT;
	}

	return SOAP_OK;
}
