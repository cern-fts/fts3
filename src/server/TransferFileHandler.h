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
 *
 * TransferFileHandler.h
 *
 *  Created on: Feb 25, 2013
 *      Author: simonm
 */

#ifndef TRANSFERFILEHANDLER_H_
#define TRANSFERFILEHANDLER_H_


#include "db/generic/SingleDbInstance.h"

#include <list>
#include <map>
#include <set>
#include <string>

#include <boost/optional.hpp>

namespace fts3 {
namespace server {

using namespace std;
using namespace boost;
using namespace db;

typedef pair<string, int> FileIndex;

class TransferFileHandler {

public:

	TransferFileHandler(map< string, list<TransferFiles*> >& files);
	virtual ~TransferFileHandler();

	TransferFiles* get(string vo);

	set<string>::iterator begin();
	set<string>::iterator end();

	bool empty();

	void remove(string source, string destination);

private:

	TransferFiles* getFile(FileIndex index);

	optional<FileIndex> getIndex(string vo);

	map< FileIndex, list<TransferFiles*> > fileIndexToFiles;

	map< string, list<FileIndex> > voToFileIndexes;

	set<string> vos;

	set< pair<string, string> > notScheduled;

	void freeList(list<TransferFiles*>& l);

	/// DB interface
	GenericDbIfce* db;

};

} /* namespace cli */
} /* namespace fts3 */
#endif /* TRANSFERFILEHANDLER_H_ */
