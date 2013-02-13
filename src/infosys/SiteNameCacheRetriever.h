/*
 * SiteNameCacheRetriver.h
 *
 *  Created on: Feb 13, 2013
 *      Author: simonm
 */

#ifndef SITENAMECACHERETRIVER_H_
#define SITENAMECACHERETRIVER_H_

#include <map>
#include <string>

#include <boost/property_tree/ptree.hpp>

#include "BdiiBrowser.h"
#include "OsgParser.h"

#include "common/ThreadSafeInstanceHolder.h"

namespace fts3 {
namespace infosys {

using namespace std;
using namespace boost;
using namespace boost::property_tree;

class SiteNameCacheRetriever: public ThreadSafeInstanceHolder<SiteNameCacheRetriever> {

	friend class ThreadSafeInstanceHolder<SiteNameCacheRetriever>;

public:

	virtual ~SiteNameCacheRetriever();

	ptree get();

private:

	SiteNameCacheRetriever() {};
	SiteNameCacheRetriever(SiteNameCacheRetriever const&);
	SiteNameCacheRetriever& operator=(SiteNameCacheRetriever const&);

	void fromGlue1();
	void fromGlue2();

	map<string, string> cache;

	// glue1
	static const char* ATTR_GLUE1_SERVICE;
	static const char* ATTR_GLUE1_LINK;
	static const char* ATTR_GLUE1_SITE;

	static const string FIND_SE_SITE_GLUE1;
	static const char* FIND_SE_SITE_ATTR_GLUE1[];
};

} /* namespace infosys */
} /* namespace fts3 */
#endif /* SITENAMECACHERETRIVER_H_ */
