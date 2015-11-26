/*
 * PythonProxyDelegator.h
 *
 *  Created on: 1 Apr 2014
 *      Author: simonm
 */

#ifndef PYTHONPROXYDELEGATOR_H_
#define PYTHONPROXYDELEGATOR_H_

#include "ServiceAdapterFallbackFacade.h"
#include "MsgPrinter.h"

#include <sstream>

#include <boost/python.hpp>

namespace fts3
{
namespace cli
{

namespace py = boost::python;

class PythonProxyDelegator
{

public:
    PythonProxyDelegator(py::str endpoint, py::str delegationId, long expTime);
    virtual ~PythonProxyDelegator();

    void delegate();
    long isCertValid();

private:
    std::stringstream out;
    std::unique_ptr<ServiceAdapterFallbackFacade> delegator;
    std::string delegationId;
    long expTime;
};

}
}

#endif /* PYTHONPROXYDELEGATOR_H_ */
