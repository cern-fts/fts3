#include <boost/lexical_cast.hpp>
#include <sstream>
#include "common/definitions.h"
#include "args.h"

UrlCopyOpts UrlCopyOpts::instance;


const option UrlCopyOpts::long_options[] =
{
    {"monitoring",        no_argument,       0, 'P'},
    {"auto-tunned",       no_argument,       0, 'O'},
    {"manual-config",     no_argument,       0, 'N'},
    {"infosystem",        required_argument, 0, 'M'},
    {"token-bringonline", required_argument, 0, 'L'},
    {"file-metadata",     required_argument, 0, 'K'},
    {"job-metadata",      required_argument, 0, 'J'},
    {"user-filesize",     required_argument, 0, 'I'},
    {"bringonline",       required_argument, 0, 'H'},
    {"reuse",             no_argument,       0, 'G'},
    {"multi-hop",         no_argument,       0, 'R'},
    {"debug",             optional_argument, 0, 'F'},
    {"source-site",       required_argument, 0, 'D'},
    {"dest-site",         required_argument, 0, 'E'},
    {"vo",                required_argument, 0, 'C'},
    {"checksum",          required_argument, 0, 'z'},
    {"compare-checksum",  required_argument, 0, 'A'},
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
    {"stderr",            no_argument,       0, '1'},
    {"udt",               no_argument,       0, 'U'},
    {"ipv6",              no_argument,       0, 'X'},
    {"global-timeout",    no_argument,       0, 'Z'},
    {"sec-per-mb",    	  required_argument, 0, 'V'},
    {"user-dn",    	  required_argument, 0, 'Y'},
    {"alias",    	  required_argument, 0, '7'},
    {"oauth",             required_argument, 0, '@'},
    {"strict-copy",       no_argument,       0, 'S'},
    {"hide-user-dn",      no_argument,       0, '8'},
    {"level",    	  required_argument, 0, '9'},
    {"active",    	  required_argument, 0, '2'},
    {"retry",    	  required_argument, 0, '3'},
    {"retry_max",    	  required_argument, 0, '4'},        
    {0, 0, 0, 0}
};

const char UrlCopyOpts::short_options[] = "PONM:L:K:J:I:H:GRFD:E:C:z:A:t:a:b:c:de:f:h:ij:k:B:5:UXZV:Y:7:@:S:8:9:2:3:4:";

UrlCopyOpts::UrlCopyOpts(): monitoringMessages(false), autoTunned(false),
    manualConfig(false), overwrite(false), daemonize(false),
    logToStderr(false), reuse(false), multihop(false), enable_udt(false), enable_ipv6(false),
    global_timeout(false), strictCopy(false),hide_user_dn(false), level(1), active(0),
    debugLevel(0),
    compareChecksum(CHECKSUM_DONT_CHECK),
    fileId(0), userFileSize(0), bringOnline(-1), copyPinLifetime(-1),
    nStreams(DEFAULT_NOSTREAMS), tcpBuffersize(DEFAULT_BUFFSIZE),
    blockSize(0), timeout(DEFAULT_TIMEOUT), secPerMb(0), retry(0), retry_max(0)
{
}



UrlCopyOpts& UrlCopyOpts::getInstance()
{
    return UrlCopyOpts::instance;
}



std::string UrlCopyOpts::usage(const std::string& bin)
{
    std::stringstream msg;
    msg << "Usage: " << bin << " [options]" << std::endl
        << "Options: " << std::endl;
    for (int i = 0; long_options[i].name; ++i)
        {
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

    try
        {
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
                            userFileSize = boost::lexical_cast<long long>(optarg);
                            break;
                        case 'H':
                            bringOnline = boost::lexical_cast<int>(optarg);
                            break;
                        case 'G':
                            reuse = true;
                            break;
                        case 'R':
                            multihop = true;
                            break;
                        case 'F':
                            if (optarg)
                                debugLevel = boost::lexical_cast<unsigned>(optarg);
                            else
                                debugLevel = 1;
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
                        case 'U':
                            enable_udt = true;
                            break;
                        case 'X':
                            enable_ipv6 = true;
                            break;
                        case 'Z':
                            global_timeout = true;
                            break;
                        case 'z':
                            checksumValue = optarg;
                            break;
                        case 'A':
                            if (strcmp("relaxed", optarg) == 0 || strcmp("r", optarg) == 0)
                                compareChecksum = CHECKSUM_RELAXED;
                            else
                                compareChecksum = CHECKSUM_STRICT;
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
                        case 'V':
                            secPerMb = boost::lexical_cast<int>(optarg);
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
                        case 'Y':
                            user_dn = optarg;
                            break;
                        case '7':
                            alias = optarg;
                            break;
                        case 'k':
                            sourceTokenDescription = optarg;
                            break;
                        case 'B':
                            fileId = boost::lexical_cast<unsigned>(optarg);
                            break;
                        case '5':
                            proxy = optarg;
                            break;
                        case '1':
                            logToStderr = true;
                            break;
                        case '@':
                            oauthFile = optarg;
                            break;
                        case 'S':
                            strictCopy = true;
                            break;
                        case '8':
                            hide_user_dn = true;
                            break;
                        case '?':
                            errorMessage = usage(argv[0]);
                            return -1;
                        case '9':
                            level = boost::lexical_cast<int>(optarg);
                            break;
                        case '2':
                            active = boost::lexical_cast<int>(optarg);
                            break;
                        case '3':
                            retry = boost::lexical_cast<int>(optarg);
                            break;
                        case '4':
                            retry_max = boost::lexical_cast<int>(optarg);
                            break;			    
                        }
                }
        }
    catch (boost::bad_lexical_cast &e)
        {
            errorMessage = "Expected a numeric value for option -";
            errorMessage += static_cast<char>(opt);
            return -1;
        }

    return 0;
}



std::ostream& operator << (std::ostream& out, const UrlCopyOpts::CompareChecksum& c)
{
    switch (c)
        {
        case UrlCopyOpts::CHECKSUM_DONT_CHECK:
            out << "No checksum comparison";
            break;
        case UrlCopyOpts::CHECKSUM_STRICT:
            out << "Strict comparison";
            break;
        case UrlCopyOpts::CHECKSUM_RELAXED:
            out << "Relaxed comparison";
            break;
        default:
            out << "Uknown value!";
        }
    return out;
}
