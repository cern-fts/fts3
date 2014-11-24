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

CancelCli::CancelCli()
{

    specific.add_options()
    ("file,f", po::value<std::string>(&bulk_file), "Name of a configuration file.")
    ;
}

CancelCli::~CancelCli()
{

}

void CancelCli::validate()
{
    if (!vm.count("file") && !vm.count("jobid"))
        {
            throw bad_option("file", "Either the bulk file or job ID list may be used, can't use both!");
        }

    prepareJobIds();
}

void CancelCli::prepareJobIds()
{
    // first check if the -f option was used, try to open the file with bulk-job description
    std::ifstream ifs(bulk_file.c_str());
    if (ifs)
        {
            // Parse the file
            int lineCount = 0;
            std::string line;
            // read and parse the lines one by one
            do
                {
                    lineCount++;
                    getline(ifs, line);

                    if (line.empty()) continue;

                    // regular expression for matching job ID
                    // job ID example: 11d24106-a24b-4d9f-9360-7c36c5176327
                    static const boost::regex re("^\\w+-\\w+-\\w+-\\w+-\\w+$");
                    boost::smatch what;
                    if (!boost::regex_match(line, what, re, boost::match_extra)) throw bad_option("jobid", "Wrong job ID format: " + line);

                    jobIds.push_back(line);

                }
            while (!ifs.eof());
        }
    else
        {
            // check whether jobid has been given as a parameter
            if (vm.count("jobid"))
                {
                    jobIds = vm["jobid"].as< std::vector<std::string> >();
                }
        }
}

} /* namespace cli */
} /* namespace fts3 */
