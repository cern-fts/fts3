/*
 * InternalLogRetriever.h
 *
 *  Created on: Oct 18, 2012
 *      Author: simonm
 */

#ifndef INTERNALLOGRETRIEVER_H_
#define INTERNALLOGRETRIEVER_H_

#include "ws-ifce/gsoap/gsoap_stubs.h"

#include <string>
#include <list>

namespace fts3 { namespace ws {

using namespace std;

class InternalLogRetriever {

public:
	InternalLogRetriever(string endpoint);
	virtual ~InternalLogRetriever();

	list<string> getInternalLogs(string jobId);

private:
	///
	string endpoint;

	///
	soap* ctx;
};

}
}

#endif /* INTERNALLOGRETRIEVER_H_ */
