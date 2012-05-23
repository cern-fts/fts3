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
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or impltnsied.
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

#include "gsoap_stubs.h"
#include <string>
#include <typeinfo>

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
template<typename EX>
class GSoapExceptionHandler {
public:
	/**
	 * Constructor - creates a GSoapExceptionHandler object.
	 *
	 * @param soap - the soap object that is serving the given request
	 * @param ex - the exception that has to be handled
	 *
	 */
	GSoapExceptionHandler(::soap* soap, EX* ex): ex(ex), soap(soap){};

	/**
	 * Destructor.
	 */
	virtual ~GSoapExceptionHandler() {};

	/**
	 * Handles the exception!
	 */
	void handle() {
		handleReceiverFault(); // TODO check which exceptions are sender and which receiver faults
	}

	/**
	 * General pointer to exception instantiator functions
	 */
	typedef EX* (*soap_instantiate__ExceptionFunction)(::soap*, int, const char*, const char*, size_t *);

	/**
	 * Returns a pointer to a function that creates an exception
	 * 	of a type specified as the template parameter
	 *
	 * 	@return pointer to an exception instantiator
	 */
	inline static soap_instantiate__ExceptionFunction getExceptionInstantiator() {

		if (typeid(EX) == typeid(tns3__InvalidArgumentException)) {
			return (soap_instantiate__ExceptionFunction) &soap_instantiate_tns3__InvalidArgumentException;
		}

		if (typeid(EX) == typeid(tns3__AuthorizationException)) {
			return (soap_instantiate__ExceptionFunction) &soap_instantiate_tns3__AuthorizationException;
		}

		if (typeid(EX) == typeid(config__AuthorizationException)) {
			return (soap_instantiate__ExceptionFunction) &soap_instantiate_config__AuthorizationException;
		}

		if (typeid(EX) == typeid(tns3__InternalException)) {
			return (soap_instantiate__ExceptionFunction) &soap_instantiate_tns3__InternalException;
		}

		if (typeid(EX) == typeid(config__InternalException)) {
			return (soap_instantiate__ExceptionFunction) &soap_instantiate_config__InternalException;
		}

		if (typeid(EX) == typeid(tns3__NotExistsException)) {
			return (soap_instantiate__ExceptionFunction) &soap_instantiate_tns3__NotExistsException;
		}

		if (typeid(EX) == typeid(config__InvalidConfigurationException)) {
			return (soap_instantiate__ExceptionFunction) &soap_instantiate_config__InvalidConfigurationException;
		}

		if (typeid(EX) == typeid(config__ConfigurationException)) {
			return (soap_instantiate__ExceptionFunction) &soap_instantiate_config__ConfigurationException;
		}

		if (typeid(EX) == typeid(tns3__TransferException)) {
			return (soap_instantiate__ExceptionFunction) &soap_instantiate_tns3__TransferException;
		}

		return 0;
	}

	/**
	 * Creates an exception of type EX.
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
	 * @return pointer to the EX exception
	 */
	inline static EX* createException(::soap* soap, string msg) {

		static soap_instantiate__ExceptionFunction soap_instantiate__Exception = 0;
		if (!soap_instantiate__Exception) {
			soap_instantiate__Exception = getExceptionInstantiator();
		}

		if (!soap_instantiate__Exception) return 0;

		EX* ex;
		ex = soap_instantiate__Exception(soap, -1, 0, 0, 0);
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
	void handleSenderFault() {
		// sets the faultstring field in SOAP_ENV__Fault struct
		// also allocates SOAP_ENV__Detail struct and sets _any field
		soap_sender_fault(soap, ex->message->c_str(), "TransferException");
		//dispatchException();
	}

	/**
	 * Handles exceptions that were caused by the server side (e.g. service busy)
	 */
	void handleReceiverFault() {
		// sets the faultstring field in SOAP_ENV__Fault struct
		// also allocates SOAP_ENV__Detail struct and sets _any field
		soap_receiver_fault(soap, ex->message->c_str(), "TransferException");
		//dispatchException();
	}


	/**
	 * Checks the class of the exception object and dispatches the
	 * object to the right field in SOAP_ENV__Detail struct
	 */
	void dispatchException () {

	    SOAP_ENV__Detail *detail = NULL;
	    SOAP_ENV__Fault *fault = soap->fault;//NULL;
		if (soap->version == v12) {
			// soap version 1.2
			detail = fault->SOAP_ENV__Detail;
		} else {
			// soap version 1.1
			detail = fault->detail;
		}

		// dispatch the exception to the right field in SOAP_ENV_DETAIL

		// tns3 exceptions:
		detail->tns3__AuthorizationExceptionElement = dynamic_cast<tns3__AuthorizationException*>(ex);
		if (detail->tns3__AuthorizationExceptionElement) return;

		detail->tns3__CannotCancelExceptionElement = dynamic_cast<tns3__CannotCancelException*>(ex);
		if (detail->tns3__CannotCancelExceptionElement) return;

		detail->tns3__ExistsExceptionElement = dynamic_cast<tns3__ExistsException*>(ex);
		if (detail->tns3__ExistsExceptionElement) return;

		detail->tns3__InternalExceptionElement = dynamic_cast<tns3__InternalException*>(ex);
		if (detail->tns3__InternalExceptionElement) return;

		detail->tns3__InvalidArgumentExceptionElement = dynamic_cast<tns3__InvalidArgumentException*>(ex);
		if (detail->tns3__InvalidArgumentExceptionElement) return;

		detail->tns3__NotExistsExceptionElement = dynamic_cast<tns3__NotExistsException*>(ex);
		if (detail->tns3__NotExistsExceptionElement) return;

		detail->tns3__ServiceBusyExceptionElement = dynamic_cast<tns3__ServiceBusyException*>(ex);
		if(detail->tns3__ServiceBusyExceptionElement) return;

		// config exceptions:
		detail->config__AuthorizationExceptionElement = dynamic_cast<config__AuthorizationException*>(ex);
		if(detail->config__AuthorizationExceptionElement) return;

		detail->config__InternalExceptionElement = dynamic_cast<config__InternalException*>(ex);
		if(detail->config__InternalExceptionElement) return;

		detail->config__InvalidConfigurationExceptionElement = dynamic_cast<config__InvalidConfigurationException*>(ex);
		if(detail->config__InvalidConfigurationExceptionElement) return;

		detail->config__ServiceBusyExceptionElement = dynamic_cast<config__ServiceBusyException*>(ex);
	}

	/// soap version 1.2
	static const int v12 = 2;

	/// the exception object
	EX* ex;

	/// the soap object that is serving the given request
	::soap* soap;
};

}
}

#endif /* GSOAPEXCEPTIONHANDLER_H_ */
