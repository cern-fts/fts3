/*
 * Copyright (c) CERN 2025
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

#ifndef GFAL2_CPP_H
#define GFAL2_CPP_H

#include <vector>

#include <gfal_api.h>
#include "common/Uri.h"

using fts3::common::Uri;

class Gfal2Exception: public std::exception {
private:
    GError *error;

public:
    /// Constructor. Acquire ownership of error!
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
};


class Gfal2TransferParams {
private:
    friend class Gfal2;
    gfalt_params_t params;
    std::string src_token;
    std::string dst_token;

    operator gfalt_params_t () {
        return params;
    }
    
public:

    /// Constructor
    Gfal2TransferParams() {
        GError *error = NULL;
        params = gfalt_params_handle_new(&error);
        if (params == NULL) {
            throw Gfal2Exception(error);
        }
    }

    /// Copy constructor
    Gfal2TransferParams(const Gfal2TransferParams &orig) {
        GError *error = NULL;
        params = gfalt_params_handle_copy(orig.params, &error);
        if (params == NULL) {
            throw Gfal2Exception(error);
        }
        src_token = orig.src_token;
        dst_token = orig.dst_token;
    }


    /// Move constructor
    Gfal2TransferParams(Gfal2TransferParams &&orig) {
        params = orig.params;
        orig.params = NULL;
        src_token = std::move(orig.src_token);
        dst_token = std::move(orig.dst_token);
    }

    /// Destructor
    ~Gfal2TransferParams() {
        GError *error = NULL;
        gfalt_params_handle_delete(params, &error);
        // Destructors mustn't throw
        g_clear_error(&error);
    }

    Gfal2TransferParams& operator = (const Gfal2TransferParams &orig) {
        GError *error = NULL;
        params = gfalt_params_handle_copy(orig.params, &error);
        if (params == NULL) {
            throw Gfal2Exception(error);
        }
        return *this;
    }

    void setCreateParentDir(bool value) {
        GError *error = NULL;
        if (gfalt_set_create_parent_dir(params, value, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void setTransferCleanUp(bool value) {
        GError *error = NULL;
        if (gfalt_set_transfer_cleanup(params, value, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void setSourceSpacetoken(const std::string &stoken)
    {
        GError *error = NULL;
        if (gfalt_set_src_spacetoken(params, stoken.c_str(), &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void setDestSpacetoken(const std::string &stoken)
    {
        GError *error = NULL;
        if (gfalt_set_dst_spacetoken(params, stoken.c_str(), &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void setStrictCopy(bool value)
    {
        GError *error = NULL;
        if (gfalt_set_strict_copy_mode(params, value, NULL) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void setReplaceExistingFile(bool value)
    {
        GError *error = NULL;
        if (gfalt_set_replace_existing_file(params, value, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void setChecksum(int mode, const std::string &type, const std::string &value){
    	GError *error = NULL;
    	if (gfalt_set_checksum(params, (gfalt_checksum_mode_t) mode, type.c_str(), value.c_str(), &error) < 0) {
    		throw Gfal2Exception(error);
    	}
    }

    int getChecksum(std::string &checksum_type, std::string &user_checksum)
    {
        char csum_type[1024] = {0};
        char user_csum[1024] = {0};

        GError *error = NULL;
        int ret = gfalt_get_checksum(params, csum_type, sizeof(csum_type),
                                     user_csum, sizeof(user_csum), &error);
        if (error) {
            throw Gfal2Exception(error);
        }

        checksum_type = csum_type;
        user_checksum = user_csum;
        return ret;
    }

    void setTimeout(unsigned timeout)
    {
        GError *error = NULL;
        if (gfalt_set_timeout(params, timeout, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void setNumberOfStreams(unsigned nStreams)
    {
        GError *error = NULL;
        if (gfalt_set_nbstreams(params, nStreams, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    uint32_t getNumberOfStreams()
    {
        GError *error = NULL;
        uint32_t val = gfalt_get_nbstreams(params, &error);
        if (error) {
            throw Gfal2Exception(error);
        }
        return val;
    }

    void setTcpBuffersize(unsigned buffersize)
    {
        GError *error = NULL;
        if (gfalt_set_tcp_buffer_size(params, buffersize, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    uint64_t getTcpBuffersize()
    {
        GError *error = NULL;
        uint64_t val = gfalt_get_tcp_buffer_size(params, &error);
        if (error) {
            throw Gfal2Exception(error);
        }
        return val;
    }

    uint64_t getTimeout()
    {
        GError *error = NULL;
        uint64_t val = gfalt_get_timeout(params, &error);
        if (error) {
            throw Gfal2Exception(error);
        }
        return val;
    }

    void setDelegationFlag(bool value)
    {
        GError *error = NULL;
        if (gfalt_set_use_proxy_delegation(params, value, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void setStreamingFlag(bool value)
    {
        GError *error = NULL;
        if (gfalt_set_local_transfer_perm(params, value, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void setBringonlineToken(const std::string &token)
    {
        GError *error = NULL;
        if (gfalt_set_stage_request_id(params, token.c_str(), &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void setArchiveMetadata(const std::string &metadata)
    {
        GError *error = NULL;
        if (gfalt_set_archive_metadata(params, metadata.c_str(), &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void setScitag(const uint scitag)
    {
        GError *error = NULL;
        if (gfalt_set_scitag(params, scitag, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void addEventCallback(gfalt_event_func callback, void *udata)
    {
        GError *error = NULL;
        if (gfalt_add_event_callback(params, callback, udata, NULL, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void addMonitorCallback(gfalt_monitor_func callback, void *udata)
    {
        GError *error = NULL;
        if (gfalt_add_monitor_callback(params, callback, udata, NULL, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }
};


class Gfal2 {
public:

    /// Constructor
    Gfal2() {
        GError *error = NULL;
        context = gfal2_context_new(&error);

        if (context == NULL) {
            throw Gfal2Exception(error);
        }
    }

    /// Destructor (empty on purpose: FTS-1996)
    ~Gfal2() {}

    /// Can not copy
    Gfal2(const Gfal2&) = delete;
    Gfal2& operator = (const Gfal2&) = delete;

    /// Load configuration from a file
    void loadConfigFile(const std::string &path) {
        GError *error = NULL;
        if (gfal2_load_opts_from_file(context, path.c_str(), &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    /// Set source bearer token credential
    void setSourceToken(const std::string& source, const std::string& token)
    {
        GError* error = nullptr;

        if (!token.empty()) {
            src_token = token;
            auto* token_cred = gfal2_cred_new("BEARER", token.c_str());

            if (gfal2_cred_set(context, source.c_str(), token_cred, &error) < 0) {
                throw Gfal2Exception(error);
            }
        }
    }

    /// Set destination bearer token credential
    void setDestinationToken(const std::string& destination, const std::string& token, const bool mapToHost = true)
    {
        GError* error = nullptr;

        if (!token.empty()) {
            dst_token = token;
            auto* token_cred = gfal2_cred_new("BEARER", token.c_str());
            auto destination_url = mapToHost ? Uri::parse(destination).host : destination.c_str();

            if (gfal2_cred_set(context, destination_url.c_str(), token_cred, &error) < 0) {
                throw Gfal2Exception(error);
            }
        }
    }

    std::string getSourceToken() const {
        return src_token;
    }

    std::string getDestinationToken() const {
        return dst_token;
    }

    /// Set a boolean config value
    void set(const std::string &group, const std::string &key, bool value) {
        GError *error = NULL;
        if (gfal2_set_opt_boolean(context, group.c_str(), key.c_str(), value, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    /// Set a string config value
    void set(const std::string &group, const std::string &key, const std::string &value) {
        GError *error = NULL;
        if (gfal2_set_opt_string(context, group.c_str(), key.c_str(), value.c_str(), &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    /// Set a C string config value
    void set(const std::string &group, const std::string &key, const char* value) {
        GError *error = NULL;
        if (gfal2_set_opt_string(context, group.c_str(), key.c_str(), value, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    /// Set user agent
    void setUserAgent(const std::string &id, const std::string &version) {
        GError *error = NULL;
        if (gfal2_set_user_agent(context, id.c_str(), version.c_str(), &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    /// Add additional metadata
    void addClientInfo(const std::string &key, const std::string &value) {
        GError *error = NULL;
        if (gfal2_add_client_info(context, key.c_str(), value.c_str(), &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    /// Cancel any running operation
    void cancel() {
        gfal2_cancel(context);
    }

    /// Stat a file
    struct stat stat(const std::string &url) {
        GError *error = NULL;
        struct stat buffer;

        if (gfal2_stat(context, url.c_str(), &buffer, &error) < 0) {
            throw Gfal2Exception(error);
        }

        return buffer;
    }

    /// Copy a file
    void copy(Gfal2TransferParams &params, const std::string &source, const std::string &destination)
    {
        GError *error = NULL;

        if (gfalt_copy_file(context, params, source.c_str(), destination.c_str(), &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void access(const std::string &url, int amode)
    {
        GError *error = NULL;

        if (gfal2_access(context, url.c_str(), amode, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }


    /// Remove file
    void rm(const std::string &url)
    {
        GError *error = NULL;

        if (gfal2_unlink(context, url.c_str(), &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    /// Release file
    void releaseFile(const std::string &url, const std::string &token)
    {
        GError *error = NULL;

        if (gfal2_release_file(context, url.c_str(), token.c_str(), &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void mkdir_recursive(const std::string &url)
    {
        GError *error = NULL;

        if (gfal2_mkdir_rec(context, url.c_str(), 0775, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    /// Get the checksum of a file
    std::string getChecksum(const std::string &url, const std::string &type) {
        char buffer[512];
        GError *error = NULL;

        if (gfal2_checksum(context, url.c_str(), type.c_str(), 0, 0, buffer, sizeof(buffer), &error)) {
            throw Gfal2Exception(error);
        }

        return buffer;
    }

    /// Get the extended attribute of a resource
    std::string getXattr(const std::string &url, const std::string &name) {
        char buffer[1024];
        GError *error = NULL;

        if (gfal2_getxattr(context, url.c_str(), name.c_str(), buffer, sizeof(buffer), &error) < 0) {
            throw Gfal2Exception(error);
        }

        return buffer;
    }

    /// Get a string config value
    std::string get(const std::string &group, const std::string &key) {
        std::string value = "N/A";
        GError *error = NULL;
        char* cfgvalue = gfal2_get_opt_string(context, group.c_str(), key.c_str(), &error);

        if (error != NULL) {
            g_error_free(error);
        } else {
            value = cfgvalue;
            g_free(cfgvalue);
        }

        return value;
    }

    std::string tokenRetrieve(const std::string& url, const std::string& issuer, unsigned validity,
                              const std::vector<std::string>& activities) {
        char buff[2048];
        GError* error = NULL;
        std::vector<const char*> activity_list;

        activity_list.reserve(activities.size() + 1);
        for (auto it = activities.begin(); it != activities.end(); it++) {
            activity_list.push_back(it->c_str());
        }
        activity_list.push_back(NULL);

        ssize_t ret = gfal2_token_retrieve(context, url.c_str(), issuer.c_str(), false, validity,
                                           &activity_list[0], buff, sizeof(buff), &error);

        if (ret == -1) {
            throw Gfal2Exception(error);
        }

        return std::string(buff);
    }

private:
    gfal2_context_t context; ///< Gfal2 context object
    std::string src_token; ///< Source bearer token
    std::string dst_token; ///< Destination bearer token
};


#endif // GFAL2_CPP_H
