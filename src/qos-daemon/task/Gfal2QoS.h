/*
 * Copyright (c) CERN 2020
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

#ifndef GFAL2QOS_CPP_H
#define GFAL2QOS_CPP_H

#include <gfal_api.h>
#include "common/Uri.h"

using fts3::common::Uri;

class Gfal2Exception: public std::exception {
public:

    /// Constructor
    Gfal2Exception(GError *error): error(error) {
    }

    ~Gfal2Exception() throw() {
        g_error_free(error);
    }

    const char *what() const throw() {
        return error->message;
    }

    int code() const throw () {
        return error->code;
    }

private:
    GError *error;
};

class Gfal2QoS {
public:

    /// Constructor
    Gfal2QoS() {
        GError *error = NULL;
        context = gfal2_context_new(&error);
        if (context == NULL) {
            throw Gfal2Exception(error);
        }
    }

    /// Destructor
    ~Gfal2QoS() {
        gfal2_context_free(context);
    }

    /// Disallow copy constructor
    Gfal2QoS(const Gfal2QoS&) = delete;
    Gfal2QoS operator = (const Gfal2QoS&) = delete;

    /// Retrieve file QoS class
    std::string getFileQoS(const std::string &url, const std::string &token) {
        tokenInit(url, token);

        char buff[2048];
        GError *error = NULL;

        if (gfal2_check_file_qos(context, url.c_str(), buff, 2048, &error) < 0) {
            throw Gfal2Exception(error);
        }

        return std::string(buff);
    }

    /// Retrieve file target QoS class
    std::string getFileTargetQoS(const std::string &url, const std::string &token) {
        tokenInit(url, token);

        char buff[2048];
        GError *error = NULL;

        if (gfal2_check_target_qos(context, url.c_str(), buff, 2048, &error) < 0) {
            throw Gfal2Exception(error);
        }

        return std::string(buff);
    }

    /// Change file QoS class
    int changeFileQoS(const std::string &url, const std::string &target_qos, const std::string &token) {
        tokenInit(url, token);

        GError *error = NULL;

        if (gfal2_change_object_qos(context, url.c_str(), target_qos.c_str(), &error) < 0) {
            throw Gfal2Exception(error);
        }

        return 0;
    }

private:
    gfal2_context_t context;

    /// Configure OIDC token for host
    void tokenInit(const std::string &url, const std::string &token) {
        GError *error = NULL;
        gfal2_cred_t *cred = gfal2_cred_new(GFAL_CRED_BEARER, token.c_str());

        if (gfal2_cred_set(context, Uri::parse(url).host.c_str(), cred, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }
};

#endif // GFAL2QOS_CPP_H
