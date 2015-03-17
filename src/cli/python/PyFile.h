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
 * PyFile.h
 *
 *  Created on: Feb 18, 2013
 *      Author: Michał Simon
 */

#ifndef PYFILE_H_
#define PYFILE_H_

#include "File.h"

#include <boost/python.hpp>

namespace fts3
{
namespace cli
{

namespace py = boost::python;

class PyFile : public File
{

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

    File getFileCpp() const;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* PYFILE_H_ */
