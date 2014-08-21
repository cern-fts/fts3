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
 *      Author: Michal Simon
 */

#ifndef TRANSFERFILEHANDLER_H_
#define TRANSFERFILEHANDLER_H_


#include "db/generic/SingleDbInstance.h"

#include <list>
#include <map>
#include <set>
#include <string>

#include <boost/optional.hpp>

namespace fts3
{
namespace server
{

using namespace std;
using namespace boost;
using namespace db;

typedef pair<string, int> FileIndex;

class TransferFileHandler
{

    enum GET_MAP_OPTS
    {
        SOURCE_TO_DESTINATIONS,
        SOURCE_TO_VOS,
        DESTINATION_TO_SOURCES,
        DESTINATION_TO_VOS
    };

public:

    TransferFileHandler(map< string, list<TransferFiles> >& files);
    virtual ~TransferFileHandler();

    optional<TransferFiles> get(string vo);

    set<string>::iterator begin();
    set<string>::iterator end();

    bool empty();

    int size();

    const set<string> getSources(string se) const;
    const set<string> getDestinations(string se) const;

    const set<string> getSourcesVos(string se) const;
    const set<string> getDestinationsVos(string se) const;

private:

    optional<TransferFiles> getFile(FileIndex index);

    optional<FileIndex> getIndex(string vo);

    optional< pair<string, string> > getNextPair(string vo);

    map< string, set<string> >& getMapFromCache(map< string, list<TransferFiles> >& files, GET_MAP_OPTS opt);

    // maps file indexes to file replicas
    map< FileIndex, list<TransferFiles> > fileIndexToFiles;

    // maps VOs to file indexes
    // file indexes are organized in map: source-destination pair is mapped to file indexes
    map< string, map< pair<string, string>, list<FileIndex> > > voToFileIndexes;

    set<string> vos;

    void freeList(list<TransferFiles>& l);

    /// mutex that ensures thread safety
    mutex m;

    /// next pair in given VO queue
    map< string, map< pair<string, string>, list<FileIndex> >::iterator > nextPairForVo;

    /// cache for the following maps
    vector< map< string, set<string> > > init_cache;

    /// outgoing/incoming transfers/vo for a given SE
    const map< string, set<string> > sourceToDestinations;
    const map< string, set<string> > sourceToVos;
    const map< string, set<string> > destinationToSources;
    const map< string, set<string> > destinationToVos;

    /// DB interface
    GenericDbIfce* db;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* TRANSFERFILEHANDLER_H_ */
