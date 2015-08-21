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

#include "transfer_web_service.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

FTS3_SERVER_NAMESPACE_START

/* -------------------------------------------------------------------------- */

TransferWebService::TransferWebService()
    : WebServiceHandler <TransferWebServiceTraits> ("Transfer web service")
{}

FTS3_SERVER_NAMESPACE_END

