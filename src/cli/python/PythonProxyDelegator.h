/*
 * PythonProxyDelegator.h
 *
 *  Created on: 1 Apr 2014
 *      Author: simonm
 */

#ifndef PYTHONPROXYDELEGATOR_H_
#define PYTHONPROXYDELEGATOR_H_

#include "delegation/ProxyCertificateDelegator.h"
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
    long isCertValid(py::str filename);

private:
    stringstream out;
    std::unique_ptr<ProxyCertificateDelegator> delegator;
};

}
}

#endif /* PYTHONPROXYDELEGATOR_H_ */
