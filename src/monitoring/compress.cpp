#include <iostream>
#include <sstream>
#include <boost/assign/list_of.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include "compress.h"




typedef boost::posix_time::ptime TimeType;



//----------------------------------------------------------------------------

void

add_compressor

(

        boost::iostreams::filtering_streambuf<boost::iostreams::output>& out,

        CompressionScheme scheme

        )
 {

    switch (scheme)
 {

        case ZLIB_COMPRESSION:

        {

            out.push(boost::iostreams::zlib_compressor());

            break;

        }

        case BZIP2_COMPRESSION:

        {

            out.push(boost::iostreams::bzip2_compressor());

            break;

        }

        default:

        {

            break;

        }

    }

}



//----------------------------------------------------------------------------

void

add_decompressor

(

        boost::iostreams::filtering_streambuf<boost::iostreams::input>& in,

        CompressionScheme scheme

        )
 {

    switch (scheme)
 {

        case ZLIB_COMPRESSION:

        {

            in.push(boost::iostreams::zlib_decompressor());

            break;

        }

        case BZIP2_COMPRESSION:

        {

            in.push(boost::iostreams::bzip2_decompressor());

            break;

        }

        default:

        {

            break;

        }

    }

}



//----------------------------------------------------------------------------

std::string

compress

(

        const std::string& data

        )
 {

    std::stringstream compressed;

    std::stringstream decompressed;

    decompressed << data;

    boost::iostreams::filtering_streambuf<boost::iostreams::input> out;

    out.push(boost::iostreams::zlib_compressor());

    out.push(decompressed);

    boost::iostreams::copy(out, compressed);

    return compressed.str();

}



//----------------------------------------------------------------------------

std::string

compress1

(

        const std::string& data,

        CompressionScheme scheme

        )
 {

    std::string compressed;

    boost::iostreams::filtering_streambuf<boost::iostreams::output> out;

    add_compressor(out, scheme);

    out.push(boost::iostreams::back_inserter(compressed));

    boost::iostreams::copy(boost::make_iterator_range(data), out);

    return compressed;

}



//----------------------------------------------------------------------------

void

compress2

(

        const std::string& data,

        std::string& buffer,

        CompressionScheme scheme

        )
 {

    buffer.clear();

    boost::iostreams::filtering_streambuf<boost::iostreams::output> out;

    add_compressor(out, scheme);

    out.push(boost::iostreams::back_inserter(buffer));

    boost::iostreams::copy(boost::make_iterator_range(data), out);

}



//----------------------------------------------------------------------------

std::string

decompress

(

        const std::string& data

        )
 {

    std::stringstream compressed;

    std::stringstream decompressed;

    compressed << data;

    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;

    in.push(boost::iostreams::zlib_decompressor());

    in.push(compressed);

    boost::iostreams::copy(in, decompressed);

    return decompressed.str();

}



//----------------------------------------------------------------------------

std::string

decompress1

(

        const std::string& data,

        CompressionScheme scheme

        )
 {

    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;

    add_decompressor(in, scheme);

    in.push(boost::make_iterator_range(data));

    std::string decompressed;

    boost::iostreams::copy(in, boost::iostreams::back_inserter(decompressed));

    return decompressed;

}



//----------------------------------------------------------------------------

void

decompress2

(

        const std::string& buffer,

        std::string& data,

        CompressionScheme scheme

        )
 {

    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;

    add_decompressor(in, scheme);

    in.push(boost::make_iterator_range(buffer));

    data.clear();

    boost::iostreams::copy(in, boost::iostreams::back_inserter(data));

}



//----------------------------------------------------------------------------

TimeType

current_time

(

        )
 {

    return boost::posix_time::microsec_clock::local_time();

}



//----------------------------------------------------------------------------

double

elapsed_seconds

(

        const TimeType& start_time

        )
 {

    static const double MSEC_PER_SEC = 1000.0;

    TimeType end_time = current_time();

    boost::posix_time::time_duration elapsed = end_time - start_time;

    return boost::numeric_cast<double>(elapsed.total_milliseconds()) / MSEC_PER_SEC;

}



