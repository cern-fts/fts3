/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
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
 */

#include "ws/gsoap_config_stubs.h"
#include "common/logger.h"

using namespace fts3::common;


int config::SoapBindingService::setConfiguration
(
    config__Configuration *_configuration,
    struct impl__setConfigurationResponse &response
)
{
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'setConfiguration' request" << commit;
    return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int config::SoapBindingService::getConfiguration
(
    struct impl__getConfigurationResponse & response
)
{
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getConfiguration' request" << commit;
	response.configuration = soap_new_config__Configuration(this, -1);

	response.configuration->key.push_back("A");
	response.configuration->key.push_back("B");
	response.configuration->key.push_back("C");

	response.configuration->value.push_back("1");
	response.configuration->value.push_back("2");
	response.configuration->value.push_back("3");

    return SOAP_OK;
}

