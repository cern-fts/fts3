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
 * GSoapContextAdapter.h
 *
 * PythonBindings.cpp
 *
 *  Created on: Sep 11, 2012
 *      Author: Michał Simon
 */

#include "GSoapContextAdapter.h"
#include "python/PythonApi.h"
#include "python/Job.h"
#include "python/FileTransfer.h"

#include <boost/optional/optional.hpp>
#include <boost/python.hpp>

using namespace boost;
using namespace boost::python;


void exTranslator(string const& ex) {
   PyErr_SetString(PyExc_UserWarning, ex.c_str());
}


BOOST_PYTHON_MODULE(libftspython) {

	using namespace fts3::cli;

	register_exception_translator<string>(exTranslator);

	class_<PythonApi>("FtsApi", init<str>())
			.def("submit", &fts3::cli::PythonApi::submit)
			.def("cancel", &fts3::cli::PythonApi::cancel)
			.def("getStatus", &fts3::cli::PythonApi::getStatus)
			;

	class_<fts3::cli::FileTransfer>("FileTransfer")
			.def(init<str, str, str>())
			.add_property("source", &FileTransfer::getSource, &FileTransfer::setSource)
			.add_property("destination", &FileTransfer::getDestination, &FileTransfer::setDestination)
			.add_property("checksum", &FileTransfer::getChecksum, &FileTransfer::setChecksum)
			;

	class_<fts3::cli::Job>("Job")
			.def("add", &Job::add)
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
}
