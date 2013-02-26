/*
 * TransferFileHandler.h
 *
 *  Created on: Feb 25, 2013
 *      Author: simonm
 */

#ifndef TRANSFERFILEHANDLER_H_
#define TRANSFERFILEHANDLER_H_


#include "db/generic/TransferFiles.h"

#include <list>
#include <map>
#include <set>
#include <string>

#include <boost/optional.hpp>

namespace fts3 {
namespace server {

using namespace std;
using namespace boost;

typedef pair<string, int> FileIndex;

class TransferFileHandler {

public:

	TransferFileHandler(map< string, list<TransferFiles*> >& files);
	virtual ~TransferFileHandler();

	TransferFiles* get(string vo);

	set<string>::iterator begin();
	set<string>::iterator end();

	bool empty();

private:

	TransferFiles* getFile(FileIndex index);

	optional<FileIndex> getIndex(string vo);

	map< FileIndex, list<TransferFiles*> > fileIndexToFiles;

	map< string, list<FileIndex> > voToFileIndexes;

	set<string> vos;

	void freeList(list<TransferFiles*>& l);

};

} /* namespace cli */
} /* namespace fts3 */
#endif /* TRANSFERFILEHANDLER_H_ */
