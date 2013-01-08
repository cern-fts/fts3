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
 * Job.cpp
 *
 *  Created on: Dec 20, 2012
 *      Author: Michał Simon
 */

#include "Job.h"

#include "common/JobParameterHandler.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

namespace fts3 {
namespace cli {

using namespace fts3::common;

Job::Job() : checksum(false), expiration(0) {

}

Job::~Job() {
}

std::vector<JobElement> Job::getJobElementsCpp() {
	return elements;
}

std::map<std::string, std::string> Job::getJobParametersCpp() {
	return parameters;
}

bool Job::useChecksumCpp() {
	return checksum;
}

void Job::add(FileTransfer file) {
	// prepare the job element
	JobElement e;
	boost::get<SOURCE>(e) = file.getSourceCpp();
	boost::get<DESTINATION>(e) = file.getDestinationCpp();
	boost::get<CHECKSUM>(e) = file.getChecksumCpp();
	// if at least one file transfer uses checksum set it to true
	checksum |= file.getChecksumCpp().is_initialized();
	// add the element
	elements.push_back(e);
}


void Job::addAll(py::list elements) {
	// check list size
	int size = len(elements);
	// iterate over all files
	for (int i = 0; i < size; i++) {
		FileTransfer file = py::extract<FileTransfer>(elements[i]);
		add(file);
	}
}

void Job::clear() {
	// remove all elements
	elements.erase(elements.begin(), elements.end());
}

py::list Job::files() {

	py::list ret;

	std::vector<JobElement>::iterator it;
	for (it = elements.begin(); it != elements.end(); it++) {
		JobElement e = *it;
		FileTransfer file (
				boost::get<SOURCE>(e),
				boost::get<DESTINATION>(e),
				boost::get<CHECKSUM>(e)
			);
		ret.append(file);
	}

	return ret;
}

void Job::setDelegationId(py::str id) {
	// set the value
	parameters[JobParameterHandler::DELEGATIONID] = py::extract<std::string>(id);
}

py::object Job::getDelegationId() {
	// if it's not in the map return none
	if (parameters.find(JobParameterHandler::DELEGATIONID) == parameters.end()) return py::object();
	// otherwise return the value in the map
	return py::str(parameters[JobParameterHandler::DELEGATIONID].c_str());
}

void Job::setGridParam(py::str param) {
	// set the value
	parameters[JobParameterHandler::GRIDFTP] = py::extract<std::string>(param);
}

py::object Job::getGridParam() {
	// if it's not in the map return none
	if (parameters.find(JobParameterHandler::GRIDFTP) == parameters.end()) return py::object();
	// otherwise return the value in the map
	return py::str(parameters[JobParameterHandler::GRIDFTP].c_str());
}

void Job::setExpirationTime(long expiration) {
	this->expiration = expiration;
}

py::object Job::getExpirationTime() {
	return py::object(expiration);
}

void Job::setOverwrite(bool overwrite) {
	// if true add to parameters, otherwise remove from parameters
	if (overwrite) {
		parameters[JobParameterHandler::OVERWRITEFLAG] = "Y";
	} else {
		parameters.erase(JobParameterHandler::OVERWRITEFLAG);
	}
}

py::object Job::overwrite() {
	return py::object(parameters.find(JobParameterHandler::OVERWRITEFLAG) != parameters.end());
}

void Job::setDestinationToken(py::str token) {
	// set the value
	parameters[JobParameterHandler::SPACETOKEN] = py::extract<std::string>(token);
}

py::object Job::getDestinationToken() {
	// if it's not in the map return none
	if (parameters.find(JobParameterHandler::SPACETOKEN) == parameters.end()) return py::object();
	// otherwise return the value in the map
	return py::str(parameters[JobParameterHandler::SPACETOKEN].c_str());
}

void Job::setCompareChecksum(bool cmp) {
	// if true add to parameters, otherwise remove from parameters
	if (cmp) {
		parameters[JobParameterHandler::CHECKSUM_METHOD] = "compare";
	} else {
		parameters.erase(JobParameterHandler::CHECKSUM_METHOD);
	}
}

py::object Job::compareChecksum() {
	return py::object(parameters.find(JobParameterHandler::CHECKSUM_METHOD) != parameters.end());
}

void Job::setCopyPinLifetime(int pin) {
	parameters[JobParameterHandler::COPY_PIN_LIFETIME] = boost::lexical_cast<string>(pin);
}

py::object Job::getCopyPinLifetime() {
	// if it's not in the map return 0
	if (parameters.find(JobParameterHandler::COPY_PIN_LIFETIME) == parameters.end()) return py::object();
	// otherwise
	return py::object(
			boost::lexical_cast<int>(parameters[JobParameterHandler::COPY_PIN_LIFETIME])
		);
}

void Job::setLanConnection(bool conn) {
	// if true add to parameters, otherwise remove from parameters
	if (conn) {
		parameters[JobParameterHandler::LAN_CONNECTION] = "Y";
	} else {
		parameters.erase(JobParameterHandler::LAN_CONNECTION);
	}
}

py::object Job::lanConnection() {
	return py::object(parameters.find(JobParameterHandler::LAN_CONNECTION) != parameters.end());
}

void Job::setFailNearline(bool fail) {
	// if true add to parameters, otherwise remove from parameters
	if (fail) {
		parameters[JobParameterHandler::FAIL_NEARLINE] = "Y";
	} else {
		parameters.erase(JobParameterHandler::FAIL_NEARLINE);
	}
}

py::object Job::failNearline() {
	return py::object(parameters.find(JobParameterHandler::FAIL_NEARLINE) != parameters.end());
}

void Job::setSessionReuse(bool reuse) {
	// if true add to parameters, otherwise remove from parameters
	if (reuse) {
		parameters[JobParameterHandler::REUSE] = "Y";
	} else {
		parameters.erase(JobParameterHandler::REUSE);
	}
}

py::object Job::sessionReuse() {
	return py::object(parameters.find(JobParameterHandler::REUSE) != parameters.end());
}

} /* namespace cli */
} /* namespace fts3 */
