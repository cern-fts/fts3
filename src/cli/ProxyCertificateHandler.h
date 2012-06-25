/*
 * ProxyCertificateHandler.h
 *
 *  Created on: Jun 4, 2012
 *      Author: simonm
 */

#ifndef PROXYCERTIFICATEHANDLER_H_
#define PROXYCERTIFICATEHANDLER_H_

#include <string>
#include <delegation-cli/delegation-simple.h>

using namespace std;

class ProxyCertificateHandler {
public:
	ProxyCertificateHandler(string endpoint, string delegationId, int userDefinedExpTime);
	virtual ~ProxyCertificateHandler();


	// user defined the value should be set from cli
	int user_requested_delegation_exp_time;
	static const int redelegation_time_limit = 3600*6;
	static const int maximum_time_for_delegation_request = 3600 * 24;
	// user defined should be set using cli
	string delegationId;
	string endpoint;
	bool delegate();



	void delegateProxyCert(string);

	long isCertValid(string filename);
};

#endif /* PROXYCERTIFICATEHANDLER_H_ */
