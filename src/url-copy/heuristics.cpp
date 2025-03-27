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

#include <errno.h>
#include <json/json.h>
#include <cryptopp/base64.h>
#include <boost/algorithm/string.hpp>

#include "heuristics.h"


static bool findSubstring(const std::string &stack, const char *needles[])
{
    for (size_t i = 0; needles[i] != NULL; ++i) {
        if (stack.find(needles[i]) != std::string::npos)
            return true;
    }
    return false;
}

static Json::Value decodeAccessTokenPayload(std::string token)
{
    auto start = token.find('.');
    auto end = token.rfind('.');

    if ((start == std::string::npos) || (end == std::string::npos) || (start == end)) {
        throw std::exception();
    }

    token = token.substr(start + 1, end - start - 1);

    std::string decoded;
    CryptoPP::StringSource ss(token, true,
                              new CryptoPP::Base64Decoder(
                                      new CryptoPP::StringSink(decoded)));

    Json::Value jsonDecoded;
    std::istringstream(decoded) >> jsonDecoded;

    return jsonDecoded;
}


bool retryTransfer(int errorNo, const std::string &category, const std::string &message)
{
    // If the following strings appear in the error message, assume retryable
    const char *msg_imply_retry[] = {
        "performance marker",
        "Name or service not known",
        "Connection timed out",
        "end-of-file was reached",
        "end of file occurred",
        "SRM_INTERNAL_ERROR",
        "was forcefully killed",
        "operation timeout",
        NULL
    };

    if (findSubstring(message, msg_imply_retry))
        return true;

    // ETIMEDOUT shortcuts the following
    if (errorNo == ETIMEDOUT)
        return true;

    // Do not retry cancelation
    if (errorNo == ECANCELED)
        return false;

    // If the following strings appear in the error message, assume non-retriable
    const char *msg_imply_do_not_retry[] = {
        "proxy expired",
        "with an error 550 File not found",
        "File exists and overwrite",
        "No such file or directory",
        "SRM_INVALID_PATH",
        "The certificate has expired",
        "The available CRL has expired",
        "SRM Authentication failed",
        "SRM_DUPLICATION_ERROR",
        "SRM_AUTHENTICATION_FAILURE",
        "SRM_NO_FREE_SPACE",
        "digest too big for rsa key",
        "Can not determine address of local host",
        "Permission denied",
        "System error in write into HDFS",
        "File exists",
        "checksum do not match",
        "CHECKSUM MISMATCH",
        "The source file is not ONLINE",
        NULL
    };

    if (findSubstring(message, msg_imply_do_not_retry))
        return false;

    // Rely on the error codes otherwise
    if (category == "SOURCE") {
        switch (errorNo) {
            case ENOENT:          // No such file or directory
            case EPERM:           // Operation not permitted
            case EACCES:          // Permission denied
            case EISDIR:          // Is a directory
            case ENAMETOOLONG:    // File name too long
            case E2BIG:           // Argument list too long
            case ENOTDIR:         // Part of the path is not a directory
            case EPROTONOSUPPORT: // Protocol not supported by gfal2 (plugin missing?)
                return false;
        }
    }
    else if (category == "DESTINATION") {
        switch (errorNo) {
            case EPERM:           // Operation not permitted
            case EACCES:          // Permission denied
            case EISDIR:          // Is a directory
            case E2BIG:           // Argument list too long
            case EROFS:           // Read-only file system
            case EFAULT:          // Bad address
            case ENAMETOOLONG:    // File name too long
            case EPROTONOSUPPORT: // Protocol not supported by gfal2 (plugin missing?)
            case EEXIST:          // Destination file exists
                return false;
        }
    }
    else //TRANSFER
    {
        switch (errorNo) {
            case ENOSPC:          // No space left on device
            case EPERM:           // Operation not permitted
            case EACCES:          // Permission denied
            case EEXIST:          // File exists
            case EFBIG:           // File too big
            case EROFS:           // Read-only file system
            case ENAMETOOLONG:    // File name too long
            case EPROTONOSUPPORT: // Protocol not supported by gfal2 (plugin missing?)
                return false;
        }
    }

    return true;
}


unsigned adjustTimeoutBasedOnSize(uint64_t sizeInBytes, const unsigned addSecPerMb)
{
    static const unsigned long MB = 1 << 20;

    // Reasonable time to wait per MB transferred (default 2 seconds / MB)
    unsigned timeoutPerMBLocal = (addSecPerMb > 0) ? addSecPerMb : 2;

    // Final timeout adjusted considering transfer timeout
    return static_cast<unsigned int>(600 + ceil(timeoutPerMBLocal * (static_cast<double>(sizeInBytes) / MB)));
}


std::string mapErrnoToString(int err)
{
    char buf[256] = {0};
    char const *str = strerror_r(err, buf, sizeof(buf));
    if (str) {
        std::string rep(str);
        std::replace(rep.begin(), rep.end(), ' ', '_');
        return boost::to_upper_copy(rep);
    }
    return "GENERAL ERROR";
}


std::string replaceMetadataString(const std::string &text)
{
    std::string copy = boost::replace_all_copy(text, "?"," ");
    copy = boost::replace_all_copy(copy, "\\\"","\"");
    return copy;
}


std::string sanitizeQueryString(const std::string& text)
{
    const std::string allowedOpaque = "abcdefghijklmnopqrstuvwxyz"
                                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                      "0123456789~-_.+&=%#?";

    std::string copy(text);
    auto pos = copy.find('?');

    while (pos != std::string::npos) {
        auto endpos = copy.find_first_not_of(allowedOpaque, pos + 1);

        if (endpos == std::string::npos) {
            endpos = copy.size();
        }

        copy.replace(pos + 1, endpos - pos - 1, "<redacted>");
        pos = copy.find('?', pos + 1);
    }

    return copy;
}


std::string extractAccessTokenField(const std::string& token, const std::string& field)
{
    try {
        auto decoded = decodeAccessTokenPayload(token);

        if (decoded.isMember(field)) {
            return decoded.get(field, "").asString();
        }
    } catch (...) { }

    return "";
}


std::string accessTokenPayload(const std::string& token)
{
    std::ostringstream message;

    try {
        auto decoded = decodeAccessTokenPayload(token);
        Json::Value filtered;

        auto tokenFields = { "aud", "iat", "nbf", "exp", "iss", "scope", "wlcg.ver" };
        for (const auto& key: tokenFields) {
            if (decoded.isMember(key)) {
                filtered[key] = decoded.get(key, "");
            }
        }

        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";

        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        writer->write(filtered, &message);
    } catch (std::exception&) {
        message << "Failed to decode token!";

        if (token.size() > 50) {
            message << " (" << (token.substr(0, 5) + "..." + token.substr(token.size() - 5)) << ")";
        }
    }

    return message.str();
}
