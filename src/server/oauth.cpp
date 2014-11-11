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

#include <common/parse_url.h>
#include <common/error.h>


static
bool isCloudStorage(const Uri& storage)
{
    return storage.Protocol == "dropbox" ||
           storage.Protocol == "s3" ||
           storage.Protocol == "s3s";
}

static
std::string stripBucket(const std::string& host)
{
    size_t index = host.find('.');
    if (index == std::string::npos)
        return host;
    else
        return host.substr(index + 1);
}

static
std::string getCloudStorageDefaultName(const Uri& storage)
{
    std::string prefix = storage.Protocol;
    boost::to_upper(prefix);
    if (prefix == "S3")
        {
            // S3 is a bit special, so generate two: S3:HOST and S3:HOST removing first component (might be bucket)
            std::string cs_name = prefix + ":" + storage.Host + ";";
            cs_name += prefix + ":" + stripBucket(storage.Host);

            return cs_name;
        }
    else
        {
            return prefix + ":" + storage.Host;
        }
}

static
std::string generateCloudStorageNames(const TransferFiles& tf)
{
    std::string cs_name;

    Uri source_se = Uri::Parse(tf.SOURCE_SE);
    Uri dest_se   = Uri::Parse(tf.DEST_SE);

    if (isCloudStorage(source_se))
        cs_name = getCloudStorageDefaultName(source_se);
    if (isCloudStorage(dest_se))
        {
            if (!cs_name.empty())
                cs_name += ";";
            cs_name += getCloudStorageDefaultName(dest_se);
        }
    return cs_name;
}

std::string fts3::generateOauthConfigFile(GenericDbIfce* db, const TransferFiles& tf)
{
    std::string cs_name;

    if (!tf.USER_CREDENTIALS.empty())
        cs_name = tf.USER_CREDENTIALS;
    else
        cs_name = generateCloudStorageNames(tf);

    if (cs_name.empty())
        return "";

    char oauth_path[] = "/tmp/fts-oauth-XXXXXX";
    int fd = mkstemp(oauth_path);
    if (fd < 0)
        {
            char err_descr[128];
            strerror_r(errno, err_descr, sizeof(err_descr));
            throw fts3::common::Err_Custom(std::string(__func__) + ": Can not open temporary file, " + err_descr);
        }
    FILE* f = fdopen(fd, "w");

    // For each credential (i.e. DROPBOX;S3:s3.cern.ch)
    std::vector<std::string> cs_vector;
    boost::split(cs_vector, cs_name, boost::is_any_of(";"));
    std::vector<std::string>::const_iterator i;
    for (i = cs_vector.begin(); i != cs_vector.end(); ++i)
        {
            std::string upper_cs_name = *i;
            boost::to_upper(upper_cs_name);

            OAuth oauth;
            if(db->getOauthCredentials(tf.DN, tf.VO_NAME, upper_cs_name, oauth))
                {
                    fprintf(f, "[%s]\n", upper_cs_name.c_str());
                    fprintf(f, "APP_KEY=%s\n", oauth.app_key.c_str());
                    fprintf(f, "APP_SECRET=%s\n", oauth.app_secret.c_str());
                    fprintf(f, "ACCESS_TOKEN=%s\n", oauth.access_token.c_str());
                    fprintf(f, "ACCESS_TOKEN_SECRET=%s\n", oauth.access_token_secret.c_str());
                }
        }

    fclose(f);
    close(fd);
    return oauth_path;
}
