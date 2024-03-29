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

#ifndef MSGPRINTER_H_
#define MSGPRINTER_H_

#include "JobStatus.h"
#include "JsonOutput.h"
#include "exception/cli_exception.h"

#include <iostream>
#include <string>
#include <vector>
#include <ostream>

#include <boost/property_tree/ptree.hpp>

namespace fts3
{
namespace cli
{

namespace pt = boost::property_tree;

class MsgPrinter
{

public:

    bool getVerbose() const {
        return this->verbose;
    }

    static MsgPrinter & instance()
    {
        static MsgPrinter printer;
        return printer;
    }

    void setOutputStream(std::ostream & ostr)
    {
        this->ostr = &ostr;
        jout.setOutputStream(ostr);
    }

    void print_info(std::string const & ostr_subject, std::string const & json_subject, long int h, long int m);
    void print_info(std::string const & msg);
    void print_info(std::string const & ostr_subject, std::string const & json_subject, std::string const & msg);
    void print_info(std::string const & json_subject, std::string const & msg);
    void print_info(std::string const & ostr_subject, std::string const & json_subject, bool flag);

    template<typename T>
    void print(std::vector<T> const & v);
    void print(std::string job_id, std::vector<DetailedFileStatus> const & v);

    void print(cli_exception const & ex);
    void print(std::exception const & ex);

    void print(std::string const & subject, std::string const & msg);
    void print(std::string const & ostr_subject, std::string const & json_subject, std::string const & msg);

    void print(JobStatus const & status);


    virtual ~MsgPrinter() {}

    void setVerbose(bool verbose)
    {
        this->verbose = verbose;
    }

    void setJson(bool json)
    {
        this->json = json;
    }


private:

    MsgPrinter() : ostr(&std::cout), jout(std::cout), verbose(false), json(false) {}
    MsgPrinter(MsgPrinter const &);
    MsgPrinter & operator=(MsgPrinter const &);

    void print_ostr(std::pair<std::string, std::string> const & id_status);
    void print_json(std::pair<std::string, std::string> const & id_status);

    void print_ostr(std::pair<std::string, int> const & keyval);
    void print_json(std::pair<std::string, int> const & keyval);

    void print_ostr(JobStatus const & j, bool short_out);
    void print_ostr(JobStatus const & j);
    void print_json(JobStatus const & j);

    template<typename T>
    void print_ostr() {}

    template<typename T>
    void print_json() {}

    ///
    std::ostream * ostr;
    ///
    JsonOutput jout;
    ///
    bool verbose;
    ///
    bool json;
};

template<typename T>
void MsgPrinter::print(std::vector<T> const & v)
{
    if (v.empty())
        {
            if (json) print_json<T>();
            else print_ostr<T>();
            return;
        }

    void (MsgPrinter::*print)(T const &);

    if (json) print = &MsgPrinter::print_json;
    else print = &MsgPrinter::print_ostr;

    std::for_each(v.begin(), v.end(), boost::bind(print, this, _1));
}

template<>
inline void MsgPrinter::print_ostr<JobStatus>()
{
    (*ostr) << "No data have been found for the specified state(s) and/or user VO/VOMS roles." << std::endl;
}

template<>
inline void MsgPrinter::print_json<JobStatus>()
{
    jout.print("job", "[]");
}

} /* namespace server */
} /* namespace fts3 */

#endif /* MSGPRINTER_H_ */
