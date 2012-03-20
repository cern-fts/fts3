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
 * RequestLister.h
 *
 *  Created on: Mar 9, 2012
 *      Author: simonm
 */

#ifndef REQUESTLISTER_H_
#define REQUESTLISTER_H_

#include "gsoap_transfer_stubs.h"
#include "db/generic/JobStatus.h"

#include <string>
#include <vector>

using namespace std;

namespace fts3 { namespace ws {

/**
 * RequestLister class takes care of listing submitted requests.
 *
 * Depending on the request (listRequest or listRequest2)
 * different constructors should be used.
 */
class RequestLister {
public:

	/**
	 * Constructor - creates a lister object that should be used
	 * by listRequest requests.
	 *
	 * @param soap - the soap object that is serving the given request
	 * @param inGivenStates - the states of interest that should be listed
	 */
	RequestLister(::soap* soap, impl__ArrayOf_USCOREsoapenc_USCOREstring *inGivenStates);

	/**
	 * Constructor - creates a lister object that should be used
	 * by listRequest2 requests.
	 *
	 * @param soap - the soap object that is serving the given request
	 * @param inGivenStates - the states of interest that should be listed
	 * @param dn - user DN
	 * @param vo - user VO
	 */
	RequestLister(::soap* soap, impl__ArrayOf_USCOREsoapenc_USCOREstring *inGivenStates, string dn, string vo);

	/**
	 * Retrieves job statuses from DB.
	 *
	 * The impl__ArrayOf_USCOREtns3_USCOREJobStatus object is created using gSOAP
	 * memory-allocation utility, it will be garbage collected! If there is a need
	 * to delete it manually gSOAP dedicated functions should be used (in particular
	 * 'soap_unlink'!).
	 *
	 * @return impl__ArrayOf_USCOREtns3_USCOREJobStatus object containing statuses of interest
	 */
	impl__ArrayOf_USCOREtns3_USCOREJobStatus* list();

	/**
	 * Destructor
	 */
	virtual ~RequestLister();

private:
	/**
	 * Default constructor.
	 *
	 * Private, should not be used.
	 */
	RequestLister();

	/**
	 * Check weather the states given in impl__ArrayOf_USCOREsoapenc_USCOREstring
	 * are correct, if not a impl__InvalidArgumentException is thrown
	 *
	 * @param inGivenStates - the states of interest that should be listed
	 */
	void checkGivenStates(impl__ArrayOf_USCOREsoapenc_USCOREstring *inGivenStates);

	/// the job statuses retrived from the DB
	vector<JobStatus*> jobs;

	/// the soap object that is serving the given request
	::soap* soap;
};

}
}

#endif /* REQUESTLISTER_H_ */
