/*
 * PythonProxyDelegator.cpp
 *
 *  Created on: 1 Apr 2014
 *      Author: simonm
 */

#include "PythonProxyDelegator.h"

namespace fts3
{
namespace cli
{

PythonProxyDelegator::PythonProxyDelegator(py::str endpoint, py::str delegationId, long expTime) :
    delegator(py::extract<string>(endpoint), py::extract<string>(delegationId), expTime)
{

}

PythonProxyDelegator::~PythonProxyDelegator()
{

}

void PythonProxyDelegator::delegate()
{
    delegator.delegate();
}

long PythonProxyDelegator::isCertValid(py::str filename)
{
    return delegator.isCertValid(py::extract<string>(filename));
}

}
}
