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
 
#include "gsoap_acceptor.h"
#include "gsoap_request_handler.h"
#include "ws-ifce/gsoap/fts3.nsmap"

#include <cgsi_plugin.h>

FTS3_SERVER_NAMESPACE_START

GSoapAcceptor::GSoapAcceptor(const unsigned int port, const std::string& ip) {

	ctx = soap_new();
	soap_cgsi_init(ctx,  CGSI_OPT_SERVER | CGSI_OPT_SSL_COMPATIBLE | CGSI_OPT_DISABLE_MAPPING);// | CGSI_OPT_DISABLE_NAME_CHECK);
	soap_set_namespaces(ctx, fts3_namespaces);

	SOAP_SOCKET sock = soap_bind(ctx, ip.c_str(), port, 100);

    if (sock >= 0) {
        FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Soap service bound to socket " << sock << commit;
    } else {
        FTS3_COMMON_EXCEPTION_THROW (Err_System ("Unable to bound to socket."));
        exit(1);
    }
}

GSoapAcceptor::~GSoapAcceptor() {

	soap* tmp;
	while (!recycle.empty()) {

		tmp = recycle.front();
		recycle.pop();

		soap_done(tmp);
		soap_free(tmp);
	}

	soap_destroy(ctx);
	soap_end(ctx);
	soap_free(ctx);
}

boost::shared_ptr<GSoapRequestHandler> GSoapAcceptor::accept() {
    SOAP_SOCKET sock = soap_accept(ctx);
    boost::shared_ptr<GSoapRequestHandler> handler;

    if (sock >= 0) {

        FTS3_COMMON_LOGGER_NEWLOG (INFO) << "New connection, bound to socket " << sock << commit;
        handler.reset (
        		new GSoapRequestHandler(*this)
        	);
    } else {
        FTS3_COMMON_EXCEPTION_LOGERROR (Err_System ("Unable to accept connection request."));
    }

    return handler;
}

soap* GSoapAcceptor::getSoapContext() {

	FTS3_COMMON_MONITOR_START_CRITICAL
	if (!recycle.empty()) {
		soap* ctx = recycle.front();
		recycle.pop();
		ctx->socket = this->ctx->socket;
		return ctx;
	}
	FTS3_COMMON_MONITOR_END_CRITICAL

	return soap_copy(ctx);
}

void GSoapAcceptor::recycleSoapContext(soap* ctx) {

	FTS3_COMMON_MONITOR_START_CRITICAL
	soap_destroy(ctx);
	soap_end(ctx);
	recycle.push(ctx);
	FTS3_COMMON_MONITOR_END_CRITICAL
}

FTS3_SERVER_NAMESPACE_END
