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
 * InternalLogRetriever.cpp
 *
 *  Created on: Oct 18, 2012
 *      Author: simonm
 */

#include "InternalLogRetriever.h"

//#include "ws/transfer/LogFileStreamer.h"

//#include "ws-ifce/gsoap/fts3.nsmap"

#include <cgsi_plugin.h>

using namespace fts3::ws;

InternalLogRetriever::InternalLogRetriever(string endpoint): endpoint(endpoint), ctx(soap_new1(SOAP_ENC_MTOM))
{
    // TODO Auto-generated constructor stub

    int  err = 0;

    // initialize cgsi plugin
    err = soap_cgsi_init(ctx,  CGSI_OPT_DISABLE_NAME_CHECK | CGSI_OPT_SSL_COMPATIBLE);
    soap_set_omode(ctx, SOAP_ENC_MTOM);

//	ctx->fmimewriteopen = LogFileStreamer::writeOpen;
//	ctx->fmimewriteclose = LogFileStreamer::writeClose;
//	ctx->fmimewrite = LogFileStreamer::write;

    if (err)
        {
            // TODO
        }

    // set the namespaces
//	if (soap_set_namespaces(ctx, fts3_namespaces)) {
//		// TODO
//	}
}

InternalLogRetriever::~InternalLogRetriever()
{
    soap_destroy(ctx);
    soap_end(ctx);
    soap_free(ctx);
}

list<string> InternalLogRetriever::getInternalLogs(string jobId)
{

    log__GetLogInternalResponse resp;
//	ctx->user = LogFileStreamer::getOutputHandler(resp);
    soap_call_log__GetLogInternal(ctx, endpoint.c_str(), 0, jobId, resp);

    list<string> ret (resp.logs->lognames.begin(), resp.logs->lognames.end());
    return ret;
}

