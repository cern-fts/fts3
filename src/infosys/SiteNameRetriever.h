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
 * SiteNameRetriever.h
 *
 *  Created on: Feb 6, 2013
 *      Author: Michal Simon
 */

#ifndef SITENAMERETRIEVER_H_
#define SITENAMERETRIEVER_H_

#include <map>
#include <string>

#include <boost/thread.hpp>

#include "BdiiBrowser.h"
#include "OsgParser.h"

#include "common/ThreadSafeInstanceHolder.h"

namespace fts3 {
namespace infosys {

using namespace std;
using namespace boost;

class SiteNameRetriever: public ThreadSafeInstanceHolder<SiteNameRetriever>   {

public:
	SiteNameRetriever();
	virtual ~SiteNameRetriever();

	string getSiteName(string se);

private:

	mutex m;
	map<string, string> seToSite;

	BdiiBrowser& bdii;
	OsgParser& myosg;
};

} /* namespace infosys */
} /* namespace fts3 */
#endif /* SITENAMERETRIEVER_H_ */
