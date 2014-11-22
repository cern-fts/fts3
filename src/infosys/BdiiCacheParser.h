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
 * BdiiCacheParser.h
 *
 *  Created on: Feb 14, 2013
 *      Author: Michał Simon
 */

#ifndef BDIICACHEPARSER_H_
#define BDIICACHEPARSER_H_

#include "common/ThreadSafeInstanceHolder.h"

#include <string>

#include <pugixml.hpp>

using namespace pugi;

using namespace fts3::common;

namespace fts3
{
namespace infosys
{

/**
 *
 */
class BdiiCacheParser : public ThreadSafeInstanceHolder<BdiiCacheParser>
{

    friend class ThreadSafeInstanceHolder<BdiiCacheParser>;

public:

    virtual ~BdiiCacheParser();

    /**
     * Gets the site name for the given SE name
     *
     * @param se - name of the SE
     *
     * @return the site name
     */
    std::string getSiteName(std::string se);

private:

    /**
     * Constructor
     */
    BdiiCacheParser(std::string path = bdii_cache_path);

    /// not implemented
    BdiiCacheParser(BdiiCacheParser const&);

    /// not implemented
    BdiiCacheParser& operator=(BdiiCacheParser const&);

    static std::string xpath_entry(std::string se);

    /// the xml document that is being parsed
    xml_document doc;

    /// default path to BDII cache
    static const std::string bdii_cache_path;
};

} /* namespace infosys */
} /* namespace fts3 */
#endif /* BDIICACHEPARSER_H_ */
