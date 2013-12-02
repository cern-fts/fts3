#pragma once

#include <sys/types.h>

/**
 * Return true if the error is considered recoverable
 * (i.e. ENOENT is not recoverable)
 */
bool retryTransfer(int errorNo, const std::string& category, const std::string& message);

/**
 * Return the best number of streams for the given file size
 */
unsigned adjustStreamsBasedOnSize(off_t sizeInBytes, unsigned int currentStreams);

/**
 * Return a reasonable timeout for the given filesize
 */
unsigned adjustTimeoutBasedOnSize(off_t sizeInBytes, unsigned int timeout);

/**
 * Extract domain name from url and compare to see if it's a LAN transfer
 */
bool lanTransfer(const std::string source, const std::string dest);


