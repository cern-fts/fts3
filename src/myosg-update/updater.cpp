#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>

#include "config/serverconfig.h"

#include "common/error.h"
#include "common/logger.h"

#include <string>

using namespace std;
using namespace fts3::common;
using namespace fts3::config;

/**
 * Function that writes the file down
 *
 * It has to be complaint with the libcurl requirements.
 */
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}


/* -------------------------------------------------------------------------- */
int main(int argc, char** argv) {

	// exit status
	int ret = EXIT_SUCCESS;

	theServerConfig().read(argc, argv);

	// path to local the MyOSG XML file
	const string myosg_path = "/var/lib/fts3/myosg.xml";
	// false string
	const string false_str = "false";
	// URL of the MyOSG
	const string url = theServerConfig().get<string>("MyOSG");

	// if the MyOSG file is not specified return
	if (url == false_str) return ret;

	// curl context
	CURL *curl;
	// return code
	CURLcode res;
	// output file descriptor
	FILE *fp = fopen(myosg_path.c_str(), "wb");

	if (!fp) {
		FTS3_COMMON_LOGGER_NEWLOG(ERR) << "failed to create/open file (" << myosg_path << ")" << commit;
		return EXIT_FAILURE;
	}

	curl = curl_easy_init();
	if(curl) {
		/* Set options */
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);

		/* Check for errors */
		if(res != CURLE_OK) {
			ret = EXIT_FAILURE;
			FTS3_COMMON_LOGGER_NEWLOG(ERR) << "curl_easy_perform() failed: " << curl_easy_strerror(res) << commit;
		}
		/* always cleanup */
		curl_easy_cleanup(curl);

	} else {
		ret = EXIT_FAILURE;
		FTS3_COMMON_LOGGER_NEWLOG(ERR) << "failed to initialize curl context (curl_easy_init)" << commit;
	}

    return ret;
}
