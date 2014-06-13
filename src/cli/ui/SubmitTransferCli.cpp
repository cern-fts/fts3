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
 */

/*
 * SubmitTransferCli.cpp
 *
 *  Created on: Feb 2, 2012
 *      Author: Michal Simon
 */

#include "SubmitTransferCli.h"
#include "BulkSubmissionParser.h"

#include "exception/bad_option.h"

#include "common/JobParameterHandler.h"
#include "common/parse_url.h"

#include <iostream>
#include <fstream>
#include <termios.h>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/assign.hpp>

using namespace boost::algorithm;
using namespace boost;
using namespace boost::assign;
using namespace fts3::cli;
using namespace fts3::common;

SubmitTransferCli::SubmitTransferCli()
{
    delegate = true;

    /// 8 housrs in seconds
    static const int eight_hours = 28800;

    // by default we don't use checksum
    checksum = false;

    // add commandline options specific for fts3-transfer-submit
    specific.add_options()
    ("blocking,b", "Blocking mode, wait until the operation completes.")
    ("file,f", value<string>(&bulk_file), "Name of a the bulk submission file.")
    ("gparam,g", value<string>(), "Gridftp parameters.")
    ("interval,i", value<int>(), "Interval between two poll operations in blocking mode.")
//			("myproxysrv,m", value<string>(), "MyProxy server to use.")
//			("password,p", value<string>(), "MyProxy password to send with the job")
    ("overwrite,o", "Overwrite files.")
    ("dest-token,t", value<string>(),  "The destination space token or its description (for SRM 2.2 transfers).")
    ("source-token,S", value<string>(), "The source space token or its description (for SRM 2.2 transfers).")
    ("compare-checksum,K", "Compare checksums between source and destination.")
    ("copy-pin-lifetime", value<int>()->implicit_value(eight_hours)->default_value(-1), "Pin lifetime of the copy of the file (seconds), if the argument is not specified a default value of 28800 seconds (8 hours) is used.")
    ("bring-online", value<int>()->implicit_value(eight_hours)->default_value(-1), "Bring online timeout expressed in seconds, if the argument is not specified a default value of 28800 seconds (8 hours) is used.")
    ("reuse,r", "enable session reuse for the transfer job")
    ("multi-hop,m", "enable multi-hopping")
    ("job-metadata", value<string>(), "transfer-job metadata")
    ("file-metadata", value<string>(), "file metadata")
    ("file-size", value<double>(), "file size (in Bytes)")
    ("json-submission", "The bulk submission file will be expected in JSON format")
    ("retry", value<int>(), "Number of retries. If 0, the server default will be used. If negative, there will be no retries.")
    ("retry-delay", value<int>()->default_value(0), "Retry delay in seconds")
    ("nostreams", value<int>(), "number of streams that will be used for the given transfer-job")
    ("timeout", value<int>(), "timeout (expressed in seconds) that will be used for the given job")
    ("buff-size", value<int>(), "buffer size (expressed in bytes) that will be used for the given transfer-job")
    ;

    // add hidden options
    hidden.add_options()
    ("checksum", value<string>(), "Specify checksum algorithm and value (ALGORITHM:1234af).")
    ;

    // add positional (those used without an option switch) command line options
    p.add("checksum", 1);

}

SubmitTransferCli::~SubmitTransferCli()
{

}

void SubmitTransferCli::parse(int ac, char* av[])
{

    // do the basic initialization
    CliBase::parse(ac, av);

    // check whether to use delegation
    if (vm.count("id"))
        {
            delegate = true;
        }
}

bool SubmitTransferCli::validate()
{

    // do the standard validation
    if (!CliBase::validate()) return false;

    // perform standard checks in order to determine if the job was well specified
    if (!performChecks()) return false;

    // prepare job elements
    if (!createJobElements()) return false;

    return true;
}

optional<string> SubmitTransferCli::getMetadata()
{

    if (vm.count("job-metadata"))
        {
            return vm["job-metadata"].as<string>();
        }
    return optional<string>();
}

bool SubmitTransferCli::checkValidUrl(const std::string &uri, MsgPrinter& msgPrinter)
{
    Uri u0 = Uri::Parse(uri);
    bool ok = u0.Host.length() != 0 && u0.Protocol.length() != 0 && u0.Path.length() != 0;
    if (!ok)
        {
            std::string errMsg = "Not valid uri format, check submitted uri's";
            msgPrinter.error_msg(errMsg);
            return false;
        }

    return true;
}


