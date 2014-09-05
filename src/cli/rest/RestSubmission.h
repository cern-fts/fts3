/*
 * RestSubmission.h
 *
 *  Created on: 19 Aug 2014
 *      Author: simonm
 */

#ifndef RESTSUBMISSION_H_
#define RESTSUBMISSION_H_

#include "File.h"

#include <vector>
#include <string>
#include <map>
#include <ostream>

#include <boost/property_tree/ptree.hpp>

namespace fts3
{
namespace cli
{

namespace pt = boost::property_tree;

class RestSubmission
{

public:
    RestSubmission(std::vector<File> const & files, std::map<std::string, std::string> const & parameters) :
        files(files), parameters(parameters) {}

    virtual ~RestSubmission() {}

    friend std::ostream& operator<<(std::ostream& os, RestSubmission const & me);

protected:

    static void to_output(std::ostream & os, pt::ptree const & root);
    static std::string strip_values(std::string const & json);
    static void strip_values(std::string & json, std::string const & token);
    static void to_array(std::vector<std::string> const & v, pt::ptree & t);

    std::vector<File> const & files;
    std::map<std::string, std::string> const & parameters;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* RESTSUBMISSION_H_ */
