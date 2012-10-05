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

#include <boost/optional/optional.hpp>
#include <boost/python.hpp>

using namespace boost;
using namespace boost::python;


void exTranslator(string const& ex) {
   PyErr_SetString(PyExc_UserWarning, ex.c_str());
}


BOOST_PYTHON_MODULE(libftspython) {

	register_exception_translator<string>(exTranslator);

	class_<fts3::cli::PythonApi>("Fts3Api", init<str>())
			.def("submit", &fts3::cli::PythonApi::submit)
			.def("cancel", &fts3::cli::PythonApi::cancel)
			.def("getStatus", &fts3::cli::PythonApi::getStatus)
			;
}
