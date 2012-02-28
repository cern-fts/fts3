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

/*
 * FtsService.h
 *
 *  Created on: Feb 18, 2012
 *      Author: simonm
 */

#ifndef FTSSERVICE_H_
#define FTSSERVICE_H_

#include <string>
#include "GsoapStubs.h"

using namespace std;

namespace fts { namespace ws {

/**
 * FtsServiceTask is a functional object meant to serve FTS3 soap requests
 */
class FtsServiceTask {
public:
	/**
	 * Default constructor.
	 */
	FtsServiceTask();

	/**
	 * Destructor.
	 */
	virtual ~FtsServiceTask();

	/**
	 * Function call operator.
	 *
	 * @param copy - copy of FTS3 service proxy, the object will be deleted after use
	 */
	void operator()(FileTransferSoapBindingService* copy);

	/**
	 * Copy Assignment operator
	 */
	FtsServiceTask& operator= (const FtsServiceTask& other);


	static map<string, int> index;

	static vector<string> getParams(transfer__TransferJob *_job, int & copyPinLifeTime);

	/**
	 * Copies FTS3 proxy.
	 *
	 * @param srv - FTS3 proxy object
	 *
	 * @return a copy of FTS3 proxy object (the object is not garbage collected!)
	 */
	//static FileTransferSoapBindingService* copyService(FileTransferSoapBindingService& srv);
};

}
}

#endif /* FTSSERVICE_H_ */
