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
 *      Author: Michal Simon
 */

#include "TransferFileHandler.h"

namespace fts3
{
namespace server
{

map< string, set<string> >& TransferFileHandler::getMapFromCache(map< string, list<TransferFiles> >& files, GET_MAP_OPTS opt)
{
    if (init_cache.empty())
        {
            init_cache.resize(4); // there are four maps

            map<string, list<TransferFiles> >::iterator it_v;

            map< string, set<FileIndex> > unique;

            // iterate over all VOs
            for (it_v = files.begin(); it_v != files.end(); ++it_v)
                {
                    // the vo name
                    string vo = it_v->first;
                    // unique vo names
                    vos.insert(vo);
                    // ref to the list of files (for the given VO)
                    list<TransferFiles>& tfs = it_v->second;
                    list<TransferFiles>::iterator it_tf;
                    // iterate over all files in a given VO
                    for (it_tf = tfs.begin(); it_tf != tfs.end(); ++it_tf)
                        {

                            TransferFiles tmp = *it_tf;

                            init_cache[SOURCE_TO_DESTINATIONS][tmp.SOURCE_SE].insert(tmp.DEST_SE);
                            init_cache[SOURCE_TO_VOS][tmp.SOURCE_SE].insert(tmp.VO_NAME);
                            init_cache[DESTINATION_TO_SOURCES][tmp.DEST_SE].insert(tmp.SOURCE_SE);
                            init_cache[DESTINATION_TO_VOS][tmp.DEST_SE].insert(tmp.VO_NAME);

                            // create index (job ID + file index)
                            FileIndex index((*it_tf).JOB_ID, tmp.FILE_INDEX);
                            // file index to files mapping
                            fileIndexToFiles[index].push_back(tmp);
                            // check if the mapping for the VO already exists
                            if (!unique[vo].count(index))
                                {
                                    // if not creat it
                                    unique[vo].insert(index);
                                    voToFileIndexes[it_v->first][make_pair(tmp.SOURCE_SE, tmp.DEST_SE)].push_back(index);
                                }
                        }

                    nextPairForVo[it_v->first] = voToFileIndexes[it_v->first].begin();
                }
        }

    return init_cache[opt];
}


TransferFileHandler::TransferFileHandler(map< string, list<TransferFiles> >& files) :
    sourceToDestinations(getMapFromCache(files, SOURCE_TO_DESTINATIONS)),
    sourceToVos(getMapFromCache(files, SOURCE_TO_VOS)),
    destinationToSources(getMapFromCache(files, DESTINATION_TO_SOURCES)),
    destinationToVos(getMapFromCache(files, DESTINATION_TO_VOS)),
    db (DBSingleton::instance().getDBObjectInstance())
{
    init_cache.clear();
}

TransferFileHandler::~TransferFileHandler()
{
    map< FileIndex, list<TransferFiles> >::iterator it;
    for (it = fileIndexToFiles.begin(); it != fileIndexToFiles.end(); ++it)
        {
            freeList(it->second);
        }
}

optional<TransferFiles> TransferFileHandler::get(string vo)
{
    // get the index of the next File in turn for the VO
    optional<FileIndex> index = getIndex(vo);
    // if the index exists return the file
    if (index) return getFile(*index);

    return optional<TransferFiles>();
}

optional< pair<string, string> > TransferFileHandler::getNextPair(string vo)
{
    // if there are no pairs for the given VO ...
    if (voToFileIndexes[vo].empty()) return optional< pair<string, string> >();

    // if it is the end wrap around
    if (nextPairForVo[vo] == voToFileIndexes[vo].end()) nextPairForVo[vo] = voToFileIndexes[vo].begin();

    // get the iterator
    map< pair<string, string>, list<FileIndex> >::iterator ret = nextPairForVo[vo];

    // set the next pair
    nextPairForVo[vo]++;

    return ret->first;
}

optional<FileIndex> TransferFileHandler::getIndex(string vo)
{
    // find the item
    map<string, map< pair<string, string>, list<FileIndex> > >::iterator it = voToFileIndexes.find(vo);

    // if the VO has no mapping or no files are assigned to the VO ...
    if (it == voToFileIndexes.end() || it->second.empty()) return optional<FileIndex>();

    optional< pair<string, string> > src_dst = getNextPair(vo);

    if (!src_dst.is_initialized()) return optional<FileIndex>();

    // get the index value
    FileIndex index = it->second[*src_dst].front();
    it->second[*src_dst].pop_front();

    // if there are no more values assigned to either VO or pair remove it from respective mapping
    if (it->second[*src_dst].empty())
        {
            it->second.erase(*src_dst);

            if (it->second.empty())
                {
                    voToFileIndexes.erase(it);
                }
        }

    return index;
}

optional<TransferFiles> TransferFileHandler::getFile(FileIndex index)
{
    optional<TransferFiles> ret;
    // if there's no mapping for this index ..
    if (fileIndexToFiles.find(index) == fileIndexToFiles.end()) return ret;

    if (!fileIndexToFiles[index].empty())
        {
            // get the first in the list
            ret = fileIndexToFiles[index].front();
            // remove it from the list
            fileIndexToFiles[index].pop_front();
        }

    return ret;
}

void TransferFileHandler::freeList(list<TransferFiles>& l)
{
    l.clear();
}

set<string>::iterator TransferFileHandler::begin()
{
    return vos.begin();
}

set<string>::iterator TransferFileHandler::end()
{
    return vos.end();
}

bool TransferFileHandler::empty()
{
    return voToFileIndexes.empty();
}

const set<string> TransferFileHandler::getSources(string se) const
{
    map< string, set<string> >::const_iterator it = destinationToSources.find(se);
    if (it != destinationToSources.end())
        {
            return it->second;
        }

    return set<string>();
}

const set<string> TransferFileHandler::getDestinations(string se) const
{
    map< string, set<string> >::const_iterator it = sourceToDestinations.find(se);
    if (it != sourceToDestinations.end())
        {
            return it->second;
        }

    return set<string>();
}


const set<string> TransferFileHandler::getSourcesVos(string se) const
{
    map< string, set<string> >::const_iterator it = destinationToVos.find(se);
    if (it != destinationToVos.end())
        {
            return it->second;
        }

    return set<string>();
}

const set<string> TransferFileHandler::getDestinationsVos(string se) const
{
    map< string, set<string> >::const_iterator it = sourceToVos.find(se);
    if (it != sourceToVos.end())
        {
            return it->second;
        }

    return set<string>();
}

int TransferFileHandler::size()
{
    int sum = 0;

    map< string, map< pair<string, string>, list<FileIndex> > >::iterator iout;
    map< pair<string, string>, list<FileIndex> >::iterator iin;

    for (iout = voToFileIndexes.begin(); iout != voToFileIndexes.end(); iout++)
        for (iin = iout->second.begin(); iin != iout->second.end(); iin++)
            sum += (unsigned int) iin->second.size();

    return sum;
}

} /* namespace cli */
} /* namespace fts3 */
