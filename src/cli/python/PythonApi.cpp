/*
 * PythonCliWrapper.cpp
 *
 *  Created on: Sep 11, 2012
 *      Author: simonm
 */

#include "PythonApi.h"
#include "TransferTypes.h"

#include <boost/optional/optional.hpp>

#include <string>
#include <vector>
#include <map>


namespace fts3 { namespace cli {

using namespace boost;
using namespace std;

const object PythonApi::none = object();

PythonApi::PythonApi(str endpoint) : ctx(extract<string>(endpoint)) {
	ctx.init();
}

PythonApi::~PythonApi() {
}

str PythonApi::submit(list elements, dict parameters, bool checksum) {

	if (elements == none || parameters == none) {
		// TODO handle none
	}

	vector<JobElement> c_elements;

	int size = len(elements);
	for (int i = 0; i < size; i++) {
		boost::python::tuple element = extract<boost::python::tuple>(elements[i]);
		object source = element[0];
		object destination = element[1];
		object checksum = element[2];

		if (source == none || destination == none) {
//			 TODO handle none
		}


		JobElement e (
				extract<string>(source),
				extract<string>(destination),
				checksum == object() ? boost::optional<string>() : string(extract<string>(checksum))
			);

		c_elements.push_back(e);
	}

	map<string, string> c_parameters;

	list keys = parameters.keys();
	size = len(keys);
	for (int i = 0; i < size; i++) {

		object obj = keys[i];

		if (obj == none) {
			// TODO handle none
		}
		string param = extract<string>(obj);

		obj = parameters[param];
		if (obj == none) {
			// TODO handle none
		}
		string val = extract<string>(obj);

		c_parameters[param] = val;
	}

	return ctx.transferSubmit(c_elements, c_parameters, checksum).c_str();
}

void PythonApi::cancel(list ids) {

	if (ids == none) {
		// TODO handle none
	}

	vector<string> c_ids;

	int size = len(ids);
	for (int i = 0; i < size; i++) {

		object obj = ids[i];
		if (obj == none) {
			// TODO handle none
		}

		c_ids.push_back(extract<string>(obj));
	}

	ctx.cancel(c_ids);
}

str PythonApi::getStatus(str id) {

	if (id == none) {
		// TODO handle none
	}

	JobStatus s = ctx.getTransferJobStatus(extract<string>(id));
	return s.jobStatus.c_str();
}

}
}

