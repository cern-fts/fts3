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

#ifndef URLCOPY_ERROR_H
#define URLCOPY_ERROR_H

#include "common/Exceptions.h"
#include "Gfal2.h"
#include "heuristics.h"

#include <boost/optional.hpp>

// ERROR SCOPE
#define TRANSFER "TRANSFER"
#define DESTINATION "DESTINATION"
#define AGENT "GENERAL_FAILURE"
#define SOURCE "SOURCE"

// ERROR PHASE
#define TRANSFER_PREPARATION "TRANSFER_PREPARATION"
//#define TRANSFER "TRANSFER"
#define TRANSFER_FINALIZATION "TRANSFER_FINALIZATION"
#define TRANSFER_SERVICE "TRANSFER_SERVICE"


class UrlCopyError: public fts3::common::BaseException {

private:
    std::string scope_;
    std::string phase_;
    int code_;
    std::string msg_;

public:
    UrlCopyError(const std::string &scope, const std::string &phase, int code, const std::string &msg):
        scope_(scope), phase_(phase), code_(code), msg_(msg) {
    }

    UrlCopyError(const std::string &scope, const std::string &phase, const Gfal2Exception &ex):
        scope_(scope), phase_(phase), code_(ex.code()), msg_(ex.what()) {
    }

    ~UrlCopyError() throw () {
    }

    const char *what() const throw () {
        return msg_.c_str();
    }

    const char *scope() const throw() {
        return scope_.c_str();
    }

    const char *phase() const throw () {
        return phase_.c_str();
    }

    int code(void) const throw () {
        return code_;
    }

    bool isRecoverable(void) const throw() {
        return retryTransfer(code(), scope(), what());
    }
};

#endif // URLCOPY_ERROR_H
