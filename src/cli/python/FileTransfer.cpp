/*
 * FileTransfer.cpp
 *
 *  Created on: Dec 21, 2012
 *      Author: simonm
 */

#include "FileTransfer.h"

namespace fts3 {
namespace cli {

FileTransfer::FileTransfer() {

}

FileTransfer::FileTransfer(py::str source, py::str destination, py::object checksum) {
	this->source = std::string(py::extract<std::string>(source));
	this->destination = std::string(py::extract<std::string>(destination));
	this->checksum = std::string(py::extract<std::string>(checksum));
}

FileTransfer::~FileTransfer() {

}

void FileTransfer::setSource(py::str source) {
	this->source = std::string(py::extract<std::string>(source));
}

py::object FileTransfer::getSource() {
	// if it has not been set
	if (!source.is_initialized()) return py::object();
	// otherwise
	return py::str(source.get().c_str());
}

void FileTransfer::setDestination(py::str destination) {
	this->destination = std::string(py::extract<std::string>(destination));
}

py::object FileTransfer::getDestination() {
	// if it has not been set
	if (!destination.is_initialized()) return py::object();
	// otherwise
	return py::str(destination.get().c_str());
}

void FileTransfer::setChecksum(py::str checksum) {
	this->checksum = std::string(py::extract<std::string>(checksum));
}

py::object FileTransfer::getChecksum() {
	// if it has not been set
	if (!checksum.is_initialized()) return py::object();
	// otherwise
	return py::str(checksum.get().c_str());
}

} /* namespace cli */
} /* namespace fts3 */
