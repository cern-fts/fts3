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
 * ProxyCertificateDelegator.h
 *
 *  Created on: Jun 4, 2012
 *      Author: simonm
 */

#ifndef ProxyCertificateDelegator_H_
#define ProxyCertificateDelegator_H_

#include <string>
#include <delegation-cli/delegation-simple.h>

using namespace std;

class ProxyCertificateDelegator {
public:
	ProxyCertificateDelegator(string endpoint, string delegationId, int userRequestedDelegationExpTime);
	virtual ~ProxyCertificateDelegator();

	bool delegate();
	long isCertValid(string filename);

	static const int REDELEGATION_TIME_LIMIT = 3600*6;
	static const int MAXIMUM_TIME_FOR_DELEGATION_REQUEST = 3600 * 24;

private:
	string delegationId;
	string endpoint;
	int userRequestedDelegationExpTime;
	glite_delegation_ctx *dctx;
};

#endif /* ProxyCertificateDelegator_H_ */
