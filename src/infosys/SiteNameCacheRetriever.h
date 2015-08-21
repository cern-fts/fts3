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

#ifndef SITENAMECACHERETRIVER_H_
#define SITENAMECACHERETRIVER_H_

#include <map>
#include <string>

#include <boost/property_tree/ptree.hpp>

#include "BdiiBrowser.h"

#include "common/ThreadSafeInstanceHolder.h"

namespace fts3
{
namespace infosys
{

//namespace pt = boost::property_tree;

struct EndpointInfo
{
    std::string sitename;
    std::string type;
    std::string version;
};

class SiteNameCacheRetriever: public ThreadSafeInstanceHolder<SiteNameCacheRetriever>
{

    friend class ThreadSafeInstanceHolder<SiteNameCacheRetriever>;

public:

    virtual ~SiteNameCacheRetriever();

    void get(std::map<std::string, EndpointInfo>& cache);

private:

    SiteNameCacheRetriever() {};
    SiteNameCacheRetriever(SiteNameCacheRetriever const&);
    SiteNameCacheRetriever& operator=(SiteNameCacheRetriever const&);

    void fromGlue1(std::map<std::string, EndpointInfo>& cache);
    void fromGlue2(map<string, EndpointInfo>& cache);

    // glue1
    static const char* ATTR_GLUE1_SERVICE;
    static const char* ATTR_GLUE1_LINK;
    static const char* ATTR_GLUE1_SITE;
    static const char* ATTR_GLUE1_ENDPOINT;
    static const char* ATTR_GLUE1_TYPE;
    static const char* ATTR_GLUE1_VERSION;

    static const std::string FIND_SE_SITE_GLUE1;
    static const char* FIND_SE_SITE_ATTR_GLUE1[];

    // glue2
    static const char* ATTR_GLUE2_FK;
    static const char* ATTR_GLUE2_ENDPOINT;
    static const char* ATTR_GLUE2_SITE;
    static const char* ATTR_GLUE2_TYPE;
    static const char* ATTR_GLUE2_VERSION;

    static const std::string FIND_SE_FK_GLUE2;
    static const char* FIND_SE_FK_ATTR_GLUE2[];

    static const std::string FIND_FK_SITE_GLUE2(std::string fk);
    static const char* FIND_FK_SITE_ATTR_GLUE2[];

};

} /* namespace infosys */
} /* namespace fts3 */
#endif /* SITENAMECACHERETRIVER_H_ */
