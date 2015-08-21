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

#pragma once
#include "sociConversions.h"

#include <soci.h>
#include <time.h>

namespace soci
{
template <>
struct type_conversion<SourceAndDestSE>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, SourceAndDestSE& pair)
    {
        pair.sourceStorageElement = v.get<std::string>("source_se");
        pair.destinationStorageElement = v.get<std::string>("dest_se");
    }
};

template <>
struct type_conversion<ConfigAudit>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, ConfigAudit& audit)
    {
        struct tm aux_tm;

        audit.action = v.get<std::string>("action");
        audit.config = v.get<std::string>("config");
        audit.userDN = v.get<std::string>("dn");
        aux_tm = v.get<struct tm>("datetime");
        audit.when = timegm(&aux_tm);
    }
};

template <>
struct type_conversion<ReasonOccurrences>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, ReasonOccurrences& reason)
    {
        reason.count   = v.get<unsigned>("count");
        reason.reason = v.get<std::string>("reason");
    }
};

template <>
struct type_conversion<SePairThroughput>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, SePairThroughput& pair)
    {
        pair.averageThroughput = v.get<double>("throughput");
        pair.duration          = static_cast<long>(v.get<double>("duration"));
        pair.storageElements.destinationStorageElement = v.get<std::string>("dest_se");
        pair.storageElements.sourceStorageElement      = v.get<std::string>("source_se");
    }
};

template <>
struct type_conversion<JobVOAndSites>
{
    typedef values base_type;

    static void from_base(values const& v, indicator, JobVOAndSites& voandsite)
    {
        voandsite.destinationSite = v.get<std::string>("dest_site");
        voandsite.sourceSite      = v.get<std::string>("source_site");
        voandsite.vo              = v.get<std::string>("vo_name");
    }
};
}
