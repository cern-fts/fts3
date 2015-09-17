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

#pragma once

#include <boost/any.hpp>

#include "ws-ifce/gsoap/gsoap_stubs.h"


#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

namespace fts3 {
namespace server {

class GSoapAcceptor;

class GSoapRequestHandler
{

public:
    GSoapRequestHandler(GSoapAcceptor& acceptor);
    GSoapRequestHandler(const fts3::server::GSoapRequestHandler&);
    ~GSoapRequestHandler();

    void run(boost::any&);

private:
    soap* ctx;
    GSoapAcceptor& acceptor;
};

} // end namespace server
} // end namespace fts3
