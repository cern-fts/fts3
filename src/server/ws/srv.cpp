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
 */

#include "GsoapStubs.h"
#include "UuidGenerator.h"
#include <iostream>
#include "FtsServiceTask.h"

#include "evn.h"
#include "fts.nsmap"
#include <pthread.h>

using namespace fts::ws;

void *process_request(void *soap) {

   pthread_detach(pthread_self());

   FileTransferSoapBindingService* dup = (FileTransferSoapBindingService*) soap;
   FtsServiceTask task;
   task(dup);
   return 0;
}


int main(int ac, char* av[]) {

    int ret;
    FileTransferSoapBindingService srv;

    srv.send_timeout = 60; // 60 seconds
    srv.recv_timeout = 60; // 60 seconds
    srv.accept_timeout = 3600; // server stops after 1 hour of inactivity
    srv.max_keep_alive = 100; // max keep-alive sequence

    ret = srv.bind("localhost", 8443, 0);
    if (ret < 0) {
    	cout << "busy" << endl;
    	return 0;
    }
    do {
		ret = srv.accept();
		if (ret < 0) {
	    	cout << "busy" << endl;
	    	return 0;
	    }

		cout << "rqst accepted" << endl;
		FileTransferSoapBindingService* copy = FtsServiceTask::copyService(srv);

		cout << "create new thread" << endl;
		// in new thread
		pthread_t tid;
		pthread_create(&tid, NULL, process_request, (void*)copy);

    } while (true);

    return 0;
}
