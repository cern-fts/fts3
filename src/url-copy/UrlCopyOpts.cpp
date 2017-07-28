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

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <boost/filesystem.hpp>
#include "Transfer.h"
#include "UrlCopyOpts.h"


const option UrlCopyOpts::long_options[] =
{
    {"job-id",            required_argument, 0, 100},
    {"file-id",           required_argument, 0, 101},
    {"source",            required_argument, 0, 102},
    {"destination",       required_argument, 0, 103},
    {"user-filesize",     required_argument, 0, 104},
    {"bulk-file",         required_argument, 0, 105},

    {"reuse",             no_argument,       0, 200},
    {"multi-hop",         no_argument,       0, 201},
    {"job_m_replica",     no_argument,       0, 202},
    {"last_replica",      no_argument,       0, 203},

    {"checksum",          required_argument, 0, 300},
    {"checksum-mode",     required_argument, 0, 301},
    {"strict-copy",       no_argument,       0, 302},

    {"token-bringonline", required_argument, 0, 400},
    {"dest-token-desc",   required_argument, 0, 401},
    {"source-token-desc", required_argument, 0, 402},

    {"vo",                required_argument, 0, 500},
    {"user-dn",           required_argument, 0, 501},
    {"proxy",             required_argument, 0, 502},
    {"oauth",             required_argument, 0, 503},

    {"infosystem",        required_argument, 0, 600},
    {"alias",             required_argument, 0, 601},
    {"monitoring",        no_argument,       0, 602},
    {"active",            required_argument, 0, 603},

    {"file-metadata",     required_argument, 0, 700},
    {"job-metadata",      required_argument, 0, 701},

    {"level",             required_argument, 0, 801},
    {"overwrite",         no_argument,       0, 802},
    {"nstreams",          required_argument, 0, 803},
    {"tcp-buffersize",    required_argument, 0, 804},
    {"timeout",           required_argument, 0, 805},
    {"udt",               no_argument,       0, 806},
    {"ipv6",              no_argument,       0, 807},
    {"sec-per-mb",        required_argument, 0, 808},

    {"retry",             required_argument, 0, 820},
    {"retry_max-max",     required_argument, 0, 821},

    {"logDir",            required_argument, 0, 900},
    {"msgDir",            required_argument, 0, 901},

    {"help",              no_argument,       0, 0},
    {"debug",             required_argument, 0, 1},
    {"stderr",            no_argument,       0, 2},
    {0, 0, 0, 0}
};

const char UrlCopyOpts::short_options[] = "";

static void setChecksum(Transfer &transfer, const std::string &checksum)
{
    if (checksum == "x") {
        return;
    }

    size_t colon = checksum.find(':');
    if (colon == std::string::npos) {
        transfer.checksumAlgorithm.assign(checksum);
        transfer.checksumValue.clear();
    }
    else {
        transfer.checksumAlgorithm.assign(checksum.substr(0, colon));
        transfer.checksumValue.assign(checksum.substr(colon + 1));
    }
}


// Create a transfer out of a string
static Transfer createFromString(const Transfer &reference, const std::string &line)
{
    typedef boost::tokenizer <boost::char_separator<char>> tokenizer;

    std::string strArray[7];
    tokenizer tokens(line, boost::char_separator<char>(" "));
    std::copy(tokens.begin(), tokens.end(), strArray);

    Transfer t;
    t.jobId = reference.jobId;
    t.fileId = boost::lexical_cast<uint64_t>(strArray[0]);
    t.source = Uri::parse(strArray[1]);
    t.destination = Uri::parse(strArray[2]);
    t.checksumMode = reference.checksumMode;
    setChecksum(t, strArray[3]);
    t.userFileSize = boost::lexical_cast<uint64_t>(strArray[4]);
    t.fileMetadata = strArray[5];
    t.tokenBringOnline = strArray[6];
    if (t.tokenBringOnline == "x") {
        t.tokenBringOnline.clear();
    }
    t.isMultipleReplicaJob = false;
    t.isLastReplica = false;
    return t;
}


// Initialize a list from a file
static Transfer::TransferList initListFromFile(const Transfer &reference, const std::string &path)
{
    std::string line;
    std::ifstream infile(path.c_str(), std::ios_base::in);

    Transfer::TransferList list;
    while (std::getline(infile, line, '\n')) {
        if (!line.empty()) {
            list.push_back(createFromString(reference, line));
        }
    }

    infile.close();

    try {
        boost::filesystem::remove(path.c_str());
    }
    catch (const boost::filesystem::filesystem_error &ex) {
        std::cerr << "Failed to clean up the bulk file! " << ex.what() << std::endl;
    }

    return list;
}


UrlCopyOpts::UrlCopyOpts():
    isSessionReuse(false), isMultihop(false), isMultipleReplicaJob(false),
    strictCopy(false),
    optimizerLevel(0), overwrite(false), nStreams(0), tcpBuffersize(0),
    timeout(0), enableUdt(false), enableIpv6(false), addSecPerMb(0),
    enableMonitoring(false), active(0), retry(0), retryMax(0),
    logDir("/var/log/fts3"), msgDir("/var/lib/fts3"),
    debugLevel(0), logToStderr(false)
{
}


void UrlCopyOpts::usage(const std::string &bin)
{
    std::cout << "Usage: " << bin << " [options]" << std::endl
    << "Options: " << std::endl;
    for (int i = 0; long_options[i].name; ++i) {
        std::cout << "\t--" << long_options[i].name
        << ",-" << static_cast<char>(long_options[i].val)
        << std::endl;
    }
    exit(0);
}


