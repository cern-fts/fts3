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

#include "RestDeletion.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace fts3::cli;
namespace pt = boost::property_tree;

namespace fts3 {
namespace cli {


RestDeletion::RestDeletion(const std::vector<std::string>& files): files(files)
{

}


RestDeletion::~RestDeletion()
{

}


std::ostream& operator<<(std::ostream& os, RestDeletion const & me)
{
    pt::ptree root, files;

    // prepare the files array
    std::vector<std::string>::const_iterator it;
    for (it = me.files.begin(); it != me.files.end(); ++it)
        {
            files.push_back(std::make_pair(std::string(), pt::ptree(*it)));
        }
    // add files to the root
    root.push_back(std::make_pair("delete", files));

    // Write
    pt::write_json(os, root);
    return os;
}

}
}
