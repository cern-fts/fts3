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
 * GSoapExceptionHandler.h
 *
 *  Created on: Mar 8, 2012
 *      Author: Michal Simon
 */

#ifndef GSOAPEXCEPTIONHANDLER_H_
#define GSOAPEXCEPTIONHANDLER_H_

#include "gsoap_transfer_stubs.h"
#include <string>

using namespace std;

namespace fts3 { namespace ws {

/**
 * The GSoapExceptionHandler class takes care of caught exceptions that inherit after tns3__TransferException.
 *
 * Since, the soap object supports only pointers to exception objects, only pointers to exception object should be used!
 * All tns3__TransferException* objects have to be created using gSOAP specific functions, so they can automatically
 * garbage collected later.
 *
 */
class GSoapExceptionHandler {
public:
	/**
	 * Constructor - creates a GSoapExceptionHandler object.
	 *
	 * @param soap - the soap object that is serving the given request
	 * @param ex - the exception that has to be handled
	 *
	 */
	GSoapExceptionHandler(::soap* soap, tns3__TransferException* ex): ex(ex), soap(soap){};

	/**
	 * Destructor.
	 */
	virtual ~GSoapExceptionHandler() {};

	/**
	 * Handles the exception!
	 */
	void handle();

	/**
	 * Creates a tns3__InvalidArgumentException exception.
	 *
	 * The exception object is created using gSOAP memory-allocation utility, it will be garbage
	 * collected! If there is a need to delete it manually gSOAP dedicated functions should
	 * be used (in particular 'soap_unlink'!).
	 *
	 * The method is thread safe since it uses no internal fields of GSoapExceptionHandler, and
	 * each thread has its own copy of the soap object.
	 *
	 * @param soap - the soap object that is serving the given request
	 * @param msg - the error message of the exception
	 *
	 * @return pointer to the tns3__InvalidArgumentException
	 */
	inline static tns3__InvalidArgumentException* createInvalidArgumentException (::soap* soap, string msg) {

		tns3__InvalidArgumentException* ex;
		ex = soap_new_tns3__InvalidArgumentException(soap, -1);
		ex->message = soap_new_std__string(soap, -1);
		*ex->message = msg;

		return ex;
	}

	/**
	 * Creates a tns3__AuthorizationException exception.
	 *
	 * The exception object is created using gSOAP memory-allocation utility, it will be garbage
	 * collected! If there is a need to delete it manually gSOAP dedicated functions should
	 * be used (in particular 'soap_unlink'!).
	 *
	 * The method is thread safe since it uses no internal fields of GSoapExceptionHandler, and
	 * each thread has its own copy of the soap object.
	 *
	 * @param soap - the soap object that is serving the given request
	 * @param msg - the error message of the exception
	 *
	 * @return pointer to the tns3__AuthorizationException
	 */
	inline static tns3__AuthorizationException* createAuthorizationException (::soap* soap, string msg) {

		tns3__AuthorizationException* ex;
		ex = soap_new_tns3__AuthorizationException(soap, -1);
		ex->message = soap_new_std__string(soap, -1);
		*ex->message = msg;

		return ex;
	}

	/**
	 * Creates a tns3__NotExistsException exception.
	 *
	 * The exception object is created using gSOAP memory-allocation utility, it will be garbage
	 * collected! If there is a need to delete it manually gSOAP dedicated functions should
	 * be used (in particular 'soap_unlink'!).
	 *
	 * The method is thread safe since it uses no internal fields of GSoapExceptionHandler, and
	 * each thread has its own copy of the soap object.
	 *
	 * @param soap - the soap object that is serving the given request
	 * @param msg - the error message of the exception
	 *
	 * @return pointer to the tns3__AuthorizationException
	 */
	inline static tns3__NotExistsException* createNotExistsException (::soap* soap, string msg) {

		tns3__NotExistsException* ex;
		ex = soap_new_tns3__NotExistsException(soap, -1);
		ex->message = soap_new_std__string(soap, -1);
		*ex->message = msg;

		return ex;
	}

private:
	/**
	 * Default constructor.
	 *
	 * Private - should not be used.
	 */
	GSoapExceptionHandler(){};

	/**
	 * Handles exceptions that were caused by the client side (e.g. wrong arguments were given)
	 */
	void handleSenderFault();

	/**
	 * Handles exceptions that were caused by the server side (e.g. service busy)
	 */
	void handleReceiverFault();

	/**
	 * Checks the class of the exception object and dispatches the
	 * object to the right field in SOAP_ENV__Detail struct
	 */
	void dispatchException ();

	/// soap version 1.2
	static const int v12 = 2;

	/// the exception object
	tns3__TransferException* ex;

	/// the soap object that is serving the given request
	::soap* soap;

};

}
}

#endif /* GSOAPEXCEPTIONHANDLER_H_ */
