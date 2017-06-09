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

#include <boost/algorithm/string.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>

#include "cli/exception/cli_exception.h"
#include "cli/exception/rest_failure.h"
#include "cli/exception/rest_invalid.h"
#include "cli/exception/wrong_protocol.h"
#include "cli/rest/HttpRequest.h"

using fts3::cli::HttpRequest;
using fts3::cli::cli_exception;
using fts3::cli::rest_failure;
using fts3::cli::rest_invalid;
using fts3::cli::wrong_protocol;
using fts3::cli::CertKeyPair;


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(HttpRequestTest)


BOOST_AUTO_TEST_CASE(UrlEncode)
{
    std::string encoded;

    // Unreserved characters
    std::string unreserved("abcdefghijklmNOPQRSTUVW-_.~");
    encoded = HttpRequest::urlencode(unreserved);
    BOOST_CHECK_EQUAL(encoded, unreserved);

    // Reserved characters
    std::string reserved("!&#[];:*()=");
    encoded = HttpRequest::urlencode(reserved);
    BOOST_CHECK(boost::iequals(encoded, "%21%26%23%5B%5D%3B%3A%2A%28%29%3D"));

    // Mix
    std::string mix("(true == false)?false:true");
    encoded = HttpRequest::urlencode(mix);
    BOOST_CHECK(
        boost::iequals(encoded, "%28true%20%3D%3D%20false%29%3Ffalse%3Atrue")
        or
        boost::iequals(encoded, "%28true+%3D%3D+false%29%3Ffalse%3Atrue")
    );
}


class HttpRequestMock: public HttpRequest
{
public:
    HttpRequestMock(std::string const & url, std::string const & capath, CertKeyPair const & certkey,
    std::iostream& stream, std::string const &topname = std::string()):
        HttpRequest(url, capath, certkey, true, stream, topname),
        performCode(CURLE_OK), httpCode(0), expectBody(false)
    {
    }

    CURLcode performCode;
    long httpCode;
    std::string server;
    std::string contentType;
    std::string response;
    std::string reqBody;
    bool expectBody;

protected:
    virtual CURLcode perform(void)
    {
        if (performCode != CURLE_OK)
            return performCode;

        if (expectBody) {
            std::vector<char> buffer(2048);
            size_t nbytes = read_data(buffer.data(), 1, buffer.size(), this);
            buffer[nbytes] = '\0';
            reqBody = buffer.data();
        }

        std::string header("Content-Type: ");
        header += contentType;
        keep_header((char*)header.data(), header.size(), 1, this);

        if (!server.empty()) {
            header = "Server: " + server;
            keep_header((char*)header.data(), header.size(), 1, this);
        }

        write_data((void*)response.data(), response.size(), 1, this);

        return CURLE_OK;
    }

    virtual CURLcode get_info(CURLINFO info, long *data)
    {
        BOOST_ASSERT(info == CURLINFO_RESPONSE_CODE);
        *data = httpCode;
        return CURLE_OK;
    }
};


BOOST_AUTO_TEST_CASE(simpleGet)
{
    std::stringstream out;
    HttpRequestMock http("https://nowhere.noplace.com",
        "/etc/grid-security/certificates", CertKeyPair("/tmp/myproxy.pem"),
        out);

    http.performCode = CURLE_OK;
    http.httpCode = 200;
    http.contentType = "application/json";
    http.response = "{\"a\": \"b\"}";

    http.get();

    BOOST_CHECK_EQUAL(out.str(), http.response);
}


BOOST_AUTO_TEST_CASE(simpleDel)
{
    std::stringstream out;
    HttpRequestMock http("https://nowhere.noplace.com",
        "/etc/grid-security/certificates", CertKeyPair("/tmp/myproxy.pem"),
        out);

    http.performCode = CURLE_OK;
    http.httpCode = 200;
    http.contentType = "application/json";
    http.response = "{\"a\": \"b\"}";

    http.del();

    BOOST_CHECK_EQUAL(out.str(), http.response);
}


