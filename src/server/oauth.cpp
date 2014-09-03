/*
 *  Copyright notice:
 *  Copyright © CERN, 2014.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include "oauth.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>


std::string fts3::generateOauthConfigFile(GenericDbIfce* db,
        const std::string& dn, const std::string& vo_name,
        const std::string& cs_name)
{
    OAuth oauth;
    if (cs_name.empty() || !db->getOauthCredentials(dn, vo_name, cs_name, oauth))
        return "";

    char oauth_path[] = "/tmp/fts-oauth-XXXXXX";
    int fd = mkstemp(oauth_path);
    FILE* f = fdopen(fd, "w");

    // For each credential (i.e. DROPBOX;S3:s3.cern.ch)
    std::vector<std::string> cs_vector;
    boost::split(cs_vector, cs_name, boost::is_any_of(";"));
    std::vector<std::string>::const_iterator i;
    for (i = cs_vector.begin(); i != cs_vector.end(); ++i)
        {
            std::string upper_cs_name = *i;
            boost::to_upper(upper_cs_name);

            fprintf(f, "[%s]\n", upper_cs_name.c_str());
            fprintf(f, "APP_KEY=%s\n", oauth.app_key.c_str());
            fprintf(f, "APP_SECRET=%s\n", oauth.app_secret.c_str());
            fprintf(f, "ACCESS_TOKEN=%s\n", oauth.access_token.c_str());
            fprintf(f, "ACCESS_TOKEN_SECRET=%s\n", oauth.access_token_secret.c_str());
        }

    fclose(f);
    close(fd);
    return oauth_path;
}
