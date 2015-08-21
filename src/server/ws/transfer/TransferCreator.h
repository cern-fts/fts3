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

#ifndef TRANSFERCREATOR_H_
#define TRANSFERCREATOR_H_

#include <list>
#include <vector>
#include <string>
#include <utility>

#include <boost/tuple/tuple.hpp>

namespace fts3
{
namespace ws
{

class TransferCreator
{

    template<int I, int INC = 0>
    class to_transfer
    {

    public:

        to_transfer(std::string const & file, std::string const & state, int & fileIndex) : fileIndex(fileIndex)
        {
            boost::get<I>(t) = file;
            boost::get<2>(t) = state;
        }

        boost::tuple<std::string, std::string, std::string, int> operator() (std::string const & file)
        {
            // flip the first bit (if it's 0 it will be 1, if it's 1 it will be 0)
            boost::get<~I & 1>(t) = file;
            boost::get<3>(t) = fileIndex;
            fileIndex += INC;
            return t;
        }

    private:
        boost::tuple<std::string, std::string, std::string, int> t;
        int & fileIndex;
    };

    static std::pair<std::string, std::string> map_protocol(std::string const & file)
    {
        std::string protocol = file.substr(0, file.find("://"));
        return std::make_pair(protocol, file);
    }

public:
    TransferCreator(int fileIndex, std::string const & initialState) : fileIndex(fileIndex), initialState(initialState) {};
    virtual ~TransferCreator();

    std::list< boost::tuple<std::string, std::string, std::string, int> > pairSourceAndDestination(std::vector<std::string> const & sources, std::vector<std::string> const & destinations);

    int nextFileIndex()
    {
        return fileIndex + 1;
    }

private:

    int fileIndex;
    std::string const & initialState;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* TRANSFERCREATOR_H_ */
