/*
 * Configuration.h
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "common/CfgParser.h"

#include "db/generic/SingleDbInstance.h"

#include <string>
#include <vector>
#include <map>

#include <boost/shared_ptr.hpp>

namespace fts3 {
namespace ws {

using namespace std;
using namespace boost;
using namespace db;
using namespace fts3::common;

class Configuration {

public:

	Configuration(CfgParser& parser);
	virtual ~Configuration() {};

	/**
	 * protocol parameters
	 */
	struct Protocol {
		static const string BANDWIDTH;
		static const string NOSTREAMS;
		static const string TCP_BUFFER_SIZE;
		static const string NOMINAL_THROUGHPUT;
		static const string BLOCKSIZE;
		static const string HTTP_TO;
		static const string URLCOPY_PUT_TO;
		static const string URLCOPY_PUTDONE_TO;
		static const string URLCOPY_GET_TO;
		static const string URLCOPY_GET_DONETO;
		static const string URLCOPY_TX_TO;
		static const string URLCOPY_TXMARKS_TO;
		static const string URLCOPY_FIRST_TXMARK_TO;
		static const string TX_TO_PER_MB;
		static const string NO_TX_ACTIVITY_TO;
		static const string PREPARING_FILES_RATIO;
	};

	virtual string get() = 0;
	virtual void save() = 0;
	virtual void del() = 0;

protected:

	/// Pointer to the 'GenericDbIfce' singleton
	GenericDbIfce* db;

	static string get(map<string, int> params);
	static string get(vector<string> members);

	shared_ptr<SeProtocolConfig> getProtocolConfig(map<string, int> protocol);

	void addSe(string se, bool active = true);

	void addCfg(string symbolic_name, bool active, string source, string destination, pair<string, int> share, map<string, int> protocol);
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* CONFIGURATION_H_ */
