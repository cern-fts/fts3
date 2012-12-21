/*
 * FileTransfer.h
 *
 *  Created on: Dec 21, 2012
 *      Author: simonm
 */

#ifndef FILETRANSFER_H_
#define FILETRANSFER_H_

#include <boost/python.hpp>
#include <boost/optional.hpp>

#include <string>

namespace fts3 {
namespace cli {

namespace py = boost::python;

class FileTransfer {

public:
	FileTransfer();
	FileTransfer(py::str source, py::str destination, py::object checksum);

	virtual ~FileTransfer();

	void setSource(py::str source);
	py::object getSource();

	void setDestination(py::str destination);
	py::object getDestination();

	void setChecksum(py::str checksum);
	py::object getChecksum();

//private:
	boost::optional<std::string> source;
	boost::optional<std::string> destination;
	boost::optional<std::string> checksum;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* FILETRANSFER_H_ */
