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
 *
 * ZipCompressor.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: simonm
 */

#include "ZipCompressor.h"

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/stream.hpp>

namespace fts3
{
namespace common
{

using namespace boost::iostreams;

void ZipCompressor::compress()
{

    filtering_streambuf<input> input_filter;
    input_filter.push(gzip_compressor());
    input_filter.push(in);
    copy(input_filter, out);

}

} /* namespace common */
} /* namespace fts3 */
