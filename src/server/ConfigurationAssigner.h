/*
 * ConfigurationAssigner.h
 *
 *  Created on: Dec 10, 2012
 *      Author: simonm
 */

#ifndef CONFIGURATIONASSIGNER_H_
#define CONFIGURATIONASSIGNER_H_

#include "db/generic/SingleDbInstance.h"

#include <string>
#include <list>

#include <boost/tuple/tuple.hpp>

namespace fts3 {
namespace server {

using namespace db;
using namespace std;
using namespace boost;

class ConfigurationAssigner {

	enum {
		SHARE,
		CONTENT
	};

	enum {
		SOURCE = 0,
		DESTINATION,
		VO
	};

	typedef tuple<string, string, string> share;
	typedef pair<bool, bool> content;
	typedef tuple< share, content > cfg_type;

public:
	ConfigurationAssigner(TransferFiles* file);
	virtual ~ConfigurationAssigner();

	bool assign();

private:

	TransferFiles* file;
	GenericDbIfce* db;

	int assign_count;

	bool assignShareCfg(list<cfg_type> arg);

	/**
	 *
	 */
	string fileUrlToSeName(string url);
};

} /* namespace server */
} /* namespace fts3 */
#endif /* CONFIGURATIONASSIGNER_H_ */