void UrlCopyOpts::parse(int argc, char * const argv[])
{
    int opt;
    Transfer referenceTransfer;

    try {
        while ((opt = getopt_long_only(argc, argv, short_options, long_options, NULL)) > -1) {
            switch (opt) {
                case 0:
                    usage(argv[0]);
                    break;
                case 1:
                    if (optarg) {
                        debugLevel = boost::lexical_cast<unsigned>(optarg);
                    }
                    else {
                        debugLevel = 0;
                    }
                    break;
                case 2:
                    logToStderr = true;
                    break;

                case 100:
                    referenceTransfer.jobId = optarg;
                    break;
                case 101:
                    referenceTransfer.fileId = boost::lexical_cast<unsigned long long>(optarg);
                    break;
                case 102:
                    referenceTransfer.source = Uri::parse(optarg);
                    break;
                case 103:
                    referenceTransfer.destination = Uri::parse(optarg);
                    break;
                case 104:
                    referenceTransfer.userFileSize = boost::lexical_cast<long long>(optarg);
                    break;
                case 105:
                    bulkFile = boost::lexical_cast<std::string>(optarg);
                    break;

                case 200:
                    isSessionReuse = true;
                    break;
                case 201:
                    isMultihop = true;
                    break;
                case 202:
                    referenceTransfer.isMultipleReplicaJob = true;
                    break;
                case 203:
                    referenceTransfer.isLastReplica = true;
                    break;

                case 300:
                    setChecksum(referenceTransfer, optarg);
                    break;
                case 301:
                    if (strncmp("target", optarg, strlen("target")) == 0 || strncmp("t", optarg, strlen("t")) == 0) {
                        referenceTransfer.checksumMode = Transfer::CHECKSUM_TARGET;
                    }
                    else if (strncmp("source", optarg, strlen("source")) == 0 || strncmp("s", optarg, strlen("s")) == 0) {
                        referenceTransfer.checksumMode = Transfer::CHECKSUM_SOURCE;
                    }
                    else if (strncmp("both", optarg, strlen("both")) == 0 || strncmp("b", optarg, strlen("b")) == 0) {
                        referenceTransfer.checksumMode = Transfer::CHECKSUM_BOTH;
                    }
                    else {
            			referenceTransfer.checksumMode = Transfer::CHECKSUM_NONE;
                    }
                    break;
                case 302:
                    strictCopy = true;
                    break;

                case 400:
                    referenceTransfer.tokenBringOnline = optarg;
                    break;
                case 401:
                    referenceTransfer.destTokenDescription = optarg;
                    break;
                case 402:
                    referenceTransfer.sourceTokenDescription = optarg;
                    break;

                case 500:
                    voName = optarg;
                    break;
                case 501:
                    userDn = optarg;
                    break;
                case 502:
                    proxy = optarg;
                    break;
                case 503:
                    oauthFile = optarg;
                    break;

                case 600:
                    infosys = optarg;
                    break;
                case 601:
                    alias = optarg;
                    break;
                case 602:
                    enableMonitoring = true;
                    break;
                case 603:
                    active = boost::lexical_cast<unsigned>(optarg);
                    break;

                case 700:
                    referenceTransfer.fileMetadata = optarg;
                    break;
                case 701:
                    jobMetadata = optarg;
                    break;

                case 801:
                    optimizerLevel = boost::lexical_cast<int>(optarg);
                    break;
                case 802:
                    overwrite = true;
                    break;
                case 803:
                    nStreams = boost::lexical_cast<int>(optarg);
                    break;
                case 804:
                    tcpBuffersize = boost::lexical_cast<unsigned>(optarg);
                    break;
                case 805:
                    timeout = boost::lexical_cast<unsigned>(optarg);
                    break;
                case 806:
                    enableUdt = true;
                    break;
                case 807:
                    enableIpv6 = true;
                    break;
                case 808:
                    addSecPerMb = boost::lexical_cast<int>(optarg);
                    break;

                case 820:
                    retry = boost::lexical_cast<int>(optarg);
                    break;
                case 821:
                    retryMax = boost::lexical_cast<int>(optarg);
                    break;

                case 900:
                    logDir = boost::lexical_cast<std::string>(optarg);
                    break;
                case 901:
                    msgDir = boost::lexical_cast<std::string>(optarg);
                    break;

                default:
                    usage(argv[0]);
            }
        }
    }
    catch (boost::bad_lexical_cast &e) {
        std::cerr << "Expected a numeric value for option -" << static_cast<char>(opt) << std::endl;
        exit(-1);
    }

    if (bulkFile.empty() &&
        (!referenceTransfer.source.fullUri.empty() && !referenceTransfer.destination.fullUri.empty())) {
        transfers.push_back(referenceTransfer);
    }
    else {
        try {
            transfers = initListFromFile(referenceTransfer, bulkFile);
        }
        catch (const std::exception &ex) {
            std::cerr << "Failed to parse the bulk file! " << ex.what() << std::endl;
            exit(-1);
        }
    }

    validateRequired();
}


void UrlCopyOpts::validateRequired()
{
    if (transfers.empty()) {
        std::cerr << "Specify --source and --destination, or --bulk-file" << std::endl;
        exit(-1);
    }
}
