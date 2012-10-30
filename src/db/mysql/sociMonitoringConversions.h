/**
 * Conversion from and into SOCI (Monitoring)
 */
#pragma once
#include "sociConversions.h"

#include <soci.h>
#include <time.h>

namespace soci
{
    template <>
    struct type_conversion<SourceAndDestSE> {
        typedef values base_type;

        static void from_base(values const& v, indicator, SourceAndDestSE& pair) {
            pair.sourceStorageElement = v.get<std::string>("source_se");
            pair.destinationStorageElement = v.get<std::string>("dest_se");
        }
    };

    template <>
    struct type_conversion<ConfigAudit> {
        typedef values base_type;

        static void from_base(values const& v, indicator, ConfigAudit& audit) {
            struct tm aux_tm;

            audit.action = v.get<std::string>("action");
            audit.config = v.get<std::string>("config");
            audit.userDN = v.get<std::string>("dn");
            aux_tm = v.get<struct tm>("datetime");
            audit.when = timegm(&aux_tm);
        }
    };

    template <>
    struct type_conversion<ReasonOccurrences> {
        typedef values base_type;

        static void from_base(values const& v, indicator, ReasonOccurrences& reason) {
            reason.count   = v.get<unsigned>("count");
            reason.reason = v.get<std::string>("reason");
        }
    };

    template <>
    struct type_conversion<SePairThroughput> {
        typedef values base_type;

        static void from_base(values const& v, indicator, SePairThroughput& pair) {
            pair.averageThroughput = v.get<double>("throughput");
            pair.duration          = static_cast<long>(v.get<double>("duration"));
            pair.storageElements.destinationStorageElement = v.get<std::string>("dest_se");
            pair.storageElements.sourceStorageElement      = v.get<std::string>("source_se");
        }
    };

    template <>
    struct type_conversion<JobVOAndSites> {
        typedef values base_type;

        static void from_base(values const& v, indicator, JobVOAndSites& voandsite) {
            voandsite.destinationSite = v.get<std::string>("dest_site");
            voandsite.sourceSite      = v.get<std::string>("source_site");
            voandsite.vo              = v.get<std::string>("vo_name");
        }
    };
}