bool SubmitTransferCli::createJobElements()
{

    // first check if the -f option was used, try to open the file with bulk-job description
    ifstream ifs(bulk_file.c_str());
    if (ifs)
        {

            if (vm.count("json-submission"))
                {

                    BulkSubmissionParser bulk(ifs);
                    files = bulk.getFiles();

                }
            else
                {

                    // Parse the file
                    int lineCount = 0;
                    string line;
                    // define the seperator characters (space) for tokenizing each line
                    char_separator<char> sep(" ");
                    // read and parse the lines one by one
                    do
                        {
                            lineCount++;
                            getline(ifs, line);

                            // split the line into tokens
                            tokenizer< char_separator<char> > tokens(line, sep);
                            tokenizer< char_separator<char> >::iterator it;

                            // we are expecting up to 3 elements in each line
                            // source, destination and optionally the checksum
                            File file;

                            // the first part should be the source
                            it = tokens.begin();
                            if (it != tokens.end())
                                {
                                    string s = *it;
                                    if (!checkValidUrl(s, msgPrinter)) return false;
                                    file.sources.push_back(s);
                                }
                            else
                                // if the line was empty continue
                                continue;

                            // the second part should be the destination
                            it++;
                            if (it != tokens.end())
                                {
                                    string s = *it;
                                    if (!checkValidUrl(s, msgPrinter)) return false;
                                    file.destinations.push_back(s);
                                }
                            else
                                {
                                    // only one element is still not enough to define a job
                                    printer().bulk_submission_error(lineCount, "destination is missing");
                                    return false;
                                }

                            // the third part should be the checksum (but its optional)
                            it++;
                            if (it != tokens.end())
                                {
                                    string checksum_str = *it;

                                    checksum = true;
                                    file.checksums.push_back(checksum_str);
                                }

                            files.push_back(file);

                        }
                    while (!ifs.eof());

                }

        }
    else
        {

            // the -f was not used, so use the values passed via CLI

            // first, if the checksum algorithm has been given check if the
            // format is correct (ALGORITHM:1234af)
            vector<string> checksums;
            if (vm.count("checksum"))
                {
                    checksums.push_back(vm["checksum"].as<string>());
                    this->checksum = true;
                }

            // check if size of the file has been specified
            optional<double> filesize;
            if (vm.count("file-size"))
                {
                    filesize = vm["file-size"].as<double>();
                }

            // check if there are some file metadata
            optional<string> file_metadata;
            if (vm.count("file-metadata"))
                {
                    file_metadata = vm["file-metadata"].as<string>();
                }

            // then if the source and destination have been given create a Task
            if (!getSource().empty() && !getDestination().empty())
                {

                    files.push_back (
                        File (list_of(getSource()), list_of(getDestination()), checksums, filesize, file_metadata)
                    );
                }
        }

    return true;
}

vector<File> SubmitTransferCli::getFiles()
{
    if (files.empty())
        {
            throw bad_option ("missing parameter", "No transfer job has been specified.");
        }

    return files;
}

bool SubmitTransferCli::performChecks()
{ 
    // in FTS3 delegation is supported by default
    delegate = true;
    
    if (((getSource().empty() || getDestination().empty())) && !vm.count("file"))
        {
            msgPrinter.error_msg("You need to specify source and destination surl's");
            return false;
        }    

    // the job cannot be specified twice
    if ((!getSource().empty() || !getDestination().empty()) && vm.count("file"))
        {
            msgPrinter.error_msg("You may not specify a transfer on the command line if the -f option is used.");
            return false;
        }

    if (vm.count("file-size") && vm.count("file"))
        {
            msgPrinter.error_msg("If a bulk submission has been used file size has to be specified inside the bulk file separately for each file and no using '--file-size' option!");
            return false;
        }

    if (vm.count("file-metadata") && vm.count("file"))
        {
            msgPrinter.error_msg("If a bulk submission has been used file metadata have to be specified inside the bulk file separately for each file and no using '--file-metadata' option!");
            return false;
        }

    return true;
}

