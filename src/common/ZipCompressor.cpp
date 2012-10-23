/*
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

namespace fts3 {
namespace common {

using namespace boost::iostreams;

void ZipCompressor::compress() {

	filtering_streambuf<input> input_filter;
	input_filter.push(gzip_compressor());
	input_filter.push(in);
	copy(input_filter, out);

}

} /* namespace common */
} /* namespace fts3 */
