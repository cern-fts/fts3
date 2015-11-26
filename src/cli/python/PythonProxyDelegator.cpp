/*
 * PythonProxyDelegator.cpp
 *
 *  Created on: 1 Apr 2014
 *      Author: simonm
 */

#include "PythonProxyDelegator.h"

#include "ServiceAdapterFallbackFacade.h"

namespace fts3
{
namespace cli
{

PythonProxyDelegator::PythonProxyDelegator(py::str endpoint, py::str delegationId, long expTime) :
    delegator(new ServiceAdapterFallbackFacade(py::extract<std::string>(endpoint))),
    delegationId(py::extract<std::string>(delegationId)), expTime(expTime)
{

}

PythonProxyDelegator::~PythonProxyDelegator()
{

}

void PythonProxyDelegator::delegate()
{
    delegator->delegate(delegationId, expTime);
}

long PythonProxyDelegator::isCertValid(void)
{
    return delegator->isCertValid();
}

}
}