BOOST_AUTO_TEST_CASE(simplePut)
{
    std::string req("blahblehblih");
    std::stringstream stream;
    stream << req;

    HttpRequestMock http("https://nowhere.noplace.com",
        "/etc/grid-security/certificates", CertKeyPair("/tmp/myproxy.pem"),
        stream);

    http.performCode = CURLE_OK;
    http.httpCode = 201;
    http.contentType = "application/json";
    http.response = "{\"a\": \"b\"}";
    http.expectBody = true;

    http.put();

    BOOST_CHECK_EQUAL(http.reqBody, req);
    char buffer[1024];
    stream.getline(buffer, sizeof(buffer));
    BOOST_CHECK_EQUAL(std::string(buffer), http.response);
}


BOOST_AUTO_TEST_CASE(simplePost)
{
    std::string req("blahblehblih");
    std::stringstream stream;
    stream << req;

    HttpRequestMock http("https://nowhere.noplace.com",
        "/etc/grid-security/certificates", CertKeyPair("/tmp/myproxy.pem"),
        stream);

    http.performCode = CURLE_OK;
    http.httpCode = 201;
    http.contentType = "application/json";
    http.response = "{\"a\": \"b\"}";
    http.expectBody = true;

    http.post();

    BOOST_CHECK_EQUAL(http.reqBody, req);
    char buffer[1024];
    stream.getline(buffer, sizeof(buffer));
    BOOST_CHECK_EQUAL(std::string(buffer), http.response);
}


BOOST_AUTO_TEST_CASE(withTopNode)
{
    std::stringstream out;
    HttpRequestMock http("https://nowhere.noplace.com",
        "/etc/grid-security/certificates", CertKeyPair("/tmp/myproxy.pem"),
        out, "top");

    http.performCode = CURLE_OK;
    http.httpCode = 200;
    http.contentType = "application/json";
    http.response = "[{\"a\": \"b\"}]";

    http.get();

    BOOST_CHECK_EQUAL(out.str(), "{\"top\":[{\"a\": \"b\"}]}");
}


BOOST_AUTO_TEST_CASE(canNotConnect)
{
    std::stringstream out;
    HttpRequestMock http("https://nowhere.noplace.com",
        "/etc/grid-security/certificates", CertKeyPair("/tmp/myproxy.pem"),
        out);

    http.performCode = CURLE_COULDNT_RESOLVE_HOST;

    BOOST_CHECK_THROW(http.get(), cli_exception);
}


BOOST_AUTO_TEST_CASE(talkingToSoap)
{
    std::stringstream out;
    HttpRequestMock http("https://nowhere.noplace.com",
        "/etc/grid-security/certificates", CertKeyPair("/tmp/myproxy.pem"),
        out);

    http.performCode = CURLE_OK;
    http.httpCode = 200;
    http.server = "gSOAP/1.0";

    BOOST_CHECK_THROW(http.get(), wrong_protocol);
}


BOOST_AUTO_TEST_CASE(errorNoJson)
{
    std::stringstream out;
    HttpRequestMock http("https://nowhere.noplace.com",
        "/etc/grid-security/certificates", CertKeyPair("/tmp/myproxy.pem"),
        out);

    http.performCode = CURLE_OK;
    http.httpCode = 400;
    http.contentType = "text/html";
    http.response = "<html></html>";

    BOOST_CHECK_THROW(http.get(), rest_invalid);
}


BOOST_AUTO_TEST_CASE(errorWithJson)
{
    std::stringstream out;
    HttpRequestMock http("https://nowhere.noplace.com",
        "/etc/grid-security/certificates", CertKeyPair("/tmp/myproxy.pem"),
        out);

    http.performCode = CURLE_OK;
    http.httpCode = 400;
    http.contentType = "application/json";
    http.response = "{\"message\": \"missing parameter\", \"status\": \"400 Bad Request\"}";

    BOOST_CHECK_THROW(http.get(), rest_failure);
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
