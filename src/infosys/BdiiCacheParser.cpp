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
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or impltnsied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * BdiiCacheParser.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: Michał Simon
 */

#include "BdiiCacheParser.h"

#include "config/serverconfig.h"

namespace fts3
{
namespace infosys
{

using namespace config;

const std::string BdiiCacheParser::bdii_cache_path = "/var/lib/fts3/bdii_cache.xml";

BdiiCacheParser::BdiiCacheParser(string path)
{

    doc.load_file(path.c_str());
}

BdiiCacheParser::~BdiiCacheParser()
{

}

std::string BdiiCacheParser::getSiteName(std::string se)
{
    xpath_node node = doc.select_single_node(xpath_entry(se).c_str());
    return node.node().child_value("sitename");
}

std::string BdiiCacheParser::xpath_entry(std::string se)
{
    static const std::string xpath_begin = "/entry[endpoint='";
    static const std::string xpath_end = "']";

    return xpath_begin + se + xpath_end;
}

} /* namespace infosys */
} /* namespace fts3 */
