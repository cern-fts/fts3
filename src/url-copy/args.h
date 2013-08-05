#pragma once

#include <string>

class UrlCopyOpts
{
public:
    static UrlCopyOpts* getInstance();

    int parse(int argc, char * const argv[]);
    std::string usage(const std::string& bin);
    std::string getErrorMessage();

    // Flags
    bool monitoringMessages;
    bool autoTunned;
    bool manualConfig;
    bool debug;
    bool compareChecksum;
    bool overwrite;
    bool daemonize;

    // Arguments
    std::string infosys;
    std::string tokenBringOnline;
    std::string fileMetadata;
    std::string jobMetadata;
    std::string reuseFile;
    std::string sourceSiteName;
    std::string destSiteName;
    std::string vo;
    std::string checksumAlgorithm;
    std::string checksumValue;
    std::string jobId;
    std::string sourceUrl;
    std::string destUrl;
    std::string sourceTokenDescription;
    std::string destTokenDescription;
    std::string fileId;
    std::string proxy;
    double      userFileSize;
    int         bringOnline;
    int         copyPinLifetime;
    int         nStreams;
    unsigned    tcpBuffersize;
    unsigned    blockSize;
    unsigned    timeout;

private:
    static UrlCopyOpts instance;

    UrlCopyOpts();

    std::string errorMessage;
};
