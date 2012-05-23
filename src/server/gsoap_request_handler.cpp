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

#include "gsoap_request_handler.h"

FTS3_SERVER_NAMESPACE_START

GSoapRequestHandler::~GSoapRequestHandler () {

	soap_destroy(soap);
	soap_end(soap);
	soap_done(soap); // ??
	soap_free(soap);
}

void GSoapRequestHandler::handle() {
    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Serving request started... " << commit;
    fts3_serve(soap);
    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Serving request finished... " << commit;
}

FTS3_SERVER_NAMESPACE_END
