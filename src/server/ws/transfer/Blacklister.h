/*
 * Blacklister.h
 *
 *  Created on: Mar 27, 2013
 *      Author: simonm
 */

#ifndef BLACKLISTER_H_
#define BLACKLISTER_H_

#include <string>

#include <boost/optional.hpp>

#include "db/generic/SingleDbInstance.h"

#include "ws-ifce/gsoap/gsoap_stubs.h"

namespace fts3 {
namespace ws {

using namespace std;
using namespace boost;
using namespace db;

class Blacklister {

public:

	Blacklister(soap* ctx, string name, string status, int timeout, bool blk);

	Blacklister(soap* ctx, string name, string vo, string status, int timeout, bool blk);

	virtual ~Blacklister();

	void executeRequest();

private:

	void handleSeBlacklisting();
	void handleDnBlacklisting();

	void handleJobsInTheQueue();

	string adminDn;

	optional<string> vo;
	string name;
	string status;
	int timeout;
	bool blk;

	GenericDbIfce* db;

};

} /* namespace ws */
} /* namespace fts3 */
#endif /* BLACKLISTER_H_ */
