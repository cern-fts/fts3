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

#include "PythonProxyDelegator.h"

#include "delegation/SoapDelegator.h"

namespace fts3
{
namespace cli
{

PythonProxyDelegator::PythonProxyDelegator(py::str endpoint, py::str delegationId, long expTime) :
    delegator(new SoapDelegator(py::extract<std::string>(endpoint), py::extract<std::string>(delegationId), expTime, std::string()))
{

}

PythonProxyDelegator::~PythonProxyDelegator()
{

}

void PythonProxyDelegator::delegate()
{
    delegator->delegate();
}

long PythonProxyDelegator::isCertValid(void)
{
    return delegator->isCertValid();
}

}
}
