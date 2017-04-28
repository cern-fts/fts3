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

#include "SubmitTransferCli.h"
#include "BulkSubmissionParser.h"

#include "JobParameterHandler.h"
#include <termios.h>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/assign.hpp>

#include "../../common/Uri.h"


using namespace fts3::cli;
using namespace fts3::common;

SubmitTransferCli::SubmitTransferCli()
{
    delegate = true;

    // by default we don't use checksum
    checksum = false;

    // add commandline options specific for fts3-transfer-submit
    specific.add_options()
        ("blocking,b", "Blocking mode, wait until the operation completes.")
        ("file,f", po::value<std::string>(&bulk_file), "Name of a the bulk submission file.")
        ("gparam,g", po::value<std::string>(), "Gridftp parameters.")
        ("interval,i", po::value<int>(), "Interval between two poll operations in blocking mode.")
//          ("myproxysrv,m", value<string>(), "MyProxy server to use.")
//          ("password,p", value<string>(), "MyProxy password to send with the job")
        ("overwrite,o", "Overwrite files.")
        ("dest-token,t", po::value<std::string>(),
            "The destination space token or its description (for SRM 2.2 transfers).")
        ("source-token,S", po::value<std::string>(),
            "The source space token or its description (for SRM 2.2 transfers).")
        ("compare-checksums,K", "Compare checksums between source and destination.")
        ("copy-pin-lifetime", po::value<int>()->default_value(-1),
            "Pin lifetime of the copy of the file (seconds), if the argument is not specified a default value of 28800 seconds (8 hours) is used.")
        ("bring-online", po::value<int>()->default_value(-1),
            "Bring online timeout expressed in seconds, if the argument is not specified a default value of 28800 seconds (8 hours) is used.")
        ("reuse,r", "enable session reuse for the transfer job")
        ("multi-hop,m", "enable multi-hopping")
        ("job-metadata", po::value<std::string>(), "transfer-job metadata")
        ("file-metadata", po::value<std::string>(), "file metadata")
        ("file-size", po::value<double>(), "file size (in Bytes)")
        ("json-submission", "The bulk submission file will be expected in JSON format")
        ("retry", po::value<int>(),
            "Number of retries. If 0, the server default will be used. If negative, there will be no retries.")
        ("retry-delay", po::value<int>()->default_value(0), "Retry delay in seconds")
        ("nostreams", po::value<int>(), "number of streams that will be used for the given transfer-job")
        ("timeout", po::value<int>(), "timeout (expressed in seconds) that will be used for the given job")
        ("buff-size", po::value<int>(), "buffer size (expressed in bytes) that will be used for the given transfer-job")
        ("strict-copy", "disable all checks, just copy the file")
        ("credentials", po::value<std::string>(), "additional credentials for the transfer (i.e. S3)");

    // add hidden options
    hidden.add_options()
        ("checksum", po::value<std::string>(), "Specify checksum algorithm and value (ALGORITHM:1234af).");

    // add positional (those used without an option switch) command line options
    p.add("checksum", 1);

}

SubmitTransferCli::~SubmitTransferCli()
{

}

void SubmitTransferCli::parse(int ac, char *av[])
{

    // do the basic initialisation
    CliBase::parse(ac, av);

    // check whether to use delegation
    if (vm.count("id")) {
        delegate = true;
    }
}

void SubmitTransferCli::validate()
{
    SrcDestCli::validate();
    DelegationCli::validate();

    // perform standard checks in order to determine if the job was well specified
    performChecks();
    // prepare job elements
    createJobElements();
}

boost::optional<std::string> SubmitTransferCli::getMetadata()
{

    if (vm.count("job-metadata")) {
        return vm["job-metadata"].as<std::string>();
    }
    return boost::optional<std::string>();
}

bool SubmitTransferCli::checkValidUrl(const std::string &uri)
{
    Uri u0 = Uri::parse(uri);
    bool ok = u0.host.length() != 0 && u0.protocol.length() != 0 && u0.path.length() != 0;
    if (!ok) {
        throw cli_exception("Not valid uri format, check submitted uri's");
    }

    return true;
}


