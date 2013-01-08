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
	FileTransfer(std::string source, std::string destination, boost::optional<std::string> checksum);
	FileTransfer(py::str source, py::str destination, py::str checksum);
	FileTransfer(py::str source, py::str destination);

	virtual ~FileTransfer();

	void setSource(py::str source);
	py::str getSource();
	std::string getSourceCpp();

	void setDestination(py::str destination);
	py::str getDestination();
	std::string getDestinationCpp();

	void setChecksum(py::str checksum);
	py::object getChecksum();
	boost::optional<std::string> getChecksumCpp();

private:

	bool wrongChecksumFormat(std::string checksum);

	std::string source;
	std::string destination;
	boost::optional<std::string> checksum;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* FILETRANSFER_H_ */
