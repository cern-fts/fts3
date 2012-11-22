/*
 * InternalLogRetriever.cpp
 *
 *  Created on: Oct 18, 2012
 *      Author: simonm
 */

#include "InternalLogRetriever.h"

#include "ws-ifce/LogFileStreamer.h"

//#include "ws-ifce/gsoap/fts3.nsmap"

#include <cgsi_plugin.h>

using namespace fts3::ws;
using namespace fts3::common;

InternalLogRetriever::InternalLogRetriever(string endpoint): endpoint(endpoint), ctx(soap_new1(SOAP_ENC_MTOM)) {
	// TODO Auto-generated constructor stub

    int  err = 0;

    // initialize cgsi plugin
   	err = soap_cgsi_init(ctx,  CGSI_OPT_DISABLE_NAME_CHECK | CGSI_OPT_SSL_COMPATIBLE);
    soap_set_omode(ctx, SOAP_ENC_MTOM);

	ctx->fmimewriteopen = LogFileStreamer::writeOpen;
	ctx->fmimewriteclose = LogFileStreamer::writeClose;
	ctx->fmimewrite = LogFileStreamer::write;

    if (err) {
    	// TODO
    }

    // set the namespaces
//	if (soap_set_namespaces(ctx, fts3_namespaces)) {
//		// TODO
//	}
}

InternalLogRetriever::~InternalLogRetriever() {
	soap_destroy(ctx);
	soap_end(ctx);
	soap_free(ctx);
}

list<string> InternalLogRetriever::getInternalLogs(string jobId) {

	log__GetLogInternalResponse resp;
	ctx->user = LogFileStreamer::getOutputHandler(resp);
	soap_call_log__GetLogInternal(ctx, endpoint.c_str(), 0, jobId, resp);

	list<string> ret (resp.logs->lognames.begin(), resp.logs->lognames.end());
	return ret;
}

