/*
 * PyFile.h
 *
 *  Created on: Feb 18, 2013
 *      Author: simonm
 */

#ifndef PYFILE_H_
#define PYFILE_H_

#include "TransferTypes.h"

#include <boost/python.hpp>

namespace fts3 {
namespace cli {

namespace py = boost::python;

class PyFile : public File {

public:
	PyFile();

	PyFile(File& file);

	virtual ~PyFile();

	void setSources(py::list src);
	py::list getSources();

	void setDestinations(py::list dest);
	py::list getDestinations();

	void setChecksums(py::list checksum);
	py::list getChecksums();

	void setFileSize(long filesize);
	py::object getFileSize();

	void setMetadata(py::str metadata);
	py::object getMetadata();

	void setSelectionStrategy(py::str select);
	py::object getSelectionStrategy();

	File getFileCpp();
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* PYFILE_H_ */