void SubmitTransferCli::createJobElements()
{
    // first check if the -f option was used, try to open the file with bulk-job description
    std::ifstream ifs(bulk_file.c_str());
    if (ifs) {
        if (vm.count("json-submission")) {
            BulkSubmissionParser bulk(ifs);
            files = bulk.getFiles();
            jobParams = bulk.getJobParameters();
        }
        else {
            // Parse the file
            int lineCount = 0;
            std::string line;
            // define the seperator characters (space) for tokenizing each line
            boost::char_separator<char> sep(" ");
            // read and parse the lines one by one
            do {
                lineCount++;
                getline(ifs, line);

                // split the line into tokens
                boost::tokenizer<boost::char_separator<char> > tokens(line, sep);
                boost::tokenizer<boost::char_separator<char> >::iterator it;

                // we are expecting up to 3 elements in each line
                // source, destination and optionally the checksum
                File file;

                // the first part should be the source
                it = tokens.begin();
                if (it != tokens.end()) {
                    std::string s = *it;
                    if (!checkValidUrl(s)) throw bad_option("file", s + " is not valid URL");
                    file.sources.push_back(s);
                }
                else
                    // if the line was empty continue
                    continue;

                // the second part should be the destination
                it++;
                if (it != tokens.end()) {
                    std::string s = *it;
                    if (!checkValidUrl(s)) throw bad_option("file", s + " is not valid URL");
                    file.destinations.push_back(s);
                }
                else {
                    // one element is not enough to define a job
                    std::string msg =
                        "submit: in line " + boost::lexical_cast<std::string>(lineCount) + " destination is missing";
                    throw cli_exception(msg);
                }

                // the third part should be the checksum (but its optional)
                it++;
                if (it != tokens.end()) {
                    std::string checksum_str = *it;

                    checksum = true;
                    file.checksums.push_back(checksum_str);
                }

                files.push_back(file);

            }
            while (!ifs.eof());

        }

    }
    else {

        // the -f was not used, so use the values passed via CLI

        // first, if the checksum algorithm has been given check if the
        // format is correct (ALGORITHM:1234af)
        std::vector<std::string> checksums;
        if (vm.count("checksum")) {
            checksums.push_back(vm["checksum"].as<std::string>());
            this->checksum = true;
        }

        // check if size of the file has been specified
        boost::optional<double> filesize;
        if (vm.count("file-size")) {
            filesize = vm["file-size"].as<double>();
        }

        // check if there are some file metadata
        boost::optional<std::string> file_metadata;
        if (vm.count("file-metadata")) {
            file_metadata = vm["file-metadata"].as<std::string>();
            parseMetadata(*file_metadata);
        }

        // then if the source and destination have been given create a Task
        if (!getSource().empty() && !getDestination().empty()) {

            files.push_back(
                File(boost::assign::list_of(getSource()),
                    boost::assign::list_of(getDestination()), checksums, filesize, file_metadata)
            );
        }
    }
}

std::vector<File> SubmitTransferCli::getFiles()
{
    if (files.empty()) {
        throw bad_option("missing parameter", "No transfer job has been specified.");
    }

    return files;
}

void SubmitTransferCli::performChecks()
{
    // in FTS3 delegation is supported by default
    delegate = true;

    if (((getSource().empty() || getDestination().empty())) && !vm.count("file")) {
        throw cli_exception("You need to specify source and destination surl's");
    }

    // the job cannot be specified twice
    if ((!getSource().empty() || !getDestination().empty()) && vm.count("file")) {
        throw bad_option("file", "You may not specify a transfer on the command line if the -f option is used.");
    }

    if (vm.count("file-size") && vm.count("file")) {
        throw bad_option("file-size",
            "If a bulk submission has been used file size has to be specified inside the bulk file separately for each file and no using '--file-size' option!");
    }

    if (vm.count("file-metadata") && vm.count("file")) {
        throw bad_option("file-metadata",
            "If a bulk submission has been used file metadata have to be specified inside the bulk file separately for each file and no using '--file-metadata' option!");
    }
}

std::string SubmitTransferCli::askForPassword()
{

    termios stdt;
    // get standard command line settings
    tcgetattr(STDIN_FILENO, &stdt);
    termios newt = stdt;
    // turn off echo while typing
    newt.c_lflag &= ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt)) {
        std::cout << "submit: could not set terminal attributes" << std::endl;
        tcsetattr(STDIN_FILENO, TCSANOW, &stdt);
        return "";
    }

    std::string pass1, pass2;

    std::cout << "Enter MyProxy password: ";
    std::cin >> pass1;
    std::cout << std::endl << "Enter MyProxy password again: ";
    std::cin >> pass2;
    std::cout << std::endl;

    // set the standard command line settings back
    tcsetattr(STDIN_FILENO, TCSANOW, &stdt);

    // compare passwords
    if (pass1.compare(pass2)) {
        std::cout << "Entered MyProxy passwords do not match." << std::endl;
        return "";
    }

    return pass1;
}

