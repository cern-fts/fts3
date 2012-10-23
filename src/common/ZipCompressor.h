/*
 * ZipCompressor.h
 *
 *  Created on: Oct 23, 2012
 *      Author: simonm
 */

#ifndef ZIPCOMPRESSOR_H_
#define ZIPCOMPRESSOR_H_

#include <iostream>
#include <string>

namespace fts3 { namespace common {

using namespace std;

class ZipCompressor {

public:
	ZipCompressor(istream& in, ostream& out): in(in), out(out) {}
	virtual ~ZipCompressor(){}

	void compress();

private:
	ostream& out; // a binary file e.g.
	istream& in;
};

} /* namespace common */ } /* namespace fts3 */
#endif /* ZIPCOMPRESSOR_H_ */
