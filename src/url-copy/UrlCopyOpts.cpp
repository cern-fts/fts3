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

#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "Transfer.h"
#include "UrlCopyOpts.h"
#include "common/Logger.h"

using fts3::common::commit;


const option UrlCopyOpts::long_options[] =
{
    {"job-id",            required_argument, 0, 100},
    {"file-id",           required_argument, 0, 101},
    {"source",            required_argument, 0, 102},
    {"destination",       required_argument, 0, 103},
    {"user-filesize",     required_argument, 0, 104},
    {"bulk-file",         required_argument, 0, 105},

    {"reuse",             no_argument,       0, 200},
    {"job-m-replica",     no_argument,       0, 202},
    {"last-replica",      no_argument,       0, 203},
    {"job-multihop",      no_argument,       0, 204},
    {"last-hop",          no_argument,       0, 205},
    {"archiving",         no_argument,       0, 206},
    {"activity",          required_argument, 0, 207},

    {"checksum",          required_argument, 0, 300},
    {"checksum-mode",     required_argument, 0, 301},
    {"strict-copy",       no_argument,       0, 302},
    {"dst-file-report",   no_argument,       0, 303},
    {"3rd-party-turl",    required_argument, 0, 304},
    {"scitag",            required_argument, 0, 305},

    {"token-bringonline",  required_argument, 0, 400},
    {"source-space-token", required_argument, 0, 401},
    {"dest-space-token",   required_argument, 0, 402},

    {"vo",                     required_argument, 0, 500},
    {"user-dn",                required_argument, 0, 501},
    {"proxy",                  required_argument, 0, 502},
    {"oauth-file",             required_argument, 0, 503},
    {"auth-method",            required_argument, 0, 504},
    {"copy-mode",              required_argument, 0, 505},
    {"disable-fallback",       no_argument,       0, 506},
    {"cloud-config",           required_argument, 0, 507},
    {"overwrite-disk-enabled", no_argument,       0, 508},
    {"disable-cleanup",        no_argument,       0, 509},

    {"src-token-id",           required_argument, 0, 520},
    {"dst-token-id",           required_argument, 0, 521},
    {"src-token-unmanaged",    no_argument,       0, 522},
    {"dst-token-unmanaged",    no_argument,       0, 523},
    {"token-refresh-margin",   required_argument, 0, 524},

    {"alias",                required_argument, 0, 600},
    {"monitoring",           no_argument,       0, 601},
    {"ping-interval",        required_argument, 0, 602},

    {"file-metadata",     required_argument, 0, 700},
    {"archive-metadata",  required_argument, 0, 701},
    {"job-metadata",      required_argument, 0, 702},

    {"level",             required_argument, 0, 801},
    {"overwrite",         no_argument,       0, 802},
    {"nstreams",          required_argument, 0, 803},
    {"tcp-buffersize",    required_argument, 0, 804},
    {"timeout",           required_argument, 0, 805},
    {"udt",               no_argument,       0, 806},
    {"ipv6",              no_argument,       0, 807},
    {"sec-per-mb",        required_argument, 0, 808},
    {"ipv4",              no_argument,       0, 809},
    {"no-delegation",     no_argument,       0, 810},
    {"no-streaming",      no_argument,       0, 811},
    {"skip-evict",        no_argument,       0, 812},
    {"overwrite-on-disk", no_argument,       0, 813},

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

static std::string formatAdler32Checksum(std::string checksum)
{
    unsigned int padding = std::max(0, 8 - static_cast<int>(checksum.length()));
    checksum.insert(0, padding, '0');
    return checksum;
}

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
        auto algorithm = checksum.substr(0, colon);
        auto formatted = checksum.substr(colon + 1);

        if (boost::iequals(algorithm, "adler32")) {
            formatted = formatAdler32Checksum(formatted);
        }

        transfer.checksumAlgorithm.assign(algorithm);
        transfer.checksumValue.assign(formatted);
    }
}


std::string translateCopyMode(const std::string &text)
{
    std::string mode;
    // Translate the copy mode from the url-copy options to the Gfal2 defined types
    if (text == "pull") {
        mode = GFAL_TRANSFER_TYPE_PULL;
    } else if (text == "push") {
        mode = GFAL_TRANSFER_TYPE_PUSH;
    } else if (text == "streamed") {
        mode = GFAL_TRANSFER_TYPE_STREAMED;
    } else {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Invalid copy-mode in the fts_url_copy options: " << text << commit;
    }
    return mode;
}