std::map<std::string, std::string> SubmitTransferCli::getParams()
{

    std::map<std::string, std::string> parameters;

    // check if the parameters were set using CLI, and if yes set them

    if (vm.count("compare-checksums")) {
        parameters[JobParameterHandler::CHECKSUM_METHOD] = "compare";
    }

    if (vm.count("overwrite")) {
        parameters[JobParameterHandler::OVERWRITEFLAG] = "Y";
    }

    if (vm.count("gparam")) {
        parameters[JobParameterHandler::GRIDFTP] = vm["gparam"].as<std::string>();
    }

    if (vm.count("id")) {
        parameters[JobParameterHandler::DELEGATIONID] = vm["id"].as<std::string>();
    }

    if (vm.count("dest-token")) {
        parameters[JobParameterHandler::SPACETOKEN] = vm["dest-token"].as<std::string>();
    }

    if (vm.count("source-token")) {
        parameters[JobParameterHandler::SPACETOKEN_SOURCE] = vm["source-token"].as<std::string>();
    }

    if (vm.count("copy-pin-lifetime")) {
        int val = vm["copy-pin-lifetime"].as<int>();
        if (val < -1) throw bad_option("copy-pin-lifetime", "The 'copy-pin-lifetime' value has to be positive!");
        parameters[JobParameterHandler::COPY_PIN_LIFETIME] = boost::lexical_cast<std::string>(val);
    }

    if (vm.count("bring-online")) {
        int val = vm["bring-online"].as<int>();
        if (val < -1) throw bad_option("bring-online", "The 'bring-online' value has to be positive!");
        parameters[JobParameterHandler::BRING_ONLINE] = boost::lexical_cast<std::string>(val);
    }

    if (vm.count("reuse")) {
        parameters[JobParameterHandler::REUSE] = "Y";
    }

    if (vm.count("multi-hop")) {
        parameters[JobParameterHandler::MULTIHOP] = "Y";
    }

    if (vm.count("job-metadata")) {
        std::string const &metadata = vm["job-metadata"].as<std::string>();
        parseMetadata(metadata);
        parameters[JobParameterHandler::JOB_METADATA] = metadata;
    }

    if (vm.count("retry")) {
        int val = vm["retry"].as<int>();
        parameters[JobParameterHandler::RETRY] = boost::lexical_cast<std::string>(val);
    }

    if (vm.count("retry-delay")) {
        int val = vm["retry-delay"].as<int>();
        if (val < 0) throw bad_option("retry-delay", "The 'retry-delay' value has to be positive!");
        parameters[JobParameterHandler::RETRY_DELAY] = boost::lexical_cast<std::string>(val);
    }
    if (vm.count("buff-size")) {
        int val = vm["buff-size"].as<int>();
        if (val <= 0) throw bad_option("buff-size", "The buffer size has to greater than 0!");
        parameters[JobParameterHandler::BUFFER_SIZE] = boost::lexical_cast<std::string>(val);
    }
    if (vm.count("nostreams")) {
        int val = vm["nostreams"].as<int>();
        if (val < 0) throw bad_option("nostreams", "The number of streams has to be greater or equal than 0!");
        parameters[JobParameterHandler::NOSTREAMS] = boost::lexical_cast<std::string>(val);
    }
    if (vm.count("timeout")) {
        int val = vm["timeout"].as<int>();
        if (val <= 0) throw bad_option("timeout", "The timeout has to be greater than 0!");
        parameters[JobParameterHandler::TIMEOUT] = boost::lexical_cast<std::string>(val);
    }
    if (vm.count("strict-copy")) {
        parameters[JobParameterHandler::STRICT_COPY] = "Y";
    }
    if (vm.count("credentials")) {
        parameters[JobParameterHandler::CREDENTIALS] = vm["credentials"].as<std::string>();
    }

    return parameters;
}

pt::ptree SubmitTransferCli::getExtraParameters()
{
    return jobParams;
}

std::string SubmitTransferCli::getPassword()
{
    return password;
}

bool SubmitTransferCli::isBlocking()
{
    return vm.count("blocking");
}

std::string SubmitTransferCli::getUsageString(std::string tool) const
{
    return "Usage: " + tool + " [options] SOURCE DEST [CHECKSUM]";
}

std::string SubmitTransferCli::getFileName()
{
    if (vm.count("file")) return vm["file"].as<std::string>();

    return std::string();
}

void SubmitTransferCli::parseMetadata(std::string const &metadata)
{
    namespace pt = boost::property_tree;

    // first check it is JSON
    if (metadata[0] != '{' || metadata[metadata.size() - 1] != '}') return;
    // than check if the JSON format is correct
    try {
        // JSON parsing
        pt::ptree pt;
        std::stringstream iostr;
        iostr << metadata;
        pt::read_json(iostr, pt);

    }
    catch (pt::json_parser_error &ex) {
        // handle errors in JSON format
        std::stringstream err;
        err << "JSON error : " << ex.message() << ". ";
        err << "Possibly single quotes around metadata are missing!";
        throw cli_exception(err.str());
    }
}
