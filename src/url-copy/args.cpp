#include <boost/lexical_cast.hpp>
#include <sstream>
#include "common/definitions.h"
#include "args.h"

UrlCopyOpts UrlCopyOpts::instance;


const option UrlCopyOpts::long_options[] = {
    {"monitoring",        no_argument,       0, 'P'},
    {"auto-tunned",       no_argument,       0, 'O'},
    {"manual-config",     no_argument,       0, 'N'},
    {"infosystem",        required_argument, 0, 'M'},
    {"token-bringonline", required_argument, 0, 'L'},
    {"file-metadata",     required_argument, 0, 'K'},
    {"job-metadata",      required_argument, 0, 'J'},
    {"user-filesize",     required_argument, 0, 'I'},
    {"bringonline",       required_argument, 0, 'H'},
    {"reuse",             required_argument, 0, 'G'},
    {"debug",             no_argument,       0, 'F'},
    {"source-site",       required_argument, 0, 'D'},
    {"dest-site",         required_argument, 0, 'E'},
    {"vo",                required_argument, 0, 'C'},
    {"algorithm",         required_argument, 0, 'y'},
    {"checksum",          required_argument, 0, 'z'},
    {"compare-checksum",  no_argument,       0, 'A'},
    {"pin-lifetime",      required_argument, 0, 't'},
    {"job-id",            required_argument, 0, 'a'},
    {"source",            required_argument, 0, 'b'},
    {"destination",       required_argument, 0, 'c'},
    {"overwrite",         no_argument,       0, 'd'},
    {"nstreams",          required_argument, 0, 'e'},
    {"tcp-buffersize",    required_argument, 0, 'f'},
    {"timeout",           required_argument, 0, 'h'},
    {"daemonize",         no_argument,       0, 'i'},
    {"dest-token-desc",   required_argument, 0, 'j'},
    {"source-token-desc", required_argument, 0, 'k'},
    {"file-id",           required_argument, 0, 'B'},
    {"proxy",             required_argument, 0, '5'},
    {0, 0, 0, 0}
};

const char UrlCopyOpts::short_options[] = "PONM:L:K:J:I:H:G:FD:E:C:y:z:At:a:b:c:de:f:h:ij:k:B:5:";


UrlCopyOpts::UrlCopyOpts(): monitoringMessages(false), autoTunned(false),
        manualConfig(false), debug(false), compareChecksum(false),
        overwrite(false), daemonize(false), userFileSize(0),
        bringOnline(-1), copyPinLifetime(-1), nStreams(DEFAULT_NOSTREAMS),
        tcpBuffersize(DEFAULT_BUFFSIZE), blockSize(0), timeout(DEFAULT_TIMEOUT)

{
}



UrlCopyOpts* UrlCopyOpts::getInstance()
{
    return &UrlCopyOpts::instance;
}



std::string UrlCopyOpts::usage(const std::string& bin)
{
    std::stringstream msg;
    msg << "Usage: " << bin << " [options]" << std::endl
        << "Options: " << std::endl;
    for (int i = 0; long_options[i].name; ++i) {
        msg << "\t--" << long_options[i].name
            << ",-" << static_cast<char>(long_options[i].val)
            << std::endl;
    }
    return msg.str();
}


std::string UrlCopyOpts::getErrorMessage()
{
    return errorMessage;
}


int UrlCopyOpts::parse(int argc, char * const argv[])
{


    int option_index;
    int opt;

    try {
        while ((opt = getopt_long_only(argc, argv, short_options, long_options, &option_index)) > -1)
        {
            switch (opt)
            {
                case 'P':
                    monitoringMessages = true;
                    break;
                case 'O':
                    autoTunned = true;
                    break;
                case 'N':
                    manualConfig = true;
                    break;
                case 'M':
                    infosys = optarg;
                    break;
                case 'L':
                    tokenBringOnline = optarg;
                    break;
                case 'K':
                    fileMetadata = optarg;
                    break;
                case 'J':
                    jobMetadata = optarg;
                    break;
                case 'I':
                    userFileSize = boost::lexical_cast<double>(optarg);
                    break;
                case 'H':
                    bringOnline = boost::lexical_cast<int>(optarg);
                    break;
                case 'G':
                    reuseFile = optarg;
                    break;
                case 'F':
                    debug = true;
                    break;
                case 'D':
                    sourceSiteName = optarg;
                    break;
                case 'E':
                    destSiteName = optarg;
                    break;
                case 'C':
                    vo = optarg;
                    break;
                case 'y':
                    checksumAlgorithm = optarg;
                    break;
                case 'z':
                    checksumValue = optarg;
                    break;
                case 'A':
                    compareChecksum = true;
                    break;
                case 't':
                    copyPinLifetime = boost::lexical_cast<int>(optarg);
                    break;
                case 'a':
                    jobId = optarg;
                    break;
                case 'b':
                    sourceUrl = optarg;
                    break;
                case 'c':
                    destUrl = optarg;
                    break;
                case 'd':
                    overwrite = true;
                    break;
                case 'e':
                    nStreams = boost::lexical_cast<int>(optarg);
                    break;
                case 'f':
                    tcpBuffersize = boost::lexical_cast<unsigned>(optarg);
                    break;
                case 'g':
                    blockSize = boost::lexical_cast<unsigned>(optarg);
                    break;
                case 'h':
                    timeout = boost::lexical_cast<unsigned>(optarg);
                    break;
                case 'i':
                    daemonize = true;
                    break;
                case 'j':
                    destTokenDescription = optarg;
                    break;
                case 'k':
                    sourceTokenDescription = optarg;
                    break;
                case 'B':
                    fileId = optarg;
                    break;
                case '5':
                    proxy = optarg;
                    break;
                case '?':
                    errorMessage = usage(argv[0]);
                    return -1;
            }
        }
    }
    catch (boost::bad_lexical_cast &e) {
        errorMessage = "Expected a numeric value for option -";
        errorMessage += static_cast<char>(opt);
        return -1;
    }

    return 0;
}
