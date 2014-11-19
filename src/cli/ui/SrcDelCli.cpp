/*
 *	Copyright notice:
 *	Copyright Â© Members of the EMI Collaboration, 2010.
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
 * SrcDelCli.cpp
 *
 *  Created on: Nov 11, 2013
 *      Author: Christina Skarpathiotaki
 */



#include "SrcDelCli.h"
#include "BulkSubmissionParser.h"

#include "common/JobParameterHandler.h"

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/assign.hpp>
#include <boost/regex.hpp>

#include <algorithm>
#include <stdexcept>

using namespace fts3::cli;


SrcDelCli::SrcDelCli()
{

    // add commandline option specific for the fts3-transfer-delete
    specific.add_options()
    ("source-token,S", value<std::string>(), "The source space token or its description (for SRM 2.2 transfers).")
    ("file,f", value<std::string>(&bulk_file), "Name of a the bulk submission file.")
    ;
    // add hidden options
    hidden.add_options()
    ("Filename", value< std::vector<std::string> >(&allFilenames)->multitoken(), "Specify the FileName(s).")
    ;
    // add optional (that used without an option switch) command line option
    p.add("Filename", -1);
}

SrcDelCli::~SrcDelCli()
{

}

void SrcDelCli::validate(bool /*init*/)
{
    // do the validation
    // In case of a user types an invalid expression...
    if (vm.count("file") && vm.count("Filename"))
        {
            // print err
            throw cli_exception("If a filename submission has been used each URL of files has to be specified inside the file separately for each file!");
        }

    // first check if the -f option was used, try to open the file with bulk-job description
    if (vm.count("file"))
        {
        std::ifstream ifs(bulk_file.c_str());
            if(ifs)// -f option is used
                {
                    // Parse the file...
                    int lineCount = 0;
                    std::string line;
                    do
                        {
                            lineCount++;
                            getline(ifs, line);
                            // expecting just 1 element for each line
                            if (!line.empty()) allFilenames.push_back(line);
                        }
                    while(!ifs.eof());
                }
            else throw bad_option("file", "does not exist");
        }

    std::for_each(allFilenames.begin(), allFilenames.end(), validateFileName);
}

void SrcDelCli::validateFileName(std::string const & url)
{
    static regex const fileUrlRegex("([a-zA-Z][a-zA-Z0-9+\\.-]*://[a-zA-Z0-9\\.-]+)(:\\d+)?/.+");

    // check the regular expression
    smatch what;
    if (!regex_match(url, what,fileUrlRegex, match_extra))
        {
            throw cli_exception("Wrong URL format: " + url);
        }
}

std::vector<std::string> SrcDelCli::getFileName()
{
    return allFilenames;
}




