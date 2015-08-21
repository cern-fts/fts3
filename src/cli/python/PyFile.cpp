/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PyFile.h"

namespace fts3
{
namespace cli
{

PyFile::PyFile()
{
}

PyFile::PyFile(File& file)
{
    sources = file.sources;
    destinations = file.destinations;
    checksums = file.checksums;
    file_size = file.file_size;
    metadata = file.metadata;
    selection_strategy = file.selection_strategy;
}

PyFile::~PyFile()
{
}

void PyFile::setSources(py::list src)
{
    // check the size
    boost::python::ssize_t size = py::len(src);
    // loop over all files
    for (int i = 0; i < size; i++)
        {
            // extract the tuple
            sources.push_back(
                py::extract<std::string>(src[i])
            );

        }
}

py::list PyFile::getSources()
{

    py::list ret;

    std::vector<std::string>::iterator it;
    for (it = sources.begin(); it != sources.end(); it++)
        {
            ret.append(py::str(*it));
        }

    return ret;
}

void PyFile::setDestinations(py::list dest)
{
    // check the size
    boost::python::ssize_t size = py::len(dest);
    // loop over all files
    for (int i = 0; i < size; i++)
        {
            // extract the tuple
            destinations.push_back(
                py::extract<std::string>(dest[i])
            );

        }
}

py::list PyFile::getDestinations()
{

    py::list ret;

    std::vector<std::string>::iterator it;
    for (it = destinations.begin(); it != destinations.end(); it++)
        {
            ret.append(py::str(*it));
        }

    return ret;
}

void PyFile::setChecksums(py::list checksum)
{
    // check the size
    boost::python::ssize_t size = py::len(checksum);
    // loop over all files
    for (int i = 0; i < size; i++)
        {
            // extract the tuple
            checksums.push_back(
                py::extract<std::string>(checksum[i])
            );

        }
}

py::list PyFile::getChecksums()
{

    py::list ret;

    std::vector<std::string>::iterator it;
    for (it = checksums.begin(); it != checksums.end(); it++)
        {
            ret.append(py::str(*it));
        }

    return ret;
}

void PyFile::setFileSize(long filesize)
{
    file_size = filesize;
}

py::object PyFile::getFileSize()
{
    if (file_size.is_initialized()) return py::object(*file_size);
    return py::object();
}

void PyFile::setMetadata(py::str metadata)
{
    std::string s = py::extract<std::string>(metadata);
    this->metadata = s;
}

py::object PyFile::getMetadata()
{
    if (metadata.is_initialized()) return py::str(*metadata);
    return py::object();
}

void PyFile::setSelectionStrategy(py::str select)
{
    std::string s = py::extract<std::string>(select);
    selection_strategy = s;
}

py::object PyFile::getSelectionStrategy()
{
    if (selection_strategy.is_initialized()) return py::str(*selection_strategy);
    return py::object();

}

File PyFile::getFileCpp() const
{

    File f;
    f.sources = sources;
    f.destinations = destinations;
    f.checksums = checksums;
    f.file_size = file_size;
    f.metadata = metadata;
    f.selection_strategy = selection_strategy;

    return f;
}

} /* namespace cli */
} /* namespace fts3 */
