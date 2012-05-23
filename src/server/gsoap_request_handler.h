/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once

#include <boost/type_traits/is_base_of.hpp>
#include <boost/static_assert.hpp>

#include "server_dev.h"
#include "common/pointers.h"
#include "common/logger.h"
#include "common/error.h"
#include "ws/gsoap_stubs.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST
    #include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

FTS3_SERVER_NAMESPACE_START

using namespace FTS3_COMMON_NAMESPACE;


class GSoapRequestHandler
{
public:
    GSoapRequestHandler (::soap* soap): soap(soap) {};
    ~GSoapRequestHandler ();

    void handle();


private:
    ::soap* soap;
};

#ifdef FTS3_COMPILE_WITH_UNITTEST

#endif // FTS3_COMPILE_WITH_UNITTESTS

FTS3_SERVER_NAMESPACE_END