// Create a transfer out of a string
static Transfer createFromString(const Transfer &reference, const std::string &line)
{
    typedef boost::tokenizer <boost::char_separator<char>> tokenizer;

    std::string strArray[9];
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
    t.fileMetadata = strArray[5] == "x" ? "" : strArray[5];
    // Must deserialize metadata string here, as value will be used in Gfal copy
    // Other metadata fields deserialize in the LegacyReporter
    t.archiveMetadata = strArray[6] == "x" ? "" : replaceMetadataString(strArray[6]);
    t.tokenBringOnline = strArray[7] == "x" ? "" : strArray[7];
    t.scitag = boost::lexical_cast<unsigned>(strArray[8]);
    t.sourceSpaceToken = reference.sourceSpaceToken;
    t.destSpaceToken = reference.destSpaceToken;
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


UrlCopyOpts::UrlCopyOpts(): isSessionReuse(false), strictCopy(false), disableCleanup(false),
                            dstFileReport(false), disableCopyFallback(false),
                            overwriteDiskEnabled(false), optimizerLevel(0), overwrite(false),
                            overwriteOnDisk(false), noDelegation(false), nStreams(0), tcpBuffersize(0),
                            timeout(0), enableUdt(false), enableIpv6(boost::indeterminate), addSecPerMb(0),
                            noStreaming(false), skipEvict(false), enableMonitoring(false),
                            pingInterval(60), tokenRefreshMargin(300),
                            retry(0), retryMax(0), logDir("/var/log/fts3"), msgDir("/var/lib/fts3"),
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
                case 202:
                    referenceTransfer.isMultipleReplicaJob = true;
                    break;
                case 203:
                    referenceTransfer.isLastReplica = true;
                    break;
                case 204:
                    referenceTransfer.isMultihopJob = true;
                    break;
                case 205:
                    referenceTransfer.isLastHop = true;
                    break;
                case 206:
                    referenceTransfer.isArchiving = true;
                    break;
                case 207:
                    referenceTransfer.activity = boost::lexical_cast<std::string>(optarg);
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
                case 303:
                    dstFileReport = true;
                    break;
                case 304:
                    thirdPartyTURL = boost::lexical_cast<std::string>(optarg);
                    break;
                case 305:
                    referenceTransfer.scitag = boost::lexical_cast<unsigned>(optarg);
                    break;

                case 400:
                    referenceTransfer.tokenBringOnline = optarg;
                    break;
                case 401:
                    referenceTransfer.sourceSpaceToken = optarg;
                    break;
                case 402:
                    referenceTransfer.destSpaceToken = optarg;
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
                case 504:
                    authMethod = optarg;
                    break;
                case 505:
                    copyMode = translateCopyMode(optarg);
                    break;
                case 506:
                    disableCopyFallback = true;
                    break;
                case 507:
                    cloudStorageConfig = optarg;
                    break;
                case 508:
                    overwriteDiskEnabled = true;
                    break;
                case 509:
                    disableCleanup = true;
                    break;

                case 520:
                    referenceTransfer.sourceTokenId = optarg;
                    break;
                case 521:
                    referenceTransfer.destTokenId = optarg;
                    break;
                case 522:
                    referenceTransfer.sourceTokenUnmanaged = true;
                    break;
                case 523:
                    referenceTransfer.destTokenUnmanaged = true;
                    break;
                case 524:
                    tokenRefreshMargin = boost::lexical_cast<unsigned>(optarg);
                    break;

                case 600:
                    alias = optarg;
                    break;
                case 601:
                    enableMonitoring = true;
                    break;
                case 602:
                    pingInterval = boost::lexical_cast<unsigned>(optarg);
                    break;

                case 700:
                    referenceTransfer.fileMetadata = optarg;
                    break;
                case 701:
                    referenceTransfer.archiveMetadata = replaceMetadataString(optarg);
                    break;
                case 702:
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
                case 809:
                    enableIpv6 = false;
                    break;
                case 810:
                    noDelegation = true;
                    break;
                case 811:
                    noStreaming = true;
                    break;
                case 812:
                    skipEvict = true;
                    break;
                case 813:
                    overwriteOnDisk = true;
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
