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
           storage.protocol == "s3s" || 
           storage.protocol == "gcloud" ||
           storage.protocol == "gclouds";
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
    if ((prefix == "S3") or (prefix == "S3S") ){
        // S3 is a bit special, so generate two: S3:HOST and S3:HOST removing first component (might be bucket)
        std::string cs_name = std::string("S3:") + storage.host + ";";
        cs_name += std::string("S3:") + stripBucket(storage.host);

        return cs_name;
    } else if ((prefix == "GCLOUD") or (prefix == "GCLOUDS")) {
        return std::string("GCLOUD:") + storage.host;
    }
    else if (prefix == "DROPBOX") {
        return prefix;
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


static void writeS3Creds(FILE *f, const std::string& csName, const CloudStorageAuth& auth, bool alternate)
{
    fprintf(f, "[%s]\n", csName.c_str());
    fprintf(f, "SECRET_KEY=%s\n", auth.accessTokenSecret.c_str());
    fprintf(f, "ACCESS_KEY=%s\n", auth.accessToken.c_str());
    if (!auth.requestToken.empty())
        fprintf(f, "TOKEN=%s\n", auth.requestToken.c_str());
    if (alternate) {
        fprintf(f, "ALTERNATE=true\n");
    }
}


static void writeOAuthToken(FILE *f, const std::string& token)
{
    fprintf(f, "[BEARER]\n");
    fprintf(f, "TOKEN=%s\n", token.c_str());
}


std::string
fts3::generateCloudStorageConfigFile(GenericDbIfce *db, const TransferFile &tf, const std::string &authMethod)
{
    char errDescr[128];

    std::string csName = generateCloudStorageNames(tf);
    if (csName.empty()) {
        return "";
    }

    char oauth_path[] = "/tmp/fts-cloud-config-XXXXXX";

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
        remove(oauth_path);
        throw fts3::common::UserError(std::string(__func__) + ": Can not fdopen temporary file, " + errDescr);
    }

    boost::optional<UserCredential> cred;

    if (authMethod == "oauth2") {
        cred.reset(UserCredential());
    } else {
        cred = db->findCredential(tf.credId, tf.userDn);

        if (!cred) {
            fclose(f);
            remove(oauth_path);
            return "";
        }
    }

    // For each different VO or VO attribute
    std::vector<std::string> vomsAttrs;
    boost::split(vomsAttrs, cred->vomsAttributes, boost::is_any_of(" "), boost::token_compress_on);
    vomsAttrs.push_back(tf.voName);

    // For each cloud storage credential (i.e. DROPBOX;S3:s3.cern.ch)
    std::vector<std::string> csVector;
    boost::split(csVector, csName, boost::is_any_of(";"), boost::token_compress_on);

    for (auto upperCsName: csVector) {
        boost::to_upper(upperCsName);

        for (const auto& voms_attr: vomsAttrs) {
            CloudStorageAuth auth;

            if (db->getCloudStorageCredentials(tf.userDn, voms_attr, upperCsName, auth)) {
                if (boost::starts_with(upperCsName, "DROPBOX")) {
                    writeDropboxCreds(f, upperCsName, auth);
                } else {
                    writeS3Creds(f, upperCsName, auth, tf.getProtocolParameters().s3Alternate);
                }

                break;
            }
        }
    }

    if (authMethod == "oauth2") {
        std::string token;

        if (isCloudStorage(Uri::parse(tf.sourceSe))) {
            token = db->findToken(tf.sourceTokenId);
        } else if (isCloudStorage(Uri::parse(tf.destSe))){
            token = db->findToken(tf.destinationTokenId);
        }

        if (token.empty()) {
            fclose(f);
            remove(oauth_path);
            return "";
        }

        writeOAuthToken(f, token);
    }

    fclose(f);
    return oauth_path;
}


static void writeTokensFile(FILE *f, const std::string& srcToken, const std::string& dstToken)
{
    fprintf(f, "%s\n", srcToken.c_str());
    fprintf(f, "%s\n", dstToken.c_str());
}


std::string fts3::generateOAuthConfigFile(GenericDbIfce* db, const TransferFile& tf)
{
    char oauth_path[] = "/tmp/fts-oauth-XXXXXX";
    char errDescr[128];
    FILE *f = nullptr;

    int fd = mkstemp(oauth_path);

    if (fd < 0) {
        strerror_r(errno, errDescr, sizeof(errDescr));
        throw fts3::common::UserError(std::string(__func__) + ": Can not open temporary file, " + errDescr);
    }
    fchmod(fd, 0660);

    f = fdopen(fd, "w");

    if (f == nullptr) {
        close(fd);
        strerror_r(errno, errDescr, sizeof(errDescr));
        remove(oauth_path);
        throw fts3::common::UserError(std::string(__func__) + ": Can not fdopen temporary file, " + errDescr);
    }

    auto src_token = db->findToken(tf.sourceTokenId);
    auto dst_token = db->findToken(tf.destinationTokenId);
    // There should be a token for the source and destination endpoints
    if (src_token.empty() || dst_token.empty()) {
    	fclose(f);
        remove(oauth_path);
        return "";
    }

    // Write both tokens in the authorization file
    writeTokensFile(f, src_token, dst_token);

    fclose(f);
    return oauth_path;
}
