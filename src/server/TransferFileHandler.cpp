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
 * TransferFileHandler.cpp
 *
 *  Created on: Feb 25, 2013
 *      Author: simonm
 */

#include "TransferFileHandler.h"

namespace fts3 {
namespace server {

TransferFileHandler::TransferFileHandler(map< string, list<TransferFiles*> >& files) {

	map<string, list<TransferFiles*> >::iterator it_v;

	map< string, set<FileIndex> > unique;

	// iterate over all VOs
	for (it_v = files.begin(); it_v != files.end(); ++it_v) {
		// the vo name
		string vo = it_v->first;
		// unique vo names
		vos.insert(vo);
		// ref to the list of files (for the given VO)
		list<TransferFiles*>& tfs = it_v->second;
		list<TransferFiles*>::iterator it_tf;
		// iterate over all files in a given VO
		for (it_tf = tfs.begin(); it_tf != tfs.end(); ++it_tf) {
			// create index (job ID + file index)
			FileIndex index((*it_tf)->JOB_ID, (*it_tf)->FILE_INDEX);
			// file index to files mapping
			fileIndexToFiles[index].push_back(*it_tf);
			// check if the mapping for the VO already exists
			if (!unique[vo].count(index)) {
				// if not creat it
				unique[vo].insert(index);
				voToFileIndexes[it_v->first].push_back(index);
			}
		}
	}
}

TransferFileHandler::~TransferFileHandler() {

	map< FileIndex, list<TransferFiles*> >::iterator it;
	for (it = fileIndexToFiles.begin(); it != fileIndexToFiles.end(); ++it) {
		freeList(it->second);
	}
}

TransferFiles* TransferFileHandler::get(string vo) {
	// get the index of the next File in turn for the VO
	optional<FileIndex> index = getIndex(vo);
	// if the index exists return the file
	if (index) return getFile(*index);
	// otherwise return null
	return 0;
}

optional<FileIndex> TransferFileHandler::getIndex(string vo) {

	// find the item
	map<string, list<FileIndex> >::iterator it = voToFileIndexes.find(vo);

	// if the VO has no mapping or no files are assigned to the VO ...
	if (it == voToFileIndexes.end() || it->second.empty()) return optional<FileIndex>();

	// get the index value
	FileIndex index = it->second.front();
	it->second.pop_front();

	// if there are no more values assigned to the VO remove it from the mapping
	if (it->second.empty()) {
		voToFileIndexes.erase(it);
	}

	return index;
}

TransferFiles* TransferFileHandler::getFile(FileIndex index) {

	// if there's no mapping for this index ..
	if (fileIndexToFiles.find(index) == fileIndexToFiles.end()) return 0;

	// the return value
	TransferFiles* ret = 0;

	// TODO for now give the first one in the future a selection strategy should be implemented
	if (!fileIndexToFiles[index].empty()) {
		// get the first in the list
		ret = fileIndexToFiles[index].front();
		// remove it from the list
		fileIndexToFiles[index].pop_front();
	}

	return ret;
}

void TransferFileHandler::freeList(list<TransferFiles*>& l) {
	// iterate over the list
	list<TransferFiles*>::iterator it;
	for (it = l.begin(); it != l.end(); ++it) {
		// and release the memory
		delete *it;
	}
	// clear the list
	l.clear();
}

set<string>::iterator TransferFileHandler::begin() {
	return vos.begin();
}

set<string>::iterator TransferFileHandler::end() {
	return vos.end();
}

bool TransferFileHandler::empty() {
	return voToFileIndexes.empty();
}

} /* namespace cli */
} /* namespace fts3 */
