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

	Configuration(string dn);
	virtual ~Configuration();

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

	virtual string json() = 0;
	virtual void save() = 0;
	virtual void del() = 0;

	static const string any;
	static const string wildcard;
	static const string on;
	static const string off;

protected:

	set<string> notAllowed;

	/// Pointer to the 'GenericDbIfce' singleton
	GenericDbIfce* db;

	static string json(map<string, int> params);
	static string json(vector<string> members);

	map<string, int> getProtocolMap(string source, string destination);
	map<string, int> getProtocolMap(LinkConfig* cfg);
	map<string, int> getShareMap(string source, string destination);

	void addSe(string se, bool active = true);
	void eraseSe(string se);
	void addGroup(string group, vector<string>& members);
	void checkGroup(string group);

	void addLinkCfg(string source, string destination, bool active, string symbolic_name, map<string, int>& protocol);
	void addShareCfg(string source, string destination, map<string, int>& share);

	void delLinkCfg(string source, string destination);
	void delShareCfg(string source, string destination);

	///
	string all;

	/// number of SQL updates triggered by configuration command
	int updateCount;
	/// number of SQL inserts triggered by configuration command
	int insertCount;
	/// number of SQL deletes triggered by configuration command
	int deleteCount;

private:

	///
	string dn;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* CONFIGURATION_H_ */
