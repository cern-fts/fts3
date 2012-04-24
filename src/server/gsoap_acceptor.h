/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once

#include <boost/type_traits/is_base_of.hpp>
#include <boost/static_assert.hpp>

#include "server_dev.h"
#include "common/pointers.h"
#include "common/error.h"
#include "common/logger.h"
#include "gsoap_method_handler.h"


FTS3_SERVER_NAMESPACE_START

using namespace FTS3_COMMON_NAMESPACE;

template <class SRV>
class GSoapAcceptor
{
private:
	static const bool RIGHT_BASE = boost::is_base_of<soap, SRV>::value;
	BOOST_STATIC_ASSERT(RIGHT_BASE);

public:
    GSoapAcceptor
    (
        const unsigned int port,
        const std::string& ip
    )
    {
        SOAP_SOCKET sock = _srv.bind (ip.c_str(), port, 0);

        if (sock >= 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Soap service bound to socket " << sock << commit;
        }
        else
        {
            FTS3_COMMON_EXCEPTION_THROW (Err_System ("Unable to bound to socket."));
        }
    }

    boost::shared_ptr< GSoapMethodHandler<SRV> > accept()
    {
        SOAP_SOCKET sock = _srv.accept();
        boost::shared_ptr< GSoapMethodHandler<SRV> > handler;

        if (sock >= 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "New connection, bound to socket " << sock << commit;
            boost::shared_ptr<SRV> service(copyService()/*_srv.copy()*/);
            handler.reset (new GSoapMethodHandler<SRV> (service));
        }
        else
        {
            FTS3_COMMON_EXCEPTION_LOGERROR (Err_System ("Unable to accept connection request."));
        }

        return handler;
    }

private:
    /**
     * Creates a copy of _srv.
     *
     * This method should be used instead of _srv.copy(), because in gSOAP version lower
     * than 2.8.3 is a bug (ID: 3293623) in soap_copy_contex that violates the stack
     * (http://sourceforge.net/tracker/index.php?func=detail&aid=3293623&group_id=52781&atid=468021)!
     * If version 2.8.3 or higher is available, the _srv.copy() is preferred!!!
     *
     * @return a copy of _srv object (the user is responsible for freeing the memory)
     */
    SRV* copyService()
    {
    	// allocate memory, the copying constructor may not be used
    	// because it uses 'soap_copy_contex'!
    	SRV* copy = new SRV();
    	// the copying assignment operator can be used because its
    	// not overloaded
    	*copy = _srv;
    	// indicate that the object is a copy
        copy->state = SOAP_COPY;
        // set error to default value
        copy->error = SOAP_OK;
        // set NULL values
        copy->userid = NULL;
        copy->passwd = NULL;
		#ifdef WITH_NTLM
			copy->ntlm_challenge = NULL;
		#endif
        copy->nlist = NULL;
        copy->blist = NULL;
        copy->clist = NULL;
        copy->alist = NULL;
        copy->attributes = NULL;
        copy->labbuf = NULL;
        copy->lablen = 0;
        copy->labidx = 0;
		#ifdef SOAP_MEM_DEBUG
			soap_init_mht(copy);
		#endif
		#ifdef SOAP_DEBUG
			soap_set_test_logfile(copy, soap->logfile[SOAP_INDEX_TEST]);
			soap_set_sent_logfile(copy, soap->logfile[SOAP_INDEX_SENT]);
			soap_set_recv_logfile(copy, soap->logfile[SOAP_INDEX_RECV]);
		#endif

		// VERY IMPORTANT!
		// Set namespaces, otherwise dispatching will not work!!!
		copy->namespaces = NULL;
		copy->local_namespaces = NULL;
		if (_srv.local_namespaces)
			soap_set_namespaces(copy, _srv.local_namespaces);
		else
			soap_set_namespaces(copy, _srv.namespaces);

		#ifdef WITH_C_LOCALE
		# ifdef WIN32
			copy->c_locale = _create_locale(LC_ALL, "C");
		# else
			copy->c_locale = duplocale(soap->c_locale);
		# endif
		#else
			copy->c_locale = NULL;
		#endif
		#ifdef WITH_OPENSSL
			copy->bio = NULL;
			copy->ssl = NULL;
			copy->session = NULL;
		#endif
		#ifdef WITH_GNUTLS
			copy->session = NULL;
		#endif
		#ifdef WITH_ZLIB
			copy->d_stream = (z_stream*)SOAP_MALLOC(copy, sizeof(z_stream));
			copy->d_stream->zalloc = Z_NULL;
			copy->d_stream->zfree = Z_NULL;
			copy->d_stream->opaque = Z_NULL;
			copy->z_buf = NULL;
		#endif
			copy->header = NULL;
			copy->fault = NULL;
			copy->action = NULL;
		#ifndef WITH_LEAN
		#ifdef WITH_COOKIES
			copy->cookies = soap_copy_cookies(copy, soap);
		#else
			copy->cookies = NULL;
		#endif
		#endif

		// copy plugins
		copy->plugins = NULL;
		for (soap_plugin* p = _srv.plugins; p; p = p->next)
		{
			register soap_plugin *q = (soap_plugin*) SOAP_MALLOC(copy, sizeof(struct soap_plugin));
			if (!q) break;
			DBGLOG(TEST, SOAP_MESSAGE(fdebug, "Copying plugin '%s'\n", p->id));
			*q = *p;
			if (p->fcopy && p->fcopy(copy, q, p))
			{
				DBGLOG(TEST, SOAP_MESSAGE(fdebug, "Could not copy plugin '%s'\n", p->id));
				SOAP_FREE(copy, q);
				break;
			}
			q->next = copy->plugins;
			copy->plugins = q;
		}

    	return copy;
    }

protected:

    static SRV _srv;
};

template <class SRV>
SRV GSoapAcceptor<SRV>::_srv;

FTS3_SERVER_NAMESPACE_END

