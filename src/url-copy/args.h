#pragma once

#include <getopt.h>
#include <iostream>
#include <string>

class UrlCopyOpts
{
public:
    static UrlCopyOpts& getInstance();

    int parse(int argc, char * const argv[]);
    std::string usage(const std::string& bin);
    std::string getErrorMessage();

    // Flags
    bool monitoringMessages;
    bool autoTunned;
    bool manualConfig;
    bool overwrite;
    bool daemonize;
    bool logToStderr;
    bool reuse;
    bool multihop;
    bool enable_udt;
    bool global_timeout;
    unsigned debugLevel;

    enum CompareChecksum
    {
        CHECKSUM_DONT_CHECK = 0, // Do not check checksum
        CHECKSUM_STRICT,          // Strict comparison
        CHECKSUM_RELAXED          // Relaxed comparision. i.e. do not fail on empty checksum on source
    } compareChecksum;

    // Arguments
    std::string infosys;
    std::string tokenBringOnline;
    std::string fileMetadata;
    std::string jobMetadata;
    std::string sourceSiteName;
    std::string destSiteName;
    std::string vo;
    std::string checksumValue;
    std::string jobId;
    std::string sourceUrl;
    std::string destUrl;
    std::string sourceTokenDescription;
    std::string destTokenDescription;
    unsigned    fileId;
    std::string proxy;
    long long   userFileSize;
    int         bringOnline;
    int         copyPinLifetime;
    int         nStreams;
    unsigned    tcpBuffersize;
    unsigned    blockSize;
    unsigned    timeout;
    int    	secPerMb;
    std::string user_dn;
    std::string alias;
    std::string oauthFile;

    bool areTransfersOnFile() const
    {
        return reuse || multihop;
    }

private:
    static UrlCopyOpts instance;
    static const option long_options[];
    static const char short_options[];

    UrlCopyOpts();
    UrlCopyOpts(const UrlCopyOpts&);

    std::string errorMessage;
};


std::ostream& operator << (std::ostream& out, const UrlCopyOpts::CompareChecksum& c);
