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
 */

#include "gsoap_stubs.h"
#include "uuid_generator.h"

#include "db/generic/SingleDbInstance.h"

#include "config/serverconfig.h"
#include "FtsServiceTask.h"

using namespace fts::ws;
using namespace boost;
using namespace db;
using namespace fts3::config;


vector<string> FtsServiceTask::getParams(transfer__TransferJob *_job, int & copyPinLifeTime) {

	if (index.empty()) {
    	index.insert(pair<string, int>("gridftp", 0));
    	index.insert(pair<string, int>("myproxy", 1));
    	index.insert(pair<string, int>("delegationid", 2));
    	index.insert(pair<string, int>("spacetoken", 3));
    	index.insert(pair<string, int>("overwrite", 4));
    	index.insert(pair<string, int>("source_spacetoken", 5));
    	index.insert(pair<string, int>("lan_connection", 6));
    	index.insert(pair<string, int>("fail_nearline", 7));
    	index.insert(pair<string, int>("checksum_method", 8));
	}

	vector<string> params (index.size());
	if (!_job->jobParams) return params;

	vector<string>::iterator key_it = _job->jobParams->keys.begin();
	vector<string>::iterator val_it = _job->jobParams->values.begin();
	map<string, int>::iterator index_it;

	for (; key_it < _job->jobParams->keys.end(); key_it++, val_it++) {
		if (key_it->compare("copy_pin_lifetime") == 0) {
			copyPinLifeTime = lexical_cast<int>(*val_it);
		} else {
			index_it = index.find(*key_it);
			if (index_it != index.end()) {
				params[index_it->second] = *val_it;
			}
		}
	}

	return params;
}

map<string, int> FtsServiceTask::index;


