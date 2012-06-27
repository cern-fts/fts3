/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use soap file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implcfgied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * ConfigurationHandler.h
 *
 *  Created on: Jun 26, 2012
 *      Author: simonm
 */

#ifndef CONFIGURATIONHANDLER_H_
#define CONFIGURATIONHANDLER_H_

#include <string>
#include <vector>

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

#include "common/error.h"
#include "common/logger.h"

#include "db/generic/SingleDbInstance.h"

namespace fts3 { namespace ws {

using namespace boost;
using namespace std;
using namespace fts3::common;
using namespace db;

class ConfigurationHandler {
public:

	enum {
		ALL = 0,
		NAME,
		TYPE,
		MEMBERS = 4,
		PROTOCOL_PARAMETERS = 6,
		BANDWIDTH,
		NOSTREAMS,
		TCP_BUFFER_SIZE,
		NOMINAL_THROUGHPUT,
		BLOCKSIZE,
		HTTP_TO,
		URLCOPY_PUT_TO,
		URLCOPY_PUTDONE_TO,
		URLCOPY_GET_TO,
		URLCOPY_GET_DONETO,
		URLCOPY_TX_TO,
		URLCOPY_TXMARKS_TO,
		URLCOPY_FIRST_TXMARK_TO,
		TX_TO_PER_MB,
		NO_TX_ACTIVITY_TO,
		PREPARING_FILES_RATIO,
		SHARE = 23,
		SHARE_TYPE,
		SHARE_ID = 26,
		IN = 28,
		OUT,
		POLICY
	};

	static const string SE_TYPE;
	static const string GROUP_TYPE;

	static const string PUBLIC_SHARE;
	static const string VO_SHARE;
	static const string PAIR_SHARE;

	ConfigurationHandler(string configuration);
	virtual ~ConfigurationHandler();


	// retrieve data from sub-configuration strings
	int getNumber(string cfg);
	string getString(string cfg );
	vector<string> getVector(string cfg);

	// checks if the SE exists, if not adds it to the DB
	void addSeIfNotExist(string name);

	// add configuration data
	void add();
	void addSeConfiguration();
	void addGroupConfiguration();
	void addShareConfiguration();

	shared_ptr<SeProtocolConfig> getProtocolConfig();

private:

	// regular expressions
	static const string cfg_exp;
	static const string get_num_exp;
	static const string get_str_exp;
	static const string get_vec_exp;

	// JSON null string = 'null'
	static const string null_str;

	// regex objects corresponding to each expression
	regex cfg_re;
	regex get_num_re;
	regex get_str_re;
	regex get_vec_re;

	// DB singleton
	GenericDbIfce* db;

	// data extracted from configuration
	string name;
	string type;
	vector<string> members;
	int bandwidth;
	int nostreams;
	int tcp_buffer_size;
	int nominal_throughput;
	int blocksize;
	int http_to;
	int urlcopy_put_to;
	int urlcopy_putdone_to;
	int urlcopy_get_to;
	int urlcopy_get_doneto;
	int urlcopy_tx_to;
	int urlcopy_txmarks_to;
	int urlcopy_first_txmark_to;
	int tx_to_per_mb;
	int no_tx_activity_to;
	int preparing_files_ratio;
	string share_type;
	string share_id;
	int in;
	int out;
	string policy;

	bool cfgMembers;
	bool cfgProtocolParams;
	bool cfgShare;
};

}
}

#endif /* TEST_H_ */
