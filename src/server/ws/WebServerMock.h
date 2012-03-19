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
 * WebServerMock.h
 *
 *  Created on: Feb 24, 2012
 *      Author: Michal Simon
 */

#ifndef WEBSERVERMOCK_H_
#define WEBSERVERMOCK_H_

#include <pthread.h>
#include "gsoap_stubs.h"

using namespace std;

namespace fts3 { namespace ws {

/**
 *
 */
class WebServerMock {
public:
	virtual ~WebServerMock();
	void run(int port);

	static const string INTERFACE;
	static const string VERSION;
	static const string SCHEMA;
	static const string METADATA;
	static const string ID;

	static WebServerMock* getInstance();

private:

	WebServerMock();

	static void *processRequest(void *ptr);

	pthread_t tid;
	int port;

	static WebServerMock* me;
};

}
}

#endif /* WEBSERVERMOCK_H_ */
