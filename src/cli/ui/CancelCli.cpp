/*
 * CancelCli.cpp
 *
 *  Created on: Mar 5, 2014
 *      Author: simonm
 */

#include "CancelCli.h"

#include "exception/bad_option.h"

#include <boost/regex.hpp>

namespace fts3
{
namespace cli
{

using namespace boost;

CancelCli::CancelCli()
{

    specific.add_options()
    ("file,f", value<string>(&bulk_file), "Name of a configuration file.")
    ;
}

CancelCli::~CancelCli()
{

}


bool CancelCli::validate()
{

    // do the standard validation
    if (!CliBase::validate()) return false;

    if (!vm.count("file") && !vm.count("jobid"))
        {
    		throw bad_option("file", "Either the bulk file or job ID list may be used, can't use both!");
        }

    prepareJobIds();

    return true;
}

void CancelCli::prepareJobIds()
{
    // first check if the -f option was used, try to open the file with bulk-job description
    ifstream ifs(bulk_file.c_str());
    if (ifs)
        {
            // Parse the file
            int lineCount = 0;
            string line;
            // read and parse the lines one by one
            do
                {
                    lineCount++;
                    getline(ifs, line);

                    if (line.empty()) continue;

                    // regular expression for matching job ID
                    // job ID example: 11d24106-a24b-4d9f-9360-7c36c5176327
                    static const regex re("^\\w+-\\w+-\\w+-\\w+-\\w+$");
                    smatch what;
                    if (!regex_match(line, what, re, match_extra)) throw bad_option("jobid", "Wrong job ID format: " + line);

                    jobIds.push_back(line);

                }
            while (!ifs.eof());
        }
    else
        {
            // check whether jobid has been given as a parameter
            if (vm.count("jobid"))
                {
                    jobIds = vm["jobid"].as< vector<string> >();
                }
        }
}

} /* namespace cli */
} /* namespace fts3 */
