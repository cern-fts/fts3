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
#include "ws-ifce/gsoap/fts3.nsmap"

FTS3_SERVER_NAMESPACE_START

GSoapAcceptor::GSoapAcceptor(const unsigned int port, const std::string& ip) {

	soap = soap_new();
	soap_set_namespaces(soap, fts3_namespaces);

	SOAP_SOCKET sock = soap_bind(soap, ip.c_str(), port, 100);

    if (sock >= 0)
    {
        FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Soap service bound to socket " << sock << commit;
    }
    else
    {
        FTS3_COMMON_EXCEPTION_THROW (Err_System ("Unable to bound to socket."));
    }
}

GSoapAcceptor::~GSoapAcceptor() {
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
}

boost::shared_ptr<GSoapRequestHandler> GSoapAcceptor::accept() {
    SOAP_SOCKET sock = soap_accept(soap);
    boost::shared_ptr<GSoapRequestHandler> handler;

    if (sock >= 0)
    {
        FTS3_COMMON_LOGGER_NEWLOG (INFO) << "New connection, bound to socket " << sock << commit;
        handler.reset (new GSoapRequestHandler (soap_copy(soap)));
    }
    else
    {
        FTS3_COMMON_EXCEPTION_LOGERROR (Err_System ("Unable to accept connection request."));
    }

    return handler;
}

FTS3_SERVER_NAMESPACE_END
