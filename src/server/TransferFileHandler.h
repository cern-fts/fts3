/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

using namespace db;

typedef std::pair<std::string, int> FileIndex;

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

    TransferFileHandler(std::map< std::string, std::list<TransferFiles> >& files);
    virtual ~TransferFileHandler();

    boost::optional<TransferFiles> get(std::string vo);

    std::set<std::string>::iterator begin();
    std::set<std::string>::iterator end();

    bool empty();

    int size();

    const std::set<std::string> getSources(std::string se) const;
    const std::set<std::string> getDestinations(std::string se) const;

    const std::set<std::string> getSourcesVos(std::string se) const;
    const std::set<std::string> getDestinationsVos(std::string se) const;

private:

    boost::optional<TransferFiles> getFile(FileIndex index);

    boost::optional<FileIndex> getIndex(std::string vo);

    boost::optional< std::pair<std::string, std::string> > getNextPair(std::string vo);

    std::map< std::string, std::set<std::string> >& getMapFromCache(std::map< std::string, std::list<TransferFiles> >& files, GET_MAP_OPTS opt);

    // maps file indexes to file replicas
    std::map< FileIndex, std::list<TransferFiles> > fileIndexToFiles;

    // maps VOs to file indexes
    // file indexes are organized in map: source-destination pair is mapped to file indexes
    std::map< std::string, std::map< std::pair<std::string, std::string>, std::list<FileIndex> > > voToFileIndexes;

    std::set<std::string> vos;

    void freeList(std::list<TransferFiles>& l);

    /// mutex that ensures thread safety
    boost::mutex m;

    /// next pair in given VO queue
    std::map< std::string, std::map< std::pair<std::string, std::string>, std::list<FileIndex> >::iterator > nextPairForVo;

    /// cache for the following maps
    std::vector< std::map< std::string, std::set<std::string> > > init_cache;

    /// outgoing/incoming transfers/vo for a given SE
    const std::map< std::string, std::set<std::string> > sourceToDestinations;
    const std::map< std::string, std::set<std::string> > sourceToVos;
    const std::map< std::string, std::set<std::string> > destinationToSources;
    const std::map< std::string, std::set<std::string> > destinationToVos;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* TRANSFERFILEHANDLER_H_ */
