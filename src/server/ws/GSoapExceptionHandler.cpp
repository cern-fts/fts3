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
 * GSoapExceptionHandler.cpp
 *
 *  Created on: Mar 8, 2012
 *      Author: Michal Simon
 */

#include "GSoapExceptionHandler.h"

using namespace fts3::ws;


void GSoapExceptionHandler::handle() {
	handleReceiverFault(); // TODO check which exceptions are sender and which receiver faults
}

void GSoapExceptionHandler::handleSenderFault() {
	// sets the faultstring field in SOAP_ENV__Fault struct
	// also allocates SOAP_ENV__Detail struct and sets _any field
	soap_sender_fault(soap, ex->message->c_str(), "TransferException");
	dispatchException();
}

void GSoapExceptionHandler::handleReceiverFault() {
	// sets the faultstring field in SOAP_ENV__Fault struct
	// also allocates SOAP_ENV__Detail struct and sets _any field
	soap_receiver_fault(soap, ex->message->c_str(), "TransferException");
	dispatchException();
}

void GSoapExceptionHandler::dispatchException() {

    SOAP_ENV__Detail *detail = NULL;
    transfer::SOAP_ENV__Fault *fault = NULL;
	if (soap->version == v12) {
		// soap version 1.2
		detail = soap->fault->SOAP_ENV__Detail;
	} else {
		// soap version 1.1
		detail = soap->fault->detail;
	}

	// dispatch the exception to the right field in SOAP_ENV_DETAIL

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
}
