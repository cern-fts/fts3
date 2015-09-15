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

#include "CredUtility.h"

#include "common/error.h"
#include <boost/thread.hpp>
#include "gridsite.h"
#include "db/generic/DbUtils.h"
#include <stdio.h>
#include <time.h>
#include <voms/voms_api.h>
#include <globus_gsi_credential.h>

#include <unistd.h>
#include "../common/Logger.h"

using namespace fts3::common;

/*
 * get_proxy_lifetime
 *
 * Return the proxy lifetime. In case of other errors, returns -1
 */
void get_proxy_lifetime(const std::string& filename, time_t *lifetime,
        time_t *vo_lifetime) /*throw ()*/
{

    *lifetime = (time_t) -1;
    *vo_lifetime = (time_t) -1;

    // Check that the file Exists
    int result = access(filename.c_str(), R_OK);
    if (0 != result) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Requested Proxy doesn't exist. A new one should be created. Reason is " << strerror(errno) << commit;
        return;
    }
    // Check if it's valid
    globus_gsi_cred_handle_t proxy_handle = 0;
    globus_gsi_cred_handle_attrs_t handle_attrs = 0;
    try {
        // Init handle attributes
        globus_result_t result = globus_gsi_cred_handle_attrs_init(
                &handle_attrs);
        if (0 != result) {
            //throw RuntimeError("Cannot Init Handle Attributes");
            FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Cannot Init Handle Attributes" << commit;
        }
        // Init handle
        result = globus_gsi_cred_handle_init(&proxy_handle, handle_attrs);
        if (0 != result) {
            //throw RuntimeError("Cannot Init Handle");
            FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Cannot Init Handle" << commit;
        }
        // Load Proxy
        result = globus_gsi_cred_read_proxy(proxy_handle,
                const_cast<char *>(filename.c_str()));
        if (0 != result) {
            //throw RuntimeError("Cannot Load Proxy File");
            FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Cannot Load Proxy File" << commit;
        }
        // Get Lifetime (in seconds)
        result = globus_gsi_cred_get_lifetime(proxy_handle, lifetime);
        if (0 != result) {
            //throw RuntimeError("Cannot Get Proxy Lifetime");
            FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Cannot Get Proxy Lifetime" << commit;
        }

        // Get VO extensions lifetime
        X509* cert = NULL;
        globus_gsi_cred_get_cert(proxy_handle, &cert);
        STACK_OF(X509)* cert_chain = NULL;
        globus_gsi_cred_get_cert_chain(proxy_handle, &cert_chain);

        vomsdata voms_data;
        voms_data.SetVerificationType((verify_type)(VERIFY_NONE));
        voms_data.Retrieve(cert, cert_chain);
        if (voms_data.data.size() > 0) {
            *vo_lifetime = INT_MAX;
            for (size_t i = 0; i < voms_data.data.size(); ++i) {
                const voms &data = voms_data.data[i];
                struct tm tm_eol;
                strptime(data.date2.c_str(), "%Y%m%d%H%M%S%Z", &tm_eol);
                time_t vo_eol = timegm(&tm_eol);
                time_t utc_now = db::getUTC(0);
                time_t vo_remaining = vo_eol - utc_now;
                if (vo_remaining < *vo_lifetime)
                    *vo_lifetime = vo_remaining;
            }
        }
        else {
            *vo_lifetime = 0;
        }

        // Clean
        X509_free(cert);
        sk_X509_pop_free(cert_chain, X509_free);
    } catch (const std::exception& exc) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR)<<" Cannot Check Proxy Validity. Reason is: " << exc.what() << commit;
        *lifetime = (time_t)-1;
        *vo_lifetime = (time_t)-1;
    }
    // Destroy handles
    if (0 != proxy_handle) {
        globus_gsi_cred_handle_destroy(proxy_handle);
    }
    if (0 != handle_attrs) {
        globus_gsi_cred_handle_attrs_destroy(handle_attrs);
    }
}
