/*
 * Copyright (c) CERN 2016
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

#include <gfal_api.h>


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
    }


    /// Move constructor
    Gfal2TransferParams(Gfal2TransferParams &&orig) {
        params = orig.params;
        orig.params = NULL;
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

    void enableChecksum(bool value)
    {
        GError *error = NULL;
        if (gfalt_set_checksum_check(params, value, &error) < 0) {
            throw Gfal2Exception(error);
        }
    }

    void setUserDefinedChecksum(const std::string &type, const std::string &value) {
        GError *error = NULL;
        if (gfalt_set_user_defined_checksum(params, type.c_str(), value.c_str(), &error) < 0) {
            throw Gfal2Exception(error);
        }
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

    unsigned getNumberOfStreams(void)
    {
        GError *error = NULL;
        unsigned val = gfalt_get_nbstreams(params, &error);
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

    unsigned getTcpBuffersize(void)
    {
        GError *error = NULL;
        unsigned val = gfalt_get_tcp_buffer_size(params, &error);
        if (error) {
            throw Gfal2Exception(error);
        }
        return val;
    }

    unsigned getTimeout(void)
    {
        GError *error = NULL;
        unsigned val = gfalt_get_timeout(params, &error);
        if (error) {
            throw Gfal2Exception(error);
        }
        return val;
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
private:
    gfal2_context_t context;

public:

    /// Constructor
    Gfal2() {
        GError *error = NULL;
        context = gfal2_context_new(&error);
        if (context == NULL ){
            throw Gfal2Exception(error);
        }
    }

    /// Destructor
    ~Gfal2() {
        gfal2_context_free(context);
    }

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
    void cancel(void) {
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
};


#endif // GFAL2_CPP_H
