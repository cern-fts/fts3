/*
 * SiteNameRetriever.h
 *
 *  Created on: Feb 6, 2013
 *      Author: simonm
 */

#ifndef SITENAMERETRIEVER_H_
#define SITENAMERETRIEVER_H_

#include <map>
#include <string>

#include <boost/thread.hpp>
#include <boost/optional.hpp>

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

	optional<string> getSiteName(string se);

private:

	mutex m;
	map<string, string> seToSite;

	BdiiBrowser& bdii;
	OsgParser myosg;
};

} /* namespace infosys */
} /* namespace fts3 */
#endif /* SITENAMERETRIEVER_H_ */
