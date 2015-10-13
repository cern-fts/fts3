/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * PythonBindings.cpp
 *
 *  Created on: Sep 11, 2012
 *      Author: Michał Simon
 */

#include "GSoapContextAdapter.h"
#include "python/PythonApi.h"
#include "python/Job.h"
#include "python/PyFile.h"
#include "python/PythonProxyDelegator.h"

#include "common/error.h"

#include <boost/optional/optional.hpp>
#include <boost/python.hpp>

#include <exception>

using namespace fts3::common;

void exceptTranslator(std::exception const& ex)
{
    PyErr_SetString(PyExc_UserWarning, ex.what());
}


void errExTranslator(Err_Custom const& ex)
{
    PyErr_SetString(PyExc_UserWarning, ex.what());
}

BOOST_PYTHON_MODULE(libftspython)
{
    using namespace fts3::cli;

#if PY_VERSION_HEX >= 0x02050000
    PyErr_WarnEx(PyExc_DeprecationWarning,
        "libftspython is deprecated. Please, migrate to fts3.rest.client(.easy)", 1);
#else
    PyErr_Warn(PyExc_DeprecationWarning,
        "libftspython is deprecated. Please, migrate to fts3.rest.client(.easy)");
#endif

    py::register_exception_translator<std::exception>(exceptTranslator);
    py::register_exception_translator<Err_Custom>(errExTranslator);

    py::class_<PythonApi>("Fts", py::init<py::str>())
    .def("submit", &PythonApi::submit)
    .def("cancel", &PythonApi::cancel)
    .def("cancel", &PythonApi::cancelAll)
    .def("status", &PythonApi::getStatus)
    .def("getVersion", &PythonApi::getVersion)
    .def("setPriority", &PythonApi::setPriority)
    ;

    py::class_<fts3::cli::PyFile>("File")
    .add_property("sources", &PyFile::getSources, &PyFile::setSources)
    .add_property("destinations", &PyFile::getDestinations, &PyFile::setDestinations)
    .add_property("checksums", &PyFile::getChecksums, &PyFile::setChecksums)
    .add_property("filesize", &PyFile::getFileSize, &PyFile::setFileSize)
    .add_property("metadata", &PyFile::getMetadata, &PyFile::setMetadata)
    .add_property("selectionStrategy", &PyFile::getSelectionStrategy, &PyFile::setSelectionStrategy)
    ;

    py::class_<fts3::cli::Job>("Job", py::init<PyFile>())
    .def(py::init<py::list>())
    .add_property("files", &Job::files)
    .add_property("delegationId", &Job::getDelegationId, &Job::setDelegationId)
    .add_property("gridParam", &Job::getGridParam, &Job::setGridParam)
    .add_property("expirationTime", &Job::getExpirationTime, &Job::setExpirationTime)
    .add_property("overwrite", &Job::overwrite, &Job::setOverwrite)
    .add_property("destinationToken", &Job::getDestinationToken, &Job::setDestinationToken)
    .add_property("compareChecksum", &Job::compareChecksum, &Job::setCompareChecksum)
    .add_property("copyPinLifetime", &Job::getCopyPinLifetime, &Job::setCopyPinLifetime)
    .add_property("lanConnection", &Job::lanConnection, &Job::setLanConnection)
    .add_property("failNearline", &Job::failNearline, &Job::setFailNearline)
    .add_property("sessionReuse", &Job::sessionReuse, &Job::setSessionReuse)
    ;

    py::class_<fts3::cli::PythonProxyDelegator, boost::noncopyable>("Delegator", py::init<py::str, py::str, long>())
    .def("delegate", &PythonProxyDelegator::delegate)
    .def("isCertValid", &PythonProxyDelegator::isCertValid)
    ;
}