string SubmitTransferCli::askForPassword()
{

    termios stdt;
    // get standard command line settings
    tcgetattr(STDIN_FILENO, &stdt);
    termios newt = stdt;
    // turn off echo while typing
    newt.c_lflag &= ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt))
        {
            cout << "submit: could not set terminal attributes" << endl;
            tcsetattr(STDIN_FILENO, TCSANOW, &stdt);
            return "";
        }

    string pass1, pass2;

    cout << "Enter MyProxy password: ";
    cin >> pass1;
    cout << endl << "Enter MyProxy password again: ";
    cin >> pass2;
    cout << endl;

    // set the standard command line settings back
    tcsetattr(STDIN_FILENO, TCSANOW, &stdt);

    // compare passwords
    if (pass1.compare(pass2))
        {
            cout << "Entered MyProxy passwords do not match." << endl;
            return "";
        }

    return pass1;
}

map<string, string> SubmitTransferCli::getParams()
{

    map<string, string> parameters;

    // check if the parameters were set using CLI, and if yes set them

    if (vm.count("compare-checksum"))
        {
            parameters[JobParameterHandler::CHECKSUM_METHOD] = "compare";
        }

    if (vm.count("overwrite"))
        {
            parameters[JobParameterHandler::OVERWRITEFLAG] = "Y";
        }

    if (vm.count("gparam"))
        {
            parameters[JobParameterHandler::GRIDFTP] = vm["gparam"].as<string>();
        }

    if (vm.count("id"))
        {
            parameters[JobParameterHandler::DELEGATIONID] = vm["id"].as<string>();
        }

    if (vm.count("dest-token"))
        {
            parameters[JobParameterHandler::SPACETOKEN] = vm["dest-token"].as<string>();
        }

    if (vm.count("source-token"))
        {
            parameters[JobParameterHandler::SPACETOKEN_SOURCE] = vm["source-token"].as<string>();
        }

    if (vm.count("copy-pin-lifetime"))
        {
            int val = vm["copy-pin-lifetime"].as<int>();
            if (val < -1) throw bad_option("copy-pin-lifetime", "The 'copy-pin-lifetime' value has to be positive!");
            parameters[JobParameterHandler::COPY_PIN_LIFETIME] = lexical_cast<string>(val);
        }

    if (vm.count("bring-online"))
        {
            int val = vm["bring-online"].as<int>();
            if (val < -1) throw bad_option("bring-online", "The 'bring-online' value has to be positive!");
            parameters[JobParameterHandler::BRING_ONLINE] = lexical_cast<string>(val);
        }

    if (vm.count("reuse"))
        {
            parameters[JobParameterHandler::REUSE] = "Y";
        }

    if (vm.count("multi-hop"))
        {
            parameters[JobParameterHandler::MULTIHOP] = "Y";
        }

    if (vm.count("job-metadata"))
        {
            parameters[JobParameterHandler::JOB_METADATA] = vm["job-metadata"].as<string>();
        }

    if (vm.count("retry"))
        {
            int val = vm["retry"].as<int>();
            parameters[JobParameterHandler::RETRY] = lexical_cast<string>(val);
        }

    if (vm.count("retry-delay"))
        {
            int val = vm["retry-delay"].as<int>();
            if (val < 0) throw bad_option("retry-delay", "The 'retry-delay' value has to be positive!");
            parameters[JobParameterHandler::RETRY_DELAY] = lexical_cast<string>(val);
        }
    if (vm.count("buff-size"))
        {
            int val = vm["buff-size"].as<int>();
            if (val <= 0) throw bad_option("buff-size", "The buffer size has to greater than 0!");
            parameters[JobParameterHandler::BUFFER_SIZE] = lexical_cast<string>(val);
        }
    if (vm.count("nostreams"))
        {
            int val = vm["nostreams"].as<int>();
            if (val <= 0) throw bad_option("nostreams", "The number of streams has to be greater than 0!");
            parameters[JobParameterHandler::NOSTREAMS] = lexical_cast<string>(val);
        }
    if (vm.count("timeout"))
        {
            int val = vm["timeout"].as<int>();
            if (val <= 0) throw bad_option("timeout", "The timeout has to be greater than 0!");
            parameters[JobParameterHandler::TIMEOUT] = lexical_cast<string>(val);
        }

    return parameters;
}

string SubmitTransferCli::getPassword()
{
    return password;
}

bool SubmitTransferCli::isBlocking()
{
    return vm.count("blocking");
}

string SubmitTransferCli::getUsageString(string tool)
{
    return "Usage: " + tool + " [options] SOURCE DEST [CHECKSUM]";
}

string SubmitTransferCli::getFileName()
{
    if (vm.count("file")) return vm["file"].as<string>();

    return string();
}

