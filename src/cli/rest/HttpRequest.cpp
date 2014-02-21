/*
 * HttpGet.cpp
 *
 *  Created on: Feb 18, 2014
 *      Author: simonm
 */

#include "HttpRequest.h"

#include <sys/stat.h>

using namespace fts3::cli;

const string HttpRequest::PORT = "8446";

HttpRequest::HttpRequest(string url, string capath, string proxy, ostream& stream) :
		stream(stream),
		curl(curl_easy_init()),
		fd(0)

{
    if (!curl) throw string("failed to initialize curl context (curl_easy_init)");

	// the url we are going to contact
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	// our proxy certificate (the '-E' option)
	curl_easy_setopt(curl, CURLOPT_SSLCERT, proxy.c_str());
	// path to certificates (the '--capath' option)
	curl_easy_setopt(curl, CURLOPT_CAPATH, capath.c_str());
	// our proxy again (the '-cacert' option)
	curl_easy_setopt(curl, CURLOPT_CAINFO, proxy.c_str());
	// the callback function
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	// the stream the data will be written to
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream);
}

HttpRequest::~HttpRequest()
{
    // RAII fashion clean up
    if(curl) curl_easy_cleanup(curl);
    if(fd) fclose(fd);
}

size_t HttpRequest::write_data(void *ptr, size_t size, size_t nmemb, stringstream* ss)
{
    // calculate the size in bytes
    size_t realsize = size * nmemb;
    // create a string from the received bytes
    string str(static_cast<char*>(ptr), realsize);
    // write it to the stream
    *ss << str;

    return realsize;
}

void HttpRequest::setPort(string& endpoint)
{
    // look for the colon that separates the hostname from port
    size_t found = endpoint.find_last_of(":");

    // if the port was not specified just add it
    if (found == string::npos)
        {
            endpoint += ":" + PORT;
            return;
        }

    // otherwise replace the port
    endpoint = endpoint.substr(0, found + 1) + PORT;
}

void HttpRequest::request()
{
    /* Perform the request, res will get the return code */
    CURLcode res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) throw string(curl_easy_strerror(res));
}

void HttpRequest::get()
{
	// do the request
	request();
}

void HttpRequest::del()
{
	// make it a delete
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
	// do the request
	request();
}

void HttpRequest::put(string path)
{
    // close the file descriptor
    if (fd) fclose(fd);

    // open new one
    fd = fopen(path.c_str(), "rb");
    // check if we were able to do fopen
    if(!fd) throw "cannot open the file: " + path;

    /* to get the file size */
    struct stat file_info;
    if(fstat(fileno(fd), &file_info) != 0) throw "cannot determine the size of the file: " + path;

    /* tell it to "upload" to the URL */
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    /* set where to read from (on Windows you need to use READFUNCTION too) */
    curl_easy_setopt(curl, CURLOPT_READDATA, fd);
    /* and give the size of the upload (optional) */
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);

	// do the request
	request();
}
