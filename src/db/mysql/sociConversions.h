/**
 * Conversion from and into SOCI
 */
#pragma once

#include <soci.h>
#include "JobStatus.h"

namespace soci
{
    template <>
    struct type_conversion<Cred> {
        typedef values base_type;

        static void from_base(values const& v, indicator, Cred& cred) {
            struct tm termination_st;
            cred.DN               = v.get<std::string>("dn");
            cred.delegationID     = v.get<std::string>("dlg_id");
            cred.proxy            = v.get<std::string>("proxy");
            termination_st        = v.get<struct tm>("termination_time");
            cred.termination_time = mktime(&termination_st);
            cred.vomsAttributes   = v.get<std::string>("voms_attrs", std::string());
        }
    };

    template <>
    struct type_conversion<CredCache> {
        typedef values base_type;

        static void from_base(values const& v, indicator, CredCache& ccache) {
            ccache.DN                 = v.get<std::string>("dn");
            ccache.certificateRequest = v.get<std::string>("cert_request");
            ccache.delegationID       = v.get<std::string>("dlg_id");
            ccache.privateKey         = v.get<std::string>("priv_key");
            ccache.vomsAttributes     = v.get<std::string>("voms_attrs", std::string());
        }
    };
}
