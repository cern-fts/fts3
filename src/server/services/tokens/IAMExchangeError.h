/*
 * Copyright (c) CERN 2023
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

#pragma once

#include "common/Exceptions.h"


class IAMExchangeError: public fts3::common::BaseException {

private:
    int _code;                     ///< HTTP response code
    std::string _message;          ///< HTTP response message
    Davix::DavixError* _dav_error;  ///< Davix request error

public:

    IAMExchangeError(int code, const std::string& message, Davix::DavixError* dav_error) :
            _code(code), _message(message), _dav_error(dav_error) {
        // Sanitize server response for newlines:
        // - remove trailing newlines
        // - replace inline newlines with space

        while (!_message.empty() && _message.back() == '\n') {
            _message.pop_back();
        }

        size_t pos;
        while ((pos = _message.find('\n')) != std::string::npos) {
            _message.replace(pos, 1, " ");
        }
    }

    ~IAMExchangeError() = default;

    const char* what() const noexcept {
        return _message.c_str();
    }

    int code() const noexcept {
        return _code;
    }

    std::string davix_error() const noexcept {
        if (_dav_error) {
            return _dav_error->getErrMsg();
        }

        return "Token exchange request failed (no additional info could be provided)";
    }
};
