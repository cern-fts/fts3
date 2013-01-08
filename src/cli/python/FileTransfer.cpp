/*
 * FileTransfer.cpp
 *
 *  Created on: Dec 21, 2012
 *      Author: simonm
 */

#include "FileTransfer.h"

namespace fts3 {
namespace cli {

FileTransfer::FileTransfer(std::string source, std::string destination, boost::optional<std::string> checksum) :
		source(source),
		destination(destination),
		checksum(checksum) {
}

FileTransfer::FileTransfer(py::str source, py::str destination, py::str checksum) {
	this->source = std::string(py::extract<std::string>(source));
	this->destination = std::string(py::extract<std::string>(destination));

	std::string tmp = py::extract<std::string>(checksum);
	if (wrongChecksumFormat(tmp)) throw std::string("checksum format is not valid (ALGORITHM:1234af)");

	this->checksum = tmp;
}

FileTransfer::FileTransfer(py::str source, py::str destination) {
	this->source = std::string(py::extract<std::string>(source));
	this->destination = std::string(py::extract<std::string>(destination));
}

FileTransfer::~FileTransfer() {

}

void FileTransfer::setSource(py::str source) {
	this->source = std::string(py::extract<std::string>(source));
}

py::str FileTransfer::getSource() {
	return py::str(source.c_str());
}

void FileTransfer::setDestination(py::str destination) {
	this->destination = std::string(py::extract<std::string>(destination));
}

py::str FileTransfer::getDestination() {
	return py::str(destination.c_str());
}

void FileTransfer::setChecksum(py::str checksum) {

	std::string tmp = py::extract<std::string>(checksum);
	if (wrongChecksumFormat(tmp)) throw std::string("checksum format is not valid (ALGORITHM:1234af)");

	this->checksum = tmp;
}

py::object FileTransfer::getChecksum() {
	// if it has not been set
	if (!checksum.is_initialized()) return py::object();
	// otherwise
	return py::str(checksum.get().c_str());
}


std::string FileTransfer::getSourceCpp() {
	return source;
}

std::string FileTransfer::getDestinationCpp() {
	return destination;
}

boost::optional<std::string> FileTransfer::getChecksumCpp() {
	return checksum;
}

bool FileTransfer::wrongChecksumFormat(std::string checksum) {
	// check if there is a colon
	std::string::size_type colon = checksum.find(":");
	if (colon == std::string::npos || colon == 0 || colon == checksum.size() - 1) return true;
	// if yes the format is not wrong
	return false;
}

} /* namespace cli */
} /* namespace fts3 */
