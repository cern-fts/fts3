#pragma once

#include <sys/types.h>

/**
 * Return true if the error is considered recoverable
 * (i.e. ENOENT is not recoverable)
 */
bool retryTransfer(int errorNo, const std::string& category);

/**
 * Return the best number of streams for the given file size
 */
unsigned adjustStreamsBasedOnSize(double sizeInBytes, unsigned int currentStreams);

/**
 * Return a reasonable timeout for the given filesize
 */
unsigned adjustTimeoutBasedOnSize(double sizeInBytes, unsigned int timeout);
