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

#include "CloudStorageConfig.h"

#include <sys/stat.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

#include "common/Exceptions.h"
#include "common/Uri.h"


using namespace fts3::common;


static
bool isCloudStorage(const Uri &storage)
{
    return storage.protocol == "dropbox" ||
           storage.protocol == "s3" ||
           storage.protocol == "s3s";
}


static
std::string stripBucket(const std::string &host)
{
    size_t index = host.find('.');
    if (index == std::string::npos) {
        return host;
    }
    else {
        return host.substr(index + 1);
    }
}


static
std::string getCloudStorageDefaultName(const Uri &storage)
{
    std::string prefix = storage.protocol;
    boost::to_upper(prefix);
    if (prefix == "S3") {
        // S3 is a bit special, so generate two: S3:HOST and S3:HOST removing first component (might be bucket)
        std::string cs_name = prefix + ":" + storage.host + ";";
        cs_name += prefix + ":" + stripBucket(storage.host);

        return cs_name;
    }
    else {
        return prefix + ":" + storage.host;
    }
}


static
std::string generateCloudStorageNames(const TransferFile &tf)
{
    std::string cs_name;

    Uri source_se = Uri::parse(tf.sourceSe);
    Uri dest_se = Uri::parse(tf.destSe);

    if (isCloudStorage(source_se)) {
        cs_name = getCloudStorageDefaultName(source_se);
    }

    if (isCloudStorage(dest_se)) {
        if (!cs_name.empty())
            cs_name += ";";
        cs_name += getCloudStorageDefaultName(dest_se);
    }

    return cs_name;
}


static void writeDropboxCreds(FILE *f, const std::string& csName, const CloudStorageAuth& auth)
{
    fprintf(f, "[%s]\n", csName.c_str());
    fprintf(f, "APP_KEY=%s\n", auth.appKey.c_str());
    fprintf(f, "APP_SECRET=%s\n", auth.appSecret.c_str());
    fprintf(f, "ACCESS_TOKEN=%s\n", auth.accessToken.c_str());
    fprintf(f, "ACCESS_TOKEN_SECRET=%s\n", auth.accessTokenSecret.c_str());
}


static void writeS3Creds(FILE *f, const std::string& csName, const CloudStorageAuth& auth)
{
    fprintf(f, "[%s]\n", csName.c_str());
    fprintf(f, "SECRET_KEY=%s\n", auth.accessTokenSecret.c_str());
    fprintf(f, "ACCESS_KEY=%s\n", auth.accessToken.c_str());
    fprintf(f, "TOKEN=%s\n", auth.requestToken.c_str());
}


std::string fts3::generateCloudStorageConfigFile(GenericDbIfce* db, const TransferFile& tf)
{
    std::string csName;
    char errDescr[128];

    if (!tf.userCredentials.empty()) {
        csName = tf.userCredentials;
    }
    else {
        csName = generateCloudStorageNames(tf);
    }

    if (csName.empty()) {
        return "";
    }

    char oauth_path[] = "/tmp/fts-oauth-XXXXXX";

    int fd = mkstemp(oauth_path);
    if (fd < 0) {
        strerror_r(errno, errDescr, sizeof(errDescr));
        throw fts3::common::UserError(std::string(__func__) + ": Can not open temporary file, " + errDescr);
    }
    fchmod(fd, 0660);

    FILE *f = fdopen(fd, "w");
    if (f == NULL) {
        close(fd);
        strerror_r(errno, errDescr, sizeof(errDescr));
        throw fts3::common::UserError(std::string(__func__) + ": Can not fdopen temporary file, " + errDescr);
    }

    // For each different VO role, group, ...
    std::vector<std::string> vomsAttrs;
    vomsAttrs.push_back(tf.voName);
    boost::split(vomsAttrs, tf.vomsAttrs, boost::is_any_of(" "), boost::token_compress_on);

    // For each credential (i.e. DROPBOX;S3:s3.cern.ch)
    std::vector<std::string> csVector;
    boost::split(csVector, csName, boost::is_any_of(";"), boost::token_compress_on);

    for (auto i = csVector.begin(); i != csVector.end(); ++i) {
        std::string upperCsName = *i;
        boost::to_upper(upperCsName);

        for (auto voI = vomsAttrs.begin(); voI != vomsAttrs.end(); ++voI) {
            CloudStorageAuth auth;
            if (db->getCloudStorageCredentials(tf.userDn, *voI, upperCsName, auth)) {
                if (boost::starts_with(upperCsName, "DROPBOX")) {
                    writeDropboxCreds(f, upperCsName, auth);
                }
                else {
                    writeS3Creds(f, upperCsName, auth);
                }
                break;
            }
        }
    }

    fclose(f);
    return oauth_path;
}
