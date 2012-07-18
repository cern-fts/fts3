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

#include "ws/gsoap_stubs.h"
#include "ws/ConfigurationHandler.h"
#include "ws/AuthorizationManager.h"

#include "db/generic/SingleDbInstance.h"

#include "common/logger.h"
#include <common/error.h>

#include <vector>
#include <string>
#include <set>
#include <exception>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace db;
using namespace fts3::common;
using namespace boost;
using namespace boost::algorithm;
using namespace fts3::ws;


int fts3::implcfg__setConfiguration(soap* soap, config__Configuration *_configuration, struct implcfg__setConfigurationResponse &response) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'setConfiguration' request" << commit;

	vector<string>& cfgs = _configuration->cfg;
	vector<string>::iterator it;

	try {
		AuthorizationManager::getInstance().authorize(soap);
		ConfigurationHandler handler;
		for(it = cfgs.begin(); it < cfgs.end(); it++) {
			handler.parse(*it);
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

int fts3::implcfg__getConfiguration(soap* soap, struct implcfg__getConfigurationResponse & response) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getConfiguration' request" << commit;

	response.configuration = soap_new_config__Configuration(soap, -1);
	try {
		AuthorizationManager::getInstance().authorize(soap);
		ConfigurationHandler handler;
		response.configuration->cfg = handler.get();

	} catch(std::exception& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "A std::exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "ConfigurationException");
		return SOAP_FAULT;

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "ConfigurationException");
	}

    return SOAP_OK;
}

