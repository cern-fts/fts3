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
 *
 *
 * config_web_service.h
 *
 *  Created on: Apr 19, 2012
 *      Author: Michal Simon
 */

#ifndef CONFIG_WEB_SERVICE_H_
#define CONFIG_WEB_SERVICE_H_

#include "web_service_handler.h"
#include "gsoap_acceptor.h"
#include "gsoap_method_handler.h"
#include "active_object.h"
#include "threadpool.h"
#include "common/traced.h"
#include "ws/gsoap_stubs.h"


FTS3_SERVER_NAMESPACE_START

/* -------------------------------------------------------------------------- */

using namespace FTS3_COMMON_NAMESPACE;

/* -------------------------------------------------------------------------- */

class ConfigWebService;

struct ConfigWebServiceTraits
{
    typedef GSoapAcceptor<SoapBindingService> Acceptor;
    typedef GSoapMethodHandler<SoapBindingService> Handler;
    typedef ActiveObject <ThreadPool::ThreadPool, Traced<ConfigWebService> > ActiveObjectType;
};

/* -------------------------------------------------------------------------- */

class ConfigWebService : public WebServiceHandler <ConfigWebServiceTraits>
{
public:
	ConfigWebService();
};

FTS3_SERVER_NAMESPACE_END

#endif /* CONFIG_WEB_SERVICE_H_ */
