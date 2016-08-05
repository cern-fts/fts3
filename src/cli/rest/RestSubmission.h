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

#ifndef RESTSUBMISSION_H_
#define RESTSUBMISSION_H_

#include "../File.h"

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
    RestSubmission(std::vector<File> const & files,
        std::map<std::string, std::string> const & parameters, boost::property_tree::ptree const& extraParams) :
        files(files), parameters(parameters), extra(extraParams) {}

    virtual ~RestSubmission() {}

    friend std::ostream& operator<<(std::ostream& os, RestSubmission const & me);

protected:

    static void to_output(std::ostream & os, pt::ptree const & root);
    static std::string strip_values(std::string const & json);
    static void strip_values(std::string & json, std::string const & token);
    static void to_array(std::vector<std::string> const & v, pt::ptree & t);

    std::vector<File> const & files;
    std::map<std::string, std::string> const & parameters;
    boost::property_tree::ptree const& extra;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* RESTSUBMISSION_H_ */
